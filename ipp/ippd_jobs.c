/*
** mouse:~ppr/src/ipp/ippd_jobs.c
** Copyright 1995--2010, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 5 August 2010.
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
#include "ippd.h"

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

/* Load all queue entries queued for destination destname[], or all queue
 * entries, if destname is NULL.
 * Set set_used to the number of entries found and return a pointer
 * to the array.  If completed is true, return completed and arrested jobs
 * only and reverse the sorting order (as specified by RFC 2911 3.2.6.1).
 * If it is false, return only jobs which are still potentially printable.
 */
static struct IPP_QUEUE_ENTRY *ipp_load_queue(const char destname[], int *set_used, gu_boolean completed, const char *filter_user)
	{
	const char function[] = "load_queue";
	struct IPP_QUEUE_ENTRY *queue = NULL;
	int queue_used = 0;
	int queue_space = 0;
	DIR *dir;
	struct dirent *direntp;
	char *p;
	char fname[MAX_PPR_PATH];
	FILE *qf;
	char *line = NULL;
	int line_space = 80;
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
		*p = '\0';		/* truncate string at end of destname */

		/* If we are filtering by destname and destname does not match, skip this job. */
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
		if(!(qf = fopen(fname, "r")))
			{
			fprintf(stderr, X_("Can't open \"%s\", errno=%d (%s)\n"), fname, errno, strerror(errno));
			continue;
			}

		/* This isn't a real loop.  It just gives us a way to skip the job
		 * without skipping the code which closes the job file. */
		do	{
			if(!(line = gu_getline(line, &line_space, qf))
					|| gu_sscanf(line, "PPRD: %hx %x %hx", &queue[queue_used].priority, &queue[queue_used].sequence_number, &status) != 3)
				{
				fprintf(stderr, X_("Can't parse contents of \"%s\"\n"), fname);
				break;
				}

			if(status == STATUS_FINISHED || status == STATUS_ARRESTED)
				{
				if(!completed)
					break;
				}
			else
				{
				if(completed)
					break;
				}
	
			/* If we are filtering by username, */
			if(filter_user)
				{
				if(!(line = gu_getline(line, &line_space, qf))
						|| !(p = lmatchp(line, "User:"))
						)
					{
					fprintf(stderr, X_("Can't parse contents of \"%s\"\n"), fname);
					break;
					}
				if(strcmp(filter_user, p) != 0)
					break;
				}

			if(destname)		/* all the same queue */
				queue[queue_used].destname = destname;
			else				/* possibly different, don't save more than one copy */
				{				/* remember that direntp->d_name has already been trimmed */
				if(!(queue[queue_used].destname = gu_pch_get(destnames, direntp->d_name)))
					{
					char *temp = gu_strdup(direntp->d_name);
					gu_pch_set(destnames, temp, temp);
					queue[queue_used].destname = temp;
					}
				}
		
			queue_used++;
			} while(FALSE);

		fclose(qf);
		}
	
	closedir(dir);

	if(completed)
		qsort(queue, queue_used, sizeof(struct IPP_QUEUE_ENTRY), ipp_queue_entry_compare_reversed);
	else
		qsort(queue, queue_used, sizeof(struct IPP_QUEUE_ENTRY), ipp_queue_entry_compare);

	*set_used = queue_used;
	return queue;
	} /* ipp_load_queue() */

