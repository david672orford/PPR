/*
** mouse:~ppr/src/libgu/gu_snmp.c
** Copyright 1995--2004, Trinity College Computing Center.
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
** Last modified 28 January 2004.
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

/*! \file
	\brief SNMP query routines

This file contains a minimal inplementation of SNMP querys.

*/

#define UNIVERSAL_FLAG		0x00		/* same meaning in all applications */
#define APPLICATION_FLAG	0x40		/* meaning dependent on application of ASN.1, for example SNMP */
#define CONTEXT_FLAG		0x80		/* meaning specific to given structure type */
#define PRIVATE_FLAG		0xC0		/* meaning specific to a specific organization */

#define PRIMITIVE_FLAG		0x00
#define CONSTRUCTOR_FLAG	0x20

/* ASN.1 Universal tag types */
#define BOOLEAN_TAG			0x01		/* datum is boolean */
#define INT_TAG				0x02		/* datum is an integer */
#define BIT_STRING_TAG		0x03		/* datum is a bit string */
#define OCTET_STRING_TAG	0x04		/* datum is a string of bytes */
#define NULL_TAG			0x05		/* datum is null (undefined) */
#define OBJECT_ID_TAG		0x06		/* datum is an SNMP object ID number */
#define SEQUENCE_TAG		0x10
#define SET_TAG				0x11

/* Bit that when set indicates a length value greater than
   0x7f encoded in ensuing bytes. */
#define LONG_LENGTH			0x80

/* See RFC 1155 page 18 */
#define SNMP_IP_ADDRESS_TAG		(APPLICATION_FLAG | 0x00)
#define SNMP_COUNTER32_TAG		(APPLICATION_FLAG | 0x01)
#define SNMP_GAGE32_TAG			(APPLICATION_FLAG | 0x02)
#define SNMP_TIMETICKS_TAG		(APPLICATION_FLAG | 0x03)
#define SNMP_OPAQUE_TAG			(APPLICATION_FLAG | 0x04)
#define SNMP_NSAP_ADDRESS_TAG	(APPLICATION_FLAG | 0x05)
#define SNMP_COUNTER64_TAG		(APPLICATION_FLAG | 0x06)
#define SNMP_UINTEGER32_TAG		(APPLICATION_FLAG | 0x07)

/*
** This function reads an ASN.1 length value.  It returns a pointer to the byte
** after the decoded length.
*/
static void *decode_length(void *vp, int *remaining, int *length)
	{
	char *p = vp;
	int skip;

	if(--(*remaining) < 0)
		return NULL;

	skip = *length = *(unsigned char *)(p++);

	if(skip & LONG_LENGTH)
		{
		switch(skip)
			{
			case LONG_LENGTH | 1:
				if(--(*remaining) < 0)
					return NULL;
				*length = *(unsigned char*)p;
				break;
			case LONG_LENGTH | 2:
				if((*remaining -= 2) < 0)
					return NULL;
				*length = *(unsigned char *)p << 8 | *(unsigned char *)(p+1);
				break;
			default:
				return (void*)NULL;
			}
		p += skip & 0x7F;
		}
	return (void*)p;
	}

/*
** This function verifies that the next data item is an ASN.1 sequence of the specified type.  It then
** reads and stores the length and returns a pointer to the byte after the decoded length.
*/
static void *decode_sequence(void *vp, int *remaining, int tag, int *len)
	{
	char *p = vp;
	if(--(*remaining) < 0)
		return NULL;
	if(*(unsigned char *)p++ != tag)
		return NULL;
	return decode_length(p, remaining, len);
	}

/*
** This function verifies that the next data item is an ASN.1 encoded integer.  It then reads
** the integer into an int n and returns an pointer to the next data byte.
*/
static void *decode_int(void *vp, int *remaining, int *n)
	{
	char *p = vp;
	int int_size;

	if(--(*remaining) < 0)
		return NULL;
	if(*(unsigned char *)p++ != INT_TAG)
		return NULL;

	if(--(*remaining) < 0)
		return NULL;
	int_size = *(unsigned char *)p++;

	if((*remaining -= int_size) < 0)
		return NULL;
	for(*n=0; int_size--; )
		{
		*n = *n << 8;
		*n |= *(unsigned char *)p++;
		}

	return (void*)p;
	}

