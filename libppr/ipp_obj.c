/*
** mouse:~ppr/src/ipp/ipp_obj.c
** Copyright 1995--2012, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified: 4 September 2012
*/

/*! \file */

#include "config.h"
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include "gu.h"
#include "global_defines.h"
#include "ipp_constants.h"
#include "ipp_utils.h"

/* If this is enabled, debugging code is compiled in and the
 * caller must provided a callback function debug().  No 
 * debugging callbacks will actually be made until the caller
 * sets the debug level to something greater than 0 when XML_DEBUG()
 * begins to work or to 5 or greater when DODEBUG() starts to work.
 */
#if 1
#define DEBUG 1
#define DODEBUG(a) if(ipp->debug_level >= 5) debug a
#define XML_DEBUG(a) if(ipp->debug_level > 0) debug a
#else
#define DODEBUG(a)
#define XML_DEBUG(a)
#endif

/** create IPP request handling object

This function creates an IPP service object.  The IPP request will be read
from stdin and the response will be sent to stdout.  IPP data of various
types can be read from the request and appended to the response using the
member functions.

*/
struct IPP *ipp_new(const char root[], const char path_info[], int content_length, int in_fd, int out_fd)
	{
	struct IPP *ipp = NULL;
	void *pool;

	GU_OBJECT_POOL_PUSH((pool = gu_pool_new()));
	ipp = gu_alloc(1, sizeof(struct IPP));
	ipp->magic = 0xAABB;
	ipp->pool = pool;

	ipp->debug_level = 0;
	
	ipp->root = root;
	ipp->path_info = path_info;
	ipp->bytes_left = content_length;
	ipp->in_fd = in_fd;
	ipp->out_fd = out_fd;

	ipp->remote_user = NULL;
	ipp->remote_addr = NULL;

	ipp->readbuf_i = 0;
	ipp->readbuf_remaining = 0;
	ipp->readbuf_guard = 42;

	ipp->writebuf_i = 0;
	ipp->writebuf_remaining = sizeof(ipp->writebuf);
	ipp->writebuf_guard = 42;

	ipp->request_attrs = NULL;

	ipp->response_attrs_operation = NULL;
	ipp->response_attrs_operation_tail_ptr = &ipp->response_attrs_operation;
	ipp->response_attrs_printer = NULL;
	ipp->response_attrs_printer_tail_ptr = &ipp->response_attrs_printer;
	ipp->response_attrs_job = NULL;
	ipp->response_attrs_job_tail_ptr = &ipp->response_attrs_job;
	ipp->response_attrs_unsupported = NULL;
	ipp->response_attrs_unsupported_tail_ptr = &ipp->response_attrs_unsupported;

	GU_OBJECT_POOL_POP(ipp->pool);
	return ipp;
	} /* ipp_new() */

/** set IPP object debug level
 *
 * The IPP object can print debug messages by calling the 
 * callback function debug().  This method sets the debug level
 * a higher debug level means more debug messages.
 */
void ipp_set_debug_level(struct IPP *ipp, int level)
	{
	ipp->debug_level = level;
	}

/*
** Read a bufferful of an IPP request from stdin.
*/
static void ipp_readbuf_load(struct IPP *p)
	{
	/*DODEBUG(("ipp_readbuf_load(): p->bytes_left = %d", p->bytes_left));*/
	if((p->readbuf_remaining = read(p->in_fd, p->readbuf, (p->bytes_left != -1 && p->bytes_left < sizeof(p->readbuf)) ? p->bytes_left : sizeof(p->readbuf))) == -1)
		gu_Throw("%s() failed, errno=%d (%s)", "read", errno, strerror(errno));
	if(p->bytes_left != -1)
		{
		if(p->readbuf_remaining < 1)
			gu_Throw("premature EOF");
		p->bytes_left -= p->readbuf_remaining;
		}
	p->readbuf_i = 0;
	}

/*
** Flush the response buffer.
*/
static void ipp_writebuf_flush(struct IPP *ipp)
	{
	char *write_ptr;
	int to_write, len;

	DODEBUG(("ipp_writebuf_flush(): %d bytes to flush to fd %d", ipp->writebuf_i, ipp->out_fd));
	
	to_write = ipp->writebuf_i;
	write_ptr = ipp->writebuf;

	while(to_write > 0)
		{
		DODEBUG(("  trying to write %d bytes", to_write));
		if((len = write(ipp->out_fd, write_ptr, to_write)) == -1)
			gu_Throw("writing response failed, errno=%d (%s)", errno, gu_strerror(errno));
		DODEBUG(("    wrote %d bytes", len));
		to_write -= len;
		write_ptr += len;
		}

	ipp->writebuf_i = 0;
	ipp->writebuf_remaining = sizeof(ipp->writebuf);
	}

