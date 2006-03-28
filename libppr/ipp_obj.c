/*
** mouse:~ppr/src/ipp/ipp_obj.c
** Copyright 1995--2006, Trinity College Computing Center.
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
** Last modified 24 March 2006.
*/

/*! \file */

#include "config.h"
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "gu.h"
#include "global_defines.h"
#include "ipp_constants.h"
#include "ipp_utils.h"

/*! create IPP request handling object

This function creates an IPP service object.  The IPP request will be read
from stdin and the response will be sent to stdout.  IPP data of various
types can be read from the request and appended to the response using the
member functions.

*/
struct IPP *ipp_new(const char root[], const char path_info[], int content_length, int in_fd, int out_fd)
	{
	struct IPP *p = gu_alloc(1, sizeof(struct IPP));

	p->magic = 0xAABB;

	p->root = root;
	p->path_info = path_info;
	p->bytes_left = content_length;
	p->in_fd = in_fd;
	p->out_fd = out_fd;
	p->subst_reply_fd = -1;

	p->remote_user = NULL;
	p->remote_addr = NULL;

	p->readbuf_i = 0;
	p->readbuf_remaining = 0;
	p->readbuf_guard = 42;

	p->writebuf_i = 0;
	p->writebuf_remaining = sizeof(p->writebuf);
	p->writebuf_guard = 42;

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
	/*DEBUG(("ipp_readbuf_load(): p->bytes_left = %d", p->bytes_left));*/
	if((p->readbuf_remaining = read(p->in_fd, p->readbuf, p->bytes_left < sizeof(p->readbuf) ? p->bytes_left : sizeof(p->readbuf))) == -1)
		gu_Throw("%s() failed, errno=%d (%s)", "read", errno, strerror(errno));
	if(p->readbuf_remaining < 1)
		gu_Throw("premature EOF");
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

	DEBUG(("ipp_writebuf_flush(): %d bytes to flush to fd %d", p->writebuf_i, p->out_fd));
	
	to_write = p->writebuf_i;
	write_ptr = p->writebuf;

	while(to_write > 0)
		{
		DEBUG(("  trying to write %d bytes", to_write));
		if((len = write(p->out_fd, write_ptr, to_write)) == -1)
			gu_Throw("writing response failed, errno=%d (%s)", errno, gu_strerror(errno));
		DEBUG(("    wrote %d bytes", len));
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
	DEBUG(("ipp_delete(): %d leftover bytes", p->bytes_left + p->readbuf_remaining));

	if(p->magic != 0xAABB)
		gu_Throw("ipp_delete(): not an IPP object");
	if(p->readbuf_guard != 42)
		gu_Throw("ipp_delete(): readbuf overflow");
	if(p->writebuf_guard != 42)
		gu_Throw("ipp_delete(): writebuf overflow");
	
	while(p->bytes_left > 0)
		{
		ipp_readbuf_load(p);
		}

	ipp_writebuf_flush(p);

	/* This complicated code deletes all of the attributes.  Should we
	 * have used a pool?  Maybe.  We did uncover some interesting
	 * bugs though when we were writing this.
	 */
	{
	ipp_attribute_t *attribs[] = {
		p->request_attrs,
		p->response_attrs_operation,
		p->response_attrs_printer,
		p->response_attrs_job,
		p->response_attrs_unsupported
		};
	int x, y;
	for(x=0; x<5; x++)
		{
		ipp_attribute_t *at, *next_at;
		at = attribs[x];
		while(at)
			{
			if(at->free_name)
				gu_free(at->name);
			if(at->free_values)
				{
				switch(ipp_tag_simplify(at->value_tag))
					{
					case IPP_TAG_END:
						break;
					case IPP_TAG_INTEGER:
						break;
					case IPP_TAG_STRING:
						for(y=0; y < at->num_values; y++)
							{
							gu_free(at->values[y].string.text);
							}
						break;
					default:
						for(y=0; y < at->num_values; y++)
							gu_free(at->values[y].unknown.data);
						break;
					}
				}
			next_at = at->next;
			gu_free(at);
			at = next_at;
			}
		}
	}

	p->magic = 0;
	gu_free(p);
	}

/** set REMOTE_USER from CGI environment
*/
void ipp_set_remote_user(struct IPP *p, const char remote_user[])
	{
	p->remote_user = remote_user;
	}

/** set REMOTE_ADDR from CGI environment
*/
void ipp_set_remote_addr(struct IPP *p, const char remote_addr[])
	{
	p->remote_addr = remote_addr;
	}

/** Save the IPP request to a file
 *
 * Write the IPP request to a file in preparation for passing it through
 * to another process for handling.  Once you have done this, it is gone,
 * you can't parse any more of it using this object.
 */
void ipp_request_to_fd(struct IPP *p, int fd)
	{
	int writelen;
	DEBUG(("ipp_request_to_fd(): p->readbuf_remaining=%d, p->readbuf_i=%d, p->bytes_left=%d", p->readbuf_remaining, p->readbuf_i, p->bytes_left));
	p->readbuf_remaining += p->readbuf_i;	/* rewind */
	p->readbuf_i = 0;
	while(p->readbuf_remaining > 0)
		{
		while(p->readbuf_remaining > 0)
			{
			if((writelen = write(fd, p->readbuf + p->readbuf_i, p->readbuf_remaining)) == -1)
				gu_Throw("write() failed, errno=%d (%s)", errno, gu_strerror(errno));
			p->readbuf_remaining -= writelen;
			p->readbuf_i += writelen;
			}
		if(p->bytes_left <= 0)
			break;
		ipp_readbuf_load(p);
		}
	}

/** Provide IPP object with an open file descriptor containing a substitute reply
 *
 * If you open a file and pass the descriptor to this function, ipp_send_reply()
 * will send the content of the file in the stead of whatever it would have sent.
 */
void ipp_reply_from_fd(struct IPP *p, int fd)
	{
	p->subst_reply_fd = fd;
	}

/* This gets called if ipp_reply_from_fd() was called. */
static void send_subst_reply(struct IPP *p, gu_boolean header)
	{
	struct stat statbuf;
	int bytes_left;

	DEBUG(("sending substitute reply to fd %d", p->subst_reply_fd));
	
	if(fstat(p->subst_reply_fd, &statbuf) == -1)
		gu_Throw("fstat() failed, errno=%d (%s)", errno, gu_strerror(errno));
	bytes_left = statbuf.st_size;

	DEBUG(("substitute reply is %d bytes long", bytes_left));
	
	if(header)
		{
		gu_snprintf(p->writebuf, sizeof(p->writebuf),
			"Content-Type: application/ipp\r\n"
			"Content-Length: %d\r\n\r\n",
			bytes_left
			);
		p->writebuf_i = strlen(p->writebuf);
		ipp_writebuf_flush(p);
		}

	while(bytes_left > 0)
		{
		if((p->writebuf_i = read(p->subst_reply_fd, p->writebuf, bytes_left < sizeof(p->writebuf) ? bytes_left : sizeof(p->writebuf))) == -1)
			gu_Throw("read() failed, errno=%d (%s)", errno, gu_strerror(errno));
		bytes_left -= p->writebuf_i;
		ipp_writebuf_flush(p);
		}

	DEBUG(("done sending substitute reply"));
	} /* send_subst_reply() */

/** fetch a block from the IPP request

The character pointer pointed two by pptr is set to point to the next
block of the file.  The length of the block is returned.  This is used
to read the print file data.

*/
int ipp_get_block(struct IPP *p, char **pptr)
	{
	int len = 0;
	
	if(p->readbuf_remaining <= 0)
		{
		if(p->bytes_left <= 0)
			return 0;
		ipp_readbuf_load(p);
		}

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
char ipp_get_byte(struct IPP *p)
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
void ipp_put_byte(struct IPP *ipp, char val)
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
char *ipp_get_bytes(struct IPP *p, int len)
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
void ipp_put_bytes(struct IPP *ipp, const char *data, int len)
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

/** read the IPP request's header and store it in the IPP object
 *
 * This function retrieves the operation ID.  This is enough for
 * the ipp CGI to decide if it can handle the request itself or 
 * needs to pass it to pprd.  If it can handle the request itself,
 * it goes on to call ipp_parse_request_body().
*/
void ipp_parse_request_header(struct IPP *ipp)
	{
	if(ipp->bytes_left < 9)
		gu_Throw("request is too short to be an IPP request");

	DEBUG(("request for %s, %d bytes", ipp->path_info, ipp->bytes_left));

	ipp->version_major = ipp_get_sb(ipp);
	ipp->version_minor = ipp_get_sb(ipp);
	ipp->operation_id = ipp_get_ss(ipp);
	ipp->request_id = ipp_get_si(ipp);

	DEBUG(("version-number: %d.%d, operation-id: 0x%.4X (%s), request-id: %d",
		ipp->version_major, ipp->version_minor,
		ipp->operation_id, ipp_operation_to_str(ipp->operation_id),
		ipp->request_id
		));
	}

/** read more of the IPP request
 *
 * This function picks up where ipp_parse_request_header() left off and
 * reads up to (but not including) the print job data.
 */
void ipp_parse_request_body(struct IPP *ipp)
	{
	char *p;
	int tag, delimiter_tag = 0, value_tag, name_length, value_length;
	char *name = NULL;
	ipp_attribute_t *ap = NULL, **ap_resize = NULL;
	int ap_i = 0;

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

			DEBUG(("0x%.2x (%s) 0x%.2x (%s) name[%d]=\"%s\", value_len=%d",
				delimiter_tag, ipp_tag_to_str(delimiter_tag),
				value_tag, ipp_tag_to_str(value_tag),
				name_length,
				name ? name : "",
				value_length));

			if(name_length > 0)
				{
				/* Where is the pointer to this ipp_attribute_t going to be?  
				 * We need to know because we will have to adjust it if we
				 * have to enlarge this object in the future.
				 */
				if(ap)
					ap_resize = &ap->next;				/* in the previous one in the chain */
				else
					ap_resize = &ipp->request_attrs;	/* no previous one, it is in the IPP struct */

				/* We allocate an initial one with room for only one value.  Note that 
				   a pointer to it is written to the location determined above. */
				ap = *ap_resize = gu_alloc(1, sizeof(ipp_attribute_t));

				ap->next = NULL;					/* last in chain */
				ap->group_tag = delimiter_tag;
				ap->value_tag = value_tag;
				ap->name = name;
				ap->free_name = TRUE;
				ap->free_values = TRUE;
				ap->num_values = 1;					/* only one value (for now) */

				ap_i = 0;
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
				ap->num_values++;
				}

			/* Don't forget to keep ipp_delete() in sync with this! */
			switch(ipp_tag_simplify(value_tag))
				{
				case IPP_TAG_INTEGER:
					ap->values[ap_i].integer = ipp_get_si(ipp);
					DEBUG(("    %s[%d]=%d", ipp_tag_to_str(value_tag), value_length, ap->values[ap_i].integer));
					break;
				case IPP_TAG_STRING:
					p = ipp_get_bytes(ipp, value_length);
					ap->values[ap_i].string.text = p;
					DEBUG(("    %s[%d]=\"%s\"", ipp_tag_to_str(value_tag), value_length, p));
					break;
				default:
					p = ipp_get_bytes(ipp, value_length);
					ap->values[ap_i].unknown.length = value_length;
					ap->values[ap_i].unknown.data = p;
					DEBUG(("    %s[%d]", ipp_tag_to_str(value_tag), value_length));
					break;
				}
			}
		else
			{
			gu_Throw("invalid tag value 0x%.2x", tag);
			}
		}

    DEBUG(("end of request read"));
    
	/* This will be the default. */
    ipp->response_code = IPP_OK;

	} /* end of ipp_parse_request() */