/*
** This function verifies that the next data item is a string of the specified
** type.  It then reads the length of the string into len and sets s to point
** to the start of the integer.  These values can later by used as arguments to
** gu_strndup().
*/
static void *decode_string(void *vp, int *remaining, int tag, char **s, int *len)
	{
	char *p = vp;

	if(--(*remaining) < 0)
		return NULL;
	if(*(unsigned char *)p++ != tag)
		return NULL;

	p = decode_length(p, remaining, len);

	if((*remaining -= *len) < 0)
		return NULL;
	*s = (char *)p;
	p+= *len;

	return (void*)p;
	}

/*
** This function produces a text description from an error code.
*/
static const char *str_snmp_error(int code)
	{
	switch(code)
		{
		case 0:
			return "noError";
		case 1:
			return "tooBig";
		case 2:
			return "noSuchName";
		case 3:
			return "badValue";
		case 4:
			return "readOnly";
		case 5:
			return "genErr";
		default:
			return "?";
		}
	} /* end of str_snmp_error() */

/** Create a gu_snmp object

This function creates an SNMP object, connects it to a remote system, and
returns a pointer to it.  If the community[] is NULL, then "public" will
be used.  This function throws exceptions on failure.

*/
struct gu_snmp *gu_snmp_open(unsigned long int ip_address, const char community[])
	{
	struct sockaddr_in server_ip;
	struct sockaddr_in my_ip;
	int fd;
	struct gu_snmp *p;

	if(community && strlen(community) > 127)
		gu_Throw("SNMP community name too long");

	memset(&server_ip, 0, sizeof(server_ip));
	server_ip.sin_family = AF_INET;
	memcpy(&server_ip.sin_addr, &ip_address, sizeof(ip_address));
	server_ip.sin_port = htons(161);

	memset(&my_ip, 0, sizeof(my_ip));
	my_ip.sin_family = AF_INET;
	my_ip.sin_addr.s_addr = htonl(INADDR_ANY);
	my_ip.sin_port = htons(0);

	if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		gu_Throw("socket() failed, errno=%d (%s)", errno, gu_strerror(errno));

	gu_Try {
		if(bind(fd, (struct sockaddr *)&my_ip, sizeof(my_ip)) < 0)
			gu_Throw("bind() failed, errno=%d (%s)", errno, gu_strerror(errno));

		if(connect(fd, (struct sockaddr *)&server_ip, sizeof(server_ip)) < 0)
			gu_Throw("connect() failed, errno=%d (%s)", errno, gu_strerror(errno));
		}
	gu_Catch
		{
		int saved_errno = errno;
		close(fd);
		errno = saved_errno;
		gu_ReThrow();
		}

	/* We have suceeded!  Go ahead and allocate the structure and fill it in. */
	p = (struct gu_snmp *)gu_alloc(1, sizeof(struct gu_snmp));
	p->socket = fd;
	p->community = community ? community : "public";
	p->request_id = (getpid() << 16) | (time((time_t *)NULL) & 0xFFFF);
	return p;
	} /* end of gu_snmp_open() */

/** destroy an SNMP object

*/
void gu_snmp_close(struct gu_snmp *p)
	{
	close(p->socket);
	gu_free(p);
	}

/*
** An array of these structures is used to hold a list of the expected return
** values, their types, and pointers to variables into which they should
** be stored.
*/
struct ITEMS {
	const char *oid;
	int type;
	void *ptr;
	};