/** Finish the IPP service transaction

Any remaining request input bytes are read and discarded.  Any bytes remaining
in the The response output buffer are sent on their way.  Finally, the IPP
service object is destroyed.

*/
void ipp_delete(struct IPP *ipp)
	{
	#ifdef DEBUG
	if(ipp->bytes_left != -1)
		debug("ipp_delete(): %d leftover bytes", ipp->bytes_left + ipp->readbuf_remaining);
	else
		DODEBUG(("ipp_delete()"));
	#endif

	if(ipp->magic != 0xAABB)
		gu_Throw("ipp_delete(): not an IPP object");
	if(ipp->readbuf_guard != 42)
		gu_Throw("ipp_delete(): readbuf overflow");
	if(ipp->writebuf_guard != 42)
		gu_Throw("ipp_delete(): writebuf overflow");
	
	while(ipp->bytes_left > 0)	/* does nothing if ipp->bytes_left == -1 */
		ipp_readbuf_load(ipp);

	ipp_writebuf_flush(ipp);

	ipp->magic = 0;		/* render object unrecognizable as an IPP object */
	gu_pool_free(ipp->pool);
	} /* ipp_delete() */

/** set REMOTE_USER from CGI environment
 *
 * This IPP server should call this function with remote_user[]
 * set to the value of the REMOTE_USER environment variable.
*/
void ipp_set_remote_user(struct IPP *ipp, const char remote_user[])
	{
	ipp->remote_user = remote_user;
	}

/** set REMOTE_ADDR from CGI environment
 *
 * This IPP server should call this function with remote_addr[]
 * set to the value of the REMOTE_ADDR environment variable.
*/
void ipp_set_remote_addr(struct IPP *ipp, const char remote_addr[])
	{
	ipp->remote_addr = remote_addr;
	}

/** fetch a block from the IPP request

The character pointer pointed two by pptr is set to point to the next
block of the file.  The length of the block is returned.  This is used
to read the print file data which follows the IPP request.

*/
int ipp_get_block(struct IPP *ipp, char **pptr)
	{
	int len = 0;

	/* If the input buffer is empty, try to fill it. */	
	if(ipp->readbuf_remaining <= 0)
		{
		if(ipp->bytes_left == 0)	/* Abort if CONTENT_LENGTH is exhausted (but not if it was -1) */
			return 0;
		ipp_readbuf_load(ipp);
		}

	if(ipp->readbuf_remaining > 0)
		{
		*pptr = &ipp->readbuf[ipp->readbuf_i];
		len = ipp->readbuf_remaining;
		ipp->readbuf_i += len;
		ipp->readbuf_remaining = 0;
		}

	return len;
	} /* ipp_get_block() */

/*=== Request parsing =====================================================*/

/** fetch an unsigned byte from the IPP request

This is used to read tags.

*/
static char ipp_get_byte(struct IPP *ipp)
	{
	if(ipp->readbuf_remaining < 1)
		ipp_readbuf_load(ipp);
	if(ipp->readbuf_remaining < 1)
		gu_Throw("Data runoff!");
	ipp->readbuf_remaining--;
	return ipp->readbuf[ipp->readbuf_i++];
	}

/** append an unsigned byte to the IPP response

This is used to write tags.

*/
static void ipp_put_byte(struct IPP *ipp, char val)
	{
	ipp->writebuf[ipp->writebuf_i++] = val;
	ipp->writebuf_remaining--;
	if(ipp->writebuf_remaining < 1)
		ipp_writebuf_flush(ipp);
	}

/** fetch a signed byte from the IPP request
*/
static int ipp_get_sb(struct IPP *ipp)
	{
	return (int)(signed char)ipp_get_byte(ipp);
	}

/** fetch a signed short from the IPP request
*/
static int ipp_get_ss(struct IPP *ipp)
	{
	unsigned char a, b;
	a = ipp_get_byte(ipp);
	b = ipp_get_byte(ipp);
	return (int)(!0xFFFF | a << 8 | b);								/* compile doesn't like this, not sure why */
	}

/** fetch a signed integer from the IPP request
*/
static int ipp_get_si(struct IPP *ipp)
	{
	unsigned char a, b, c, d;
	a = ipp_get_byte(ipp);
	b = ipp_get_byte(ipp);
	c = ipp_get_byte(ipp);
	d = ipp_get_byte(ipp);
	return (int)(!0xFFFFFFFF | a << 24 | b << 16 | c << 8 | d);		/* compiler doesn't like this, not sure why */
	}

