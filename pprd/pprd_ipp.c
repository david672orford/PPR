/*
** mouse:~ppr/src/pprd/pprd_ipp.c
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
** Last modified 10 April 2006.
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

/** Handler for IPP_GET_JOBS */
static void ipp_get_jobs(struct IPP *ipp)
	{
	const char function[] = "ipp_get_jobs";
	const char *destname = NULL;
	int destname_id = -1;
	int jobid = -1;
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

	jobid = request_attrs_jobid(req);

	lock();

	/* Loop over the queue entries. */
	for(i=0; i < queue_entries; i++)
		{
		if(destname_id != -1 && queue[i].destid != destname_id)
			continue;
		if(jobid != -1 && queue[i].id != jobid)
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

/** Handler for IPP_CANCEL_JOB
 *
 * !!! Does not yet return proper result codes !!!
 */
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
				if(printers[prnid].spool_state.status == PRNSTATUS_PRINTING)
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
					if(printers[prnid].job_destid == queue[i].destid
							&& printers[prnid].job_id == queue[i].id
							&& printers[prnid].job_subid == queue[i].subid
							)
						{
						if(printers[prnid].spool_state.status == PRNSTATUS_SEIZING)
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

	DODEBUG_IPP(("%s(): %s", function, command));
	
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
			DODEBUG_IPP(("%s(): dispatching operation 0x%.4x (%s)", function, ipp->operation_id, ipp_operation_id_to_str(ipp->operation_id)));
			switch(ipp->operation_id)
				{
				case IPP_GET_JOBS:
					ipp_get_jobs(ipp);
					break;
				case IPP_CANCEL_JOB:
					ipp_cancel_job(ipp);
					break;
				default:
					gu_Throw("unsupported operation: 0x%.2x (%s)", ipp->operation_id, ipp_operation_id_to_str(ipp->operation_id));
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
		DODEBUG_IPP(("Sending signal to IPP CGI..."));
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

	DODEBUG_IPP(("%s(): done", function));
	} /* end of ipp_dispatch() */

/* end of file */