/*
** Create the SNMP query packet.
*/
static int create_packet(struct gu_snmp *p, char *buffer, struct ITEMS *items, int items_count)
	{
	int start_pdu, start_values_list;				/* place markers to things we can't fill in until the end */
	int i = 0;
	int items_i;

	buffer[i++] = 0x30;			/* constructed sequence */
	buffer[i++] = 0x82;			/* 2 byte length */
	i += 2;						/* leave space for those two bytes */

	buffer[i++] = 0x02;			/* integer 0 (SNMP version 1.0) */
	buffer[i++] = 0x01;
	buffer[i++] = 0x00;

	/* store community name */
		{
		int len;
		buffer[i++] = 0x04;			/* octet string */
		len = strlen(p->community);
		buffer[i++] = len;
		memcpy(&buffer[i], p->community, len);
		i += len;
		}

	buffer[i++] = 0xA0;			/* start of PDU */
	buffer[i++] = 0x82;
	i += 2;
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
	i += 2;
	start_values_list = i;

	for(items_i=0; items_i < items_count; items_i++)
		{
		const char *oid = items[items_i].oid;
		int start_values_list_item;
		int values_list_item_len;
		int start_oid;
		int x1, x2;

		buffer[i++] = 0x30;						/* start of sequence {name, value} */
		buffer[i++] = 0x82;
		i += 2;									/* <-- 2 byte length */
		start_values_list_item = i;

		buffer[i++] = OBJECT_ID_TAG;			/* OID */
		i++;									/* leave room for length byte */
		start_oid = i;							/* keep a finger on this place */

		x1 = atoi(oid);							/* first two numbers are packed into one byte */
		oid += strspn(oid,  "0123456789");		/* move past the digits we read with atoi() */
		oid += strcspn(oid, "0123456789");		/* skip punctionation */
		x2 = atoi(oid);
		oid += strspn(oid,  "0123456789");
		oid += strcspn(oid, "0123456789");
		buffer[i++] = x1 * 40 + x2;

		while(*oid)								/* pack the rest of them */
			{
			x1 = atoi(oid);
			oid += strspn(oid,  "0123456789");
			oid += strcspn(oid, "0123456789");
			buffer[i++] = x1;
			}

		buffer[start_oid - 1] = (i - start_oid);	/* go back and fill in OID length */

		buffer[i++] = NULL_TAG;						/* for some reason we must supply a value, so we use null */
		buffer[i++] = 0x00;

		/* Fill in the length of the sequence {name, value}. */
		values_list_item_len = i - start_values_list_item;
		buffer[start_values_list_item - 2] = values_list_item_len >> 8;
		buffer[start_values_list_item - 1] = values_list_item_len & 0xFF;
		}

	/* Go back and fill in the length of the values list array. */
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

	return i;
	} /* end of create_packet() */

