/*
** mouse:~ppr/src/ipp/ipp_req_attrs.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 19 April 2006.
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

/** Create an object which can tell us if an attribute was requested.
 *
 * We pass the IPP object to the constructor and it reads the operation
 * attributes from it and files away the values of those which we have
 * told it are supported. 
 *
 * supported is the bitwise or of:
 *   REQ_SUPPORTS_PRINTER--for operations on one existing printer
 *   REQ_SUPPORTS_PRINTER--for operations on multiple printers
 *   REQ_SUPPORTS_JOB--for operations on one job
 *   REQ_SUPPORTS_JOBS--for operations on many jobs
 *   REQ_SUPPORTS_DEVICE--for listing devices
 *   REQ_SUPPORTS_PPDS--for listing PPD files
 *
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
			else if(supported & (REQ_SUPPORTS_PRINTER|REQ_SUPPORTS_JOB)
					&& attr->value_tag == IPP_TAG_URI
					&& strcmp(attr->name, "printer-uri") == 0
					)
				this->printer_uri = attr->values[0].string.text;
			else if(supported & REQ_SUPPORTS_JOB
					&& attr->value_tag == IPP_TAG_URI
					&& strcmp(attr->name, "job-uri") == 0
					)
				this->job_uri = attr->values[0].string.text;
			else if(supported & REQ_SUPPORTS_JOB
					&& attr->value_tag == IPP_TAG_INTEGER
					&& strcmp(attr->name, "job-id") == 0
					)
				this->job_id = attr->values[0].integer;
			else if(supported & REQ_SUPPORTS_DEVICES
					&& attr->value_tag == IPP_TAG_KEYWORD
					&& strcmp(attr->name, "device-class") == 0
					)
				this->device_class = attr->values[0].string.text;
			else if(supported & REQ_SUPPORTS_PPDS
					&& attr->value_tag == IPP_TAG_TEXT
					&& strcmp(attr->name, "ppd-make") == 0
					)
				this->ppd_make = attr->values[0].string.text;
			else if(supported & (REQ_SUPPORTS_PRINTERS|REQ_SUPPORTS_JOBS|REQ_SUPPORTS_DEVICES|REQ_SUPPORTS_PPDS)
					&& attr->value_tag == IPP_TAG_INTEGER
					&& strcmp(attr->name, "limit") == 0
					)
				this->limit = attr->values[0].integer;
			}
		else if(attr->group_tag == IPP_TAG_PRINTER)
			{
			if(supported & REQ_SUPPORTS_PCREATE
					&& attr->value_tag == IPP_TAG_URI
					&& strcmp(attr->name, "device-uri") == 0
					)
				this->device_uri = attr->values[0].string.text;
			else if(supported & REQ_SUPPORTS_PCREATE
					&& attr->value_tag == IPP_TAG_NAME
					&& strcmp(attr->name, "ppd-name") == 0
					)
				this->ppd_name = attr->values[0].string.text;
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
	} /* request_attrs_new() */

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
	} /* request_attrs_free() */

/** Return true if the indicate attributes was requested.
 * We also make a note of the fact that the caller asked about this attribute
 * since that presumably means that the request was handled.
 */
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
	if(this->printer_uri_obj)
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