/** append a signed byte to the IPP response
*/
static void ipp_put_sb(struct IPP *ipp, int val)
	{
	ipp_put_byte(ipp, (unsigned char)val);
	}

/** append a signed short to the IPP response
*/
static void ipp_put_ss(struct IPP *ipp, int val)
	{
	unsigned int temp = (unsigned int)val;
	ipp_put_byte(ipp, (temp & 0xFF00) >> 8);
	ipp_put_byte(ipp, (temp & 0X00FF));
	}

/** append a signed integer to the IPP response
*/
static void ipp_put_si(struct IPP *ipp, int val)
	{
	unsigned int temp = (unsigned int)val;
	ipp_put_byte(ipp, (temp & 0xFF000000) >> 24);
	ipp_put_byte(ipp, (temp & 0x00FF0000) >> 16);
	ipp_put_byte(ipp, (temp & 0x0000FF00) >> 8);
	ipp_put_byte(ipp, (temp & 0x000000FF));
	}

/** fetch a byte array of specified length
*/
static char *ipp_get_bytes(struct IPP *ipp, int len)
	{
	char *ptr = NULL;
	int i;
	GU_OBJECT_POOL_PUSH(ipp->pool);
	ptr = gu_alloc(len + 1, sizeof(char));
	for(i=0; i<len; i++)
		ptr[i] = ipp_get_byte(ipp);
	ptr[len] = '\0';
	GU_OBJECT_POOL_POP(ipp->pool);
	return ptr;
	}

