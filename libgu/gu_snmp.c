/*
** mouse:~ppr/src/libgu/gu_snmp.c
** Copyright 1995--2003, Trinity College Computing Center.
** Written by David Chappell.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
** 
** * Redistributions of source code must retain the above copyright notice,
** this list of conditions and the following disclaimer.
** 
** * Redistributions in binary form must reproduce the above copyright
** notice, this list of conditions and the following disclaimer in the
** documentation and/or other materials provided with the distribution.
** 
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE 
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
** POSSIBILITY OF SUCH DAMAGE.
**
** Last modified 5 April 2003.
*/

#include "before_system.h"
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdarg.h>
#include "gu.h"
#include "global_defines.h"
#include "cexcept.h"

define_exception_type(int);
static struct exception_context the_exception_context[1];

#define UNIVERSAL_FLAG 0x00
#define APPLICATION_FLAG 0x40
#define CONTEXT_FLAG 0x80
#define PRIVATE_FLAG 0xC0

#define PRIMITIVE_FLAG 0x00
#define CONSTRUCTOR_FLAG 0x20

#define BOOLEAN_TAG 0x01
#define INT_TAG 0x02
#define BIT_STRING_TAG 0x03
#define OCTET_STRING_TAG 0x04
#define NULL_TAG 0x05
#define OBJECT_ID_TAG 0x06
#define SEQUENCE_TAG 0x10
#define SET_TAG 0x11

#define LONG_LENGTH 0x80

#define SNMP_IP_ADDRESS_TAG (APPLICATION_FLAG | 0x00)
#define SNMP_COUNTER32_TAG (APPLICATION_FLAG | 0x01)
#define SNMP_GAGE32_TAG (APPLICATION_FLAG | 0x02)
#define SNMP_TIMETICKS_TAG (APPLICATION_FLAG | 0x03)
#define SNMP_OPAQUE_TAG (APPLICATION_FLAG | 0x04)
#define SNMP_NSAP_ADDRESS_TAG (APPLICATION_FLAG | 0x05)
#define SNMP_COUNTER64_TAG (APPLICATION_FLAG | 0x06)
#define SNMP_UINTEGER32_TAG (APPLICATION_FLAG | 0x07)

static void *decode_length(void *vp, int *length)
	{
	char *p = vp;
	int skip;
	skip = *length = *(unsigned char *)(p++);
	if(skip & LONG_LENGTH)
		{
		switch(skip)
			{
			case LONG_LENGTH | 1:
				*length = *(unsigned char*)p;
				break;
			case LONG_LENGTH | 2:
				*length = *(unsigned char *)p << 8 | *(unsigned char *)(p+1);
				break;
			}
		p += skip & 0xFF;
		}
	return (void*)p;
	}

static void *decode_sequence(void *vp, int tag, int *len)
	{
	char *p = vp;
	if(*(unsigned char *)p++ != tag)
		return NULL;
	return decode_length(p, len);
	}

static void *decode_int(void *vp, int *n)
	{
	char *p = vp;
	int int_size;
	if(*(unsigned char *)p++ != INT_TAG)
		return NULL;
	int_size = *(unsigned char *)p++;
	for(*n=0; int_size--; )
		{
		*n = *n << 8;
		*n |= *(unsigned char *)p++;
		}
	return (void*)p;
	}

static void *decode_string(void *vp, int tag, char **s, int *len)
	{
	char *p = vp;
	if(*(unsigned char *)p++ != tag)
		return NULL;
	p = decode_length(p, len);
	*s = (char *)p;
	p+= *len;
	return (void*)p;
	}

/*
** This function creates an SNMP object.  If the community[] is NULL, then
** "public" will be used.
*/
struct gu_snmp *gu_snmp_open(unsigned long int ip_address, const char community[], int *error_code)
	{
	int e;
	struct sockaddr_in server_ip;
	struct sockaddr_in my_ip;
	#warning Expect spurious warning concerning next line
	int fd;
	struct gu_snmp *p;

	Try {
		if(community && strlen(community) > 127)
			Throw(-1);

		memset(&server_ip, 0, sizeof(server_ip));
		server_ip.sin_family = AF_INET;
		memcpy(&server_ip.sin_addr, &ip_address, sizeof(ip_address));
		server_ip.sin_port = htons(161);

		memset(&my_ip, 0, sizeof(my_ip));
		my_ip.sin_family = AF_INET;
		my_ip.sin_addr.s_addr = htonl(INADDR_ANY);
		my_ip.sin_port = htons(0);

		if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
			Throw(-2);

		Try {
			if(bind(fd, (struct sockaddr *)&my_ip, sizeof(my_ip)) < 0)
				Throw(-3);

			if(connect(fd, (struct sockaddr *)&server_ip, sizeof(server_ip)) < 0)
				Throw(-4);
			}
		Catch(e)
			{
			int saved_errno = errno;
			close(fd);
			errno = saved_errno;
			Throw(e);
			}
		}
	Catch(e)
		{
		if(error_code)
			*error_code = e;
		return NULL;
		}

	/* We have suceeded!  Go ahead and allocate the structure and fill it in. */
	p = (struct gu_snmp *)gu_alloc(1, sizeof(struct gu_snmp));
	p->socket = fd;
	p->community = community ? community : "public";
	p->request_id = (getpid() << 16) | (time((time_t *)NULL) & 0xFFFF);
	return p;
	}