/** validate a request, set an error if it is bad
*/
gu_boolean ipp_validate_request(struct IPP *ipp)
	{
	/* ipp_attribute_t *attr; */

	/* For now, English is all we are capable of. */
	ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_CHARSET, "attributes-charset", "utf-8", FALSE);
	ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE, "attributes-natural-language", "en", FALSE);

/*	if(!(attr = ipp_find_attribute(ipp, IPP_TAG_OPERATION, IPP_TAG_CHARSET, "attributes-charset")))
*/		

	return TRUE;
	}

/** append an attribute to the IPP response
*/
void ipp_put_attr(struct IPP *ipp, ipp_attribute_t *attr)
	{
	ipp_value_t *p;
	int i, len;
	
	for(i=0 ; i < attr->num_values; i++)
		{
		DEBUG(("  encoding 0x%.2x (%s) name=\"%s\"",
			attr->value_tag, ipp_tag_to_str(attr->value_tag),
			attr->name
			));

		p = &attr->values[i];
		ipp_put_byte(ipp, attr->value_tag);

		/* If this is the first value, we store the name. */
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

		/* prepend the "http://host:port" stuff to URLs. */
		if(attr->value_tag == IPP_TAG_URI)
			{
			if(attr->template)
				{
				char temp[64];

				if(strstr(attr->template, "%d"))
					gu_snprintf(temp, sizeof(temp), attr->template, p->integer);
				else
					gu_snprintf(temp, sizeof(temp), attr->template, p->string.text);

				DEBUG(("    \"%s\" + \"%s\"", ipp->root, temp));
				len = strlen(ipp->root) + strlen(temp);
				ipp_put_ss(ipp, len);
				ipp_put_bytes(ipp, ipp->root, strlen(ipp->root));
				ipp_put_bytes(ipp, temp, strlen(temp));
				continue;
				}
			else if(p->string.text[0] == '/')
				{
				DEBUG(("    \"%s\" + \"%s\"", ipp->root, p->string.text));
				len = strlen(ipp->root) + strlen(p->string.text);
				ipp_put_ss(ipp, len);
				ipp_put_bytes(ipp, ipp->root, strlen(ipp->root));
				ipp_put_bytes(ipp, p->string.text, strlen(p->string.text));
				continue;
				}
			}

		switch(ipp_tag_simplify(attr->value_tag))
			{
			case IPP_TAG_INTEGER:
				DEBUG(("    %d", p->integer));
				ipp_put_ss(ipp, 4);
				ipp_put_si(ipp, p->integer);
				break;
			case IPP_TAG_STRING:
				DEBUG(("    \"%s\"", p->string.text));
				len = strlen(p->string.text);
				ipp_put_ss(ipp, len);
				ipp_put_bytes(ipp, p->string.text, len);
				break;
			case IPP_TAG_BOOLEAN:
				DEBUG(("    %s", p->boolean ? "TRUE" : "FALSE"));
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
void ipp_send_reply(struct IPP *ipp, gu_boolean header)
	{
	ipp_attribute_t *p;
	
	DEBUG(("ipp_send_reply()"));

	if(ipp->subst_reply_fd != -1)
		{
		send_subst_reply(ipp, header);
		return;
		}

	if(header)
		ipp_put_string(ipp, "Content-Type: application/ipp\r\n\r\n");

	ipp_put_sb(ipp, 1);		/* version number 1.0 */
	ipp_put_sb(ipp, 0);

	DEBUG(("response_code: %d", ipp->response_code));
	ipp_put_ss(ipp, ipp->response_code);
	ipp_put_si(ipp, ipp->request_id);
	
	if(!(p = ipp->response_attrs_operation))
		gu_Throw("no response_attrs_operation");
	DEBUG(("encoding operation tags"));
	ipp_put_byte(ipp, IPP_TAG_OPERATION);
	for( ; p; p = p->next)
		{
		ipp_put_attr(ipp, p);
		}

	if((p = ipp->response_attrs_printer))
		{
		DEBUG(("encoding printer tags"));
		ipp_put_byte(ipp, IPP_TAG_PRINTER);
		for( ; p; p = p->next)
			{
			DEBUG((" printer:"));
			if(p->value_tag == IPP_TAG_END)
				{
				if(p->next)
					{
					ipp_put_byte(ipp, IPP_TAG_PRINTER);
					DEBUG(("-------------------------------------------------"));
					}
				}
			else
				ipp_put_attr(ipp, p);
			}
		}
	
	if((p = ipp->response_attrs_job))
		{
		DEBUG(("encoding job tags"));
		ipp_put_byte(ipp, IPP_TAG_JOB);

		for( ; p; p = p->next)
			{
			if(p->value_tag == IPP_TAG_END)
				{
				if(p->next)
					{
					ipp_put_byte(ipp, IPP_TAG_JOB);
					DEBUG(("-------------------------------------------------"));
					}
				}
			else
				ipp_put_attr(ipp, p);
			}
		}

	if((p = ipp->response_attrs_unsupported))
		{
		DEBUG(("encoding unsupported tags"));
		ipp_put_byte(ipp, IPP_TAG_UNSUPPORTED);
		for( ; p; p = p->next)
			{
			ipp_put_attr(ipp, p);
			}
		}

	ipp_put_byte(ipp, IPP_TAG_END);

	ipp_writebuf_flush(ipp);
	} /* end of ipp_send_reply() */

/*
** add an attribute to the IPP response
*/
static ipp_attribute_t *ipp_add_attribute(struct IPP *ipp, int group, int tag, const char name[], int num_values)
	{
	ipp_attribute_t **ap1;
	ipp_attribute_t	*ap = gu_alloc(1, sizeof(ipp_attribute_t) + sizeof(ipp_value_t) * (num_values - 1));
	ap->next = NULL;
	ap->group_tag = 0;				/* consider this unused */
	ap->value_tag = tag;
	ap->name = (char*)name;
	ap->template = NULL;
	ap->free_name = FALSE;			/* we are probably only borrowing the name */
	ap->free_values = FALSE;		/* we are probably borrowing these too */
	ap->num_values = num_values;

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
		default:
			gu_Throw("assertion failed");
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

/** copy an attribute from the request to the response
 */
void ipp_copy_attribute(struct IPP *ipp, int group, ipp_attribute_t *attr)
	{
	ipp_attribute_t *new_attr = ipp_add_attribute(ipp, group, attr->value_tag, attr->name, attr->num_values);
	memcpy(new_attr->values, attr->values, sizeof(attr->values[0]) * attr->num_values);
	}

/** add an object divider to the IPP response
*/
void ipp_add_end(struct IPP *ipp, int group)
	{
	ipp_add_attribute(ipp, group, IPP_TAG_END, NULL, 1);	/* !!! is 1 correct? !!! */
	}

/** add an integer to the IPP response
*/
void ipp_add_integer(struct IPP *ipp, int group, int tag, const char name[], int value)
	{
	ipp_attribute_t *ap;
	if(ipp_tag_simplify(tag) != IPP_TAG_INTEGER && tag != IPP_TAG_URI)
		gu_Throw("ipp_add_integer(): %s is a %s", name, ipp_tag_to_str(tag));
	ap = ipp_add_attribute(ipp, group, tag, name, 1);
	ap->values[0].integer = value;
	}

/** add a list of integers to the IPP response
*/
void ipp_add_integers(struct IPP *ipp, int group, int tag, const char name[], int num_values, int values[])
	{
	ipp_attribute_t *ap;
	int i;
	if(ipp_tag_simplify(tag) != IPP_TAG_INTEGER && tag != IPP_TAG_URI)
		gu_Throw("ipp_add_integer(): %s is a %s", name, ipp_tag_to_str(tag));
	ap = ipp_add_attribute(ipp, group, tag, name, num_values);
	for(i=0; i<num_values; i++)
		ap->values[i].integer = values[i];
	}

/** add a string to the IPP response
 *
 * This function keeps a pointer to name[] and value[], so they had better not
 * change!
*/
void ipp_add_string(struct IPP *ipp, int group, int tag, const char name[], const char value[], gu_boolean free_value)
	{
	ipp_attribute_t *ap;
	if(ipp_tag_simplify(tag) != IPP_TAG_STRING)
		gu_Throw("ipp_add_string(): %s is a %s", name, ipp_tag_to_str(tag));
	ap = ipp_add_attribute(ipp, group, tag, name, 1);
	ap->values[0].string.text = (char*)value;
	ap->free_values = free_value;
	}

/** add a list of strings to the IPP response
 *
 * This function keeps a pointer to name[] and all of the values[], so they 
 * had better not change!
*/
void ipp_add_strings(struct IPP *ipp, int group, int tag, const char name[], int num_values, const char *values[], gu_boolean free_values)
	{
	ipp_attribute_t *ap;
	int i;
	if(ipp_tag_simplify(tag) != IPP_TAG_STRING)
		gu_Throw("ipp_add_strings(): %s is a %s", name, ipp_tag_to_str(tag));
	ap = ipp_add_attribute(ipp, group, tag, name, num_values);
	for(i=0; i<num_values; i++)
		ap->values[i].string.text = (char*)values[i];
	ap->free_values = free_values;
	}

/** add a formatted string to the IPP response
*/
void ipp_add_printf(struct IPP *ipp, int group, int tag, const char name[], const char value[], ...)
	{
	ipp_attribute_t *ap;
	va_list va;
	char *p;

	if(ipp_tag_simplify(tag) != IPP_TAG_STRING)
		gu_Throw("ipp_add_printf(): %s is a %s", name, ipp_tag_to_str(tag));

	ap = ipp_add_attribute(ipp, group, tag, name, 1);

	va_start(va, value);
	gu_vasprintf(&p, value, va);
	ap->values[0].string.text = p;
	va_end(va);

	ap->free_values = TRUE;		/* copy belongs to object */
	}

/** add a very basic formatted string to the IPP response
*/
void ipp_add_template(struct IPP *ipp, int group, int tag, const char name[], const char template[], ...)
	{
	ipp_attribute_t *ap;
	va_list va;

	if(ipp_tag_simplify(tag) != IPP_TAG_STRING)
		gu_Throw("ipp_add_printf(): %s is a %s", name, ipp_tag_to_str(tag));

	ap = ipp_add_attribute(ipp, group, tag, name, 1);

	ap->template = template;

	va_start(va, template);
	if(strstr(template, "%d"))
		ap->values[0].integer = va_arg(va, int);
	else
		ap->values[0].string.text = va_arg(va, char*);
	va_end(va);
	}

/** add a boolean  to the IPP response 
*/
void ipp_add_boolean(struct IPP *ipp, int group, int tag, const char name[], gu_boolean value)
	{
	ipp_attribute_t *ap;
	if(tag != IPP_TAG_BOOLEAN)
		gu_Throw("ipp_add_boolean(): %s is a %s", name, ipp_tag_to_str(tag));
	ap = ipp_add_attribute(ipp, group, tag, name, 1);
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

/* end of file */
