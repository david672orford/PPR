/*
** mouse:~ppr/src/ppr/ppr_outfile.c
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
** Last modified 14 January 2005.
*/

/*
** This module contains routines used for creating the queue files.
*/

#include "config.h"
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
void get_next_id(struct QFile *q)
	{
	const char function[] = "get_next_id";
	const char filename[] = NEXTIDFILE;
	int fd;
	FILE *f;
	int tid;						/* for holding id */
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

			gu_fscanf(f, "%d", &tid);			/* get last value used */
			tid++;								/* add one to it */
			if(tid < 1 || tid > 9999)			/* if resulting id unreasonable or too large */
				tid = 1;						/* force it to one */
			rewind(f);							/* move to start of file for write */
			}

		/* Open failed, assume file does not exist and create a new one. */
		else
			{
			tid = 1;					/* start id's at 1 */
			if(!(f = fopen(filename, "w")))
				fatal(PPREXIT_OTHERERR, "can't create new id file \"%s\"", filename);
			}

		fprintf(f, "%d\n", tid);		/* Write the new id */
		fclose(f);

		/* Perform a crude test to alieviate the problems of ID wraparound.
		   If a job exists with which we would collide, then skip this id. */
		ppr_fnamef(tempqfname, "%s/%s-%d.%d", QUEUEDIR, qentry.destname, tid, 0);
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
	ppr_fnamef(temp, "%s/%s-%d.%d-comments", DATADIR, qentry.destname, qentry.id, qentry.subid);
	if(!(comments = fopen(temp, "wb")))
		fatal(PPREXIT_OTHERERR, _("can't open \"%s\", errno=%d (%s)"), temp, errno, gu_strerror(errno));

	/* file for page comments */
	ppr_fnamef(temp, "%s/%s-%d.%d-pages", DATADIR, qentry.destname, qentry.id, qentry.subid);
	if(!(page_comments = fopen(temp, "wb")))
		fatal(PPREXIT_OTHERERR, _("can't open \"%s\", errno=%d (%s)"), temp, errno, gu_strerror(errno));

	/* file for the PostScript text */
	ppr_fnamef(temp,"%s/%s-%d.%d-text", DATADIR, qentry.destname, qentry.id, qentry.subid);
	if(!(text = fopen(temp, "wb")))
		fatal(PPREXIT_OTHERERR, _("can't open \"%s\", errno=%d (%s)"), temp, errno, gu_strerror(errno));
	} /* end of open_output() */

/* end of file */