/* Add the indicated job the list of jobs in the IPP response. */
static void ipp_add_job(struct IPP *ipp, struct REQUEST_ATTRS *req, const char destname[], int id, int subid)
	{
	const char function[] = "ipp_add_job";
	void *pool;
	struct QEntryFile qentryfile;
	int job_state;
	const char *job_state_reasons[10];
	int job_state_reasons_count = 0;

	GU_OBJECT_POOL_PUSH((pool = gu_pool_new()));

	/* Read and parse the queue file.  We already know the file exists
	 * and we can open it. */
	{
	char fname[MAX_PPR_PATH];
	FILE *qfile;
	int ret;
	ppr_fnamef(fname, "%s/%s-%d.%d", QUEUEDIR, destname, id, subid);
	DODEBUG(("%s(): reading \"%s\"", function, fname));
	if(!(qfile = fopen(fname, "r")))
		gu_Throw(X_("%s(): can't open \"%s\", errno=%d (%s)"), function, fname, errno, gu_strerror(errno) );
	qentryfile_clear(&qentryfile);
	qentryfile.jobname.destname = destname;
	qentryfile.jobname.id = id;
	qentryfile.jobname.subid = subid;
	ret = qentryfile_load(&qentryfile, qfile);
	fclose(qfile);
	if(ret == -1)
		gu_Throw(X_("%s(): invalid queue file: %s"), function, fname);
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
		case STATUS_RECEIVING:
			job_state = IPP_JOB_HELD;
			job_state_reasons[job_state_reasons_count++] = "job-data-insufficient";
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
			"job-uri", "/jobs/%d", qentryfile.jobname.id);
		}

	/* job-printer-up-time */
	if(request_attrs_attr_requested(req, "job-printer-up-time"))
		{
		ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_INTEGER,
			"job-printer-up-time", time(NULL));
		}

	/* printer-uri (probably spurious) */

	if(request_attrs_attr_requested(req, "job-originating-user-name"))
		{
		ipp_add_string(ipp, IPP_TAG_JOB, IPP_TAG_NAME,
			"job-originating-user-name", gu_pool_return(qentryfile.user));
		}

	if(request_attrs_attr_requested(req, "job-name"))
		{
		const char *ptr;
		if(!(ptr = qentryfile.Title))
			if(!(ptr = qentryfile.lpqFileName))
				ptr = NULL;
		ipp_add_string(ipp, IPP_TAG_JOB, IPP_TAG_NAME,
			"job-name", ptr ? gu_pool_return(ptr) : "");
		}

	/* document-format */

	/* job-sheets (RFC 2911 4.2.3, banner pages) */
	
	if(request_attrs_attr_requested(req, "job-hold-until"))
		{
		ipp_add_string(ipp, IPP_TAG_JOB, IPP_TAG_KEYWORD,
			"job-hold-until",
			qentryfile.spool_state.status == STATUS_HELD ? "indefinite" : "no-hold"
			);
		}

	/* job-priority */
	if(request_attrs_attr_requested(req, "job-priority"))
		{
		ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_INTEGER,
			"job-priority", qentryfile.spool_state.priority);
		}

	/* job-originating-host-name */
	
	if(request_attrs_attr_requested(req, "job-id"))
		{
		ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_INTEGER,
			"job-id", qentryfile.jobname.id);
		}

	if(request_attrs_attr_requested(req, "job-state"))
		{
		ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_ENUM,
			"job-state", job_state);
		}

	/* Not yet available */
	#if 0
	if(request_attrs_attr_requested(req, "job-media-sheets-completed"))
		{
		ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_INTEGER,
			"job-media-sheets-completed",
			???
			);
		}
	#endif

	if(request_attrs_attr_requested(req, "job-printer-uri"))
		{
		ipp_add_template(ipp, IPP_TAG_JOB, IPP_TAG_URI,
			"job-printer-uri", destname_to_uri_template(qentryfile.jobname.destname), qentryfile.jobname.destname);
		}						/* no need for gu_pool_return() */

	/* Derived from "ppop lpq" */
	/* May not be correct */
	if(request_attrs_attr_requested(req, "job-k-octets"))
		{
		long int bytes = qentryfile.PassThruPDL ? qentryfile.attr.input_bytes : qentryfile.attr.postscript_bytes;
		ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_INTEGER,
			"job-k-octets", (bytes + 512) / 1024);
		}

	/* time-at-creation */
	if(request_attrs_attr_requested(req, "time-at-creation"))
		{
		ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_INTEGER,
			"time-at-creation", qentryfile.time);
		}

	/* time-at-processing */

	/* time-at-completed */
	
	if(request_attrs_attr_requested(req, "job-state-reasons"))
		{
		const char **temp = gu_alloc(job_state_reasons_count, sizeof(char*));
		memcpy(temp, job_state_reasons, job_state_reasons_count * sizeof(char*));
		ipp_add_strings(ipp, IPP_TAG_JOB, IPP_TAG_KEYWORD,
			"job-state-reasons", job_state_reasons_count, temp);
		}

	/*
	 * RFC 2911 4.3.13.3 seems to indicate that this is the actually number
	 * of pieces of paper that will come out of the printer once all copies
	 * have been printed.
	 * CUPS 1.2 does not seem to support this attribute.
	 */		
	if(request_attrs_attr_requested(req, "job-media-sheets"))
		{
		if(qentryfile.attr.pages >= 0)
			{
			int total = qentryfile.attr.pages;
			total = (total + qentryfile.attr.pagefactor - 1) / qentryfile.attr.pagefactor;
			if(qentryfile.opts.copies > 1)
				total *= qentryfile.opts.copies;
			ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_INTEGER,
				"job-media-sheets", total
				);
			}
		else
			{
			ipp_add_out_of_band(ipp, IPP_TAG_JOB, IPP_TAG_UNKNOWN,
				"job-media-sheets"
				);
			}
		}

	/* RFC 2911 4.3.17.2 seems to indicate that this is the number of media
	 * sides covered before accounting for multiple copies.
	 * CUPS 1.2 does not seem to support this attribute.
	 */
	if(request_attrs_attr_requested(req, "job-impressions"))
		{
		if(qentryfile.attr.pages >= 0)
			{
			ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_INTEGER,
				"job-impressions",
				((qentryfile.attr.pages + qentryfile.N_Up.N - 1) / qentryfile.N_Up.N)
				);
			}
		else
			{
			ipp_add_out_of_band(ipp, IPP_TAG_JOB, IPP_TAG_UNKNOWN, "job-impressions");
			}
		}

	/*qentryfile_free(&qentryfile);*/
	GU_OBJECT_POOL_POP(pool);
	gu_pool_free(pool);
	}