/** read an IPP request from stdin and store it in the IPP object
 *
*/
void ipp_parse_request(struct IPP *ipp)
	{
	char *p;
	int tag, delimiter_tag = 0, value_tag, name_length, value_length;
	char *name = NULL;
	ipp_attribute_t *ap = NULL, **ap_resize = NULL;
	int ap_i = 0;

	/* For XML description of request. */
	#ifdef DEBUG
	int prev_delim = -1;
	int prev_value = -1;
	#endif

	if(ipp->bytes_left != -1 && ipp->bytes_left < 9)	/* -1 means unknown */
		gu_Throw("request is too short (%d bytes) to be an IPP request", ipp->bytes_left);

	DODEBUG(("request for %s, %d bytes", ipp->path_info, ipp->bytes_left));

	ipp->version_major = ipp_get_sb(ipp);
	ipp->version_minor = ipp_get_sb(ipp);
	ipp->operation_id = ipp_get_ss(ipp);
	ipp->request_id = ipp_get_si(ipp);

	#ifdef DEBUG
	if(ipp->debug_level > 0)
		{
		debug("<ipp>");
		debug("<request>");
		debug(" <version-number>%d.%d</version-number>",
			ipp->version_major, ipp->version_minor
			);
		debug(" <operation-id>%s</operation-id>",
			ipp_operation_id_to_str(ipp->operation_id)
			);
		debug(" <request-id>%d</request-id>",
			ipp->request_id
			);
		}
	#endif

	GU_OBJECT_POOL_PUSH(ipp->pool);

	while((tag = ipp_get_byte(ipp)) != IPP_TAG_END)
		{
		/* Delimiter tags change interpretation of subsequent tags. */
		if(tag >= 0x00 && tag <= 0x0f)
			{
			/* Emmit XML description of attribute group. */
			#ifdef DEBUG
			if(ipp->debug_level > 0)
				{
				if(prev_value != -1)
					debug(" </%s>", ipp_tag_to_str(prev_value));
				if(prev_delim != -1)
					debug("</%s>", ipp_tag_to_str(prev_delim));
				debug("<%s>", ipp_tag_to_str(tag));		
				prev_delim = tag;
				}
			#endif

			delimiter_tag = tag;
			name = NULL;
			}
		/* Value tags announce name-value pairs. */
		else if(tag >= 0x10 && tag <= 0xff)
			{
			value_tag = tag;		/* Accept as a value tag. */

			/* Read in the length of the name, the name, and the 
			 * length of the value, but hold off on actually 
			 * reading the value until we have allocated storage
			 * for it.
			 */
			name_length = ipp_get_ss(ipp);
			if(name_length > 0)
				name = ipp_get_bytes(ipp, name_length);
			value_length = ipp_get_ss(ipp);

			DODEBUG(("0x%.2x (%s) 0x%.2x (%s) name[%d]=\"%s\", value_len=%d",
				delimiter_tag, ipp_tag_to_str(delimiter_tag),
				value_tag, ipp_tag_to_str(value_tag),
				name_length,
				name ? name : "",
				value_length));

			/* If the name is greater than zero, then this is a new
			 * item rather than an additional value for the previous 
			 * item. */
			if(name_length > 0)
				{
				/* Emmit start of XML description of this value.
				 * Be sure to close any previous value description first.
				 */
				#ifdef DEBUG
				if(ipp->debug_level > 0)
					{
					if(prev_value != -1)
						debug(" </%s>", ipp_tag_to_str(prev_value));
					debug(" <%s>", ipp_tag_to_str(value_tag));
					prev_value = value_tag;
					debug("  <name>%s</name>", name);
					}
				#endif

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

			/* Store the value in the space allocated above. */
			switch(ipp_tag_simplify(value_tag))
				{
				case IPP_TAG_INTEGER:
					ap->values[ap_i].integer = ipp_get_si(ipp);
					XML_DEBUG(("  <value>%d</value>", ap->values[ap_i].integer));
					break;
				case IPP_TAG_STRING:
					p = ipp_get_bytes(ipp, value_length);
					ap->values[ap_i].string.text = p;
					XML_DEBUG(("  <value>%s</value>", ap->values[ap_i].string.text));
					break;
				default:
					p = ipp_get_bytes(ipp, value_length);
					ap->values[ap_i].unknown.length = value_length;
					ap->values[ap_i].unknown.data = p;
					XML_DEBUG(("  <value>[%d byte value]</value>", value_length));
					break;
				}
			}
		else
			{
			gu_Throw("invalid tag value 0x%.2x", tag);
			}
		}

	#ifdef DEBUG
	if(ipp->debug_level > 0)
		{
		if(prev_value != -1)
			debug(" </%s>", ipp_tag_to_str(prev_value));
		if(prev_delim != -1)
			debug("</%s>", ipp_tag_to_str(delimiter_tag));
		debug("</request>");
		debug("</ipp>\n");
		}
	#endif

	GU_OBJECT_POOL_POP(ipp->pool);
	} /* end of ipp_parse_request() */

/*=== Reply creation ======================================================*/

/** append a byte array of a specified length
*/
static void ipp_put_bytes(struct IPP *ipp, const char *data, int len)
	{
	int i;
	for(i=0; i<len; i++)
		{
		ipp_put_byte(ipp, data[i]);
		}
	}

/** append a string to the reply
*/
static void ipp_put_string(struct IPP *ipp, const char string[])
	{
	int i;
	for(i=0; string[i]; i++)
		ipp_put_byte(ipp, string[i]);
	}

/** append an attribute to the IPP response
*/
static void ipp_put_attr(struct IPP *ipp, ipp_attribute_t *attr)
	{
	ipp_value_t *p;
	int i, len;
	
	XML_DEBUG((" <%s><name>%s</name>",
		ipp_tag_to_str(attr->value_tag),
		attr->name
		));

	for(i=0 ; i < attr->num_values; i++)
		{
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

				XML_DEBUG(("  <value>%s%s</value>", ipp->root, temp));
				len = strlen(ipp->root) + strlen(temp);
				ipp_put_ss(ipp, len);
				ipp_put_bytes(ipp, ipp->root, strlen(ipp->root));
				ipp_put_bytes(ipp, temp, strlen(temp));
				continue;
				}
			else if(p->string.text[0] == '/')
				{
				XML_DEBUG(("  <value>%s%s</value>", ipp->root, p->string.text));
				len = strlen(ipp->root) + strlen(p->string.text);
				ipp_put_ss(ipp, len);
				ipp_put_bytes(ipp, ipp->root, strlen(ipp->root));
				ipp_put_bytes(ipp, p->string.text, strlen(p->string.text));
				continue;
				}
			}

		/* non-URL items get basic treatment */
		switch(ipp_tag_simplify(attr->value_tag))
			{
			case IPP_TAG_INTEGER:
				XML_DEBUG(("  <value>%d</value>", p->integer));
				ipp_put_ss(ipp, 4);
				ipp_put_si(ipp, p->integer);
				break;
			case IPP_TAG_STRING:
				XML_DEBUG(("  <value>%s</value>", p->string.text));
				len = strlen(p->string.text);
				ipp_put_ss(ipp, len);
				ipp_put_bytes(ipp, p->string.text, len);
				break;
			case IPP_TAG_BOOLEAN:
				XML_DEBUG(("  <value>%s</value>", p->boolean ? "true" : "false"));
				ipp_put_ss(ipp, 1);
				ipp_put_sb(ipp, p->boolean ? 1 : 0);
				break;
			default:
				gu_Throw("ipp_put_attr(): missing case for value tag 0x%.2x in ipp_put_attr()", attr->value_tag);
			}
		}

	XML_DEBUG((" </%s>",
		ipp_tag_to_str(attr->value_tag)
		));
	} /* end of ipp_put_attr() */

