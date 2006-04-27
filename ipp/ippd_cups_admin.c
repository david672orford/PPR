/*
** mouse:~ppr/src/ipp/ippd_cups_admin.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 27 April 2006.
*/

/*
 * This is PPR's IPP (Internet Printer Protocol) server.
 * This module implements IPP operations for CUPS lpadmin.
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "ipp_constants.h"
#include "ipp_utils.h"
#include "queueinfo.h"
#include "ippd.h"

/*
 * Handle CUPS_GET_DEVICES
 */
void cups_get_devices(struct IPP *ipp)
	{
	int iii;
	for(iii=0; iii < 10; iii++)
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_KEYWORD, "device-class", gu_strdup("file"));
		ipp_add_printf(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT, "device-info", "Acme Port %d", iii);
		ipp_add_printf(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT, "device-make-and-model", "unknown");
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_URI, "device-uri", gu_strdup("file:///x"));
		ipp_add_end(ipp, IPP_TAG_PRINTER);
		}
	} /* cups_get_devices() */

/*
 * Handle CUPS_GET_PPDS
 */
void cups_get_ppds(struct IPP *ipp)
	{
	struct REQUEST_ATTRS *req;
	FILE *f;
	char *line = NULL;
	int line_space = 256;
	const char *ppd_make;
	int limit;
	char *p, *f_description, *f_manufacturer;
	int count = 0;

	limit = ipp_claim_positive_integer(ipp, IPP_TAG_OPERATION, "limit");
	ppd_make = ipp_claim_string(ipp, IPP_TAG_OPERATION, IPP_TAG_TEXT, "ppd-make");
	req = request_attrs_new(ipp);

	if(!(f = fopen(PPD_INDEX, "r")))
		{
		ipp->response_code = IPP_NOT_FOUND;		/* is this correct? */
		return;
		}

	while((line = gu_getline(line, &line_space, f)))
		{
		if(*line == '#')	/* skip comments */
			continue;

		/* Extract the 1st and 3rd colon-separated fields. */
		p = line;
		if(!(f_description = gu_strsep(&p,":"))
				|| !gu_strsep(&p,":")
				|| !(f_manufacturer = gu_strsep(&p,":"))
				)
			{
			DEBUG(("Bad line in PPD index"));
			continue;
			}

		/* If filtering my manufacturer, skip those that don't match. */
		if(ppd_make && strcmp(ppd_make, f_manufacturer) != 0)
			continue;

		/* Do not exceed the number of items limit imposed by the client. */
		if(limit != 0 && count >= limit)
			break;

		/* Include those attributes which were requested. */
		if(request_attrs_attr_requested(req, "natural-language"))
			ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_KEYWORD,
				"natural-language", "en");
		if(request_attrs_attr_requested(req, "ppd-make"))
			ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
				"ppd-make", gu_strdup(f_manufacturer));
		if(request_attrs_attr_requested(req, "ppd-make-and-model"))
			ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
				"ppd-make-and-model", gu_strdup(f_description));
		if(request_attrs_attr_requested(req, "ppd-name"))
			ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_URI,
				"ppd-name", gu_strdup(f_description));
		
		/* Mark the end of this record. */
		ipp_add_end(ipp, IPP_TAG_PRINTER);

		count++;
		}

	gu_free_if(line);		/* if we hit limit, line will still be allocated */
	fclose(f);
	request_attrs_free(req);
	} /* cups_get_ppds() */

/*
 * Handle CUPS_ADD_MODIFY_PRINTER
 */
void cups_add_modify_printer(struct IPP *ipp)
	{
	struct URI *printer_uri;
	ipp_attribute_t *attr;
	const char *value;
	int int_value;
	int retcode = 0;

	if(!(printer_uri = ipp_claim_uri(ipp, IPP_TAG_OPERATION, "printer-uri")))
		{
		ipp->response_code = IPP_BAD_REQUEST;
		return;
		}

	if(!printer_uri->dirname || strcmp(printer_uri->dirname, "/printers") != 0 || !printer_uri->basename)
		{
		ipp->response_code = IPP_BAD_REQUEST;
		return;
		}

	if(!user_acl_allows(extract_identity(ipp, TRUE), "ppad"))
		{
		ipp->response_code = IPP_NOT_AUTHORIZED;
		return;
		}

	/* This must be first in case we are creating the printer. */
	if(retcode == 0 && (value = ipp_claim_string(ipp, IPP_TAG_PRINTER, IPP_TAG_URI, "device-uri")))
		retcode = runl(PPAD_PATH, "interface", printer_uri->basename, "dummy", value, NULL);

	/* Set other attributes */
	if(retcode == 0 && (value = ipp_claim_string(ipp, IPP_TAG_PRINTER, IPP_TAG_NAME, "ppd-name")))
		retcode = runl(PPAD_PATH, "ppd", printer_uri->basename, value, NULL);
	if(retcode == 0 && (value = ipp_claim_string(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT, "printer-location")))
		retcode = runl(PPAD_PATH, "location", printer_uri->basename, value, NULL);
	if(retcode == 0 && (value = ipp_claim_string(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT, "printer-info")))
		retcode = runl(PPAD_PATH, "comment", printer_uri->basename, value, NULL);
	#if 0
	if(retcode == 0 && (value = ipp_claim_string(ipp, IPP_TAG_PRINTER, IPP_TAG_URI, "printer-more-info")))
		retcode = runl(PPAD_PATH, "?", printer_uri->basename, value, NULL);
	#endif

	/* Set initial printer state. */	
	if(retcode == 0 && (attr = ipp_claim_attribute(ipp, IPP_TAG_PRINTER, IPP_TAG_BOOLEAN, "printer-is-accepting-jobs")) && attr->num_values == 1)
		retcode = pprd_call("%d printer %s\n", attr->values[0].boolean ? CUPS_ACCEPT_JOBS : CUPS_REJECT_JOBS, printer_uri->basename);
	if(retcode == 0 && (int_value = ipp_claim_enum(ipp, IPP_TAG_PRINTER, "printer-state")))
		{
		if(int_value == IPP_PRINTER_IDLE)
			retcode = pprd_call("%d %s\n", IPP_RESUME_PRINTER, printer_uri->basename);
		else if(int_value == IPP_PRINTER_STOPPED)
			retcode = pprd_call("%d %s\n", IPP_PAUSE_PRINTER, printer_uri->basename);
		else
			{
			error("Bad printer-state value: %d", int_value);
			retcode = 1;
			}
		}
	#if 0
	if(retcode == 0 && (value = ipp_claim_string(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT, "printer-state-message")))
	#endif

	/* requesting-user-name-allowed: not supported */
	/* requesting-user-name-denied: not supported */
	
	if(retcode != 0)
		ipp->response_code = IPP_INTERNAL_ERROR;
	} /* cups_add_modify_printer() */