/** Handler for IPP_GET_JOBS */
void ipp_get_jobs(struct IPP *ipp)
	{
	FUNCTION4DEBUG("ipp_get_jobs")
	const char *destname;
	const char *user_at_host;
	int limit;
	const char *which_jobs;			/* "completed" or "no-completed" */
	gu_boolean my_jobs;
	struct REQUEST_ATTRS *req;
	struct IPP_QUEUE_ENTRY *queue;	/* array of matching jobs */
	int queue_num_entries;			/* number of entries in above */
	int iii;

	DODEBUG(("%s()", function));
	
	destname = extract_destname(ipp, NULL, FALSE);		/* any type, not required */

	user_at_host = extract_identity(ipp, FALSE);
	limit = ipp_claim_positive_integer(ipp, IPP_TAG_OPERATION, "limit");
	which_jobs = ipp_claim_keyword(ipp, IPP_TAG_OPERATION, "which-jobs", "not-completed", "completed", NULL);
	my_jobs = ipp_claim_boolean(ipp, IPP_TAG_OPERATION, "my-jobs", FALSE);
	req = request_attrs_new(ipp);

	queue = ipp_load_queue(
			destname,
		   	&queue_num_entries,
		   	which_jobs && strcmp(which_jobs, "completed") == 0,
			my_jobs ? user_at_host : NULL
			);
	DODEBUG(("%s(): %d entries", function, queue_num_entries));

	/* If the number of entries found exceeds the limit, clip. */
	if(limit != 0 && queue_num_entries > limit)
		queue_num_entries = limit;

	for(iii=0; iii < queue_num_entries; iii++)
		{
		ipp_add_job(ipp, req, queue[iii].destname, queue[iii].id, queue[iii].subid);
		ipp_add_end(ipp, IPP_TAG_JOB);
		}

	request_attrs_free(req);
	} /* ipp_get_jobs() */

/* For functions which work on a specific job, extract a job ID number
 * and (if possible) a queue name. */
