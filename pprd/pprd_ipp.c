/*
** mouse:~ppr/src/pprd/pprd_ipp.c
** Copyright 1995--2005, Trinity College Computing Center.
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
** Last modified 25 October 2005.
*/

/*
 * In this module we handle Internet Printing Protocol requests which have
 * been received by ppr-httpd and passed on to us by the ipp CGI program.
 *
 * These requests arrive as an "IPP" line which specifies the PID of
 * the ipp CGI and a list of HTTP headers and CGI variables.  The POSTed
 * data is in a temporary file (whose name can be derived from the PID)
 * and we send the response document to another temporary file.  Then
 * we send SIGUSR1 to the CGI program "ipp" to tell it that the response 
 * is ready.
 */

#include "config.h"
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "ipp_constants.h"
#include "ipp_utils.h"
#include "pprd.h"
#include "pprd.auto_h"
#include "respond.h"
#include "queueinfo.h"

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
static void printer_add_status(struct IPP *ipp, struct REQUEST_ATTRS *req, int prnid)
	{
	const char function[] = "printer_add_status";
	switch(printers[prnid].status)
		{
		case PRNSTATUS_IDLE:
			if(request_attrs_attr_requested(req, "printer-state"))
				{
				ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
					"printer-state", IPP_PRINTER_IDLE);
				}
			break;
		case PRNSTATUS_PRINTING:
			if(request_attrs_attr_requested(req, "printer-state"))
				{
				ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
					"printer-state", IPP_PRINTER_PROCESSING);
				}
			if(request_attrs_attr_requested(req, "printer-state-message"))
				{
				ipp_add_printf(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
					"printer-state-message", _("printing %s"), jobid(destid_to_name(prnid), printers[prnid].id, printers[prnid].subid));
				}
			break;
		case PRNSTATUS_CANCELING:
			if(request_attrs_attr_requested(req, "printer-state"))
				{
				ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
					"printer-state", IPP_PRINTER_PROCESSING);
				}
			if(request_attrs_attr_requested(req, "printer-state-message"))
				{
				ipp_add_printf(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
					"printer-state-message", _("canceling %s"), jobid(destid_to_name(prnid), printers[prnid].id, printers[prnid].subid));
				}
			break;
		case PRNSTATUS_SEIZING:
			if(request_attrs_attr_requested(req, "printer-state"))
				{
				ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
					"printer-state", IPP_PRINTER_PROCESSING);
				}
			if(request_attrs_attr_requested(req, "printer-state-message"))
				{
				ipp_add_printf(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
					"printer-state-message", _("seizing %s"), jobid(destid_to_name(prnid), printers[prnid].id, printers[prnid].subid));
				}
			break;
		case PRNSTATUS_STOPPING:
			if(request_attrs_attr_requested(req, "printer-state"))
				{
				ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
					"printer-state", IPP_PRINTER_PROCESSING);
				}
			if(request_attrs_attr_requested(req, "printer-state-message"))
				{
				ipp_add_printf(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
					"printer-state-message", _("stopping (printing %s)"), jobid(destid_to_name(prnid), printers[prnid].id, printers[prnid].subid));
				}
			break;
		case PRNSTATUS_HALTING:
			if(request_attrs_attr_requested(req, "printer-state"))
				{
				ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
					"printer-state", IPP_PRINTER_PROCESSING);
				}
			if(request_attrs_attr_requested(req, "printer-state-message"))
				{
				ipp_add_printf(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
					"printer-state-message", _("halting (printing %s)"), jobid(destid_to_name(prnid), printers[prnid].id, printers[prnid].subid));
				}
			break;
		case PRNSTATUS_STOPT:
			if(request_attrs_attr_requested(req, "printer-state"))
				{
				ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
					"printer-state", IPP_PRINTER_STOPPED);
				}
			if(request_attrs_attr_requested(req, "printer-state-message"))
				{
				ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
					"printer-state-message", _("stopt"), FALSE);
				}
			break;
		case PRNSTATUS_FAULT:
			if(request_attrs_attr_requested(req, "printer-state"))
				{
				ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
					"printer-state", IPP_PRINTER_STOPPED);
				}
			if(request_attrs_attr_requested(req, "printer-state-message"))
				{
				if(printers[prnid].next_error_retry)
					{
					ipp_add_printf(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
						"printer-state-message", _("fault, retry %d in %d seconds"), printers[prnid].next_error_retry, printers[prnid].countdown);
					}
				else
					{
					ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
						"printer-state-message", _("fault, no auto retry"), FALSE);
					}
				}
			break;
		case PRNSTATUS_ENGAGED:
			if(request_attrs_attr_requested(req, "printer-state"))
				{
				ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
					"printer-state", IPP_PRINTER_STOPPED);
				}
			if(request_attrs_attr_requested(req, "printer-state-message"))
				{
				ipp_add_printf(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
					"printer-state-message", _("otherwise engaged or off-line, retry %d in %d seconds"), printers[prnid].next_engaged_retry, printers[prnid].countdown);
				}
			break;
		case PRNSTATUS_STARVED:
			if(request_attrs_attr_requested(req, "printer-state"))
				{
				ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
					"printer-state", IPP_PRINTER_STOPPED);
				}
			if(request_attrs_attr_requested(req, "printer-state-message"))
				{
				ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
					"printer-state-message", _("waiting for resource ration"), FALSE);
				}
			break;

		default:
			error("%s(): invalid printer_status %d", function, printers[prnid].status);
			break;
		}

	/* !!! This is a possibly invalid dummy value. !!! */
	if(request_attrs_attr_requested(req, "printer-state-reasons"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_KEYWORD,
			"printer-state-reasons", "none", FALSE);
		}
	}

