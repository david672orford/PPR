/*
** mouse:~ppr/src/ppop/ppop_modify.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 19 December 2001.
*/

#include "before_system.h"
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
#include "pprd.h"
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
    struct Jobname jobname;
    char qfname[MAX_PPR_PATH];
    struct QFileEntry qentry;
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
    FILE *nqf;			/* object of new queue file */
    int i;

    ppr_fnamef(nfname, "%s/.ppjob-%ld", QUEUEDIR, (long)getpid());
    if(!(nqf = fopen(nfname, "w")))
    	{
	error("Can't create \"%s\", errno=%d (%s)", nfname, errno, gu_strerror(errno));
	return;
	}

    write_struct_QFileEntry(nqf, &(job->qentry));
    fprintf(nqf, "EndMisc\n");

    for(i = 0; i < job->addon_count; i++)
    	fprintf(nqf, "%s: %s\n", job->addon[i].name, job->addon[i].value);
    fprintf(nqf, "EndAddon\n");

    while((i = fgetc(qf)) != EOF)
    	{
	fputc(i, nqf);
    	}

    fclose(nqf);

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
    int temp;
    if((temp = gu_torf(value)) == ANSWER_UNKNOWN)
    	{
	fprintf(errors, _("The value \"%s\" is not boolean.\n"), value);
	return EXIT_SYNTAX;
    	}
    *p = temp ? TRUE : FALSE;
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

    if(!argv[0] || !argv[1])
    	{
	fputs(_("Usage: ppop modify <jobname> <name>=<value> ...\n\n"
		"This command sets the properties of an existing job.\n"), errors);
	return EXIT_SYNTAX;
    	}

    /* Break the job name up into its components. */
    if(parse_job_name(&job.jobname, argv[0]) == -1)
    	return EXIT_SYNTAX;

    /* Do we have permission to modify this job? */
    if( job_permission_check(&job.jobname) )
	return EXIT_DENIED;

    /* Take a wild guess as to the home node, if it is not specified. */
    if(strcmp(job.jobname.homenode, "*") == 0)
	strcpy(job.jobname.homenode, ppr_get_nodename());

    /* Now use those components to build the path to the queue file, which
       includes a fully qualified job name. */
    ppr_fnamef(job.qfname, "%s/%s:%s-%d.%d(%s)",
    	QUEUEDIR,
    	job.jobname.destnode,
    	job.jobname.destname,
    	job.jobname.id,
    	job.jobname.subid >= 0 ? job.jobname.subid : 0,
    	job.jobname.homenode);

    /* Open that queue file. */
    if(!(qf = fopen(job.qfname, "r")))
    	{
	fprintf(errors, _("Can't open \"%s\", errno=%d (%s).\n"), job.qfname, errno, gu_strerror(errno));
	return (errno == ENOENT) ? EXIT_NOTFOUND : EXIT_INTERNAL;
    	}

    /* Read the first part of the queue file into a special structure. */
    if(read_struct_QFileEntry(qf, &job.qentry) == -1)
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
	    if(*ptr < 32 || *ptr == 127)	/* ASCII controls !!! */
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

	/* Dispatch the name=value pair. */
	if((ret = dispatch(argv[x], ptr, &job)) != EXIT_OK)
	    break;
	}
    }

    if(ret == EXIT_OK)
	write_changes(qf, &job);

    /* Close the origional queue file (which may already be unlinked). */
    fclose(qf);

    /* Free any allocated memory. */
    destroy_struct_QFileEntry(&job.qentry);
    destroy_addon(&job);

    return ret;
    } /* end of ppop_modify() */

/* end of file */