static gu_boolean extract_jobid(struct IPP *ipp, const char **destname, int *job_id)
	{
	FUNCTION4DEBUG("extract_jobid")
	struct URI *printer_uri = NULL;
	struct URI *job_uri;

	if((printer_uri = ipp_claim_uri(ipp, IPP_TAG_OPERATION, "printer-uri")))
		{
		if(!(*destname = printer_uri_validate(printer_uri, NULL)))
			{
			DODEBUG(("%s(): not a known printer", function));
			ipp->response_code = IPP_NOT_FOUND;
			ipp->request_attrs = NULL;
			return FALSE;
			}
		if((*job_id = ipp_claim_positive_integer(ipp, IPP_TAG_OPERATION, "job-id")) <= 0)
			{
			DODEBUG(("%s(): job-id missing", function));
			ipp->response_code = IPP_BAD_REQUEST;
			ipp->request_attrs = NULL;
			return FALSE;
			}
		}
	else if((job_uri = ipp_claim_uri(ipp, IPP_TAG_OPERATION, "job-uri")))
		{
		if(job_uri->dirname && strcmp(job_uri->dirname, "/jobs") == 0
				&& job_uri->basename && (*job_id = atoi(job_uri->basename)) > 0)
			{
			DODEBUG(("%s(): job-id: %d", function, *job_id));
			}
		else
			{
			DODEBUG(("%s(): not a valid job URI", function));
			ipp->response_code = IPP_NOT_FOUND;
			ipp->request_attrs = NULL;
			return FALSE;
			}
		}
	else
		{
		DODEBUG(("%s(): nothing to identify the job", function));
		ipp->response_code = IPP_BAD_REQUEST;
		ipp->request_attrs = NULL;
		return FALSE;
		}

	return TRUE;
	} /* extract_jobid() */

/*
 * Handle IPP_GET_JOB_ATTRIBUTES
 */
void ipp_get_job_attributes(struct IPP *ipp)
	{
	const char function[] = "ipp_get_job_attributes";
	const char *destname = NULL;
	int job_id = 0;
	struct REQUEST_ATTRS *req;
	DIR *dir;
	struct dirent *direntp;
	char *p;
	int id;
	short int subid;

	if(!extract_jobid(ipp, &destname, &job_id))
		return;

	req = request_attrs_new(ipp);

	if(!(dir = opendir(QUEUEDIR)))
		gu_Throw(_("%s(): %s(\"%s\") failed, errno=%d (%s)"), function, "opendir", QUEUEDIR, errno, strerror(errno));

	while(ipp->response_code == IPP_OK && (direntp = readdir(dir)))
		{
		/* Skip ".", "..", and hidden files. */
		if(direntp->d_name[0] == '.')
			continue;

		DODEBUG(("%s(): %s", function, direntp->d_name));
		
		/* Locate hyphen between destname and ID */
		if(!(p = strrchr(direntp->d_name, '-')))
			continue;

		/* If destname does not match, skip this file. */
		if(destname && ((p - direntp->d_name) != strlen(destname) || strncmp(direntp->d_name, destname, strlen(destname)) != 0))
			continue;

		/* Parse the part of the filename after the hyphen in order to extract the 
		 * ID and subid. */
		if(gu_sscanf(p+1, "%d.%hd", &id, &subid) != 2)
			{
			fprintf(stderr, X_("Can't parse id.subid: %s\n"), p+1);
			continue;
			}
		if(job_id != id)
			continue;

		*p = '\0';
		ipp_add_job(ipp, req, p, id, subid);
		break;
		}
	
	closedir(dir);
	request_attrs_free(req);
	} /* ipp_get_job_attributes() */