/* Add an attribute to the IPP response. */
void ipp_insert_attribute(struct IPP *ipp, ipp_attribute_t *ap)
	{
	ipp_attribute_t **tail_ptr;

	/* Choose the correct linked list and get a pointer to the "next"
	 * element of its last item (or to the first item pointer if the
	 * list is empty).  Then, update the lists tail pointer to point
	 * to the "next" element of the item we are about to insert.
	 */
	switch(ap->group_tag)
		{
		case IPP_TAG_OPERATION:
			tail_ptr = ipp->response_attrs_operation_tail_ptr;
			ipp->response_attrs_operation_tail_ptr = &ap->next;
			break;
		case IPP_TAG_PRINTER:
			tail_ptr = ipp->response_attrs_printer_tail_ptr;
			ipp->response_attrs_printer_tail_ptr = &ap->next;
			break;
		case IPP_TAG_JOB:
			tail_ptr = ipp->response_attrs_job_tail_ptr;
			ipp->response_attrs_job_tail_ptr = &ap->next;
			break;
		case IPP_TAG_UNSUPPORTED:
			tail_ptr = ipp->response_attrs_unsupported_tail_ptr;
			ipp->response_attrs_unsupported_tail_ptr = &ap->next;
			break;
		default:
			gu_Throw("invalid group_tag: 0x%02x", ap->group_tag);
		}

	/* update the next pointer to point to this new one. */
	*tail_ptr = ap;
	}

/*
** Create a new attribute and add it to the IPP response
** This is an internal function.  Other functions call it in order
** to add an empty attribute which they procede to fill in.
*/
static ipp_attribute_t *ipp_new_attribute(struct IPP *ipp, int group, int tag, const char name[], int num_values)
	{
	ipp_attribute_t	*ap = NULL;

	GU_OBJECT_POOL_PUSH(ipp->pool);

	ap = gu_alloc(1, sizeof(ipp_attribute_t) + sizeof(ipp_value_t) * (num_values - 1));
	ap->next = NULL;
	ap->group_tag = group;
	ap->value_tag = tag;
	ap->name = (char*)name;
	ap->template = NULL;
	ap->num_values = num_values;

	GU_OBJECT_POOL_POP(ipp->pool);

	ipp_insert_attribute(ipp, ap);

	return ap;
	} /* end of ipp_add_attr() */

/** add an object divider to the IPP response
*/
void ipp_add_end(struct IPP *ipp, int group)
	{
	ipp_new_attribute(ipp, group, IPP_TAG_END, NULL, 0);
	}

/** add an item with an out-of-band value
*/
void ipp_add_out_of_band(struct IPP *ipp, int group_tag, int value_tag, const char name[])
	{
	ipp_new_attribute(ipp, group_tag, value_tag, name, 0);
	}

/** add an integer to the IPP response
 *
 * This keeps a pointer to name[], so it had better not change!
*/
void ipp_add_integer(struct IPP *ipp, int group, int tag, const char name[], int value)
	{
	ipp_attribute_t *ap;
	if(ipp_tag_simplify(tag) != IPP_TAG_INTEGER && tag != IPP_TAG_URI)
		gu_Throw("ipp_add_integer(): %s is a %s", name, ipp_tag_to_str(tag));
	ap = ipp_new_attribute(ipp, group, tag, name, 1);
	ap->values[0].integer = value;
	}

/** add a list of integers to the IPP response
 *
 * This keeps a pointer to name[], so it had better not change!
*/
void ipp_add_integers(struct IPP *ipp, int group, int tag, const char name[], int num_values, int values[])
	{
	ipp_attribute_t *ap;
	int i;
	if(ipp_tag_simplify(tag) != IPP_TAG_INTEGER && tag != IPP_TAG_URI)
		gu_Throw("ipp_add_integer(): %s is a %s", name, ipp_tag_to_str(tag));
	ap = ipp_new_attribute(ipp, group, tag, name, num_values);
	for(i=0; i<num_values; i++)
		ap->values[i].integer = values[i];
	}

/** add a string to the IPP response
 *
 * This function keeps a pointer to name[] and value[], so they had better not
 * change before the IPP object is destroyed!
*/
void ipp_add_string(struct IPP *ipp, int group, int tag, const char name[], const char value[])
	{
	ipp_attribute_t *ap;
	if(ipp_tag_simplify(tag) != IPP_TAG_STRING)
		gu_Throw("ipp_add_string(): %s is a %s", name, ipp_tag_to_str(tag));
	ap = ipp_new_attribute(ipp, group, tag, name, 1);
	ap->values[0].string.text = (char*)value;
	}

