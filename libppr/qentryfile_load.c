/*
** mouse:~ppr/src/libppr/qentryfile_load.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 26 April 2006.
*/

/*! \file */

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"

#define ARGS_2(a,b) a,b
#define ARGS_3(a,b,c) a,b,c
#define ARGS_4(a,b,c,d) a,b,c,d
#define ARGS_5(a,b,c,d,e) a,b,c,d,e
#define ARGS_6(a,b,c,d,e,f) a,b,c,d,e,f

#define MATCH(name, scan, test, flag) \
if(strncmp(line, name, sizeof(name)-1) == 0) \
	{ \
	flag = TRUE; \
	if(gu_sscanf(line+sizeof(name)-1, ARGS##scan) test) \
		{ \
		error("%s: bad \"%s\" line: %s", function, name, line); \
		retcode = -1; \
		} \
	continue; \
	}

/** Load a queue file into a structure
 *
 * Read the queue file up to but not including the first "Media:" line.
 * Return 0 if all goes well, -1 if there is an error in the queue file.
*/
int qentryfile_load(struct QEntryFile *job, FILE *qfile)
	{
	const char function[] = "qentryfile_load";
	gu_boolean found_time = FALSE;
	gu_boolean found_opts = FALSE;
	gu_boolean found_user = FALSE;
	gu_boolean found_for = FALSE;
	gu_boolean found_response = FALSE;
	gu_boolean found_nup = FALSE;
	gu_boolean found_banners = FALSE;
	gu_boolean found_other = FALSE;

	char *line = NULL;
	int line_available = 80;
	char *p;
	int retcode = 0;
	int tempint;

	while((line = gu_getline(line, &line_available, qfile)))
		{
		if(strcmp(line, "EndMisc") == 0)
			break;

		switch(line[0])
			{
			case '\0':
				error("%s: blank line in queue file", function);
				return -1;

			case 'A':
				if((p = lmatchp(line, "Attr-DSC:")))
					{
					if(gu_sscanf(p, "%f %S %d %d %d",
							&job->attr.DSClevel,
							&job->attr.DSC_job_type,
							&job->attr.orientation,
							&job->attr.proofmode,
							&job->attr.docdata
							) != 5)
						{
						error("%s: bad \"%s\" line", function, "Attr-DSC:");
						return -1;
						}
					if(strcmp(job->attr.DSC_job_type, "NULL") == 0)
						{
						gu_free(job->attr.DSC_job_type);
						job->attr.DSC_job_type = NULL;
						}
					continue;
					}
				if((p = lmatchp(line, "Attr-DSC-Sections:")))
					{
					if(gu_sscanf(p, "%d %d %d",
							&job->attr.prolog,
							&job->attr.docsetup,
							&job->attr.script
							) != 3)
						{
						error("%s: bad \"%s\" line", function, "Attr-DSC-Sections:");
						return -1;
						}
					continue;
					}
				if((p = lmatchp(line, "Attr-LangLevel:")))
					{
					if(gu_sscanf(p, "%d %d",
							&job->attr.langlevel,
							&job->attr.extensions
							) != 2)
						{
						error("%s: bad \"%s\" line", function, "Attr-LangLevel:");
						return -1;
						}
					continue;
					}
				if((p = lmatchp(line, "Attr-Pages:")))
					{
					if(gu_sscanf(p, "%d %d %d",
							&job->attr.pages,
							&job->attr.pageorder,
							&job->attr.pagefactor
							) != 3)
						{
						error("%s: bad \"%s\" line", function, "Attr-Pages:");
						return -1;
						}
					continue;
					}
				if((p = lmatchp(line, "Attr-ByteCounts:")))
					{
					if(gu_sscanf(p, "%ld %ld",
							&job->attr.input_bytes,
							&job->attr.postscript_bytes
							) != 2)
						{
						error("%s: bad \"%s\" line", function, "Attr-ByteCounts:");
						return -1;
						}
					continue;
					}
				if((p = lmatchp(line, "Attr-Parts:")))
					{
					if(gu_sscanf(p, "%d",
							&job->attr.parts
							) != 1)
						{
						error("%s: bad \"%s\" line", function, "Attr-Parts:");
						return -1;
						}
					continue;
					}
				break;

			case 'B':
				MATCH("Banners: ", _3("%d %d", &job->do_banner, &job->do_trailer), !=2, found_banners)
				break;

			case 'C':
				MATCH("Charge-To: ", _2("%W", &job->charge_to), !=1, found_other)
				MATCH("Creator: ", _2("%T", &job->Creator), !=1, found_other)
				MATCH("Commentary: ", _2("%d", &job->commentary), !=1, found_other)
				break;

			case 'D':
				MATCH("DraftNotice: ", _2("%T", &job->draft_notice), !=1, found_other)
				break;

			case 'F':
				MATCH("For: ", _2("%T", &job->For), !=1, found_for)
				MATCH("Filters: ", _2("%T", &job->Filters), !=1, found_other)
				break;

			case 'L':
				MATCH("LC_MESSAGES: ", _2("%S", &job->lc_messages), !=1, found_other)
				break;

			case 'l':
				MATCH("lpqFileName: ", _2("%A", &job->lpqFileName), !=1, found_other)
				break;

			case 'M':
				MATCH("MagicCookie: ", _2("%S", &job->magic_cookie), !=1, found_other)
				break;

			case 'N':
				MATCH("N-Up: ", _6("%d %d %d %d %d",
						&job->N_Up.N,				/* virtual pages per sheet side */
						&job->N_Up.borders,			/* TRUE or FALSE, should we have borders? */
						&job->N_Up.sigsheets,		/* sheets per signiture */
						&job->N_Up.sigpart,			/* part of signiture */
						&job->N_Up.job_does_n_up
						), !=5, found_nup)
						/* part of signiture */
				break;

			case 'O':
				MATCH("Opts: ", _6("%d %d %d %d %u",
						&job->opts.binselect,
						&job->opts.copies,
						&job->opts.collate,
						&job->opts.keep_badfeatures,
						&job->opts.hacks), !=5, found_opts)
				break;

			case 'P':
				if(gu_sscanf(line, "PPRD: %hx %x %hx %hx",
						&job->spool_state.priority,
						&job->spool_state.sequence_number,
						&job->spool_state.status,
						&job->spool_state.flags
						) == 4)
					{
					job->spool_state.status *= -1;
					continue;
					}
				if(gu_sscanf(line, "PPRVersion: %f", &job->PPRVersion) == 1)
					continue;
				MATCH("PassThruPDL: ", _2("%T", &job->PassThruPDL), !=1, found_other)
				if(gu_sscanf(line, "PageList: %d %T", &job->page_list.count, &job->page_list.mask) == 2)
					continue;
				break;

			case 'Q':
				MATCH("Question: ", _2("%T", &job->question), !=1, found_other)
				break;

			case 'R':
				MATCH("Routing: ", _2("%T", &job->Routing), !=1, found_other)
				MATCH("Response: ", _4("%S %S %T", &job->responder.name, &job->responder.address, &job->responder.options), <2, found_response)
				MATCH("RIPopts: ", _2("%T", &job->ripopts), !=1, found_other)
				break;

			case 'S':
				if(gu_sscanf(line, "StripPrinter: %d", &tempint) == 1)
					{
					job->StripPrinter = tempint ? TRUE : FALSE;
					continue;
					}
				break;

			case 'T':
				MATCH("Time: ", _2("%U", &job->time), !=1, found_time)
				MATCH("Title: ", _2("%T", &job->Title), !=1, found_other)
				break;

			case 'U':
				/* This is %T for consistency with the simplified code in
				 * IPP server which just takes the part of the
				 * line after "User:".
				 */
				MATCH("User: ", _2("%T", &job->user), <1, found_user)
				break;
			}

		error("%s: Unrecognized: %s", function, line);
		retcode = -1;
		} /* end of loop */

	if(! found_time)
		{
		error("%s: missing \"%s\" line", function, "Time: ");
		retcode = -1;
		}

	if(! found_opts)
		{
		error("%s: missing \"%s:\" line", function, "Opts: ");
		retcode = -1;
		}

	if(! found_user)
		{
		error("%s: missing \"%s\" line", function, "User: ");
		retcode = -1;
		}

	if(! found_for)
		{
		error("%s: missing \"%s\" line", function, "For: ");
		retcode = -1;
		}

	if(! found_response)
		{
		error("%s: missing \"%s\" line", function, "Response: ");
		retcode = -1;
		}

	if(! found_nup)
		{
		error("%s: missing \"N-Up:\" line", function);
		retcode = -1;
		}

	if(! found_banners)
		{
		error("%s: missing \"Banners:\" line", function);
		retcode = -1;
		}

	while((line = gu_getline(line, &line_available, qfile)))
		{
		if(strcmp(line, "EndIPP") == 0)
			break;

		}

	if(line)
		gu_free(line);

	return retcode;
	} /* end of read_struct_QEntryFile() */

/* end of file */