/*
** This destroys the SNMP object.
*/
void gu_snmp_close(struct gu_snmp *p)
	{
	close(p->socket);
	gu_free(p);
	}

/*
** This uses the SNMP object to perform and SNMP query.
*/
int gu_snmp_get(struct gu_snmp *p, int *error_code, ...)
	{
	int e;
	char buffer[1024];
	int i = 0, items_count;
	int start_pdu, start_values_list;
	struct { int type; void *ptr; } items[10];
	const char *oid;
	va_list va;

	buffer[i++] = 0x30;			/* constructed sequence */
	buffer[i++] = 0x82;			/* 2 byte length */
	i+=2;

	buffer[i++] = 0x02;			/* integer 0 (SNMP version 1.0) */
	buffer[i++] = 0x01;
	buffer[i++] = 0x00;

	{
	int len;
	buffer[i++] = 0x04;			/* octet string (community name) */
	len = strlen(p->community);
	buffer[i++] = len;
	memcpy(&buffer[i], p->community, len);
	i+=len;
	}

	buffer[i++] = 0xA0;			/* start of PDU */
	buffer[i++] = 0x82;
	i+=2;
	start_pdu = i;

	/* Generate a request id and encode it. */
	{
	p->request_id++;
	#ifdef TEST
	printf("request_id = 0x%X\n", p->request_id);
	#endif
	buffer[i++] = 0x02;
	buffer[i++] = 0x04;
	buffer[i++] = p->request_id >> 24;
	buffer[i++] = (p->request_id >> 16) & 0xFF;
	buffer[i++] = (p->request_id >> 8) & 0xFF;
	buffer[i++] = p->request_id & 0xFF;
	}

	buffer[i++] = 0x02;					/* error code place holder */
	buffer[i++] = 0x01;
	buffer[i++] = 0x00;

	buffer[i++] = 0x02;					/* error index place holder */
	buffer[i++] = 0x01;
	buffer[i++] = 0x00;

	buffer[i++] = 0x30;					/* start of array of questions */
	buffer[i++] = 0x82;
	i+=2;
	start_values_list = i;

	va_start(va, error_code);
	for(items_count=0; items_count < (sizeof(items) / sizeof(items[0])) && (oid = va_arg(va, const char*)); items_count++)
		{
		int start_values_list_item;
		int values_list_item_len;
		int start_oid;
		int x, x1, x2;

		items[items_count].type = va_arg(va, int);
		items[items_count].ptr = va_arg(va, void*);

		buffer[i++] = 0x30;					/* start of sequence {name, value} */
		buffer[i++] = 0x82;
		i+=2;								/* <-- 2 byte length */
		start_values_list_item = i;

		buffer[i++] = 0x06;					/* OID */
		i++;								/* <-- length byte */
		start_oid = i;

		x1 = atoi(oid);						/* first two numbers are packed into one byte */
		oid += strspn(oid, "0123456789");
		oid += strcspn(oid, "0123456789");
		x2 = atoi(oid);
		oid += strspn(oid, "0123456789");
		oid += strcspn(oid, "0123456789");
		buffer[i++] = x1 * 40 + x2;

		while(*oid)							/* pack the rest of them */
			{
			x = atoi(oid);
			oid += strspn(oid, "0123456789");
			oid += strcspn(oid, "0123456789");
			buffer[i++] = x;
			}

		buffer[start_oid - 1] = (i - start_oid);

		buffer[i++] = 0x05;					/* null */
		buffer[i++] = 0x00;

		/* Fill in the length of the sequence {name, value}. */
		values_list_item_len = i - start_values_list_item;
		buffer[start_values_list_item - 2] = values_list_item_len >> 8;
		buffer[start_values_list_item - 1] = values_list_item_len & 0xFF;
		}
	va_end(va);

	/* Go back and fill in the length of the array. */
	{
	int values_list_len = i - start_values_list;
	buffer[start_values_list - 2] = values_list_len >> 8;
	buffer[start_values_list - 1] = values_list_len & 0xFF;
	}

	/* Go back and fill in the length of the PDU. */
	{
	int pdu_len = (i - start_pdu);
	buffer[start_pdu - 2] = pdu_len >> 8;
	buffer[start_pdu - 1] = pdu_len & 0xFF;
	}

	/* Go back and fill in the length of the outermost structure. */
	{
	int message_len = (i - 4);
	buffer[2] = message_len >> 8;
	buffer[3] = message_len & 0xFF;
	}

	/* Send and resent the request until we get a response or the retries
	   are exhausted. */
	Try {
		int attempt;
		fd_set rfds;
		struct timeval timeout;
		for(attempt=0; attempt < 5; attempt++)
			{
			/* Send the request. */
			#ifdef TEST
			printf("sending %d bytes...\n", i);
			#endif
			if(send(p->socket, buffer, i, 0) < 0)
				Throw(-1);

			/* Wait up to 1 second for a response. */
			FD_ZERO(&rfds);
			FD_SET(p->socket, &rfds);
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;
			if(select(p->socket + 1, &rfds, NULL, NULL, &timeout) < 0)
				Throw(-2);

			/* If there was nothing to read, start next iteration
			   (which will result in a resend. */
			if(!FD_ISSET(p->socket, &rfds))
				continue;

			/* Receive the packet. */
			{
			int len;
			if((len = recv(p->socket, p->result, sizeof(p->result), 0)) < 0)
				Throw(-3);
			#ifdef TEST
			printf("Got %d bytes\n", len);
			#endif
			}

			/* Now the fun starts.  We will attempt to parse the response as
			   an SNMP packet.  If we fail at any point we will start the next
			   iteration.
			   */
			{
			char *ptr;
			int obj_ival; char *obj_ptr; int obj_len;

			ptr = p->result;
			if(!(ptr = decode_sequence(ptr, 0x30, &obj_len)))	/* outside structure */
				continue;
			if(!(ptr = decode_int(ptr, &obj_ival)))				/* SNMP version number */
				continue;
			if(obj_ival != 0)									/* SNMP version 1.0 is 0 */
				continue;
			if(!(ptr = decode_string(ptr, OCTET_STRING_TAG, &obj_ptr, &obj_len)))
				continue;
			if(!(ptr = decode_sequence(ptr, 0xA2, &obj_len)))	/* response PDU */
				continue;
			if(!(ptr = decode_int(ptr, &obj_ival)))				/* request id */
				continue;
			if(obj_ival != p->request_id)						/* request id should match */
				continue;
			if(!(ptr = decode_int(ptr, &obj_ival)))				/* error code */
				Throw(-50);
			if(obj_ival != 0)									/* 0 means no error */
				Throw(obj_ival);
			if(!(ptr = decode_int(ptr, &obj_ival)))				/* error index */
				Throw(-52);
			if(!(ptr = decode_sequence(ptr, 0x30, &obj_len)))	/* response data list */
				Throw(-53);

			{
			int items_i;
			for(items_i=0; items_i<items_count; items_i++)
				{
				if(!(ptr = decode_sequence(ptr, 0x30, &obj_len)))	/* first item name-value pair */
					Throw(-54);
				if(!(ptr = decode_sequence(ptr, OBJECT_ID_TAG, &obj_len)))
					Throw(-55);
				ptr += obj_len;
				switch(items[items_i].type)
					{
					case GU_SNMP_INT:
						if(!(ptr = decode_int(ptr, (int*)items[items_i].ptr)))
							Throw(-71);
						break;
					case GU_SNMP_STR:
						{
						char *temp;
						if(!(ptr = decode_string(ptr, OCTET_STRING_TAG, &temp, &obj_len)))
							Throw(-72);
						*((char**)items[items_i].ptr) = gu_strndup(temp, obj_len);
						}
						break;
					case GU_SNMP_BIT:
						{
						char *temp;
						int bit, x, y;
						unsigned int n = 0;
						if(!(ptr = decode_string(ptr, OCTET_STRING_TAG, &temp, &obj_len)))
							Throw(-72);
						for(x=0,bit=0; x<obj_len; x++)
							{
							for(y=7; y>=0; y--,bit++)
								{
								if(temp[x] & (1 << y))
									{
									n |= (1 << bit);
									}
								}
							}
						*((unsigned int*)items[items_i].ptr) = n;
						}
						break;
					default:
						Throw(-99);
						break;
					}


				}
			}
			}
			return items_count;
			}

		/* If we reach here, there was no valid response. */
		Throw(-100);
		}
	Catch(e)
		{
		if(error_code)
			*error_code = e;
		}

	return -1;
	}