/* Handle CUPS_DELETE_PRINTER */
void cups_delete_printer(struct IPP *ipp)
	{
	const char *destname;
	enum QUEUEINFO_TYPE qtype;
	if(!(destname = extract_destname(ipp, &qtype)) || qtype != QUEUEINFO_PRINTER)
		return;
	if(!user_acl_allows(extract_identity(ipp, TRUE), "ppad"))
		{
		ipp->response_code = IPP_NOT_AUTHORIZED;
		return;
		}
	if(runl(PPAD_PATH, "delete", destname, NULL) != 0)
		ipp->response_code = IPP_INTERNAL_ERROR;
	}

/*
 * Handle CUPS_ADD_MODIFY_CLASS
 */
void cups_add_modify_class(struct IPP *ipp)
	{
	struct URI *printer_uri;
	ipp_attribute_t *attr;
	const char *value;
	int retcode = 0;

	if(!(printer_uri = ipp_claim_uri(ipp, IPP_TAG_OPERATION, "printer-uri")))
		{
		ipp->response_code = IPP_BAD_REQUEST;
		return;
		}

	if(!printer_uri->dirname || strcmp(printer_uri->dirname, "/groups") != 0 || !printer_uri->basename)
		{
		ipp->response_code = IPP_BAD_REQUEST;
		return;
		}

	if(!user_acl_allows(extract_identity(ipp, TRUE), "ppad"))
		{
		ipp->response_code = IPP_NOT_AUTHORIZED;
		return;
		}

	/* This must be first in case we are creating the printer. */
	if(retcode == 0 && (attr = ipp_claim_attribute(ipp, IPP_TAG_PRINTER, IPP_TAG_URI, "member-uris")))
		{
		const char *args[4 + MAX_GROUPSIZE];
		int si = 0, di = 0;

		if(attr->num_values > MAX_GROUPSIZE)
			{
			ipp->response_code = IPP_BAD_REQUEST;
			return;
			}
		
		args[di++] = PPAD_PATH;
		args[di++] = "group";
		args[di++] = "members";
		while(di < attr->num_values)
			args[di++] = attr->values[si].string.text;
		args[di++] = NULL;
		retcode = runv(PPAD_PATH, args);
		}

	/* Set other attributes */
	if(retcode == 0 && (value = ipp_claim_string(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT, "printer-info")))
		retcode = runl(PPAD_PATH, "group", "comment", printer_uri->basename, value, NULL);
	if(retcode == 0 && (value = ipp_claim_string(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT, "printer-location")))
		retcode = runl(PPAD_PATH, "group", "location", printer_uri->basename, value, NULL);

	/* Set initial printer state. */	
	if(retcode == 0 && (attr = ipp_claim_attribute(ipp, IPP_TAG_PRINTER, IPP_TAG_BOOLEAN, "printer-is-accepting-jobs")) && attr->num_values == 1)
		retcode = pprd_call("%d group %s\n", attr->values[0].boolean ? CUPS_ACCEPT_JOBS : CUPS_REJECT_JOBS, printer_uri->basename);
	#if 0
	if(retcode == 0 && (int_value = ipp_claim_enum(ipp, IPP_TAG_PRINTER, "printer-state")))
		{
		if(int_value == IPP_PRINTER_IDLE)
			retcode = pprd_call("%d %s\n", IPP_RESUME_PRINTER, printer_uri->basename);
		else if(int_value == IPP_PRINTER_STOPPED)
			retcode = pprd_call("%d %s\n", IPP_PAUSE_PRINTER, printer_uri->basename);
		else
			{
			error("Bad printer-state value: %d", int_value);
			retcode = 1;
			}
		}
	#endif
	#if 0
	if(retcode == 0 && (value = ipp_claim_string(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT, "printer-state-message")))
	#endif

	if(retcode != 0)
		ipp->response_code = IPP_INTERNAL_ERROR;
	} /* cups_add_modify_class() */

/* Handle CUPS_DELETE_CLASS */
void cups_delete_class(struct IPP *ipp)
	{
	const char *destname;
	enum QUEUEINFO_TYPE qtype;
	if(!(destname = extract_destname(ipp, &qtype)) || qtype != QUEUEINFO_GROUP)
		return;
	if(!user_acl_allows(extract_identity(ipp, TRUE), "ppad"))
		{
		ipp->response_code = IPP_NOT_AUTHORIZED;
		return;
		}
	if(runl(PPAD_PATH, "group", "delete", destname, NULL) != 0)
		ipp->response_code = IPP_INTERNAL_ERROR;
	}

/* end of file */
