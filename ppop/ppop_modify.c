/*
** mouse:~ppr/src/ppop/ppop_modify.c
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
** Last modified 23 September 2005.
*/

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "ppop.h"
#include "util_exits.h"
#include "version.h"

/*============================================================================
** Read and write the queue file
============================================================================*/

/*
** This structure stores an addon line.
*/
struct ADDON
	{
	char *name;
	char *value;
	};

/*
** This structure will eventually be able to store all of the information in
** a queue file.
*/
#define MAX_ADDON_LINES 50
struct JOB
	{
	const struct Jobname *jobname;
	char qfname[MAX_PPR_PATH];
	struct QEntryFile qentry;
	struct ADDON addon[MAX_ADDON_LINES];
	int addon_count;
	};

/*
** This function reads the Addon section from a queue file and stores the
** lines in the a struct JOB.
*/
static void read_addon(FILE *qf, struct JOB *job)
	{
	const char function[] = "read_addon";
	char *line = NULL;
	int line_room = 256;
	char *p;

	job->addon_count = 0;
	while((line = gu_getline(line, &line_room, qf)))
		{
		if(strcmp(line, "EndAddon") == 0)
			break;

		if(!(p = strchr(line, ':')))
			{
			fprintf(errors, _("Invalid Addon line: %s\n"), line);
			continue;
			}

		if(job->addon_count >= MAX_ADDON_LINES)
			{
			fprintf(errors, X_("%s(): MAX_ADDON_LINES exceeded\n"), function);
			continue;
			}

		*p = '\0';
		p++;
		while(isspace(*p))
			p++;

		job->addon[job->addon_count].name = gu_strdup(line);
		job->addon[job->addon_count].value = gu_strdup(p);
		job->addon_count++;
		}

	if(line)
		gu_free(line);
	}

/*
** Free the heap storage occupied by the addon lines.
*/
static void destroy_addon(struct JOB *job)
	{
	int x;
	for(x=0; x < job->addon_count; x++)
		{
		gu_free(job->addon[x].name);
		gu_free(job->addon[x].value);
		}
	}

/*
** This function writes the changed queue file.
*/
static void write_changes(FILE *qf, const struct JOB *job)
	{
	char nfname[MAX_PPR_PATH];	/* temporary name of new queue file */
	FILE *nqf;					/* object of new queue file */
	int i;

	/* Create the new queue file. */
	ppr_fnamef(nfname, "%s/.ppjob-%ld", QUEUEDIR, (long)getpid());
	if(!(nqf = fopen(nfname, "w")))
		{
		error("Can't create \"%s\", errno=%d (%s)", nfname, errno, gu_strerror(errno));
		return;
		}

	qentryfile_save(&(job->qentry), nqf);

	for(i = 0; i < job->addon_count; i++)
		fprintf(nqf, "%s: %s\n", job->addon[i].name, job->addon[i].value);
	fprintf(nqf, "EndAddon\n");

	/* Copy the rest of the old file to the new one. */
	while((i = fgetc(qf)) != EOF)
		{
		fputc(i, nqf);
		}

	fclose(nqf);

	/* Replace the old queue file with the new one. */
	rename(nfname, job->qfname);
	}

/*============================================================================
** Run user commands
============================================================================*/

/*
** This complex structure describes the available commands.
*/
struct DT
	{
	const char *name;
	int (*function)(const char *name, const char *value, struct JOB *job, size_t offset);
	int offset;
	};

/*
** This macro is used to return a void pointer to certain offset within a
** struct JOB.  We define it as a macro because some complilers don't
** implement this for void.
*/
#if 0
#define JOB_OFFSET(job, offset) ((void*)job + offset)
#else
#define JOB_OFFSET(job, offset) ((char*)job + offset)
#endif

static int modify_string(const char *name, const char *value, struct JOB *job, size_t offset)
	{
	char **p = (char **)JOB_OFFSET(job, offset);
	if(*p) gu_free(*p);
	*p = (char*)NULL;
	if(strlen(value) > 0)
		*p = gu_strdup(value);
	return EXIT_OK;
	}

static int modify_positive_integer(const char *name, const char *value, struct JOB *job, size_t offset)
	{
	int *p = (int*)JOB_OFFSET(job, offset);
	*p = atoi(value);
	return EXIT_OK;
	}

static int modify_boolean(const char *name, const char *value, struct JOB *job, size_t offset)
	{
	gu_boolean *p = (int*)JOB_OFFSET(job, offset);
	if(gu_torf_setBOOL(p,value) == -1)
		{
		fprintf(errors, _("The value \"%s\" is not boolean.\n"), value);
		return EXIT_SYNTAX;
		}
	return EXIT_OK;
	}

static int modify_pagelist(const char *name, const char *value, struct JOB *job, size_t offset)
	{
	if(pagemask_encode(&job->qentry, value) == -1)
		{
		fprintf(errors, _("The value \"%s\" is not a valid page list.\n"), value);
		return EXIT_SYNTAX;
		}
	return EXIT_OK;
	}

static int modify_addon(const char *name, const char *value, struct JOB *job)
	{
	const char function[] = "modify_addon";
	int x;

	for(x=0; x < job->addon_count; x++)
		{
		if(strcmp(job->addon[x].name, name) == 0)
			{
			gu_free(job->addon[x].value);
			job->addon[x].value = gu_strdup(value);
			return EXIT_OK;
			}
		}

	if(job->addon_count >= MAX_ADDON_LINES)
		{
		fprintf(errors, X_("%s(): MAX_ADDON_LINES exceeded\n"), function);
		return EXIT_INTERNAL;
		}

	job->addon[job->addon_count].name = gu_strdup(name);
	job->addon[job->addon_count].value = gu_strdup(value);
	job->addon_count++;

	return EXIT_OK;
	}