/** add a list of strings to the IPP response
 *
 * This function keeps a pointer to name[] and all of the values[], so they 
 * had better not change before the IPP object is destroyed!
*/
void ipp_add_strings(struct IPP *ipp, int group, int tag, const char name[], int num_values, const char *values[])
	{
	ipp_attribute_t *ap;
	int i;
	if(ipp_tag_simplify(tag) != IPP_TAG_STRING)
		gu_Throw("ipp_add_strings(): %s is a %s", name, ipp_tag_to_str(tag));
	ap = ipp_new_attribute(ipp, group, tag, name, num_values);
	for(i=0; i<num_values; i++)
		ap->values[i].string.text = (char*)values[i];
	}

/** add a formatted string to the IPP response
 *
 * This keeps a pointer to name[], so it had better not change!
 *
 * The formatting is done immediately, so the values can be destroyed
 * as soon as the call returns.
*/
#if 0
void ipp_add_printf(struct IPP *ipp, int group, int tag, const char name[], const char value[], ...)
	{
	ipp_attribute_t *ap;
	va_list va;
	char *p;

	/* String formatting only makes sense for IPP types which are encoded
	 * as byte strings.
	 */
	if(ipp_tag_simplify(tag) != IPP_TAG_STRING)
		gu_Throw("ipp_add_printf(): %s is a %s", name, ipp_tag_to_str(tag));

	ap = ipp_new_attribute(ipp, group, tag, name, 1);

	GU_OBJECT_POOL_PUSH(ipp->pool);
	va_start(va, value);
	gu_vasprintf(&p, value, va);
	ap->values[0].string.text = p;
	va_end(va);
	GU_OBJECT_POOL_POP(ipp->pool);
	}
#endif

/** add a very basic formatted string to the IPP response
 *
 * This keeps a pointer to name[], so it had better not change!
 *
 * Only one format specifier is allowed.  It can be either
 * %%d or %%s.  If it is %%s, then the value should not be
 * destroyed until after the IPP object has been destroyed.
*/
void ipp_add_template(struct IPP *ipp, int group, int tag, const char name[], const char template[], ...)
	{
	ipp_attribute_t *ap;
	va_list va;

	if(ipp_tag_simplify(tag) != IPP_TAG_STRING)
		gu_Throw("ipp_add_template(): %s is a %s", name, ipp_tag_to_str(tag));

	ap = ipp_new_attribute(ipp, group, tag, name, 1);
	ap->template = template;

	va_start(va, template);
	if(strstr(template, "%d"))
		ap->values[0].integer = va_arg(va, int);
	else
		ap->values[0].string.text = va_arg(va, char*);
	va_end(va);
	}

void ipp_add_templates(struct IPP *ipp, int group_tag, int value_tag, const char name[], const char template[], int num_values, void *values)
	{
	ipp_attribute_t *ap;
	int i;

	if(ipp_tag_simplify(value_tag) != IPP_TAG_STRING)
		gu_Throw("ipp_add_templates(): %s is a %s", name, ipp_tag_to_str(value_tag));

	ap = ipp_new_attribute(ipp, group_tag, value_tag, name, num_values);
	ap->template = template;

	if(strstr(template, "%d"))
		{
		int *p = (int*)values;
		for(i=0; i<num_values; i++)
			ap->values[i].integer = *(p++);
		}
	else
		{
		char **p = (char**)values;
		for(i=0; i<num_values; i++)
			ap->values[i].string.text = *(p++); 
		}
	}

/** add a boolean  to the IPP response 
 *
 * This keeps a pointer to name[], so it had better not change!
*/
void ipp_add_boolean(struct IPP *ipp, int group_tag, int value_tag, const char name[], gu_boolean value)
	{
	ipp_attribute_t *ap;
	if(value_tag != IPP_TAG_BOOLEAN)
		gu_Throw("ipp_add_boolean(): %s is a %s", name, ipp_tag_to_str(value_tag));
	ap = ipp_new_attribute(ipp, group_tag, value_tag, name, 1);
	ap->values[0].boolean = value;
	}

