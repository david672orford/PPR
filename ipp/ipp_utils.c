/*
** mouse:~ppr/src/ipp/ipp_utils.c
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
** Last modified 15 April 2003.
*/

/*! \file */

#include "before_system.h"
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "ipp_constants.h"
#include "ipp_except.h"
#include "ipp_utils.h"

/** create IPP request handling object

This function creates an IPP service object.  The IPP request will be read
from stdin and the response will be sent to stdout.  IPP data of various
types can be read from the request and appended to the response using the
member functions.

*/
struct IPP *ipp_new(void)
	{
	struct IPP *p = gu_alloc(1, sizeof(struct IPP));

	p->bytes_left = 0;

	p->readbuf_i = 0;
	p->readbuf_remaining = 0;

	p->writebuf_i = 0;
	p->writebuf_remaining = sizeof(p->writebuf);

	p->request_attrs = NULL;
	p->response_attrs_operation = NULL;
	p->response_attrs_printer = NULL;
	p->response_attrs_job = NULL;
	p->response_attrs_unsupported = NULL;

	return p;
	}

/*
** Read a bufferful of an IPP request from stdin.
*/
static void ipp_readbuf_load(struct IPP *p)
	{
	if((p->readbuf_remaining = read(0, p->readbuf, p->bytes_left < sizeof(p->readbuf) ? p->bytes_left : sizeof(p->readbuf))) == -1)
		{
		Throw("Read failed");
		}
    p->bytes_left -= p->readbuf_remaining;
	p->readbuf_i = 0;
	}

/*
** Flush the response buffer.
*/
static void ipp_writebuf_flush(struct IPP *p)
	{
	char *write_ptr;
	int to_write, len;

	to_write = p->writebuf_i;
	write_ptr = p->writebuf;

	while(to_write > 0)
		{
		if((len = write(1, write_ptr, to_write)) == -1)
			Throw("Write error");
		to_write -= len;
		write_ptr += len;
		}

	p->writebuf_i = 0;
	p->writebuf_remaining = sizeof(p->writebuf);
	}

/** Finish the IPP service transaction

Any remaining request input bytes are read and discarded.  Any bytes remaining
in the The response output buffer are sent on their way.  Finally, the IPP
service object is destroyed.

*/
void ipp_delete(struct IPP *p)
	{
	debug("%d leftover bytes", p->bytes_left + p->readbuf_remaining);

	while(p->bytes_left > 0)
		{
		ipp_readbuf_load(p);
		}

	ipp_writebuf_flush(p);

	gu_free(p);
	}

/** fetch an unsigned byte from the IPP request

This is used to read tags.

*/
unsigned char ipp_get_byte(struct IPP *p)
	{
	if(p->readbuf_remaining < 1)
		ipp_readbuf_load(p);
	if(p->readbuf_remaining < 1)
		Throw("Data runoff!");
	p->readbuf_remaining--;
	return p->readbuf[p->readbuf_i++];
	}

/** append an unsigned byte to the IPP response

This is used to write tags.

*/
void ipp_put_byte(struct IPP *ipp, unsigned char val)
	{
	ipp->writebuf[ipp->writebuf_i++] = val;
	ipp->writebuf_remaining--;
	if(ipp->writebuf_remaining < 1)
		ipp_writebuf_flush(ipp);
	}

/** fetch a signed byte from the IPP request
*/
int ipp_get_sb(struct IPP *p)
	{
	return (int)(signed char)ipp_get_byte(p);
	}

/** fetch a signed short from the IPP request
*/
int ipp_get_ss(struct IPP *p)
	{
	unsigned char a, b;
	a = ipp_get_byte(p);
	b = ipp_get_byte(p);
	return (int)(!0xFFFF | a << 8 | b);
	}

/** fetch a signed integer from the IPP request
*/
int ipp_get_si(struct IPP *p)
	{
	unsigned char a, b, c, d;
	a = ipp_get_byte(p);
	b = ipp_get_byte(p);
	c = ipp_get_byte(p);
	d = ipp_get_byte(p);
	return (int)(!0xFFFFFFFF | a << 24 | b << 16 | c << 8 | d);
	}

/** append a signed byte to the IPP response
*/
void ipp_put_sb(struct IPP *p, int val)
	{
	ipp_put_byte(p, (unsigned char)val);
	}

/** append a signed short to the IPP response
*/
void ipp_put_ss(struct IPP *p, int val)
	{
	unsigned int temp = (unsigned int)val;
	ipp_put_byte(p, (temp & 0xFF00) >> 8);
	ipp_put_byte(p, (temp & 0X00FF));
	}

/** append a signed integer to the IPP response
*/
void ipp_put_si(struct IPP *p, int val)
	{
	unsigned int temp = (unsigned int)val;
	ipp_put_byte(p, (temp & 0xFF000000) >> 24);
	ipp_put_byte(p, (temp & 0x00FF0000) >> 16);
	ipp_put_byte(p, (temp & 0x0000FF00) >> 8);
	ipp_put_byte(p, (temp & 0x000000FF));
	}