/*
** To compile this test code:
** gcc -Wall -I ../include -DTEST -o gu_snmp gu_snmp.c ../libgu.a
**
** To run it:
** ./gu_snmp 10.0.0.42 public
*/
#ifdef TEST
int main(int argc, char *argv[])
	{
	struct gu_snmp *s;
	int error_code;
	int n1;
	unsigned int n2;
	char *str;

	if(!(s = gu_snmp_open(inet_addr(argv[1]), "public", &error_code)))
		{
		printf("gu_snmp_open() failed, error_code=%d\n", error_code);
		return 1;
		}

	if(gu_snmp_get(s, &error_code,
				"1.3.6.1.2.1.1.5.0", GU_SNMP_STR, &str,
				"1.3.6.1.2.1.25.3.5.1.1.1", GU_SNMP_INT, &n1,
				"1.3.6.1.2.1.25.3.5.1.2.1", GU_SNMP_BIT, &n2,
				NULL
				) < 0)
		{
		printf("gu_snmp_get() failed, error_code=%d\n", error_code);
		return 2;
		}

	printf("system name: %s\n", str);
	printf("printer status 1: %d\n", n1);
	printf("printer status 2: 0x%.2X\n", n2);
	gu_free(str);

	gu_snmp_close(s);

	return 0;
	}
#endif

/* end of file */
