/*
** mouse:~ppr/src/ipp/ipp_jobs.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 18 April 2006.
*/

/*
 * This is PPR's IPP (Internet Printer Protocol) server.
 * This module contains routines for reporting on printers and groups.
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
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

struct IPP_QUEUE_ENTRY {
	INT16_T priority;
	unsigned int sequence_number;
	const char *destname;
	INT16_T id;
	INT16_T subid;
	};

/* Compare two job entries for qsort(). */
static int ipp_queue_entry_compare(const void *p1, const void *p2)
	{
	const struct IPP_QUEUE_ENTRY *e1 = p1;
	const struct IPP_QUEUE_ENTRY *e2 = p2;

	if(e1->priority < e2->priority)
		return 1;
	if(e1->priority > e2->priority)
		return -1;
	if(e1->sequence_number < e2->sequence_number)
		return -1;
	if(e1->sequence_number > e2->sequence_number)
		return 1;	

	return 0;
	}

static int ipp_queue_entry_compare_reversed(const void *p1, const void *p2)
	{
	return ipp_queue_entry_compare(p2, p1);
	}

/* Load all queue entries queued for destination destname[].
 * Set set_used to the number of entries found and return a pointer
 * to the array.  If completed is true, return completed and arrested jobs
 * only and reverse the sorting order (as specified by RFC 2911 3.2.6.1).
 * If it is false, return only jobs which are still potentially printable.
 */
static struct IPP_QUEUE_ENTRY *ipp_load_queue(const char destname[], int *set_used, gu_boolean completed)
	{
	const char function[] = "load_queue";
	struct IPP_QUEUE_ENTRY *queue = NULL;
	int queue_used = 0;
	int queue_space = 0;
	DIR *dir;
	struct dirent *direntp;
	char *p;
	char fname[MAX_PPR_PATH];
	int fd;
	char buffer[64];
	int status;
	void *destnames = gu_pch_new(64);

	if(!(dir = opendir(QUEUEDIR)))
		gu_Throw(_("%s(): %s(\"%s\") failed, errno=%d (%s)"), function, "opendir", QUEUEDIR, errno, strerror(errno));

	while((direntp = readdir(dir)))
		{
		/* Skip ".", "..", and hidden files. */
		if(direntp->d_name[0] == '.')
			continue;

		/* Locate hyphen between destname and ID */
		if(!(p = strrchr(direntp->d_name, '-')))
			continue;

		/* If destname does not match, skip this file. */
		if(destname && ((p - direntp->d_name) != strlen(destname) || strncmp(direntp->d_name, destname, strlen(destname)) != 0))
			continue;
	
		/* Expand the queue array if necessary. */	
		if(queue_used == queue_space)
			{
			queue_space += 1024;
			queue = gu_realloc(queue, queue_space, sizeof(struct IPP_QUEUE_ENTRY));
			}

		/* Parse the part of the filename after the hyphen in order to extract the 
		 * ID and subid. */
		if(gu_sscanf(p+1, "%d.%hd", &queue[queue_used].id, &queue[queue_used].subid) != 2)
			{
			fprintf(stderr, X_("Can't parse id.subid: %s\n"), p+1);
			continue;
			}

		/* Open the queue file and extract the fields which we will use for sorting. */
		ppr_fnamef(fname, "%s/%s", QUEUEDIR, direntp->d_name);
		if((fd = open(fname, O_RDONLY)) == -1)
			{
			fprintf(stderr, X_("Can't open \"%s\", errno=%d (%s)\n"), fname, errno, strerror(errno));
			continue;
			}
		if(read(fd, buffer, sizeof(buffer)) != sizeof(buffer))
			{
			fprintf(stderr, X_("Can't read \"%s\"\n"), fname);
			continue;
			}
		if(gu_sscanf(buffer, "PPRD: %hx %x %hx", &queue[queue_used].priority, &queue[queue_used].sequence_number, &status) != 3)
			{
			fprintf(stderr, X_("Can't parse contents of \"%s\"\n"), fname);
			continue;
			}

		if(status == STATUS_FINISHED || status == STATUS_ARRESTED)
			{
			if(!completed)
				continue;
			}
		else
			{
			if(completed)
				continue;
			}
		
		if(destname)		/* all the same queue */
			queue[queue_used].destname = destname;
		else				/* possibly different, don't save more than one copy */
			{
			*p = '\0';
			if(!(queue[queue_used].destname = gu_pch_get(destnames, direntp->d_name)))
				{
				char *temp = gu_strdup(direntp->d_name);
				gu_pch_set(destnames, direntp->d_name, temp);
				queue[queue_used].destname = temp;
				}
			}
		
		queue_used++;
		}
	
	closedir(dir);

	if(completed)
		qsort(queue, queue_used, sizeof(struct IPP_QUEUE_ENTRY), ipp_queue_entry_compare_reversed);
	else
		qsort(queue, queue_used, sizeof(struct IPP_QUEUE_ENTRY), ipp_queue_entry_compare);

	*set_used = queue_used;
	return queue;
	} /* ipp_load_queue() */