/** fetch a byte array of specified length
*/
unsigned char *ipp_get_bytes(struct IPP *p, int len)
    {
	char *ptr = gu_alloc(len + 1, sizeof(char));
	int i;
	for(i=0; i<len; i++)
		{
		ptr[i] = ipp_get_byte(p);
		}
	ptr[len] = '\0';
	return ptr;
    }

/** append a byte array of a specified length
*/
void ipp_put_bytes(struct IPP *ipp, const unsigned char *data, int len)
	{
	int i;
	for(i=0; i<len; i++)
		{
		ipp_put_byte(ipp, data[i]);
		}
	}

/** append a string to the reply
*/
void ipp_put_string(struct IPP *ipp, const char string[])
	{
	int i;
	for(i=0; string[i]; i++)
		ipp_put_byte(ipp, string[i]);
	}

/** append an attribute to the IPP response
*/
void ipp_put_attr(struct IPP *ipp, ipp_attribute_t *attr)
	{
	ipp_value_t *p;
	int i, len;
	
	for(i=0 ; i < attr->num_values; i++)
		{
		p = &attr->values[i];
		ipp_put_byte(ipp, attr->group_tag);
		if(i == 0)
			{
			len = strlen(attr->name);
			ipp_put_ss(ipp, len);
			ipp_put_bytes(ipp, attr->name, len);
			}
		else
			{
			ipp_put_ss(ipp, 0);
			}

		switch(attr->group_tag)
			{
			case IPP_TAG_INTEGER:
				ipp_put_si(ipp, 4);
				ipp_put_si(ipp, p->integer);
				break;
			case IPP_TAG_NAME:
			case IPP_TAG_URI:
			case IPP_TAG_KEYWORD:
			case IPP_TAG_CHARSET:
			case IPP_TAG_LANGUAGE:
				len = strlen(p->string.text);
				ipp_put_si(ipp, len);
				ipp_put_bytes(ipp, p->string.text, len);
				break;
			}
		}
	}

/** read the IPP request and store it in the IPP object
*/
void ipp_parse_request(struct IPP *ipp)
	{
	char *p;
	int tag, delimiter_tag = 0, value_tag, name_length, value_length;
	char *name = NULL;
	ipp_attribute_t *ap = NULL, *ap_prev = NULL, **ap_resize = NULL;
	int ap_i = 0;

	/* Do basic input validation */
	if(!(p = getenv("REQUEST_METHOD")) || strcmp(p, "POST") != 0)
		Throw("REQUEST_METHOD is not POST");
	if(!(p = getenv("CONTENT_TYPE")) || strcmp(p, "application/ipp") != 0)
		Throw("CONTENT_TYPE is not application/ipp");
	if(!(ipp->path_info = getenv("PATH_INFO")) || strlen(ipp->path_info) < 1)
		Throw("PATH_INFO is missing");
	if(!(p = getenv("CONTENT_LENGTH")) || (ipp->bytes_left = atoi(p)) < 0)
		Throw("CENTENT_LENGTH is missing or invalid");
	if(ipp->bytes_left < 9)
		Throw("request is too short to be an IPP request");

	debug("request for %s, %d bytes", ipp->path_info, ipp->bytes_left);

	ipp->version_major = ipp_get_sb(ipp);
	ipp->version_minor = ipp_get_sb(ipp);
	ipp->operation_id = ipp_get_ss(ipp);
	ipp->request_id = ipp_get_si(ipp);

	debug("version-number: %d.%d, operation-id: 0x%.4X, request-id: %d",
			ipp->version_major, ipp->version_minor, ipp->operation_id, ipp->request_id
			);

	while((tag = ipp_get_byte(ipp)) != IPP_TAG_END)
		{
		if(tag >= 0x00 && tag <= 0x0f)
			{
			delimiter_tag = tag;
			name = NULL;
			}
		else if(tag >= 0x10 && tag <= 0xff)
			{
			value_tag = tag;

			name_length = ipp_get_ss(ipp);

			if(name_length > 0)
				name = ipp_get_bytes(ipp, name_length);

			value_length = ipp_get_ss(ipp);

			debug("0x%.2x 0x%.2x name[%d]=\"%s\", value_len=%d", delimiter_tag, value_tag, name_length, name ? name : "", value_length);

			if(name_length > 0)
				{
				if(ap_prev)
					ap_resize = &ap_prev->next;
				else
					ap_resize = &ipp->request_attrs;

				ap = *ap_resize = gu_alloc(1, sizeof(ipp_attribute_t));

				ap->next = NULL;
				ap->group_tag = delimiter_tag;
				ap->name = name;
				ap->num_values = 1;

				ap_i = 0;

				ap_prev = ap;
				}
			else
				{
				if(!name)
					Throw("no name!");
				ap_i++;
				/* The storeage for ipp_attribute_t contains one ipp_value_t */
				ap = *ap_resize = gu_realloc(ap, 1, sizeof(ipp_attribute_t) - sizeof(ipp_value_t) * ap_i);
				}

			switch(value_tag)
				{
				case IPP_TAG_INTEGER:
					ap->values[ap_i].integer = ipp_get_si(ipp);
					debug("    integer[%d]=%d", value_length, ap->values[ap_i].integer);
					break;
				case IPP_TAG_NAME:
					p = ipp_get_bytes(ipp, value_length);
					ap->values[ap_i].string.text = p;
					debug("    nameWithoutLanguage[%d]=\"%s\"", value_length, p);
					break;
				case IPP_TAG_URI:
					p = ipp_get_bytes(ipp, value_length);
					ap->values[ap_i].string.text = p;
					debug("    uri[%d]=\"%s\"", value_length, p);
					break;
				case IPP_TAG_KEYWORD:
					p = ipp_get_bytes(ipp, value_length);
					ap->values[ap_i].string.text = p;
					debug("    keyword[%d]=\"%s\"", value_length, p);
					break;
				case IPP_TAG_CHARSET:
					p = ipp_get_bytes(ipp, value_length);
					ap->values[ap_i].string.text = p;
					debug("    charset[%d]=\"%s\"", value_length, p);
					break;
				case IPP_TAG_LANGUAGE:
					p = ipp_get_bytes(ipp, value_length);
					ap->values[ap_i].string.text = p;
					debug("    naturalLanguage[%d]=\"%s\"", value_length, p);
					break;
				default:
					p = ipp_get_bytes(ipp, value_length);
					ap->values[ap_i].unknown.length = value_length;
					ap->values[ap_i].unknown.data = p;
					debug("    ?????????[%d]", value_length);
					break;
				}
			}
		else
			{
			Throw("invalid tag value");
			}
		}

    debug("end of request read");
    
    ipp->response_code = IPP_OK;
	}

