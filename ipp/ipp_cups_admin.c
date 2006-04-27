/*
** mouse:~ppr/src/ipp/ipp_cups_admin.c
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
** Last modified 12 April 2006.
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
#include "ipp-functions.h"

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
 * Handle CUPS_ADD_MODIFY_PRINTER also known as CUPS-Add-Modify-Printer
 */
void cups_add_printer(struct IPP *ipp)
	{
	struct URI *printer_uri;
	const char *value;
	int retcode = 0;

	if(!(printer_uri = ipp_claim_uri(ipp, IPP_TAG_OPERATION, "printer-uri")))
		{
		ipp->response_code = IPP_BAD_REQUEST;
		return;
		}

	/* This must be first in case we are creating the printer. */
	if(retcode == 0 && (value = ipp_claim_string(ipp, IPP_TAG_PRINTER, IPP_TAG_URI, "device-uri")))
		retcode = run(PPAD_PATH, "interface", printer_uri->basename, "dummy", value, NULL);

	/* Set attributes */
	if(retcode == 0 && (value = ipp_claim_string(ipp, IPP_TAG_PRINTER, IPP_TAG_NAME, "ppd-name")))
		retcode = run(PPAD_PATH, "ppd", printer_uri->basename, value, NULL);

	if(retcode != 0)
		ipp->response_code = IPP_BAD_REQUEST;
	} /* cups_add_printer() */

/* end of file */
