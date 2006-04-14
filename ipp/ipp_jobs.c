/*
** mouse:~ppr/src/ipp/ipp_jobs.c
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
** Last modified 14 April 2006.
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

static struct IPP_QUEUE_ENTRY *ipp_load_queue(const char destname[], int jobid, int *set_used)
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

		/* If a particular job ID was requested and this isn't it, skip it. */
		if(jobid != -1 && jobid != queue[queue_used].id)
			continue;

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
		if(gu_sscanf(buffer, "PPRD: %hx %x", &queue[queue_used].priority, &queue[queue_used].sequence_number) != 2)
			{
			fprintf(stderr, X_("Can't parse contents of \"%s\"\n"), fname);
			continue;
			}

		if(destname)		/* all the same */
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

		/* If we are searching for only one jobid, we got here so we must have found it.
		 * Bail out early.
		 */
		if(jobid != -1)
			break;
		}
	
	closedir(dir);

	qsort(queue, queue_used, sizeof(struct IPP_QUEUE_ENTRY), ipp_queue_entry_compare);

	*set_used = queue_used;
	return queue;
	}

/** Handler for IPP_GET_JOBS */
void ipp_get_jobs(struct IPP *ipp)
	{
	const char function[] = "ipp_get_jobs";
	struct REQUEST_ATTRS *req;
	const char *destname = NULL;
	int jobid = -1;
	struct IPP_QUEUE_ENTRY *queue;
	int queue_num_entries;
	int iii;
	char fname[MAX_PPR_PATH];
	FILE *qfile;
	struct QEntryFile qentryfile;

	req = request_attrs_new(ipp, REQUEST_ATTRS_SUPPORTS_PRINTER);
	destname = request_attrs_destname(req);
	jobid = request_attrs_jobid(req);

	queue = ipp_load_queue(destname, jobid, &queue_num_entries);

	for(iii=0; iii < queue_num_entries; iii++)
		{
		/* Read and parse the queue file.  We already know the file exists
		 * and we can open it. */
		ppr_fnamef(fname, "%s/%s-%d.%d", QUEUEDIR, queue[iii].destname, queue[iii].id, queue[iii].subid);
		if(!(qfile = fopen(fname, "r")))
			gu_Throw(X_("%s(): can't open \"%s\", errno=%d (%s)"), function, fname, errno, gu_strerror(errno) );
		qentryfile_clear(&qentryfile);
		{
		int ret = qentryfile_load(&qentryfile, qfile);
		fclose(qfile);
		if(ret == -1)
			{
			fprintf(stderr, X_("%s(): invalid queue file: %s"), function, fname);
			continue;
			}
		}

		if(request_attrs_attr_requested(req, "job-id"))
			{
			ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_INTEGER,
				"job-id", queue[iii].id);
			}
		if(request_attrs_attr_requested(req, "job-printer-uri"))
			{
			ipp_add_template(ipp, IPP_TAG_JOB, IPP_TAG_URI,
				"job-printer-uri", "/printers/%s", queue[iii].destname);
			}
		if(request_attrs_attr_requested(req, "job-uri"))
			{
			ipp_add_template(ipp, IPP_TAG_JOB, IPP_TAG_URI,
				"job-uri", "/jobs/%d", queue[iii].id);
			}

		/* Derived from "ppop lpq" */
		if(request_attrs_attr_requested(req, "job-name"))
			{
			const char *ptr;
			if(!(ptr = qentryfile.lpqFileName))
				if(!(ptr = qentryfile.Title))
					ptr = "";
			ipp_add_string(ipp, IPP_TAG_JOB, IPP_TAG_NAME, "job-name", gu_strdup(ptr));
			}

		/* Derived from "ppop lpq" */
		if(request_attrs_attr_requested(req, "job-originating-user-name"))
			{
			ipp_add_string(ipp, IPP_TAG_JOB, IPP_TAG_NAME,
				"job-originating-user-name", gu_strdup(qentryfile.user));
			}

		/* Derived from "ppop lpq" */
		if(request_attrs_attr_requested(req, "job-k-octets"))
			{
			long int bytes = qentryfile.PassThruPDL ? qentryfile.attr.input_bytes : qentryfile.attr.postscript_bytes;
			ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_INTEGER,
				"job-k-octets", (bytes + 512) / 1024);
			}

		if(request_attrs_attr_requested(req, "job-state"))
			{
			ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_ENUM,
				"job-state", IPP_JOB_PENDING);	
			}
			
		ipp_add_end(ipp, IPP_TAG_JOB);

		qentryfile_free(&qentryfile);
		}

	request_attrs_free(req);
	} /* ipp_get_jobs() */

/* end of file */