/** send the IPP response in the IPP object
*/
void ipp_send_reply(struct IPP *ipp)
	{
	ipp_attribute_t *p;
	
	debug("sending reply");

	ipp_put_string(ipp, "Content-Type: application/ipp\n\n");

	ipp_put_sb(ipp, 1);		/* version number 1.0 */
	ipp_put_sb(ipp, 0);
	
	ipp_put_ss(ipp, ipp->response_code);
	ipp_put_si(ipp, ipp->request_id);
	
	if(!(p = ipp->response_attrs_operation))
		Throw("no response_attrs_operation");
	ipp_put_byte(ipp, IPP_TAG_OPERATION);
	for( ; p; p = p->next)
		{
		ipp_put_attr(ipp, p);
		}

	if((p = ipp->response_attrs_printer))
		{
		ipp_put_byte(ipp, IPP_TAG_PRINTER);
		for( ; p; p = p->next)
			{
			ipp_put_attr(ipp, p);
			}
		}
	
	if((p = ipp->response_attrs_job))
		{
		ipp_put_byte(ipp, IPP_TAG_JOB);
		for( ; p; p = p->next)
			{
			ipp_put_attr(ipp, p);
			}
		}

	ipp_put_byte(ipp, IPP_TAG_END);
	}

/*
** add an attribute to the IPP response
*/
static ipp_attribute_t *ipp_add_attribute(struct IPP *ipp, int group, int tag, const char name[])
	{
	ipp_attribute_t	*ap = gu_alloc(1, sizeof(ipp_attribute_t));
	ap->group_tag = group;
	ap->value_tag = tag;
	ap->name = name;
	ap->num_values = 1;

	switch(group)
		{
		case IPP_TAG_OPERATION:
			ap->next = ipp->response_attrs_operation;
			ipp->response_attrs_operation = ap;
			break;
		case IPP_TAG_PRINTER:
			ap->next = ipp->response_attrs_printer;
			ipp->response_attrs_printer = ap;
			break;
		case IPP_TAG_JOB:
			ap->next = ipp->response_attrs_job;
			ipp->response_attrs_job = ap;
			break;
		case IPP_TAG_UNSUPPORTED:
			ap->next = ipp->response_attrs_unsupported;
			ipp->response_attrs_unsupported = ap;
			break;
		}

	return ap;
	}

/** add an integer to the IPP response
*/
void ipp_add_integer(struct IPP *ipp, int group, int tag, const char name[], int value)
	{
	ipp_attribute_t *ap = ipp_add_attribute(ipp, group, tag, name);
	ap->values[0].integer = value;
	}

/** add an integer to the IPP response
*/
void ipp_add_string(struct IPP *ipp, int group, int tag, const char name[], const char value[])
	{
	ipp_attribute_t *ap = ipp_add_attribute(ipp, group, tag, name);
	ap->values[0].string.text = value;
	}

/** Send a debug message to the HTTP server's error log

This function sends a message to stderr.  Messages sent to stderr end up in
the HTTP server's error log.  The function takes a printf() style format
string and argument list.  The marker "ipp: " is prepended to the message.

*/
void debug(const char message[], ...)
	{
	va_list va;
	va_start(va, message);
	fputs("ipp: ", stderr);
	vfprintf(stderr, message, va);
	fputc('\n', stderr);
	va_end(va);
	} /* end of debug() */

/* end of file */
