/*
** mouse:~ppr/src/ipp/ipp_destinations.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 20 April 2006.
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
#include "global_structs.h"
#include "ipp_constants.h"
#include "ipp_utils.h"
#include "queueinfo.h"
#include "ipp.h"

/** Convert a PPR printer's status to IPP status
 *
 * printer-state (RFC 2911 4.4.11)
 * 		idle
 * 		processing
 * 		stopped
 * printer-state-reasons (RFC 2911 4.4.12)
 * 			
 * printer-state-message (RFC 2911 4.4.13)
 * 		free-form human-readable text
 */
static void printer_add_status(struct IPP *ipp, struct REQUEST_ATTRS *req, QUEUE_INFO qip)
	{
	int state;
	const char *state_reasons[10];
	int state_reasons_count = 0;
	const char *state_message = NULL;

	switch(queueinfo_status(qip))
		{
		case PRNSTATUS_IDLE:
			state = IPP_PRINTER_IDLE;
			break;
		case PRNSTATUS_PRINTING:
			state = IPP_PRINTER_PROCESSING;
			break;
		case PRNSTATUS_CANCELING:
		case PRNSTATUS_SEIZING:
			state = IPP_PRINTER_PROCESSING;
			break;
		case PRNSTATUS_STOPPING:
		case PRNSTATUS_HALTING:
			state = IPP_PRINTER_PROCESSING;
			state_reasons[state_reasons_count++] = "moving-to-paused";
			break;
		case PRNSTATUS_STOPT:
			state = IPP_PRINTER_STOPPED;
			state_reasons[state_reasons_count++] = "paused";
			break;
		case PRNSTATUS_FAULT:
			state = IPP_PRINTER_STOPPED;
			state_reasons[state_reasons_count++] = "other";
			break;
		case PRNSTATUS_ENGAGED:
			state = IPP_PRINTER_STOPPED;
			state_reasons[state_reasons_count++] = "connecting-to-device";
			break;
		case PRNSTATUS_STARVED:
			state = IPP_PRINTER_STOPPED;
			state_reasons[state_reasons_count++] = "other";
			break;
		default:
			debug("invalid printer state");
			state = IPP_PRINTER_IDLE;
			state_reasons[state_reasons_count++] = "other";
			break;
		}

	if(request_attrs_attr_requested(req, "printer-state"))
		{
		ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
			"printer-state", state);
		}

	if(state_reasons_count == 0)
		state_reasons[state_reasons_count++] = "none";
	if(request_attrs_attr_requested(req, "printer-state-reasons"))
		{
		ipp_add_strings(ipp, IPP_TAG_PRINTER, IPP_TAG_KEYWORD,
			"printer-state-reasons", state_reasons_count, state_reasons);
		}

	if(request_attrs_attr_requested(req, "printer-state-message") && state_message)
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
			"printer-state-message", state_message);
		}
	}

/*
 * This function is the meat of IPP_GET_PRINTER_ATTRIBUTES,
 * CUPS_GET_PRINTERS and CUPS_GET_CLASSES.
 *
 * For compatibility with CupsClient (which is a badly broken IPP client), we
 * return these attributes in the same order as CUPS does.
 */