/** Handler for IPP_GET_JOBS */
static void ipp_get_jobs(struct IPP *ipp)
	{
	const char function[] = "ipp_get_jobs";
	const char *destname = NULL;
	int destname_id = -1;
	int i;
	char fname[MAX_PPR_PATH];
	FILE *qfile;
	struct QEntryFile qentryfile;
	struct REQUEST_ATTRS *req;

	req = request_attrs_new(ipp, REQUEST_ATTRS_SUPPORTS_PRINTER);

	if((destname = request_attrs_destname(req)))
		{
		if((destname_id = destid_by_name(destname)) == -1)
			{
			request_attrs_free(req);
			ipp->response_code = IPP_NOT_FOUND;
			return;
			}
		}

	lock();

	/* Loop over the queue entries. */
	for(i=0; i < queue_entries; i++)
		{
		if(destname_id != -1 && queue[i].destid != destname_id)
			continue;

		/* Read and parse the queue file. */
		ppr_fnamef(fname, "%s/%s-%d.%d", QUEUEDIR, destid_to_name(queue[i].destid), queue[i].id, queue[i].subid);
		if(!(qfile = fopen(fname, "r")))
			{
			error("%s(): can't open \"%s\", errno=%d (%s)", function, fname, errno, gu_strerror(errno) );
			continue;
			}
		qentryfile_clear(&qentryfile);
		{
		int ret = qentryfile_load(&qentryfile, qfile);
		fclose(qfile);
		if(ret == -1)
			{
			error("%s(): invalid queue file: %s", function, fname);
			continue;
			}
		}

		if(request_attrs_attr_requested(req, "job-id"))
			{
			ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_INTEGER,
				"job-id", queue[i].id);
			}
		if(request_attrs_attr_requested(req, "job-printer-uri"))
			{
			ipp_add_template(ipp, IPP_TAG_JOB, IPP_TAG_URI,
				"job-printer-uri", "/printers/%s", destid_to_name(queue[i].destid));
			}
		if(request_attrs_attr_requested(req, "job-uri"))
			{
			ipp_add_template(ipp, IPP_TAG_JOB, IPP_TAG_URI,
				"job-uri", "/jobs/%d", queue[i].id);
			}

		/* Derived from "ppop lpq" */
		if(request_attrs_attr_requested(req, "job-name"))
			{
			const char *ptr;
			if(!(ptr = qentryfile.lpqFileName))
				if(!(ptr = qentryfile.Title))
					ptr = "";
			ipp_add_string(ipp, IPP_TAG_JOB, IPP_TAG_NAME, "job-name", gu_strdup(ptr), TRUE);
			}

		/* Derived from "ppop lpq" */
		if(request_attrs_attr_requested(req, "job-originating-user-name"))
			{
			ipp_add_string(ipp, IPP_TAG_JOB, IPP_TAG_NAME, "job-originating-user-name", gu_strdup(qentryfile.user), TRUE);
			}

		/* Derived from "ppop lpq" */
		if(request_attrs_attr_requested(req, "job-k-octets"))
			{
			long int bytes = qentryfile.PassThruPDL ? qentryfile.attr.input_bytes : qentryfile.attr.postscript_bytes;
			ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_INTEGER, "job-k-octets", (bytes + 512) / 1024);
			}

		if(request_attrs_attr_requested(req, "job-state"))
			{
			ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_ENUM, "job-state", IPP_JOB_PENDING);	
			}
			
		ipp_add_end(ipp, IPP_TAG_JOB);

		qentryfile_free(&qentryfile);
		}

	unlock();

	request_attrs_free(req);
	} /* ipp_get_jobs() */