/** Handler for IPP_GET_JOBS */
void ipp_get_jobs(struct IPP *ipp)
	{
	const char function[] = "ipp_get_jobs";
	struct URI *printer_uri;
	int limit;
	struct REQUEST_ATTRS *req;
	const char *which_jobs;
	struct IPP_QUEUE_ENTRY *queue;
	int queue_num_entries;
	int iii;

	DEBUG(("%s()", function));
	
	printer_uri = ipp_claim_uri(ipp, "printer-uri");
	ipp_claim_name(ipp, "requesting-user-name");
	limit = ipp_claim_positive_integer(ipp, "limit");
	which_jobs = ipp_claim_keyword(ipp, "which-jobs", "not-completed", "completed", NULL);
	
	req = request_attrs_new(ipp, REQ_SUPPORTS_JOBS);

	if(!printer_uri)	
		{
		DEBUG(("%s(): no printer-uri", function));
		ipp->response_code = IPP_BAD_REQUEST;
		return;
		}

	if(!printer_uri->dirname 
			|| (strcmp(printer_uri->dirname, "/printers") != 0 && strcmp(printer_uri->dirname, "/classes") != 0)
			|| !printer_uri->basename)
		{
		DEBUG(("%s(): not a known printer", function));
		ipp->response_code = IPP_NOT_FOUND;
		return;
		}

	queue = ipp_load_queue(
			printer_uri->basename,
		   	&queue_num_entries,
		   	which_jobs && strcmp(which_jobs, "completed") == 0
			);
	DEBUG(("%s(): %d entries", function, queue_num_entries));

	/* If the number of entries found exceeds the limit, clip. */
	if(req->limit != -1 && queue_num_entries > req->limit)
		queue_num_entries = req->limit;

	for(iii=0; iii < queue_num_entries; iii++)
		{
		struct QEntryFile qentryfile;
		int job_state;
		const char *job_state_reasons[10];
		int job_state_reasons_count = 0;

		/* Read and parse the queue file.  We already know the file exists
		 * and we can open it. */
		{
		char fname[MAX_PPR_PATH];
		FILE *qfile;
		int ret;
		ppr_fnamef(fname, "%s/%s-%d.%d", QUEUEDIR, queue[iii].destname, queue[iii].id, queue[iii].subid);
		DEBUG(("%s(): reading \"%s\"", function, fname));
		if(!(qfile = fopen(fname, "r")))
			gu_Throw(X_("%s(): can't open \"%s\", errno=%d (%s)"), function, fname, errno, gu_strerror(errno) );
		qentryfile_clear(&qentryfile);
		ret = qentryfile_load(&qentryfile, qfile);
		fclose(qfile);
		if(ret == -1)
			{
			fprintf(stderr, X_("%s(): invalid queue file: %s"), function, fname);
			continue;
			}
		}

		/* Convert the job state from PPR format to IPP format. */
		switch(qentryfile.spool_state.status)
			{
			case STATUS_WAITING:				/* waiting for printer */
				job_state = IPP_JOB_PENDING;
				job_state_reasons[job_state_reasons_count++] = "job-queued";
				break;
			case STATUS_HELD:					/* put on hold by user */
				job_state = IPP_JOB_HELD;
				job_state_reasons[job_state_reasons_count++] = "";
				break;
			case STATUS_WAITING4MEDIA:			/* proper media not mounted */
				job_state = IPP_JOB_HELD;
				job_state_reasons[job_state_reasons_count++] = "resources-not-ready";
				break;
			case STATUS_ARRESTED:				/* automaticaly put on hold because of a job error */
				job_state = IPP_JOB_COMPLETED;
				job_state_reasons[job_state_reasons_count++] = "document-format-error";
				job_state_reasons[job_state_reasons_count++] = "job-completed-with-errors";
				break;
			case STATUS_CANCEL:					/* being canceled */
			case STATUS_SEIZING:				/* going from printing to held (get rid of this) */
				job_state = IPP_JOB_PROCESSING;
				job_state_reasons[job_state_reasons_count++] = "processing-to-stop-point";
				job_state_reasons[job_state_reasons_count++] = "job-canceled-by-user";
				break;
			case STATUS_STRANDED:				/* no printer can print it */
				job_state = IPP_JOB_HELD;
				job_state_reasons[job_state_reasons_count++] = "resources-not-ready";
				break;
			case STATUS_FINISHED:				/* job has been printed */
				job_state = IPP_JOB_COMPLETED;
				job_state_reasons[job_state_reasons_count++] = "job-completed-successfully";
				break;
			case STATUS_FUNDS:					/* insufficient funds to print it */
				job_state = IPP_JOB_HELD;
				job_state_reasons[job_state_reasons_count++] = "resources-not-ready";
				break;
			default:
				job_state = IPP_JOB_PROCESSING;
				job_state_reasons[job_state_reasons_count++] = "job-printing";
				break;
			}
		if(job_state_reasons_count == 0)
			job_state_reasons[job_state_reasons_count++] = "none";

		/* job-more-info */
		
		if(request_attrs_attr_requested(req, "job-uri"))
			{
			ipp_add_template(ipp, IPP_TAG_JOB, IPP_TAG_URI,
				"job-uri", "/jobs/%d", queue[iii].id);
			}

		/* job-printer-up-time */

		/* printer-uri */

		/* Derived from "ppop lpq" */
		if(request_attrs_attr_requested(req, "job-originating-user-name"))
			{
			ipp_add_string(ipp, IPP_TAG_JOB, IPP_TAG_NAME,
				"job-originating-user-name", gu_strdup(qentryfile.user));
			}

		/* Derived from "ppop lpq" */
		/* Not correct */
		if(request_attrs_attr_requested(req, "job-name"))
			{
			const char *ptr;
			if(!(ptr = qentryfile.lpqFileName))
				if(!(ptr = qentryfile.Title))
					ptr = "";
			ipp_add_string(ipp, IPP_TAG_JOB, IPP_TAG_NAME, "job-name", gu_strdup(ptr));
			}

		/* document-format */

		/* job-sheets */
		
		/* job-hold-until */

		/* job-priority */

		/* job-originating-host-name */
		
		if(request_attrs_attr_requested(req, "job-id"))
			{
			ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_INTEGER,
				"job-id", queue[iii].id);
			}

		if(request_attrs_attr_requested(req, "job-state"))
			{
			ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_ENUM,
				"job-state", job_state);
			}

		/* job-media-sheets-completed */

		if(request_attrs_attr_requested(req, "job-printer-uri"))
			{
			ipp_add_template(ipp, IPP_TAG_JOB, IPP_TAG_URI,
				"job-printer-uri", "/printers/%s", queue[iii].destname);
			}

		/* Derived from "ppop lpq" */
		/* May not be correct */
		if(request_attrs_attr_requested(req, "job-k-octets"))
			{
			long int bytes = qentryfile.PassThruPDL ? qentryfile.attr.input_bytes : qentryfile.attr.postscript_bytes;
			ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_INTEGER,
				"job-k-octets", (bytes + 512) / 1024);
			}

		/* time-at-creation */

		/* time-at-processing */

		/* time-at-completed */
		
		if(request_attrs_attr_requested(req, "job-state-reasons"))
			{
			ipp_add_strings(ipp, IPP_TAG_JOB, IPP_TAG_KEYWORD,
				"job-state-reasons", job_state_reasons_count, job_state_reasons);
			}
			
		ipp_add_end(ipp, IPP_TAG_JOB);

		qentryfile_free(&qentryfile);
		}

	request_attrs_free(req);
	} /* ipp_get_jobs() */

/* end of file */