/** send the IPP response in the IPP object
*/
void ipp_send_reply(struct IPP *ipp, gu_boolean header)
	{
	ipp_attribute_t *attr;
	
	DODEBUG(("ipp_send_reply()"));

	/* Any operation or job-template attributes which have not yet been
	 * read (and removed) must be unsupported.
	 */
	for(attr = ipp->request_attrs; attr; attr = attr->next)
		{
		if(attr->group_tag == IPP_TAG_OPERATION || attr->group_tag == IPP_TAG_JOB)
			ipp_new_attribute(ipp, IPP_TAG_UNSUPPORTED, IPP_TAG_UNSUPPORTED_VALUE, attr->name, 0);
		}

	/* If we are communicating over HTTP, we need an HTTP header. */
	if(header)
		ipp_put_string(ipp, "Content-Type: application/ipp\r\n\r\n");

	#ifdef DEBUG
	if(ipp->debug_level > 0)
		{
		debug("<ipp>");
		debug("<response>");
		/*debug("<version-number>%d.%d</version-number>", IPP_SUPPORTED_MAJOR, IPP_SUPPORTED_MINOR);*/
		debug("<version-number>%d.%d</version-number>", ipp->version_major, ipp->version_minor);
		debug("<status-code>0x%04x</status-code>", ipp->response_code);
		debug("<request-id>%d</request-id>", ipp->request_id);
		}
	#endif

	/* Claiming support for IPP version 1.1 is probably the correct thing to do.  Unfortunately,
	 * Early implementations of IPP in MS-Windows will refuse to add our printers if we claim
	 * support for version 1.1.  So, we claim support for it only if the client does.
	 */
	#if 0
	ipp_put_sb(ipp, IPP_SUPPORTED_MAJOR);
	ipp_put_sb(ipp, IPP_SUPPORTED_MINOR);
	#else
	ipp_put_sb(ipp, ipp->version_major);
	ipp_put_sb(ipp, ipp->version_minor);
	#endif

	ipp_put_ss(ipp, ipp->response_code);
	ipp_put_si(ipp, ipp->request_id);
	
	if(!(attr = ipp->response_attrs_operation))			/* see RFC 2911 3.1.4.2 */
		gu_Throw("no response_attrs_operation");
	XML_DEBUG(("<operation-attributes>"));
	ipp_put_byte(ipp, IPP_TAG_OPERATION);
	for( ; attr; attr = attr->next)
		ipp_put_attr(ipp, attr);
	XML_DEBUG(("</operation-attributes>"));

	if((attr = ipp->response_attrs_printer))
		{
		XML_DEBUG(("<printer-attributes>"));
		ipp_put_byte(ipp, IPP_TAG_PRINTER);
		for( ; attr; attr = attr->next)
			{
			if(attr->value_tag == IPP_TAG_END)
				{
				XML_DEBUG(("</printer-attributes>"));
				if(attr->next)
					{
					XML_DEBUG(("<printer-attributes>"));
					ipp_put_byte(ipp, IPP_TAG_PRINTER);
					}
				}
			else
				ipp_put_attr(ipp, attr);
			}
		}
	
	if((attr = ipp->response_attrs_job))
		{
		XML_DEBUG(("<job-attributes>"));
		ipp_put_byte(ipp, IPP_TAG_JOB);

		for( ; attr; attr = attr->next)
			{
			if(attr->value_tag == IPP_TAG_END)
				{
				XML_DEBUG(("</job-attributes>"));
				if(attr->next)
					{
					XML_DEBUG(("<job-attributes>"));
					ipp_put_byte(ipp, IPP_TAG_JOB);
					}
				}
			else
				ipp_put_attr(ipp, attr);
			}
		}

	if((attr = ipp->response_attrs_unsupported))
		{
		XML_DEBUG(("<unsupported-attributes>"));
		ipp_put_byte(ipp, IPP_TAG_UNSUPPORTED);
		for( ; attr; attr = attr->next)
			ipp_put_attr(ipp, attr);
		XML_DEBUG(("</unsupported-attributes>"));
		}

	/* end of IPP response */
	ipp_put_byte(ipp, IPP_TAG_END);
	XML_DEBUG(("</response>"));
	XML_DEBUG(("</ipp>\n"));	/* leave blank line */

	ipp_writebuf_flush(ipp);
	} /* end of ipp_send_reply() */

/*=========================================================================*/

/** find an attribute in the IPP request
 *
 * A pointer to the first attribute which matches group, tag, and name[]
 * is returned.
*/
ipp_attribute_t *ipp_find_attribute(struct IPP *ipp, int group, int tag, const char name[])
	{
	ipp_attribute_t *p;
	
	for(p=ipp->request_attrs; p; p=p->next)
		{
		if(p->group_tag == group
				&& p->value_tag == tag
			   	&& strcmp(p->name, name) == 0
				)
			{
			return p;
			}
		}
		
	return NULL;
	} /* ipp_find_attribute() */

/** find an attribute in the IPP request and remove it
 *
 * A pointer to the first attribute which matches group, tag, and name[]
 * is returned.  The attribute is then removed from the IPP object so
 * that we will later know that it was handled.
*/
ipp_attribute_t *ipp_claim_attribute(struct IPP *ipp, int group, int tag, const char name[])
	{
	ipp_attribute_t *p;
	ipp_attribute_t **pp;
	
	for(p=ipp->request_attrs,pp=&(ipp->request_attrs); p; pp=&(p->next),p=p->next)
		{
		if(p->group_tag == group
				&& p->value_tag == tag
			   	&& strcmp(p->name, name) == 0
				)
			{
			*pp = p->next;
			p->next = NULL;
			return p;
			}
		}
		
	return NULL;
	} /* ipp_claim_attribute() */

