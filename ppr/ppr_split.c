/*
** mouse:~ppr/src/ppr/ppr_split.c
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
** This file contains those routines which are related to breaking a single
** input file into several jobs.  It should eventually be possible to break
** a job into multiple sections so that the portions requiring certain
** types of media or colour can be sent to a different printer.
**
** Currently the only feature implemented is the breaking of a large job
** into several pieces.  This is however still experimental.  All of the
** pieces inherit all of the resource and feature requirements of the
** originally job; which is incorrect behavior.
**
** The action of this module is controled by the -Y switch.  The argument
** to the -Y switch should be a space seperated list of name=value pairs
** Here are the acceptable arguments to the -Y switch:
**
** -Y 'segments=x'		;break the job into the indicated number of segments
*/

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

#include "global_structs.h"
#include "ppr.h"
#include "ppr_exits.h"

/*
** The function read_defaults() in ppr_dscdoc.c fills this in if it
** encounters a "%%PageMedia:" comment in the document defaults section.
**
** If this is filled in, it is used as the media for all pages which
** do not have "%%PageMedia:" comments.
*/
char default_pagemedia[MAX_MEDIANAME+1] = { '\0' };

/*
** Parameters which describe how to split.
*/
static int splitting = FALSE;	/* should we gather splitting information? */
static int segments = 0;		/* number of segments to split into */

/*
** A table of booleans which describe which `Things' are needed
** by which pages.
*/
static unsigned char *page_segment_assignments;			/* an array with segment number of each page */
static int segments_created;							/* number of segments */

/*
** -Y switch handler.
*/
void Y_switch(const char *optarg)
	{
	struct OPTIONS_STATE o;
	char name[16], value[16];			/* parser extracted words */
	int rval;							/* parser return value */

	splitting = TRUE;					/* collect split information */

	/* Parse -Y switch options */
	options_start(optarg, &o);
	while((rval = options_get_one(&o, name, sizeof(name), value, sizeof(value))) == 1)
		{
		if(gu_strcasecmp(name,"segments") == 0)
			{
			if((segments = atoi(value)) < 2 || segments > MAX_SEGMENTS)
				fatal(PPREXIT_SYNTAX, "Option error: -Y 'segments=%d' must be between 2 and %d", segments, MAX_SEGMENTS);
			}
		else
			{
			fatal(PPREXIT_SYNTAX, "Unrecognized -Y option: %s=%s", name, value);
			}
		}

	/* detect parsing error */
	if(rval == -1)
		fatal(PPREXIT_SYNTAX, "-Y option error: %s", o.error);

	} /* end of Y_switch() */