void ipp_X_job(struct IPP *ipp)
	{
	const char function[] = "ipp_X_job";
	const char *destname = NULL;
	int job_id = 0;
	const char *user_at_host;
	gu_boolean is_administrator;
	DIR *dir;
	struct dirent *direntp;
	int count = 0;
	char *p;
	int id;
	short int subid;
	char *line = NULL;
	int line_space = 80;

	DODEBUG(("%s()", function));

	if(!extract_jobid(ipp, &destname, &job_id))
		return;

	user_at_host = extract_identity(ipp, TRUE);
	is_administrator = user_acl_allows(user_at_host, "ppop");

	if(!(dir = opendir(QUEUEDIR)))
		gu_Throw(_("%s(): %s(\"%s\") failed, errno=%d (%s)"), function, "opendir", QUEUEDIR, errno, strerror(errno));

	while(ipp->response_code == IPP_OK && (direntp = readdir(dir)))
		{
		/* Skip ".", "..", and hidden files. */
		if(direntp->d_name[0] == '.')
			continue;

		DODEBUG(("%s(): %s", function, direntp->d_name));
		
		/* Locate hyphen between destname and ID */
		if(!(p = strrchr(direntp->d_name, '-')))
			continue;

		/* If destname does not match, skip this file. */
		if(destname && ((p - direntp->d_name) != strlen(destname) || strncmp(direntp->d_name, destname, strlen(destname)) != 0))
			continue;

		/* Parse the part of the filename after the hyphen in order to extract the 
		 * ID and subid. */
		if(gu_sscanf(p+1, "%d.%hd", &id, &subid) != 2)
			{
			fprintf(stderr, X_("Can't parse id.subid: %s\n"), p+1);
			continue;
			}
		if(job_id != id)
			continue;

		/* We have a match. */
		count++;
		
		/* Non administrators can manipulate only their own jobs. */
		if(!is_administrator)
			{
			char fname[MAX_PPR_PATH];
			FILE *f;
			char *job_user = NULL;
			ppr_fnamef(fname, "%s/%s", QUEUEDIR, direntp->d_name);
			if(!(f = fopen(fname, "r")))
				{
				fprintf(stderr, X_("Can't open \"%s\", errno=%d (%s)\n"), fname, errno, strerror(errno));
				continue;
				}
			while((line = gu_getline(line, &line_space, f)))
				{
				if((job_user = lmatchp(line, "User:")))
					break;	
				}
			fclose(f);
			if(!job_user || strcmp(job_user, user_at_host) != 0)
				continue;
			}
		
		DODEBUG(("%s(): asking pprd to delete job %d", function, id));
		ipp->response_code = pprd_status_code(
			pprd_call("IPP %d %d\n", ipp->operation_id, id)
			);
		DODEBUG(("%s(): pprd says: %s", function, ipp_status_code_to_str(ipp->response_code)));

		break;		/* If we get as far as here, we are done. */
		}
	
	closedir(dir);

	if(ipp->response_code == IPP_OK && count == 0)
		ipp->response_code = IPP_NOT_FOUND;
	} /* ipp_X_job() */

void cups_move_job(struct IPP *ipp)
	{
	const char function[] = "ipp_get_job_attributes";
	const char *destname = NULL;
	struct URI *job_printer_uri;
	const char *new_destname;
	enum QUEUEINFO_TYPE new_qtype;
	int job_id = 0;
	DIR *dir;
	struct dirent *direntp;
	char *p;
	int id;
	short int subid;

	if(!extract_jobid(ipp, &destname, &job_id))
		return;

	if(!(job_printer_uri = ipp_claim_uri(ipp, IPP_TAG_JOB, "job-printer-uri")))
		{
		DODEBUG(("%s(): no job-printer-uri", function));
		ipp->response_code = IPP_BAD_REQUEST;
		return;
		}

	if(!(new_destname = printer_uri_validate(job_printer_uri, &new_qtype)))
		{
		DODEBUG(("%s(): not a known printer", function));
		ipp->response_code = IPP_NOT_FOUND;
		return;
		}

	if(!(dir = opendir(QUEUEDIR)))
		gu_Throw(_("%s(): %s(\"%s\") failed, errno=%d (%s)"), function, "opendir", QUEUEDIR, errno, strerror(errno));

	while(ipp->response_code == IPP_OK && (direntp = readdir(dir)))
		{
		/* Skip ".", "..", and hidden files. */
		if(direntp->d_name[0] == '.')
			continue;

		DODEBUG(("%s(): %s", function, direntp->d_name));
		
		/* Locate hyphen between destname and ID */
		if(!(p = strrchr(direntp->d_name, '-')))
			continue;

		/* If destname does not match, skip this file. */
		if(destname && ((p - direntp->d_name) != strlen(destname) || strncmp(direntp->d_name, destname, strlen(destname)) != 0))
			continue;

		/* Parse the part of the filename after the hyphen in order to extract the 
		 * ID and subid. */
		if(gu_sscanf(p+1, "%d.%hd", &id, &subid) != 2)
			{
			fprintf(stderr, X_("Can't parse id.subid: %s\n"), p+1);
			continue;
			}
		if(job_id != id)
			continue;

		*p = '\0';
		DODEBUG(("%s(): asking pprd to move job %d", function, id));
		ipp->response_code = pprd_status_code(
			pprd_call("IPP %d %d %s %s\n",
				ipp->operation_id,
				id,
				new_qtype == QUEUEINFO_GROUP ? "group" : "printer",
				new_destname
				)
			);
		DODEBUG(("%s(): pprd says: %s", function, ipp_status_code_to_str(ipp->response_code)));
		break;
		}
	
	closedir(dir);
	}

/* end of file */
