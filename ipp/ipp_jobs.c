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

struct IPP_QUEUE_ENTRY {
	INT16_T id;
	INT16_T subid;
	INT16_T priority;
	time_t priority_time;
	struct IPP_QUEUE_ENTRY *prev;
	struct IPP_QUEUE_ENTRY *next;
	};

struct IPP_QUEUE {
	void *pool;
	struct IPP_QUEUE_ENTRY *first;
	struct IPP_QUEUE_ENTRY *last;
	struct IPP_QUEUE_ENTRY *current;
	struct IPP_QUEUE_ENTRY *new;
	int supply_remaining;
	};

static struct IPP_QUEUE ipp_queue_new(const char destname[], int limit)
	{
	const char function[] = "ipp_queue_new";
	void *pool = gu_pool_new();
	GU_OBJECT_POOL_PUSH(pool);
	struct IPP_QUEUE *this = gu_alloc(1, sizeof(struct IPP_QUEUE));
	this->pool = pool;

	{
	DIR *dir;
	struct dirent *direntp;
	char *p;
	char fname[MAX_PPR_PATH];
	int fd;
	char buffer[64];
	int destname_len = strlen(destname);

	struct IPP_QUEUE_ENTRY *first = NULL;
	struct IPP_QUEUE_ENTRY *last = NULL;
	struct IPP_QUEUE_ENTRY *current = NULL;
	struct IPP_QUEUE_ENTRY *old_current = NULL;
	struct IPP_QUEUE_ENTRY *new = NULL;
	int supply_remaining = 0;
	
	if(!(dir = opendir(QUEUEDIR)))
		gu_Throw(_("%s(): %s(\"%s\") failed, errno=%d (%s)"), function, "opendir", QUEUEDIR, errno, strerror(errno));

	while((direntp = readdir(dir)))
		{
		if(dirent->d_name[0] == '.')
			continue;
		/* Locate hyphen between destname and ID */
		if(!(p = strrchr(direntp->d_name, '-')))
			continue;
		/* If destname does not match, skip this file. */
		if((p - direntp->d_name) != destname_len || strncmp(direntp->d_name, destname, destname_len) != 0)
			continue;
		ppr_fnamef(fname, "%s/%s", QUEUEDIR, direntp->d_name);
		if((fd = open(fname, O_RDONLY)) == -1)
			{
			fprintf(stderr, _("Can't open \"%s\", errno=%d (%s)\n"), fname, errno, strerror(errno));
			continue;
			}
		if(read(fd, buffer, sizeof(buffer)) != sizeof(buffer))
			{
			fprintf(stderr, _("Can't read \"%s\"\n"), fname);
			continue;
			}
		if(this->supply_remaining == 0)
			{
			new = gu_alloc(1024, sizeof(struct IPP_QUEUE_ENTRY));
			supply_remaining = 1024;
			}
		if(gu_sscanf(buffer, "PPRD: %hx %x", &new->priority, &new->priority_time) != 2)
			{
			fprintf(stderr, _("Can't parse \"%s\"\n"), fname);
			continue;
			}

		if(!this->current)
			{
			current = first = last = new;
			}
		else
			{
			#if 0
			while(TRUE)
				{
				if(current->priority <= new->priority && current->priority_time > new->priority_time
						&& (!current->prev || prev->))
					{
					new->next = current;
					new->prev = current->prev;
					break;
					}
				if(!current->next)
					{

					break;
					}
				}
			#endif
			}
		
		this->new++;
		this->supply_remaining--;
		}
	
	closedir(dir);	
	}
	
	GU_OBJECT_POOL_POP(this->pool);		
	}

static struct ipp_queue_free(struct IPP_QUEUE *this)
	{
	gu_pool_free(this->pool);
	}

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
			ipp_add_string(ipp, IPP_TAG_JOB, IPP_TAG_NAME, "job-name", gu_strdup(ptr));
			}

		/* Derived from "ppop lpq" */
		if(request_attrs_attr_requested(req, "job-originating-user-name"))
			{
			ipp_add_string(ipp, IPP_TAG_JOB, IPP_TAG_NAME, "job-originating-user-name", gu_strdup(qentryfile.user));
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

/* end of file */