/*
** This function is called just before ppr_main.c calls
** write_queue_file().  This function may split the job
** and submit it to the spooler in pieces.
**
** For each piece we must make a link to the .0-text file, and
** the .0-comments file and make an abreviated -pages file
** and call write_queue_file() and inform the spooler.
**
** If it returns TRUE, then the code in ppr_main.c will
** understand that this code has submitted the job in
** pieces and will not call write_queue_file() or submit
** the job to the spooler.
*/
int split_job(struct QFileEntry *qentry)
	{
	int x;

	if(! splitting)						/* If we haven't been asked to, */
		return FALSE;					/* don't split it. */

	#ifdef DEBUG_SPLIT
	printf("split_job()\n");
	#endif

	/* If no "%%Page:" comments, */
	if(pagenumber == 0)
		{
		warning(WARNING_SEVERE,"No delimited pages, splitting is impossible.");
		return FALSE;					/* there is nothing to split. */
		}

	/* If pages are inter-dependent, don't risk splitting them. */
	if(qentry->attr.pageorder == PAGEORDER_SPECIAL)
		{
		warning(WARNING_SEVERE,"PageOrder is \"Special\", splitting is impossible.");
		return FALSE;
		}

	/* If not enough pages to split. */
	if(pagenumber <= qentry->attr.pagefactor)
		{
		warning(WARNING_SEVERE,"%d pages present, at least %d needed to split job.",pagenumber,qentry->attr.pagefactor);
		return FALSE;
		}

	segments_created = 0;				/* in case none below make segments */

	/* Allocate an array to hold the segment number assigned to each page. */
	page_segment_assignments = (unsigned char*)gu_alloc(pagenumber,sizeof(unsigned char));

	/* Should we break job into arbitrary segments? */
	if(segments)
		{
		int this_frag;

		for(x = this_frag = 0, segments_created = 1; x < pagenumber; x++)
			{
			/* Round up to next multiple of pagefactor. */
			int pages_per_fragment = ((pagenumber / segments) + qentry->attr.pagefactor - 1)
										/ qentry->attr.pagefactor * qentry->attr.pagefactor;

			/* If this fragment has attained full size, start a new one. */
			if( this_frag++ > pages_per_fragment )
				{
				segments_created++;
				this_frag=0;
				}

			/* Assign this page to the current fragment. */
			page_segment_assignments[x] = segments_created;
			}
		} /* end of if(segments) */

	/*
	** !!!!!
	** additional job breaking algorithms may be inserted here
	** !!!!!
	*/

	/*
	** If multiple fragments were described by the one or more
	** of the code segments above, write the files for each fragment.
	*/
	if(segments_created > 1)
		{
		char fname[MAX_PPR_PATH];		/* temporary space to build file names */
		char line[512];			/* big enought for DSC comment lines */
		int page_counts[MAX_SEGMENTS];

		/*
		** Store the number of segments created in the
		** queue entry structure.
		*/
		qentry->attr.parts = segments_created;

		/*
		** Append a statement that we will split the job into fragments
		** to the job log file.
		*/
		{
		FILE *log;

		ppr_fnamef(fname, DATADIR"/%s-%d.0", qentry->destname, qentry->id);

		if((log = fopen(fname, "ab")) == (FILE*)NULL)
			fatal(PPREXIT_OTHERERR, "ppr_split.c: split_job(): failed to open \"%s\" for append, errno=%d (%s)", fname, errno, gu_strerror(errno) );

		fprintf(log, "NOTE: Splitting job into %d fragments.\n", segments_created);

		fclose(log);
		}

		/*----------------------------------------------------------
		** Create the -pages file for each fragment
		----------------------------------------------------------*/
		{
		FILE *pages;			/* .0-pages file */
		FILE **segfiles;		/* array of open segment files */

		/* Open the -pages file. */
		ppr_fnamef(fname, "%s/%s-%d.0-pages",
				DATADIR,
				qentry->destname,qentry->id);
		if((pages = fopen(fname, "rb")) == (FILE*)NULL)
			fatal(PPREXIT_OTHERERR, _("Failed to open \"%s\" for read, errno=%d (%s)"), fname, errno, gu_strerror(errno) );

		/* Create an array of file streams for the segment files. */
		segfiles = (FILE**)gu_alloc(segments_created, sizeof(FILE*));

		/* Open each new -pages file. */
		for(x=0; x < segments_created; x++)
			{
			ppr_fnamef(fname,"%s/%s-%d.%d-pages",
				DATADIR,
				qentry->destname,qentry->id,(x+1));
			#ifdef DEBUG_SPLIT
			printf("Creating \"%s\"\n",fname);
			#endif
			if(!(segfiles[x] = fopen(fname, "wb")))
				fatal(PPREXIT_OTHERERR, _("Failed to open \"%s\" for write, errno=%d, (%s)"), fname, errno, gu_strerror(errno) );
			}

		/* Copy the document defaults section to each segment file. */
		if(fgets(line, sizeof(line), pages))
			{
			if(strncmp(line,"%%BeginDefaults",16)==0)
				{
				int done=FALSE;
				while(!done)
					{
					#ifdef DEBUG_SPLIT
					printf("Defaults line: %s",line);
					#endif

					for(x=0; x < segments_created; x++)
						fputs(line,segfiles[x]);

					if(strncmp(line,"%%EndDefaults",14)==0)
						done=TRUE;

					if( fgets(line,sizeof(line),pages) == (char*)NULL )
						fatal(PPREXIT_OTHERERR, "ppr: ppr_split.c: split_job(): internal error");
					}
				}
			}

		/* Zero the count of pages in each segment. */
		for(x=0; x < segments_created; x++)
			page_counts[x] = 0;

		/* Copy each page record to the apropriate segment file. */
		for(x=0; x < pagenumber; x++)
			{
			#ifdef DEBUG_SPLIT
			printf("page %d of %d, segment %d\n", (x+1), pagenumber, page_segment_assignments[x]);
			#endif

			page_counts[page_segment_assignments[x] - 1]++;

			do
				{
				#ifdef DEBUG_SPLIT
				printf("Page line: %s",line);
				#endif

				fputs(line,segfiles[page_segment_assignments[x] - 1]);

				if( fgets(line,sizeof(line),pages) == (char*)NULL )
					fatal(PPREXIT_OTHERERR, "ppr: ppr_split.c: split_job(): internal error");
				} while( strncmp(line,"%%Page:",7) && strncmp(line,"%%Trailer",9) );
			}

		/* Copy the trailer record to all the segment files. */
		while(TRUE)
			{
			#ifdef DEBUG_SPLIT
			printf("Trailer line: %s",line);
			#endif

			for(x=0; x < segments_created; x++)
				fputs(line,segfiles[x]);

			if( fgets(line,sizeof(line),pages) == (char*)NULL )
				break;
			}

		/* Close all the files. */
		fclose(pages);
		for(x=0; x < segments_created; x++)
			fclose(segfiles[x]);
		}

		/*------------------------------------------------------------
		** Make links for the -text, and -log file for each segment.
		------------------------------------------------------------*/
		{
		char fname_text[MAX_PPR_PATH];
		char fname_log[MAX_PPR_PATH];

		ppr_fnamef(fname_text, "%s/%s-%d.0-text", DATADIR, qentry->destname, qentry->id);
		ppr_fnamef(fname_log,"%s/%s-%d.0-log", DATADIR, qentry->destname,qentry->id);

		for(x=1; x <= segments_created; x++)
			{
			ppr_fnamef(fname, "%s/%s-%d.%d-text", DATADIR, qentry->destname, qentry->id, x);
			link(fname_text, fname);
			#ifdef DEBUG_SPLIT
			printf("link(\"%s\",\"%s\")\n", fname_text, fname);
			#endif

			ppr_fnamef(fname, "%s/%s-%d.%d-log", DATADIR, qentry->destname,qentry->id,x);
			link(fname_log, fname);
			#ifdef DEBUG_SPLIT
			printf("link(\"%s\",\"%s\")\n", fname_log, fname);
			#endif
			}
		}

		/*----------------------------------------------
		** Write a comments file for each fragment
		----------------------------------------------*/
		{
		FILE *cfile, *ncfile;

		ppr_fnamef(fname,"%s/%s-%d.0-comments", DATADIR, qentry->destname,qentry->id);
		if(!(cfile = fopen(fname,"rb")))
			fatal(PPREXIT_OTHERERR, "ppr: ppr_split.c: split_job(): failed to open \"%s\" for read, errno=%d (%s)", fname, errno, gu_strerror(errno) );

		for(x=1; x <= segments_created; x++)
			{
			ppr_fnamef(fname, "%s/%s-%d.%d-comments", DATADIR, qentry->destname, qentry->id, x);
			if(!(ncfile = fopen(fname,"wb")))
				fatal(PPREXIT_OTHERERR, "ppr: ppr_split.c: split_job(): failed to open \"%s\" for write, errno=%d (%s)", fname, errno, gu_strerror(errno) );

			rewind(cfile);

			while(fgets(line, sizeof(line), cfile))
				{
				if( strncmp(line,"%%DocumentMedia:",16) )
					fputs(line,ncfile);
				}

			dump_document_media(ncfile,x);

			fclose(ncfile);
			}

		fclose(cfile);
		}

		/*-----------------------------------------------------------
		** Write a queue file for each fragment
		-----------------------------------------------------------*/
		for(x=0; x < segments_created; x++)
			{
			#ifdef DEBUG_SPLIT
			printf("calling: write_queue_file(): segment=%d, pages=%d)\n", (x+1), page_counts[x]);
			#endif
			qentry->attr.pages = page_counts[x];
			qentry->subid = (x + 1);
			write_queue_file(qentry);
			}

		/*----------------------------------------------------------
		** Submit each fragment to the spooler.
		----------------------------------------------------------*/
		for(x=1; x <= segments_created; x++)
			{
			#ifdef DEBUG_SPLIT
			printf("submitting job %s-%d.%d\n", qentry->destname,qentry->id, x);
			#endif
			submit_job(qentry, x);
			}

		return TRUE;
		} /* end of if(segments_created > 0) */

	return FALSE;						/* We haven't split it */
	} /* end of job_split() */

