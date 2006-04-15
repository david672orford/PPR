/*
** mouse:~ppr/src/ipp/ipp_utils.c
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
** Last modified 30 March 2006.
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

#if 0
#define DEBUG(a) debug a
#else
#define DEBUG(a)
#endif

/* ======================================================================= */

/** Create an object which can tell us if an attribute was requested.
 *
 * We pass the IPP object to the constructor and it reads the operation
 * attributes from it and files away the values of those which we have
 * told it are supported. 
 */ 
struct REQUEST_ATTRS *request_attrs_new(struct IPP *ipp, int supported)
	{
	struct REQUEST_ATTRS *this;
	ipp_attribute_t *attr;

	this = gu_alloc(1, sizeof(struct REQUEST_ATTRS));
	this->requested_attributes = gu_pch_new(25);
	this->requested_attributes_all = FALSE;
	this->printer_uri = NULL;
	this->printer_uri_obj = NULL;
	this->printer_name = NULL;
	this->job_uri = NULL;
	this->job_uri_obj = NULL;
	this->job_id = -1;
	this->device_class = NULL;
	this->device_uri = NULL;
	this->ppd_make = NULL;
	this->ppd_name = NULL;
	this->limit = -1;

	/* Traverse the request's operation attributes. */
	for(attr = ipp->request_attrs; attr; attr = attr->next)
		{
		if(attr->group_tag == IPP_TAG_OPERATION)
			{
			if(attr->value_tag == IPP_TAG_CHARSET && strcmp(attr->name, "attributes-charset") == 0)
				{
				}
			else if(attr->value_tag == IPP_TAG_LANGUAGE && strcmp(attr->name, "attributes-natural-language") == 0)
				{
				}
			else if(attr->value_tag == IPP_TAG_KEYWORD && strcmp(attr->name, "requested-attributes") == 0)
				{
				int iii;
				for(iii=0; iii<attr->num_values; iii++)
					gu_pch_set(this->requested_attributes, attr->values[iii].string.text, "TRUE");
				}
			else if(supported & REQUEST_ATTRS_SUPPORTS_PRINTER && attr->value_tag == IPP_TAG_URI
					&& strcmp(attr->name, "printer-uri") == 0)
				this->printer_uri = attr->values[0].string.text;
			else if(supported & (REQUEST_ATTRS_SUPPORTS_PRINTER|REQUEST_ATTRS_SUPPORTS_JOB) && attr->value_tag == IPP_TAG_NAME
					&& strcmp(attr->name, "printer-name") == 0)
				this->printer_name = attr->values[0].string.text;
			else if(supported & REQUEST_ATTRS_SUPPORTS_JOB && attr->value_tag == IPP_TAG_URI
					&& strcmp(attr->name, "job-uri") == 0)
				this->job_uri = attr->values[0].string.text;
			else if(supported & REQUEST_ATTRS_SUPPORTS_JOB && attr->value_tag == IPP_TAG_INTEGER
					&& strcmp(attr->name, "job-id") == 0)
				this->job_id = attr->values[0].integer;
			else if(supported & REQUEST_ATTRS_SUPPORTS_DEVICE_CLASS && attr->value_tag == IPP_TAG_KEYWORD
					&& strcmp(attr->name, "device-class") == 0)
				this->device_class = attr->values[0].string.text;
			else if(supported & REQUEST_ATTRS_SUPPORTS_PPD_MAKE && attr->value_tag == IPP_TAG_TEXT
					&& strcmp(attr->name, "ppd-make") == 0)
				this->ppd_make = attr->values[0].string.text;
			else if(supported & REQUEST_ATTRS_SUPPORTS_LIMIT && attr->value_tag == IPP_TAG_INTEGER
					&& strcmp(attr->name, "limit") == 0)
				this->limit = attr->values[0].integer;
			else
				ipp_copy_attribute(ipp, IPP_TAG_UNSUPPORTED, attr);
			}
		else if(attr->group_tag == IPP_TAG_PRINTER)
			{
			if(supported & REQUEST_ATTRS_SUPPORTS_PCREATE && attr->value_tag == IPP_TAG_URI
					&& strcmp(attr->name, "device-uri") == 0)
				this->device_uri = attr->values[0].string.text;
			else if(supported & REQUEST_ATTRS_SUPPORTS_PCREATE && attr->value_tag == IPP_TAG_NAME
					&& strcmp(attr->name, "ppd-name") == 0)
				this->ppd_name = attr->values[0].string.text;
			else
				ipp_copy_attribute(ipp, IPP_TAG_UNSUPPORTED, attr);
			}
		else
			ipp_copy_attribute(ipp, IPP_TAG_UNSUPPORTED, attr);
		}

	/* If no attributes were requested or "all" were requested, */
	if(gu_pch_size(this->requested_attributes) == 0 || gu_pch_get(this->requested_attributes, "all"))
		this->requested_attributes_all = TRUE;

	/* Parse the URLs and store the results in objects. */
	if(this->printer_uri)
		this->printer_uri_obj = gu_uri_new(this->printer_uri);
	if(this->job_uri)
		this->job_uri_obj = gu_uri_new(this->job_uri);

	return this;
	}

/** Destroy a requested attributes object.
 *
 * Remember that we mustn't try to free printer_uri or printer_name since
 * they are simply pointers to the values allocated in the IPP object.
 */ 
void request_attrs_free(struct REQUEST_ATTRS *this)
	{
	char *name, *value;
	for(gu_pch_rewind(this->requested_attributes); (name = gu_pch_nextkey(this->requested_attributes, (void*)&value)); )
		{
		if(strcmp(value, "TOUCHED") != 0)
			{
			DEBUG(("requested attribute \"%s\" not implemented", name));
			}
		}
	gu_pch_free(this->requested_attributes);
	if(this->printer_uri_obj)
		gu_uri_free(this->printer_uri_obj);
	if(this->job_uri_obj)
		gu_uri_free(this->job_uri_obj);
	gu_free(this);
	}

/** Return true if the indicate attributes was requested. */
gu_boolean request_attrs_attr_requested(struct REQUEST_ATTRS *this, char name[])
	{
	if(this->requested_attributes_all)
		return TRUE;
	if(gu_pch_get(this->requested_attributes, name))
		{
		gu_pch_set(this->requested_attributes, name, "TOUCHED");
		return TRUE;
		}
	return FALSE;
	}

/** Return the requested queue name or NULL if none requested. */
char *request_attrs_destname(struct REQUEST_ATTRS *this)
	{
	if(this->printer_name)
		return this->printer_name;
	if(this->printer_uri)
		return this->printer_uri_obj->basename;		/* possibly null */
	return NULL;
	}

/** Return the requested job ID or -1 if none was requested. */
int request_attrs_jobid(struct REQUEST_ATTRS *this)
	{
	if(this->job_id != -1)
		return this->job_id;
	if(this->job_uri_obj && this->job_uri_obj->basename)
		{
		int id = atoi(this->job_uri_obj->basename);
		if(id > 0)
			return id;
		}
	return -1;
	}

/* end of file */