/* This extends ipp_claim_attribute() to accept only single values. */
static ipp_attribute_t *ipp_claim_attribute_single_value(struct IPP *ipp, int group, int tag, const char name[])
	{
	ipp_attribute_t *attr;
	if((attr = ipp_claim_attribute(ipp, group, tag, name)))
		{
		if(attr->num_values == 1)
			return attr;
		else
			ipp->response_code = IPP_BAD_REQUEST;
		}
	return NULL;
	}

/** Find a single value URI attribute, remove it, and return a gu_uri object. */
struct URI *ipp_claim_uri(struct IPP *ipp, int group_tag, const char name[])
	{
	ipp_attribute_t *attr;
	if((attr = ipp_claim_attribute_single_value(ipp, group_tag, IPP_TAG_URI, name)))
		{
		struct URI *uri = NULL;
		GU_OBJECT_POOL_PUSH(ipp->pool);
		uri = gu_uri_new(attr->values[0].string.text);
		#ifdef DEBUG
		if(uri && ipp->debug_level >= 5)
			{
			debug("URI = {");
			debug("    method->\"%s\",", uri->method ? uri->method : "");
			debug("    node->\"%s\",", uri->node ? uri->node : "");
			debug("    port->%d,", uri->port);
			debug("    path->\"%s\",", uri->path ? uri->path : "");
			debug("    dirname->\"%s\",", uri->dirname ? uri->dirname : "");
			debug("    basename->\"%s\",", uri->basename ? uri->basename : "");
			debug("    query->\"%s\",", uri->query ? uri->query : "");
			debug("    }");
			}
		#endif
		GU_OBJECT_POOL_POP(ipp->pool);
		return uri;
		}
	return NULL;
	}

/** Find a single value positive integer attribute, remove it, and return the integer.
 * If the integer is not found, we return 0. */
int ipp_claim_positive_integer(struct IPP *ipp, int group_tag, const char name[])
	{
	ipp_attribute_t *attr;
	if((attr = ipp_claim_attribute_single_value(ipp, group_tag, IPP_TAG_INTEGER, name)))
		{
		if(attr->values[0].integer <= 0)
			{
			attr->group_tag = IPP_TAG_UNSUPPORTED;
			ipp_insert_attribute(ipp, attr);
			return 0;
			}
		return attr->values[0].integer;
		}
	return 0;
	}

/** Find a single value enumberation, remove it, and return the number. */
int ipp_claim_enum(struct IPP *ipp, int group_tag, const char name[])
	{
	ipp_attribute_t *attr;
	if((attr = ipp_claim_attribute_single_value(ipp, group_tag, IPP_TAG_ENUM, name)))
		return attr->values[0].integer;
	return 0;
	}

/** Find a single value string attribute, remove it, and return the string. */
const char *ipp_claim_string(struct IPP *ipp, int group_tag, int value_tag, const char name[])
	{
	ipp_attribute_t *attr;
	if((attr = ipp_claim_attribute_single_value(ipp, group_tag, value_tag, name)))
		return attr->values[0].string.text;
	return NULL;
	}

/** Find a single value keyword attribute, remove it, and return the string.
 * The string is tested against each of the supplied posibilities and returned
 * only if it matches one of them.  If not, it is entered as an unsupported
 * attribute.
 */
const char *ipp_claim_keyword(struct IPP *ipp, int group_tag, const char name[], ...)
	{
	ipp_attribute_t *attr;
	if((attr = ipp_claim_attribute_single_value(ipp, group_tag, IPP_TAG_KEYWORD, name)))
		{
		va_list va;
		const char *p;
		va_start(va, name);
		while((p = va_arg(va, char*)))
			{
			if(strcmp(attr->values[0].string.text, p) == 0)
				break;
			}
		va_end(va);
		if(!p)
			{
			attr->group_tag = IPP_TAG_UNSUPPORTED;
			ipp_insert_attribute(ipp, attr);
			}
		return p;
		}
	return NULL;
	}

/** Find a single value boolean attribute, remove it, and return its value */
gu_boolean ipp_claim_boolean(struct IPP *ipp, int group_tag, const char name[], gu_boolean default_value)
	{
	ipp_attribute_t *attr;
	if((attr = ipp_claim_attribute_single_value(ipp, group_tag, IPP_TAG_BOOLEAN, name)))
		return attr->values[0].boolean;
	return default_value;
	}

/* end of file */