/** Handler for IPP_CANCEL_JOB */
static void ipp_cancel_job(struct IPP *ipp)
	{
	const char function[] = "ipp_cancel_job";
	struct REQUEST_ATTRS *req;
	int jobid;
	int i;

	req = request_attrs_new(ipp, REQUEST_ATTRS_SUPPORTS_JOB);
	
	if((jobid = request_attrs_jobid(req)) == -1)
		{
		request_attrs_free(req);
		ipp->response_code = IPP_NOT_FOUND;
		return;
		}

	lock();

	/* Loop over the queue entries.
	 * This code is copied from pprd_ppop.c.  We haven't tried to generalize 
	 * it because we plan to remove the command from pprd_ppop.c and have
	 * ppop use the IPP command.
	 */ 
	for(i=0; i < queue_entries; i++)
		{
		if(queue[i].id == jobid)
			{
			int prnid;

			/* !!! Access checking should go here !!! */
			
			if((prnid = queue[i].status) >= 0)	/* if pprdrv is active */
				{
				/* If it is printing we can say it is now canceling, but
				   if it is halting or stopping we don't want to mess with
				   that.
				   */
				if(printers[prnid].status == PRNSTATUS_PRINTING)
					printer_new_status(&printers[prnid], PRNSTATUS_CANCELING);

				/* Set flag so that job will be deleted when pprdrv dies. */
				printers[prnid].cancel_job = TRUE;

				/* Change the job status to "being canceled". */
				queue_p_job_new_status(&queue[i], STATUS_CANCEL);

				/* Kill pprdrv. */
				pprdrv_kill(prnid);
				}

			/* If a cancel is in progress, */
			else if(prnid == STATUS_CANCEL)
				{
				/* nothing to do */
				}

			/* If a hold is in progress, do what we woudld do if the job were being
			   printed, but without the need to kill() pprdrv again.  This
			   is tough because the queue doesn't contain the printer
			   id anymore. */
			else if(prnid == STATUS_SEIZING)
				{
				for(prnid = 0; prnid < printer_count; prnid++)
					{
					if(printers[prnid].jobdestid == queue[i].destid
							&& printers[prnid].id == queue[i].id
							&& printers[prnid].subid == queue[i].subid
							)
						{
						if(printers[prnid].status == PRNSTATUS_SEIZING)
							printer_new_status(&printers[prnid], PRNSTATUS_CANCELING);
						printers[prnid].hold_job = FALSE;
						printers[prnid].cancel_job = TRUE;
						queue_p_job_new_status(&queue[i], STATUS_CANCEL);
						break;
						}
					}
				if(prnid == printer_count)
					error("%s(): couldn't find printer that job is printing on", function);
				}

			/* If the job is not being printed, we can delete it right now. */
			else
				{
				/*
				** If job status is not arrested,
				** use the responder to inform the user that we are canceling it.
				*/
				if(queue[i].status != STATUS_ARRESTED)
					{
					respond(queue[i].destid, queue[i].id, queue[i].subid,
							-1,	  /* impossible printer */
							RESP_CANCELED);
					}

				/* Remove the job from the queue array and its files form the spool directories. */
				queue_dequeue_job(queue[i].destid, queue[i].id, queue[i].subid);

				i--;		/* compensate for deletion */
				}
			}
		}
	
	unlock();

	request_attrs_free(req);
	} /* ipp_cancel_job() */

