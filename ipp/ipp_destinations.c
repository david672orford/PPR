/*
** mouse:~ppr/src/ipp/ipp_destinations.c
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
 * This module contains routines for reporting on printers and groups.
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "ipp_constants.h"
#include "ipp_utils.h"
#include "queueinfo.h"
#include "ipp.h"

/*
 * This function is the meat of IPP_GET_PRINTER_ATTRIBUTES,
 * CUPS_GET_PRINTERS and CUPS_GET_CLASSES.
 */
static void add_queue_attributes(struct IPP *ipp, struct REQUEST_ATTRS *req, QUEUE_INFO qip)
	{
	const char *p;
	const char *destname;

	destname = queueinfo_hoist_value(qip, queueinfo_name(qip));

	if(request_attrs_attr_requested(req, "printer-name"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_NAME,
			"printer-name", destname);
		}

	if(request_attrs_attr_requested(req, "printer-uri"))
		{
		ipp_add_template(ipp, IPP_TAG_PRINTER, IPP_TAG_URI,	
			"printer-uri", "/printers/%s", destname);
		}

	if(request_attrs_attr_requested(req, "printer-is-accepting-jobs"))
		{
		ipp_add_boolean(ipp, IPP_TAG_PRINTER, IPP_TAG_BOOLEAN,
			"printer-is-accepting-jobs",
			queueinfo_accepting(qip)
			);
		}

#if 0
	if(destid_is_printer(destid))
		printer_add_status(ipp, req, destid);
#endif

#if 0	
	if(destid_is_group(destid) && request_attrs_attr_requested(req, "member-names"))
		{
		int iii;
		const char *members[MAX_GROUPSIZE];
		for(iii=0; iii < groups[iii].members; iii++)
			members[iii] = printers[groups[destid_to_gindex(destid)].printers[iii]].name;
		ipp_add_strings(ipp, IPP_TAG_PRINTER, IPP_TAG_NAME, "member-names", groups[iii].members, members, FALSE);
		}
#endif

	if(request_attrs_attr_requested(req, "queued-job-count"))
		{
		ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_INTEGER,
			"queued-job-count", queueinfo_queued_job_count(qip));
		}

#if 0
	if(request_attrs_attr_requested(req, "printer-uri-supported"))
		{
		ipp_add_template(ipp, IPP_TAG_PRINTER, IPP_TAG_URI,
			"printer-uri-supported", destid_is_group(destid) ? "/classes/%s" : "/printers/%s", destid_to_name(destid));
		}
#endif

	if(request_attrs_attr_requested(req, "uri_security_supported"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_KEYWORD,
			"uri-security-supported", "none");
		}

	if(request_attrs_attr_requested(req, "printer-location"))
		{
		if((p = queueinfo_location(qip, 0)))
			{
			p = queueinfo_hoist_value(qip, p);
			ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
				"printer-location", p);
			}
		}

	if(request_attrs_attr_requested(req, "printer-info"))
		{
		if((p = queueinfo_comment(qip)))
			{
			p = queueinfo_hoist_value(qip, p);
			ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
				"printer-info", p);
			}
		}

	if(request_attrs_attr_requested(req, "color-supported"))
		{
		ipp_add_boolean(ipp, IPP_TAG_PRINTER, IPP_TAG_BOOLEAN,
			"color-supported", queueinfo_colorDevice(qip));
		}

	if(request_attrs_attr_requested(req, "pages-per-minute"))
		{
		ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_INTEGER, "pages-per-minute", 42);
		}

	if(request_attrs_attr_requested(req, "printer-make-and-model"))
		{
		if((p = queueinfo_modelName(qip)))
			{
			p = queueinfo_hoist_value(qip, p);
			ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
				"printer-make-and-model", p);
			}
		}

	if(request_attrs_attr_requested(req, "device-uri"))
		{
		if((p = queueinfo_device_uri(qip, 0)))
			{
			p = queueinfo_hoist_value(qip, p);
			ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_URI,
				"device-uri", p);
			}
		}

	/* Which operations are supported for this printer object? */
	if(request_attrs_attr_requested(req, "operations-supported"))
		{
		int supported[] =
			{
			IPP_PRINT_JOB,
			/* IPP_PRINT_URI, */
			/* IPP_VALIDATE_JOB, */
			/* IPP_CREATE_JOB, */
			/* IPP_SEND_DOCUMENT, */
			/* IPP_SEND_URI, */
			IPP_CANCEL_JOB,
			/* IPP_GET_JOB_ATTRIBUTES, */
			IPP_GET_JOBS,
			IPP_GET_PRINTER_ATTRIBUTES,
			CUPS_GET_PRINTERS,
			CUPS_GET_CLASSES
			};
		ipp_add_integers(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
			"operations-supported", sizeof(supported) / sizeof(supported[0]), supported);
		}

	if(request_attrs_attr_requested(req, "charset-configured"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_CHARSET, 
			"charset-configured", "utf-8");
		}
	if(request_attrs_attr_requested(req, "charset-supported"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_CHARSET, 
			"charset-supported", "utf-8");
		}
	
	if(request_attrs_attr_requested(req, "natural-language-configured"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_LANGUAGE, 
			"natural-language-configured", "en-us");
		}
	if(request_attrs_attr_requested(req, "generated-natural-language-supported"))
		{
		const char *list[] = {
			"en-us",
			"ru-ru"
			};
		ipp_add_strings(ipp, IPP_TAG_PRINTER, IPP_TAG_LANGUAGE, 
			"generated-natural-language-supported", sizeof(list) / sizeof(list[0]), list);
		}

	if(request_attrs_attr_requested(req, "document-format-default"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_MIMETYPE,
			"document-format-default", "text/plain");
		}

	if(request_attrs_attr_requested(req, "document-format-supported"))
		{
		const char *list[] = {
			"text/plain",
			"application/postscript",
			"application/octet-stream"
			};
		ipp_add_strings(ipp, IPP_TAG_PRINTER, IPP_TAG_MIMETYPE,
			"document-format-supported", sizeof(list) / sizeof(list[0]), list);
		}

	/* On request, PPR will attempt to override job options
	 * already selected in the job body. */
	if(request_attrs_attr_requested(req, "pdl-override-supported"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_KEYWORD,
			"pdl-override-supported", "attempted");
		}

