/*
** mouse:~ppr/src/ppr/ppr_outfile.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 24 May 2001.
*/

/*
** This module contains routines used for creating the queue files.
*/

#include "before_system.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "ppr.h"
#include "ppr_exits.h"

/*
** Return the next id number.  The string "file" is the full path and
** name of the file to use.  The file is locked, the last id is read,
** and incremented making the new id, and the new id is written back
** to the file, the file is closed, and the new id is returned.
*/
void get_next_id(struct QFileEntry *q)
    {
    const char function[] = "get_next_id";
    const char filename[] = NEXTIDFILE;
    int fd;
    FILE *f;
    int tid;                        /* for holding id */
    char tempqfname[MAX_PPR_PATH];
    struct stat statbuf;
    int paranoid = 0;

    do	{
	/* Let's not chew CPU time under some wierd circumstance. */
	if(paranoid++ > 10000)
	    fatal(PPREXIT_OTHERERR, "%s(): all job id numbers used", function);

	/* If we can open an existing id file: */
        if((fd = open(filename, O_RDWR, S_IRUSR | S_IWUSR)) >= 0)
            {
            /* lock the file. */
            if(gu_lock_exclusive(fd, TRUE))
                fatal(PPREXIT_OTHERERR, "can't lock \"%s\", errno=%d (%s)", filename, errno, gu_strerror(errno));

	    /* Create a streams object for read and update. */
            f = fdopen(fd, "r+");

            gu_fscanf(f, "%d", &tid);		/* get last value used */
            tid++;				/* add one to it */
            if(tid < 1 || tid > 9999)		/* if resulting id unreasonable or too large */
                tid = 1;			/* force it to one */
	    rewind(f);				/* move to start of file for write */
            }

        /* Open failed, assume file does not exist and create a new one. */
        else
            {
            tid = 1;			/* start id's at 1 */
            if(!(f = fopen(filename, "w")))
                fatal(PPREXIT_OTHERERR, "can't create new id file \"%s\"", filename);
            }

	fprintf(f, "%d\n", tid);	/* Write the new id */
	fclose(f);

	/* Perform a crude test to alieviate the problems of ID wraparound.
	   If a job exists with which we would collide, then skip this id. */
	ppr_fnamef(tempqfname, "%s/%s:%s-%d.%d(%s)",
		QUEUEDIR,
		qentry.destnode,
		qentry.destname,
		tid, 0,
		qentry.homenode);
	} while(stat(tempqfname, &statbuf) == 0);

    q->id = tid;
    } /* end of get_next_id() */

/*
** Open the output files.
** These are three in number:  one for the text, one for the header
** and trailer comments, and one for the page level comments.
*/
void open_output(void)
    {
    char temp[MAX_PPR_PATH];

    /* file for header and trailer comments */
    ppr_fnamef(temp, "%s/%s:%s-%d.%d(%s)-comments",
    		DATADIR,
    		qentry.destnode,
    		qentry.destname,
    		qentry.id, qentry.subid,
    		qentry.homenode);
    if(!(comments = fopen(temp, "wb")))
	fatal(PPREXIT_OTHERERR, _("can't open \"%s\", errno=%d (%s)"), temp, errno, gu_strerror(errno));

    /* file for page comments */
    ppr_fnamef(temp, "%s/%s:%s-%d.%d(%s)-pages",
		DATADIR,
		qentry.destnode,
		qentry.destname,
		qentry.id, qentry.subid,
		qentry.homenode);
    if(!(page_comments = fopen(temp, "wb")))
	fatal(PPREXIT_OTHERERR, _("can't open \"%s\", errno=%d (%s)"), temp, errno, gu_strerror(errno));

    /* file for the PostScript text */
    ppr_fnamef(temp,"%s/%s:%s-%d.%d(%s)-text",
    		DATADIR,
    		qentry.destnode,
    		qentry.destname,
    		qentry.id, qentry.subid,
    		qentry.homenode);
    if(!(text = fopen(temp, "wb")))
	fatal(PPREXIT_OTHERERR, _("can't open \"%s\", errno=%d (%s)"), temp, errno, gu_strerror(errno));
    } /* end of open_output() */

/* end of file */