/*
** Allocate a thing bitmap for the next page.
** This is called at the start of each page.
*/
void prepare_thing_bitmap(void)
	{
	if(!splitting)				/* Don't bother if we won't */
		return;					/* be trying to split. */

	#ifdef DEBUG_SPLIT
	printf("Collecting split information for page %d\n",pagenumber);
	#endif

	/* missing code */

	} /* end of prepare_thing_bitmap() */

/*
** In the part of the bitmap which applies to the current page,
** set the specified bit.  This is called from resource()
** in "ppr_res.c", requirement() in "ppr_req.c", and media()
** in "ppr_media.c".
*/
void set_thing_bit(int bitoffset)
	{
	if(!splitting)				/* Don't bother if we won't be */
		return;					/* trying to split the job. */

	#ifdef DEBUG_SPLIT
	printf("set_thing_bit(%d)\n",bitoffset);
	#endif

	/* missing code */

	} /* end of setbit() */

/*
** write_queue_file() callback routine.  This is how the subset
** of the things which should go into a fragments queue file is
** selected.
**
** This routine will tell an inquirer if a specific thing is
** required for the current fragment.
**
** This routine is called by write_queue_file() routines
** such as "write_media_lines()".
*/
int is_thing_in_current_fragment(int thing_number, int fragment)
	{
	if(!splitting)
		return TRUE;

	#ifdef DEBUG_SPLIT
	printf("is_thing_in_current_fragment(%d,%d)\n",thing_number,fragment);
	#endif

	if(fragment == 0)			/* all things are in the whole job */
		return TRUE;			/* (fragment 0 is the whole job) */

	/* missing code */

	return TRUE;
	} /* end of does_page_need() */

/* end of file */