#if 0	
	/* measured in seconds */
	if(request_attrs_attr_requested(req, "printer-uptime"))
		{
		ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_INTEGER,
			"printer-uptime", time(NULL) - daemon_start_time);
		}
#endif
	} /* add_queue_attributes() */

/** Handler for IPP_GET_PRINTER_ATTRIBUTES */
void ipp_get_printer_attributes(struct IPP *ipp)
	{
	const char *destname;
	struct REQUEST_ATTRS *req = request_attrs_new(ipp, REQUEST_ATTRS_SUPPORTS_PRINTER);
	void *qip;
	if(!(destname = request_attrs_destname(req)))
		ipp->response_code = IPP_BAD_REQUEST;
	else if(!(qip = queueinfo_new_load_config(QUEUEINFO_PRINTER, destname)))
		ipp->response_code = IPP_NOT_FOUND;
	else
		add_queue_attributes(ipp, req, qip);
	request_attrs_free(req);
	} /* ipp_get_printer_attributes() */

/** Handler for CUPS_GET_PRINTERS */
void cups_get_printers(struct IPP *ipp)
	{
	struct REQUEST_ATTRS *req;
	DIR *dir;
	struct dirent *direntp;
	void *qip;

	DEBUG(("cups_get_printers()"));
	
	req = request_attrs_new(ipp, 0);
	
	if(!(dir = opendir(PRCONF)))
		gu_Throw("Can't open \"%s\", errno=%d (%s)", PRCONF, errno, strerror(errno));

	while((direntp = readdir(dir)))
		{
		if(direntp->d_name[0] == '.')
			continue;
		qip = queueinfo_new_load_config(QUEUEINFO_PRINTER, direntp->d_name);
		add_queue_attributes(ipp, req, qip);
		ipp_add_end(ipp, IPP_TAG_PRINTER);
		queueinfo_free(qip);
		}

	closedir(dir);
	request_attrs_free(req);
	} /* cups_get_printers() */
	
/* CUPS_GET_CLASSES */
void cups_get_classes(struct IPP *ipp)
	{
	struct REQUEST_ATTRS *req = request_attrs_new(ipp, 0);
	DIR *dir;
	struct dirent *direntp;
	void *qip;

	if(!(dir = opendir(GRCONF)))
		gu_Throw("Can't open \"%s\", errno=%d (%s)", GRCONF, errno, strerror(errno));

	while((direntp = readdir(dir)))
		{
		if(direntp->d_name[0] == '.')
			continue;
		qip = queueinfo_new_load_config(QUEUEINFO_GROUP, direntp->d_name);
		add_queue_attributes(ipp, req, qip);
		ipp_add_end(ipp, IPP_TAG_PRINTER);
		queueinfo_free(qip);
		}

	closedir(dir);
	request_attrs_free(req);
	} /* cups_get_classes() */

/*
 * Handle CUPS_GET_DEFAULT
 */
void cups_get_default(struct IPP *ipp)
	{
	struct REQUEST_ATTRS *req;
	FILE *f;
	gu_boolean found = FALSE;

	req = request_attrs_new(ipp, REQUEST_ATTRS_SUPPORTS_PPD_MAKE | REQUEST_ATTRS_SUPPORTS_LIMIT);

	/* In PPR the default destination is set by defining an alias "default".
	 * Here we open its config file, read what it points to, and return that
	 * as the default destination.
	 */ 
	if((f = fopen(ALIASCONF"/default", "r")))
		{
		char *line = NULL;
		int line_len = 80;
		char *p;
		while((line = gu_getline(line, &line_len, f)))
			{
			if((p = lmatchp(line, "ForWhat:")))
				{
				if(request_attrs_attr_requested(req, "printer-name"))
					ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_NAME, "printer-name", gu_strdup(p));
				found = TRUE;
				break;
				}
			}
		if(line)
			gu_free(line);
		fclose(f);
		}

	request_attrs_free(req);

	if(!found)
		ipp->response_code = IPP_NOT_FOUND;
	} /* cups_get_default() */

/* end of file */
