/*
** mouse:~ppr/src/libppr/readqfile.c
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
** Last modified 30 November 2001.
*/

#include "before_system.h"
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

/*
** Read the queue file up to but not including the first "Media:" line.
** Return 0 if all goes well, -1 if there is an error in the queue file.
*/
int read_struct_QFileEntry(FILE *qfile, struct QFileEntry *job)
    {
    const char function[] = "read_struct_QFileEntry";
    gu_boolean found_time = FALSE;
    gu_boolean found_opts = FALSE;
    gu_boolean found_attr = FALSE;
    gu_boolean found_user = FALSE;
    gu_boolean found_for = FALSE;
    gu_boolean found_priority = FALSE;
    gu_boolean found_response = FALSE;
    gu_boolean found_nup = FALSE;
    gu_boolean found_banners = FALSE;
    gu_boolean found_other = FALSE;

    char *line = NULL;
    int line_available = 80;
    int retcode = 0;
    int tempint;

    /* Clear these things because we won't read them.
       (They are encoded in the queue file name.) */
    job->destnode = (char*)NULL;
    job->destname = (char*)NULL;
    job->id = 0;
    job->subid = 0;
    job->homenode = (char*)NULL;

    job->PPRVersion = 0.0;
    job->username = (char*)NULL;
    job->For = (char*)NULL;
    job->proxy_for = (char*)NULL;		/* optional */
    job->priority = 20;
    job->responder = (char*)NULL;
    job->responder_address = (char*)NULL;
    job->responder_options = (char*)NULL;	/* optional */
    job->time = 0;
    job->Title = (char*)NULL;			/* optional */
    job->charge_to = (char*)NULL;		/* optional */
    job->Creator = (char*)NULL;			/* optional */
    job->Routing = (char*)NULL;			/* optional */
    job->lpqFileName = (char*)NULL;		/* optional */
    job->draft_notice = (char*)NULL;		/* optional */
    job->commentary = 0;			/* optional */
    job->PassThruPDL = (const char *)NULL;	/* optional */
    job->Filters = (const char *)NULL;		/* optional */
    job->CachePriority = CACHE_PRIORITY_LOW;	/* optional for now */
    job->StripPrinter = TRUE;
    job->commentator.interests = 0;
    job->commentator.progname = NULL;
    job->commentator.address = NULL;
    job->commentator.options = NULL;
    job->commentator.next = NULL;		/* unused at present */
    job->page_list.mask = NULL;

    /* We do not actually read these items: */
    job->PJL = (const char *)NULL;

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
		if(strncmp(line, "Attr: ", 6) == 0)
		    {
		    int tempint;
		    found_attr = TRUE;
		    if(sscanf(line+6, "%d %f %d %d %d %d %d %d %d %d %d %ld %ld %d %d",
			    &job->attr.langlevel,
			    &job->attr.DSClevel,
			    &job->attr.pages,
			    &job->attr.pageorder,
			    &job->attr.prolog,
			    &job->attr.docsetup,
			    &job->attr.script,
			    &job->attr.extensions,
			    &job->attr.pagefactor,
			    &job->attr.orientation,
			    &job->attr.proofmode,
			    &job->attr.input_bytes,
			    &job->attr.postscript_bytes,
			    &job->attr.parts,
			    &tempint
			    ) != 15)
			{
			error("%s: bad \"%s\" line", function, "Attr: ");
			return -1;
			}

		    switch(tempint)
			{
			case CODES_UNKNOWN:
			case CODES_Clean7Bit:
			case CODES_Clean8Bit:
			case CODES_Binary:
			    job->attr.docdata = (enum CODES)tempint;
			    break;
			default:
			    error("%s: bad value for attr.docdata", function);
			    return -1;
			    break;
			}

		    continue;
		    } /* "Attr: " */
		if(strncmp(line, "Attr-", 5) == 0)
		    {
		    continue;
		    }
		break;

	    case 'B':
		MATCH("Banners: ", _3("%d %d", &job->do_banner, &job->do_trailer), !=2, found_banners)
		break;

	    case 'C':
		MATCH("Charge-To: ", _2("%Z", &job->charge_to), !=1, found_other)
		MATCH("Creator: ", _2("%Z", &job->Creator), !=1, found_other)
		MATCH("Commentator: ", _5("%d %Q %Q %Q", &job->commentator.interests, &job->commentator.progname, &job->commentator.address, &job->commentator.options), !=4, found_other)
		{
		int tempint;
		if(gu_sscanf(line, "CachePriority: %d", &tempint) == 1)
		    {
		    switch(tempint)
		    	{
			case CACHE_PRIORITY_LOW:
			case CACHE_PRIORITY_HIGH:
			    job->CachePriority = (enum CACHE_PRIORITY)tempint;
			    break;
			default:
			    error("%s(): bad value for cache_priority", function);
			    return -1;
			    break;
		    	}
		    continue;
		    }
		}
		break;

	    case 'D':
	    	MATCH("DraftNotice: ", _2("%Z", &job->draft_notice), !=1, found_other)
		break;

	    case 'F':
		MATCH("For: ", _2("%Z", &job->For), !=1, found_for)
		MATCH("Filters: ", _2("%Z", &job->Filters), !=1, found_other)
		break;

	    case 'l':
		MATCH("lpqFileName: ", _2("%Z", &job->lpqFileName), !=1, found_other)
		break;

	    case 'N':
		MATCH("N-Up: ", _5("%d %d %d %d",
	    		&job->N_Up.N,			/* virtual pages per sheet side */
	    		&job->N_Up.borders,		/* TRUE or FALSE, should we have borders? */
	    		&job->N_Up.sigsheets,		/* sheets per signiture */
	    		&job->N_Up.sigpart), !=4, found_nup)	/* part of signiture */
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
		MATCH("Priority: ", _2("%d", &job->priority), !=1, found_priority)
		if(sscanf(line, "PPRVersion: %f", &job->PPRVersion) == 1)
		    continue;
		MATCH("PassThruPDL: ", _2("%Z", &job->PassThruPDL), !=1, found_other)
		if(gu_sscanf(line, "PageList: %d %Z", &job->page_list.count, &job->page_list.mask) == 2)
		    continue;
		break;

	    case 'R':
		MATCH("Routing: ", _2("%Z", &job->Routing), !=1, found_other)
		MATCH("Response: ", _4("%S %S %Z", &job->responder, &job->responder_address, &job->responder_options), <2, found_response)
		break;

	    case 'S':
		if(sscanf(line, "StripPrinter: %d", &tempint) == 1)
		    {
		    job->StripPrinter = tempint ? TRUE : FALSE;
		    continue;
		    }
		if(sscanf(line, "Status-and-Flags: %d %hx", &job->status, &job->flags) == 1)
		    continue;
	    	break;

	    case 'T':
	    	MATCH("Time: ", _2("%ld", &job->time), !=1, found_time)
		MATCH("Title: ", _2("%Z", &job->Title), !=1, found_other)
		break;

	    case 'U':
		MATCH("User: ", _4("%ld %S %S", &job->user, &job->username, &job->proxy_for), <2, found_user)
		break;
	    }

	error("%s: Unrecognized: %s", function, line);
	retcode = -1;
	} /* end of loop */

    if(line) gu_free(line);

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

    if(! found_attr)
	{
	error("%s: missing \"%s\" line", function, "Attr: ");
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

    if(! found_priority)
	{
	error("%s: missing \"%s\" line", function, "Priority: ");
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

    return retcode;
    } /* end of read_struct_QFileEntry() */

/* end of file */