/*
 * This function is the meat of IPP_GET_PRINTER_ATTRIBUTES,
 * CUPS_GET_PRINTERS and CUPS_GET_CLASSES.
 */
static void add_queue_attributes(struct IPP *ipp, struct REQUEST_ATTRS *req, int destid)
	{
	if(request_attrs_attr_requested(req, "printer-name"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_NAME,
			"printer-name", destid_to_name(destid), FALSE);
		}

	if(request_attrs_attr_requested(req, "printer-uri"))
		{
		ipp_add_template(ipp, IPP_TAG_PRINTER, IPP_TAG_URI,	
			"printer-uri", "/printers/%s", destid_to_name(destid));
		}

	if(request_attrs_attr_requested(req, "printer-is-accepting-jobs"))
		{
		ipp_add_boolean(ipp, IPP_TAG_PRINTER, IPP_TAG_BOOLEAN,
			"printer-is-accepting-jobs",
			destid_is_group(destid)
				? groups[destid_to_gindex(destid)].accepting
				: printers[destid].accepting
			);
		}

	if(destid_is_printer(destid))
		printer_add_status(ipp, req, destid);
	/* else */

	if(destid_is_group(destid) && request_attrs_attr_requested(req, "member-names"))
		{
		int iii;
		const char *members[MAX_GROUPSIZE];
		for(iii=0; iii < groups[iii].members; iii++)
			members[iii] = printers[groups[destid_to_gindex(destid)].printers[iii]].name;
		ipp_add_strings(ipp, IPP_TAG_PRINTER, IPP_TAG_NAME, "member-names", groups[iii].members, members, FALSE);
		}

	if(request_attrs_attr_requested(req, "queued-job-count"))
		{
		int iii, count;
		lock();
		for(iii=count=0; iii < queue_entries; iii++)
			{
			if(queue[iii].destid == destid)
				count++;
			}
		unlock();
		ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_INTEGER,
			"queued-job-count", count);
		}

	if(request_attrs_attr_requested(req, "printer-uri-supported"))
		{
		ipp_add_template(ipp, IPP_TAG_PRINTER, IPP_TAG_URI,
			"printer-uri-supported", destid_is_group(destid) ? "/classes/%s" : "/printers/%s", destid_to_name(destid));
		}

	if(request_attrs_attr_requested(req, "uri_security_supported"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_KEYWORD,
			"uri-security-supported", "none", FALSE);
		}

	gu_Try {
		void *qip = queueinfo_new_load_config(QUEUEINFO_PRINTER, destid_to_name(destid));
		const char *p;

		if(request_attrs_attr_requested(req, "printer-location"))
			{
			if((p = queueinfo_location(qip, 0)))
				{
				p = queueinfo_hoist_value(qip, p);
				ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT, "printer-location", p, TRUE);
				}
			}

		if(request_attrs_attr_requested(req, "printer-info"))
			{
			if((p = queueinfo_comment(qip)))
				{
				p = queueinfo_hoist_value(qip, p);
				ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT, "printer-info", p, TRUE);
				}
			}

		if(request_attrs_attr_requested(req, "color-supported"))
			{
			ipp_add_boolean(ipp, IPP_TAG_PRINTER, IPP_TAG_BOOLEAN, "color-supported", queueinfo_colorDevice(qip));
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
				ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT, "printer-make-and-model", p, TRUE);
				}
			}

		if(request_attrs_attr_requested(req, "device-uri"))
			{
			if((p = queueinfo_device_uri(qip, 0)))
				{
				p = queueinfo_hoist_value(qip, p);
				ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_URI, "device-uri", p, TRUE);
				}
			}

		queueinfo_free(qip);
		}
	gu_Catch
		{
		/* nothing to do, just don't issue those items */
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
			"charset-configured", "utf-8", FALSE);
		}
	if(request_attrs_attr_requested(req, "charset-supported"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_CHARSET, 
			"charset-supported", "utf-8", FALSE);
		}
	
	if(request_attrs_attr_requested(req, "natural-language-configured"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_LANGUAGE, 
			"natural-language-configured", "en-us", FALSE);
		}
	if(request_attrs_attr_requested(req, "generated-natural-language-supported"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_LANGUAGE, 
			"generated-natural-language-supported", "en-us", FALSE);
		}

	if(request_attrs_attr_requested(req, "document-format-default"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_MIMETYPE,
			"document-format-default", "text/plain", FALSE);
		}

	if(request_attrs_attr_requested(req, "document-format-supported"))
		{
		const char *list[] = {
			"text/plain",
			"application/postscript",
			"application/octet-stream"
			};
		ipp_add_strings(ipp, IPP_TAG_PRINTER, IPP_TAG_MIMETYPE,
			"document-format-supported", sizeof(list) / sizeof(list[0]), list, FALSE);
		}

	/* On request, PPR will attempt to override job options
	 * already selected in the job body. */
	if(request_attrs_attr_requested(req, "pdl-override-supported"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_KEYWORD,
			"pdl-override-supported", "attempted", FALSE);
		}
	
	/* measured in seconds */
	if(request_attrs_attr_requested(req, "printer-uptime"))
		{
		ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_INTEGER,
			"printer-uptime", time(NULL) - daemon_start_time);
		}

	} /* add_queue_attributes() */