/*
** parse an SNMP response packet
*/
static int parse_response(struct gu_snmp *p, struct ITEMS *items, int items_count)
	{
	char *ptr;
	int len;
	int obj_ival, obj_ival2; char *obj_ptr; int obj_len;
	int items_i;

	ptr = p->result;
	len = p->result_len;

	/* In this block we make sure the received packet constitutes a response to
	   our query and not some garbage from out of the blue.  If it doesn't match
	   up with our query, we bail out, silently ignoring it.
	   */
	if(!(ptr = decode_sequence(ptr, &len, 0x30, &obj_len)))		/* outside structure */
		return -1;
	if(!(ptr = decode_int(ptr, &len, &obj_ival)))				/* SNMP version number */
		return -1;
	if(obj_ival != 0)											/* SNMP version 1.0 is 0 */
		return -1;
	if(!(ptr = decode_string(ptr, &len, OCTET_STRING_TAG, &obj_ptr, &obj_len)))	/* the community name */
		return -1;
	if(!(ptr = decode_sequence(ptr, &len, 0xA2, &obj_len)))		/* SNMP response PDU */
		return -1;
	if(!(ptr = decode_int(ptr, &len, &obj_ival)))				/* request id */
		return -1;
	if(obj_ival != p->request_id)								/* request id should match */
		return -1;

	/* OK, now things are serious.  If the packet is bad, we treat it as a fatal error.
	   */
	if(!(ptr = decode_int(ptr, &len, &obj_ival)))				/* SNMP error code */
		gu_Throw("failed to read SNMP error code");
	if(!(ptr = decode_int(ptr, &len, &obj_ival2)))				/* error index */
		gu_Throw("failed to read SNMP error index");
	if(obj_ival != 0)											/* 0 means no error */
		gu_Throw("SNMP query item %d of %d failed, ErrorStatus=%d (%s)", obj_ival2, items_count, obj_ival, str_snmp_error(obj_ival));
	if(!(ptr = decode_sequence(ptr, &len, 0x30, &obj_len)))		/* response data list */
		gu_Throw("failed to find SNMP response data list");

	for(items_i=0; items_i < items_count; items_i++)
		{
		if(!(ptr = decode_sequence(ptr, &len, 0x30, &obj_len)))	/* first item name-value pair */
			gu_Throw("Parse error 1 on item %d", items_i);
		if(!(ptr = decode_sequence(ptr, &len, OBJECT_ID_TAG, &obj_len)))
			gu_Throw("Parse error 2 on item %d", items_i);
		ptr += obj_len;
		switch(items[items_i].type)
			{
			case GU_SNMP_INT:
				if(!(ptr = decode_int(ptr, &len, (int*)items[items_i].ptr)))
					gu_Throw("failed to decode integer");
				break;
			case GU_SNMP_STR:
				{
				char *temp;
				if(!(ptr = decode_string(ptr, &len, OCTET_STRING_TAG, &temp, &obj_len)))
					gu_Throw("failed to decode string");
				*((char**)items[items_i].ptr) = gu_strndup(temp, obj_len);
				}
				break;
			case GU_SNMP_BIT:
				{
				char *temp;
				int bit, x, y;
				unsigned int n = 0;
				if(!(ptr = decode_string(ptr, &len, OCTET_STRING_TAG, &temp, &obj_len)))
					gu_Throw("failed to decode bit string");
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
				gu_Throw("unrecognized data type");
				break;
			} /* end of switch */
		}

	return 0;
	} /* end of parse_response() */

/** Perform an SNMP query

This uses a gu_snmp object to perform and SNMP query.  The first argument is
a pointer to a gu_snmp object.  The subsequent arguments are a list of data
types exepcted and pointers to storeage for them.  This funciton throws
exceptions on errors.

*/
void gu_snmp_get(struct gu_snmp *p, ...)
	{
	char buffer[1024];						/* buffer into which we build query packet */
	int packet_length;						/* length of query packet in bytes */
	struct ITEMS items[10];					/* queried data items list */
	int items_count;						/* length of the queried data items list */

	/* Read the arguments array into an easy-to-use array of structures. */
		{
		va_list va;
		const char *oid;
		va_start(va, p);
		for(items_count=0; items_count < (sizeof(items) / sizeof(items[0])) && (oid = va_arg(va, const char*)); items_count++)
			{
			items[items_count].oid = oid;
			items[items_count].type = va_arg(va, int);
			items[items_count].ptr = va_arg(va, void*);
			}
		va_end(va);
		}

	/* Construct the SNMP query packet. */
	packet_length = create_packet(p, buffer, items, items_count);

	/* Send and resent the request until we get a response or the retries
	   are exhausted. */
	{
	int attempt;
	fd_set rfds;
	struct timeval timeout;
	for(attempt=0; attempt < 5; attempt++)
		{
		/* Send the request. */
		#ifdef TEST
		printf("sending %d bytes...\n", packet_length);
		#endif
		if(send(p->socket, buffer, packet_length, 0) < 0)
			gu_Throw("%s() failed, errno=%d (%s)", "send", errno, gu_strerror(errno));

		/* Wait up to 1 second for a response. */
		FD_ZERO(&rfds);
		FD_SET(p->socket, &rfds);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if(select(p->socket + 1, &rfds, NULL, NULL, &timeout) < 0)
			gu_Throw("%s() failed, errno=%d (%s)", "select", errno, gu_strerror(errno));

		/* If there was nothing to read, start next iteration
		   (which will result in a resend. */
		if(!FD_ISSET(p->socket, &rfds))
			continue;

		/* Receive the packet. */
		if((p->result_len = recv(p->socket, p->result, sizeof(p->result), 0)) < 0)
			gu_Throw("%s() failed, errno=%d (%s)", "recv", errno, gu_strerror(errno));
		#ifdef TEST
		printf("Got %d bytes\n", p->result_len);
		#endif

		/* If the packet isn't an SNMP response, then we get -1,
		   for other errors an exception is thrown. */
		if(parse_response(p, items, items_count) != -1)
			return;
		} /* end of retry loop */
	}

	/* If we reach here, there was no valid response. */
	gu_Throw("no response");
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
	int n1;
	unsigned int n2;
	char *str;

	s = gu_snmp_open(inet_addr(argv[1]), "public");

	gu_snmp_get(s,
				"1.3.6.1.2.1.1.5.0", GU_SNMP_STR, &str,
				"1.3.6.1.2.1.25.3.5.1.1.1", GU_SNMP_INT, &n1,
				"1.3.6.1.2.1.25.3.5.1.2.1", GU_SNMP_BIT, &n2,
				NULL
				);

	printf("system name: %s\n", str);
	printf("printer status 1: %d\n", n1);
	printf("printer status 2: 0x%.2X\n", n2);
	gu_free(str);

	gu_snmp_close(s);

	return 0;
	}
#endif

/* end of file */