#define OFFSET(member) (int)&(((struct JOB *)0)->member)
const struct DT commands[] =
	{
	{ "title", modify_string, OFFSET(qentry.Title) },
	{ "for", modify_string, OFFSET(qentry.For) },
	{ "routing", modify_string, OFFSET(qentry.Routing) },
	{ "copies", modify_positive_integer, OFFSET(qentry.opts.copies) },
	{ "copiescollate", modify_positive_integer, OFFSET(qentry.opts.collate) },
	{ "nupn", modify_positive_integer, OFFSET(qentry.N_Up.N) },
	{ "nupborders", modify_boolean, OFFSET(qentry.N_Up.borders) },
	{ "draft-notice", modify_string, OFFSET(qentry.draft_notice) },
	{ "page-list", modify_pagelist, 0 },
	{ "question", modify_string, OFFSET(qentry.question) },
	{ NULL, NULL }
	};

/*
** This function dispatches commands to change the queue file.
*/
static int dispatch(const char name[], const char value[], struct JOB *job)
	{
	const struct DT *p;

	if(strncmp(name, "addon:", 6) == 0)
		{
		return modify_addon(name+6, value, job);
		}

	for(p = commands; p->name; p++)
		{
		if(strcmp(p->name, name) == 0)
			{
			return (*p->function)(name, value, job, p->offset);
			}
		}

	fprintf(errors, _("The job property \"%s\" does not exist or is not writable.\n"), name);
	return EXIT_SYNTAX;
	} /* end of dispatch() */

/*
** This is the action routine for the "ppop modify" command.
*/
int ppop_modify(char *argv[])
	{
	FILE *qf;
	struct JOB job;
	int ret = EXIT_OK;
	gu_boolean question_touched = FALSE;

	if(!argv[0] || !argv[1])
		{
		fputs(_("Usage: ppop modify <jobname> <name>=<value> ...\n\n"
				"This command sets the properties of an existing job.\n"), errors);
		return EXIT_SYNTAX;
		}

	/* Break the job name up into its components. */
	if(!(job.jobname = parse_jobname(argv[0])))
		return EXIT_SYNTAX;

	/* Do we have permission to modify this job? */
	if(!job_permission_check(job.jobname))
		return EXIT_DENIED;

	/* Now use those components to build the path to the queue file, which
	   includes a fully qualified job name. */
	ppr_fnamef(job.qfname, "%s/%s-%d.%d",
		QUEUEDIR,
		job.jobname->destname,
		job.jobname->id,
		job.jobname->subid >= 0 ? job.jobname->subid : 0
		);

	/* Open that queue file. */
	if(!(qf = fopen(job.qfname, "r")))
		{
		fprintf(errors, _("Can't open \"%s\", errno=%d (%s).\n"), job.qfname, errno, gu_strerror(errno));
		return (errno == ENOENT) ? EXIT_NOTFOUND : EXIT_INTERNAL;
		}

	/* Read the first part of the queue file into a special structure. */
	qentryfile_clear(&job.qentry);
	if(qentryfile_load(&job.qentry, qf) == -1)
		return EXIT_INTERNAL;

	/* Read in the extensions section.  This section will contain things that
	   the PPR core doesn't care about such as job tickets. */
	read_addon(qf, &job);

	/* Loop thru the name=value pairs, calling dispatch() on each one. */
	{
	int x;
	char *ptr;
	for(x = 1; argv[x]; x++)
		{
		/* Convert illegal characters to spaces.  Perhaps we should
		   consider it a fatal error to include them? */
		for(ptr = argv[x]; *ptr; ptr++)
			{
			if(*ptr < 32 || *ptr == 127)		/* ASCII controls !!! */
				*ptr = ' ';
			}

		/* Find the equals sign that separates the name and the value. */
		if(!(ptr = strchr(argv[x], '=')))
			{
			fprintf(errors, _("Parameter \"%s\" is not a name=value pair.\n"), argv[x]);
			ret = EXIT_SYNTAX;
			break;
			}

		/* Replace it with a NULL and move ptr past it. */
		*(ptr++) = '\0';

		/* Dispatch the name=value pair.  Abort if this one isn't ok. */
		if((ret = dispatch(argv[x], ptr, &job)) != EXIT_OK)
			break;

		/* If the question has changed, we will need to inform pprd,
		   so make a note of it. */
		if(strcmp(argv[x], "question") == 0)
			question_touched = TRUE;

		}
	}

	/* If all of the changes were valid, */
	if(ret == EXIT_OK)
		{
		/* Write the new structure and the remainder of the queue file to a new queue file. */
		write_changes(qf, &job);

		/* If the question has changed, */
		if(question_touched)
			{
			/* Open a connection to pprd. */
			FILE *FIFO = get_ready();

			/* Let it know what has happened. */
			fprintf(FIFO, "q %s %d %d %d\n",
				job.jobname->destname, job.jobname->id, job.jobname->subid,
				&job.qentry.question ? 1 : 0);
			fflush(FIFO);

			/* Wait for it to respond. */
			wait_for_pprd(TRUE);

			/* Print the response and accept its code as our return code. */
			ret = print_reply();
			}
		}

	/* Close the origional queue file (which may already be unlinked). */
	fclose(qf);

	/* Free any allocated memory. */
	qentryfile_free(&job.qentry);
	destroy_addon(&job);

	return ret;
	} /* end of ppop_modify() */

/* end of file */