static void add_queue_attributes(struct IPP *ipp, struct REQUEST_ATTRS *req, QUEUE_INFO qip)
	{
	const char *p;
	const char *destname;

	destname = queueinfo_hoist_value(qip, queueinfo_name(qip));

	if(request_attrs_attr_requested(req, "printer-uri-supported"))
		{
		ipp_add_template(ipp, IPP_TAG_PRINTER, IPP_TAG_URI,
			"printer-uri-supported", queueinfo_is_group(qip) ? "/classes/%s" : "/printers/%s", destname);
		}

	/* Add printer-state, printer-state-reasons, and printer-state-message. */
	if(!queueinfo_is_group(qip))
		printer_add_status(ipp, req, qip);

	if(request_attrs_attr_requested(req, "printer-is-accepting-jobs"))
		{
		ipp_add_boolean(ipp, IPP_TAG_PRINTER, IPP_TAG_BOOLEAN,
			"printer-is-accepting-jobs",
			queueinfo_accepting(qip)
			);
		}

	/* printer-is-shared */

	/* RFC 2911 4.4.29, measured in seconds
	 * We follow the lead of CUPS and just return Unix time.
	 */
	if(request_attrs_attr_requested(req, "printer-up-time"))
		{
		ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_INTEGER,
			"printer-up-time", time(NULL));
		}

	/* printer-state-change-time */

	/* printer-current-time */

	/* printer-error-policy */

	/* printer-op-policy */

	if(request_attrs_attr_requested(req, "queued-job-count"))
		{
		ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_INTEGER,
			"queued-job-count", queueinfo_queued_job_count(qip));
		}

	/* uri-authentication-supported */

	if(request_attrs_attr_requested(req, "uri-security-supported"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_KEYWORD,
			"uri-security-supported", "none");
		}

	if(request_attrs_attr_requested(req, "printer-name"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_NAME,
			"printer-name", destname);
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

	/* printer-more-info */

	/* job-quota-period */

	/* job-k-limit */

	/* job-page-limit */

	/* job-sheets-default */

	if(request_attrs_attr_requested(req, "device-uri"))
		{
		if((p = queueinfo_device_uri(qip, 0)))
			{
			p = queueinfo_hoist_value(qip, p);
			ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_URI,
				"device-uri", p);
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

	/* media-supported */

	/* media-default */

	/* port-monitor */

	/* port-monitor-supported */

	/* finishings-supported */

	/* finishings-default */

	/* printer-type */

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

	/* compression-supported */

	/* copies-default */

	/* copies-supported */

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

	if(request_attrs_attr_requested(req, "generated-natural-language-supported"))
		{
		const char *list[] = {
			"en-us",
			"ru-ru"
			};
		ipp_add_strings(ipp, IPP_TAG_PRINTER, IPP_TAG_LANGUAGE, 
			"generated-natural-language-supported", sizeof(list) / sizeof(list[0]), list);
		}

	/* ipp-versions-supported */

	/* job-hold-until-default */

	/* job-hold-until-supported */

	/* job-priority-default */

	/* job-priority-supported */

	/* job-sheets-supported */

	/* multiple-document-handling-supported */

	/* multiple-document-jobs-supported */

	/* multiple-operation-timeout */

	if(request_attrs_attr_requested(req, "natural-language-configured"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_LANGUAGE, 
			"natural-language-configured", "en-us");
		}

	/* notify-attributes-supported */

	/* notify-lease-duration-supported */

	/* notify-max-events-supported */

	/* notify-events-default */

	/* notify-events-supported */

	/* notify-pull-method-supported */

	/* notify-schemes-supported */

	/* number-up-default */

	/* number-up-supported */

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

	/* orientation-requested-default */

	/* orientation-requested-supported */

	/* page-ranges-supported */

	/* On request, PPR will attempt to override job options
	 * already selected in the job body. */
	if(request_attrs_attr_requested(req, "pdl-override-supported"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_KEYWORD,
			"pdl-override-supported", "attempted");
		}


	/* printer-error-policy-supported */

	/* printer-op-policy-supported */	
	
	if(queueinfo_is_group(qip) && request_attrs_attr_requested(req, "member-names"))
		{
		int iii;
		const char *members[MAX_GROUPSIZE];
		const char *temp;
		for(iii=0; iii < MAX_GROUPSIZE; iii++)
			{
			if(!(temp = queueinfo_membername(qip, iii)))
				break;
			members[iii] = queueinfo_hoist_value(qip, temp);
			}
		ipp_add_strings(ipp, IPP_TAG_PRINTER, IPP_TAG_NAME, "member-names", iii, members);
		}

	} /* add_queue_attributes() */

/** Handler for IPP_GET_PRINTER_ATTRIBUTES */
void ipp_get_printer_attributes(struct IPP *ipp)
	{
	struct REQUEST_ATTRS *req;
	struct URI *printer_uri;
	void *qip;

	req = request_attrs_new(ipp, 0);
	if(!(printer_uri = ipp_claim_uri(ipp, "printer-uri")))
		ipp->response_code = IPP_BAD_REQUEST;
	else if(!printer_uri->dirname
			|| strcmp(printer_uri->dirname, "/printer") != 0
			|| !printer_uri->basename
			|| !(qip = queueinfo_new_load_config(QUEUEINFO_PRINTER, printer_uri->basename))
			)
		ipp->response_code = IPP_NOT_FOUND;
	else
		add_queue_attributes(ipp, req, qip);

	if(printer_uri)
		gu_uri_free(printer_uri);
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
	struct REQUEST_ATTRS *req;
	DIR *dir;
	struct dirent *direntp;
	void *qip;

	DEBUG(("cups_get_classes()"));

   	req = request_attrs_new(ipp, 0);

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
	void *qip;

	req = request_attrs_new(ipp, 0);

	/* In PPR the default destination is set by defining an alias "default". */ 
	if((qip = queueinfo_new_load_config(QUEUEINFO_ALIAS, "default")))
		{
		add_queue_attributes(ipp, req, qip);
		ipp_add_end(ipp, IPP_TAG_PRINTER);
		queueinfo_free(qip);
		}
	else
		{
		ipp->response_code = IPP_NOT_FOUND;
		}

	request_attrs_free(req);
	} /* cups_get_default() */

/* end of file */