/** Handler for IPP_GET_PRINTER_ATTRIBUTES */
static void ipp_get_printer_attributes(struct IPP *ipp)
    {
	FUNCTION4DEBUG("ipp_get_printer_attributes")
	const char *destname;
	int destid;
	struct REQUEST_ATTRS *req = request_attrs_new(ipp, REQUEST_ATTRS_SUPPORTS_PRINTER);
	if(!(destname = request_attrs_destname(req)))
		ipp->response_code = IPP_BAD_REQUEST;
	else if((destid = destid_by_name(destname)) == -1)
		ipp->response_code = IPP_NOT_FOUND;
	else
		add_queue_attributes(ipp, req, destid);
	request_attrs_free(req);
    } /* ipp_get_printer_attributes() */

/** Handler for CUPS_GET_PRINTERS */
static void cups_get_printers(struct IPP *ipp)
	{
	int i;
	struct REQUEST_ATTRS *req = request_attrs_new(ipp, 0);
	lock();
	for(i=0; i < printer_count; i++)
		{
		if(printers[i].status == PRNSTATUS_DELETED)
			continue;
		add_queue_attributes(ipp, req, i);
		ipp_add_end(ipp, IPP_TAG_PRINTER);
		}
	unlock();
	request_attrs_free(req);
	} /* cups_get_printers() */
	
/* CUPS_GET_CLASSES */
static void cups_get_classes(struct IPP *ipp)
	{
	int i;
	struct REQUEST_ATTRS *req = request_attrs_new(ipp, 0);
	lock();
	for(i=0; i < group_count; i++)
		{
		if(groups[i].deleted)
			continue;
		add_queue_attributes(ipp, req, i);
		ipp_add_end(ipp, IPP_TAG_PRINTER);
		}
	unlock();
	request_attrs_free(req);
	} /* cups_get_classes() */

