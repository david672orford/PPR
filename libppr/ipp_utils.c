/*
** mouse:~ppr/src/ipp/ipp_utils.c
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
** Last modified 29 January 2004.
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
#include "ipp_utils.h"

/** create IPP request handling object

This function creates an IPP service object.  The IPP request will be read
from stdin and the response will be sent to stdout.  IPP data of various
types can be read from the request and appended to the response using the
member functions.

*/
struct IPP *ipp_new(const char path_info[], int content_length, int in_fd, int out_fd)
	{
	struct IPP *p = gu_alloc(1, sizeof(struct IPP));

	p->path_info = path_info;
	p->bytes_left = content_length;
	p->in_fd = in_fd;
	p->out_fd = out_fd;

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
	if((p->readbuf_remaining = read(p->in_fd, p->readbuf, p->bytes_left < sizeof(p->readbuf) ? p->bytes_left : sizeof(p->readbuf))) == -1)
		{
		gu_Throw("Read failed");
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
		if((len = write(p->out_fd, write_ptr, to_write)) == -1)
			gu_Throw("Write error");
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

/*
** Convert a tag to a string (for debugging purposes).  The names printed
** are from the RFC 2565.
*/
static const char *tag_to_str(int tag)
	{
	switch(tag)
		{
		case IPP_TAG_OPERATION:
			return "operation-attributes-tag";
		case IPP_TAG_JOB:
			return "job-attributes-tag";
		case IPP_TAG_END:
			return "end-of-attributes-tag";
		case IPP_TAG_PRINTER:
			return "printer-attributes-tag";
		case IPP_TAG_UNSUPPORTED:
			return "unsupported-attributes-tag";

		case IPP_TAG_INTEGER:
			return "integer";
		case IPP_TAG_BOOLEAN:
			return "boolean";
		case IPP_TAG_ENUM:
			return "enum";

		case IPP_TAG_STRING:
			return "octetString";

		case IPP_TAG_TEXT:
			return "text";
		case IPP_TAG_NAME:
			return "name";
		case IPP_TAG_KEYWORD:
			return "keyword";
		case IPP_TAG_URI:
			return "uri";
		case IPP_TAG_CHARSET:
			return "charset";
		case IPP_TAG_LANGUAGE:
			return "naturalLanguage";

		default:
			return "unknown";
		}
	}

/*
** Convert an operation code to a string.
*/
static const char *operation_to_str(int op)
	{
	switch(op)
		{
		case IPP_PRINT_JOB:
			return "Print-Job";
		case IPP_PRINT_URI:
			return "Print-URI";
		case IPP_VALIDATE_JOB:
			return "Validate-Job";
		case IPP_CREATE_JOB:
			return "Create-Job";
		case IPP_SEND_DOCUMENT:
			return "Send-Document";
		case IPP_SEND_URI:
			return "Send-URI";
		case IPP_CANCEL_JOB:
			return "Cancel-Job";
		case IPP_GET_JOB_ATTRIBUTES:
			return "Get-Job-Attributes";
		case IPP_GET_JOBS:
			return "Get-Jobs";
		case IPP_GET_PRINTER_ATTRIBUTES:
			return "Get-Printer-Attributes";
		case IPP_HOLD_JOB:
			return "Get-Jobs";
		case IPP_RELEASE_JOB:
			return "Release-Job";
		case IPP_RESTART_JOB:
			return "Restart-Job";
		case IPP_PAUSE_PRINTER:
			return "Pause-Printer";
		case IPP_RESUME_PRINTER:
			return "Resume-Printer";
		case IPP_PURGE_JOBS:
			return "Purge-Jobs";
		case IPP_SET_PRINTER_ATTRIBUTES:
			return "Set-Printer-Attributes";
		case IPP_SET_JOB_ATTRIBUTES:
			return "Set-Job-Attributes";
		case IPP_GET_PRINTER_SUPPORTED_VALUES:
			return "Get-Printer-Supported-Values";
		case CUPS_GET_DEFAULT:
			return "CUPS-Get-Default";
		case CUPS_GET_PRINTERS:
			return "CUPS-Get-Printers";
		case CUPS_ADD_PRINTER:
			return "CUPS-Add-Printer";
		case CUPS_DELETE_PRINTER:
			return "CUPS-Delete-Printer";
		case CUPS_GET_CLASSES:
			return "CUPS-Get-Classes";
		case CUPS_ADD_CLASS:
			return "CUPS-Add-Class";
		case CUPS_DELETE_CLASS:
			return "CUPS-Delete-Class";
		case CUPS_ACCEPT_JOBS:
			return "CUPS-Accept-Jobs";
		case CUPS_REJECT_JOBS:
			return "CUPS-Reject-Jobs";
		case CUPS_SET_DEFAULT:
			return "CUPS-Set-Default";
		case CUPS_GET_DEVICES:
			return "CUPS-Get-Devices";
		case CUPS_GET_PPDS:
			return "CUPS-Get-PPDS";
		case CUPS_MOVE_JOB:
			return "CUPS-Move-Job";
		default:
			return "unknown";
		}
	} /* end of operation_to_str() */

/** fetch a block from the IPP request

This is used to read file data.

*/
int ipp_get_block(struct IPP *p, char **pptr)
	{
	int len = 0;
	
	if(p->readbuf_remaining < 1)
		ipp_readbuf_load(p);

	if(p->readbuf_remaining > 0)
		{
		*pptr = &p->readbuf[p->readbuf_i];
		len = p->readbuf_remaining;
		p->readbuf_i += len;
		p->readbuf_remaining = 0;
		}

	return len;
	}

/** fetch an unsigned byte from the IPP request

This is used to read tags.

*/
unsigned char ipp_get_byte(struct IPP *p)
	{
	if(p->readbuf_remaining < 1)
		ipp_readbuf_load(p);
	if(p->readbuf_remaining < 1)
		gu_Throw("Data runoff!");
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

/** read the IPP request and store it in the IPP object
*/
void ipp_parse_request(struct IPP *ipp)
	{
	char *p;
	int tag, delimiter_tag = 0, value_tag, name_length, value_length;
	char *name = NULL;
	ipp_attribute_t *ap = NULL, *ap_prev = NULL, **ap_resize = NULL;
	int ap_i = 0;

	if(ipp->bytes_left < 9)
		gu_Throw("request is too short to be an IPP request");

	debug("request for %s, %d bytes", ipp->path_info, ipp->bytes_left);

	ipp->version_major = ipp_get_sb(ipp);
	ipp->version_minor = ipp_get_sb(ipp);
	ipp->operation_id = ipp_get_ss(ipp);
	ipp->request_id = ipp_get_si(ipp);

	debug("version-number: %d.%d, operation-id: 0x%.4X (%s), request-id: %d",
			ipp->version_major, ipp->version_minor,
			ipp->operation_id, operation_to_str(ipp->operation_id),
			ipp->request_id
			);

	while((tag = ipp_get_byte(ipp)) != IPP_TAG_END)
		{
		if(tag >= 0x00 && tag <= 0x0f)		/* changes interpretation of subsequent tags */
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

			debug("0x%.2x (%s) 0x%.2x (%s) name[%d]=\"%s\", value_len=%d",
				delimiter_tag, tag_to_str(delimiter_tag),
				value_tag, tag_to_str(value_tag),
				name_length,
				name ? name : "",
				value_length);

			if(name_length > 0)
				{
				/* Where is the pointer to this ipp_attribute_t going to be?  We need to know because
				   we will have to adjust it if we have to enlarge this object in the future. */
				if(ap_prev)
					ap_resize = &ap_prev->next;				/* in the previous one in the chain */
				else
					ap_resize = &ipp->request_attrs;		/* no previous one, it is in the IPP struct */

				/* We allocate an initial one with room for only one value.  Note that 
				   a pointer to it is written to the location determined above. */
				ap = *ap_resize = gu_alloc(1, sizeof(ipp_attribute_t));

				ap->next = NULL;					/* last in chain */
				ap->group_tag = delimiter_tag;
				ap->value_tag = value_tag;
				ap->name = name;
				ap->num_values = 1;					/* only one value (for now) */

				ap_i = 0;

				ap_prev = ap;
				}
			else			/* same name as last one */
				{
				/* If there wasn't a previous one, something is seriously wrong. */
				if(!name)
					gu_Throw("no name!");

				ap_i++;		/* advance value array index */

				/* Expand the storeage to make room for an additional ipp_value_t.  Because the storeage 
				   for ipp_attribute_t contains one ipp_value_t, we don't need to use one more than ap_i.
				   */
				ap = *ap_resize = gu_realloc(ap, 1, sizeof(ipp_attribute_t) + sizeof(ipp_value_t) * ap_i);
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
				case IPP_TAG_MIMETYPE:
					p = ipp_get_bytes(ipp, value_length);
					ap->values[ap_i].string.text = p;
					debug("    mimeMediaType[%d]=\"%s\"", value_length, p);
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
			gu_Throw("invalid tag value 0x%.2x", tag);
			}
		}

    debug("end of request read");
    
	/* This will be the default. */
    ipp->response_code = IPP_OK;

	} /* end of ipp_parse_request() */

/** append an attribute to the IPP response
*/
void ipp_put_attr(struct IPP *ipp, ipp_attribute_t *attr)
	{
	ipp_value_t *p;
	int i, len;
	
	for(i=0 ; i < attr->num_values; i++)
		{
		debug("  encoding 0x%.2x (%s) name=\"%s\"",
			attr->value_tag, tag_to_str(attr->value_tag),
			attr->name
			);

		p = &attr->values[i];
		ipp_put_byte(ipp, attr->value_tag);

		/* If this is the first value, we need to store the name. */
		if(i == 0)
			{
			len = strlen(attr->name);
			ipp_put_ss(ipp, len);
			ipp_put_bytes(ipp, attr->name, len);
			}

		/* Otherwise, we store a zero-length name which means "same name as previous". */
		else
			{
			ipp_put_ss(ipp, 0);
			}

		switch(attr->value_tag)
			{
			case IPP_TAG_INTEGER:
			case IPP_TAG_ENUM:
				ipp_put_ss(ipp, 4);
				ipp_put_si(ipp, p->integer);
				break;
			case IPP_TAG_TEXT:
			case IPP_TAG_NAME:
			case IPP_TAG_KEYWORD:
			case IPP_TAG_URI:
			case IPP_TAG_CHARSET:
			case IPP_TAG_LANGUAGE:
			case IPP_TAG_MIMETYPE:
				len = strlen(p->string.text);
				ipp_put_ss(ipp, len);
				ipp_put_bytes(ipp, p->string.text, len);
				break;
			case IPP_TAG_BOOLEAN:
				ipp_put_ss(ipp, 1);
				ipp_put_sb(ipp, p->boolean ? 1 : 0);
				break;
			default:
				gu_Throw("ipp_put_attr(): missing case for value tag 0x%.2x in ipp_put_attr()", attr->value_tag);
			}
		}
	} /* end of ipp_put_attr() */

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
		gu_Throw("no response_attrs_operation");
	debug("encoding operation tags");
	ipp_put_byte(ipp, IPP_TAG_OPERATION);
	for( ; p; p = p->next)
		{
		ipp_put_attr(ipp, p);
		}

	if((p = ipp->response_attrs_printer))
		{
		debug("encoding printer tags");
		ipp_put_byte(ipp, IPP_TAG_PRINTER);
		for( ; p; p = p->next)
			{
			if(p->value_tag == IPP_TAG_END)
				{
				if(p->next)
					ipp_put_byte(ipp, IPP_TAG_PRINTER);
				}
			else
				ipp_put_attr(ipp, p);
			}
		}
	
	if((p = ipp->response_attrs_job))
		{
		debug("encoding job tags");
		ipp_put_byte(ipp, IPP_TAG_JOB);
		for( ; p; p = p->next)
			{
			if(p->value_tag == IPP_TAG_END)
				{
				if(p->next)
					ipp_put_byte(ipp, IPP_TAG_JOB);
				}
			else
				ipp_put_attr(ipp, p);
			}
		}

	if((p = ipp->response_attrs_unsupported))
		{
		debug("encoding unsupported tags");
		ipp_put_byte(ipp, IPP_TAG_UNSUPPORTED);
		for( ; p; p = p->next)
			{
			ipp_put_attr(ipp, p);
			}
		}

	ipp_put_byte(ipp, IPP_TAG_END);
	} /* end of ipp_send_reply() */

/*
** add an attribute to the IPP response
*/
static ipp_attribute_t *ipp_add_attribute(struct IPP *ipp, int group, int tag, const char name[])
	{
	ipp_attribute_t **ap1;
	ipp_attribute_t	*ap = gu_alloc(1, sizeof(ipp_attribute_t));
	ap->group_tag = 0;		/* consider this unused */
	ap->value_tag = tag;
	ap->name = name;
	ap->num_values = 1;
	ap->next = NULL;

	switch(group)
		{
		case IPP_TAG_OPERATION:
			ap1 = &ipp->response_attrs_operation;
			break;
		case IPP_TAG_PRINTER:
			ap1 = &ipp->response_attrs_printer;
			break;
		case IPP_TAG_JOB:
			ap1 = &ipp->response_attrs_job;
			break;
		case IPP_TAG_UNSUPPORTED:
			ap1 = &ipp->response_attrs_unsupported;
			break;
		}

	/* chug to the end */
	while(*ap1)
		{
		ap1 = &(*ap1)->next;
		}

	/* update the next pointer to point to this new one. */
	*ap1 = ap;

	return ap;
	} /* end of ipp_add_attr() */

/** add an object divider to the IPP response
*/
void ipp_add_end(struct IPP *ipp, int group)
	{
	ipp_add_attribute(ipp, group, IPP_TAG_END, NULL);
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

/** add an enum to the IPP response 
*/
void ipp_add_boolean(struct IPP *ipp, int group, int tag, const char name[], gu_boolean value)
	{
	ipp_attribute_t *ap = ipp_add_attribute(ipp, group, tag, name);
	ap->values[0].boolean = value;
	}

/** find an attribute in the IPP request
*/
ipp_attribute_t *ipp_find_attribute(struct IPP *ipp, int group, int tag, const char name[])
	{
	ipp_attribute_t *p;
	
	for(p = ipp->request_attrs; p; p = p->next)
		{
		if(p->group_tag == group && p->value_tag == tag && strcmp(p->name, name) == 0)
			break;
		}
		
	return p;
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
