/*
** mouse:~ppr/src/ipp/ipp_req_attrs.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 20 April 2006.
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
 */ 
struct REQUEST_ATTRS *request_attrs_new(struct IPP *ipp)
	{
	struct REQUEST_ATTRS *this;
	ipp_attribute_t *attr;

	this = gu_alloc(1, sizeof(struct REQUEST_ATTRS));
	this->requested_attributes = gu_pch_new(25);
	this->requested_attributes_all = FALSE;

	/* Traverse the request's operation attributes. */
	if((attr = ipp_claim_attribute(ipp, IPP_TAG_OPERATION, IPP_TAG_KEYWORD, "requested-attributes")))
		{
		int iii;
		for(iii=0; iii<attr->num_values; iii++)
			gu_pch_set(this->requested_attributes, attr->values[iii].string.text, "TRUE");
		}

	/* If no attributes were requested or "all" were requested, */
	if(gu_pch_size(this->requested_attributes) == 0 || gu_pch_get(this->requested_attributes, "all"))
		this->requested_attributes_all = TRUE;

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

/* end of file */