void ipp_dispatch(const char command[])
	{
	const char function[] = "ipp_dispatch";
	char fname_in[MAX_PPR_PATH];
	char fname_out[MAX_PPR_PATH];
	const char *p;
	long int ipp_cgi_pid;
	int in_fd, out_fd;
	struct stat statbuf;
	char *command_scratch = NULL;
	struct IPP *ipp = NULL;

	debug("%s(): %s", function, command);
	
	if(!(p = lmatchsp(command, "IPP")))
		{
		error("%s(): command missing", function);
		return;
		}

	if((ipp_cgi_pid = atol(p)) == 0)
		{
		error("%s(): no PID for reply", function);
		return;
		}
	p += strspn(p, "0123456789");
	p += strspn(p, " ");

	in_fd = out_fd = -1;
	gu_Try
		{
		ppr_fnamef(fname_in, "%s/ppr-ipp/%ld-in", TEMPDIR, ipp_cgi_pid);
		if((in_fd = open(fname_in, O_RDONLY)) == -1)
			gu_Throw("can't open \"%s\", errno=%d (%s)", fname_in, errno, gu_strerror(errno));
		gu_set_cloexec(in_fd);
		if(fstat(in_fd, &statbuf) == -1)
			gu_Throw("fstat(%d, &statbuf) failed, errno=%d (%s)", in_fd, errno, gu_strerror(errno));

		ppr_fnamef(fname_out, "%s/ppr-ipp/%ld-out", TEMPDIR, ipp_cgi_pid);
		if((out_fd = open(fname_out, O_WRONLY | O_EXCL | O_CREAT, UNIX_660)) == -1)
			gu_Throw("can't create \"%s\", errno=%d (%s)", fname_out, errno, gu_strerror(errno));
		gu_set_cloexec(out_fd);

		{
		char *root=NULL, *path_info=NULL, *remote_user=NULL, *remote_addr=NULL;
		char *opts = command_scratch = gu_strdup(p);
		char *name, *value;
		while((name = gu_strsep(&opts, " ")) && *name)
			{
			if(!(value = strchr(name, '=')))
				gu_Throw("parse error, no = in \"%s\"", name);
			*(value++) = '\0';

			if(strcmp(name, "ROOT") == 0)
				root = value;
			else if(strcmp(name, "PATH_INFO") == 0)
				path_info = value;
			else if(strcmp(name, "REMOTE_USER") == 0)
				remote_user = value;
			else if(strcmp(name, "REMOTE_ADDR") == 0)
				remote_addr = value;
			else
				debug("%s(): unknown parameter %s=\"%s\"", function, name, value);
			}

		/* Create an IPP object and read the request from the temporary file. */
		ipp = ipp_new(root, path_info, statbuf.st_size, in_fd, out_fd);
		if(remote_user)
			ipp_set_remote_user(ipp, remote_user);
		if(remote_addr)
			ipp_set_remote_addr(ipp, remote_addr);
		ipp_parse_request_header(ipp);
		ipp_parse_request_body(ipp);
		}
		
		if(ipp_validate_request(ipp))
			{
			debug("%s(): dispatching operation 0x%.2x", function, ipp->operation_id);
			switch(ipp->operation_id)
				{
				case IPP_GET_JOBS:
					ipp_get_jobs(ipp);
					break;
				case IPP_GET_PRINTER_ATTRIBUTES:
					ipp_get_printer_attributes(ipp);
					break;
				case IPP_CANCEL_JOB:
					ipp_cancel_job(ipp);
					break;
				case CUPS_GET_CLASSES:
					cups_get_classes(ipp);
					break;
				case CUPS_GET_PRINTERS:
					cups_get_printers(ipp);
					break;
				default:
					gu_Throw("unsupported operation");
					break;
				}
			}

		if(ipp->response_code == IPP_OK)
			ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_TEXT, "status-message", "successful-ok", FALSE);

		ipp_send_reply(ipp, FALSE);
		}
	gu_Final {
		if(ipp)
			ipp_delete(ipp);
		if(command_scratch)
			gu_free(command_scratch);
		if(in_fd != -1)
			close(in_fd);
		if(out_fd != -1)
			close(out_fd);

		/* Close the output file and tell the IPP CGI program to take it away. */
		debug("Sending signal to IPP CGI...");
		if(kill((pid_t)ipp_cgi_pid, SIGUSR1) == -1)
			{
			debug("%s(): kill(%ld, SIGUSR1) failed, errno=%d (%s), deleting reply file", function, (long)ipp_cgi_pid, errno, gu_strerror(errno));
			unlink(fname_in);
			unlink(fname_out);
			}
		}
	gu_Catch {
		error("%s(): %s", function, gu_exception);
		}

	debug("%s(): done", function);
	} /* end of ipp_dispatch() */

/* end of file */
