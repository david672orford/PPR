/*
** mouse:~ppr/src/libppr/writeqfile.c
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
** Last modified 5 December 2001.
*/

#include "before_system.h"
#include <stdio.h>
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "version.h"

int write_struct_QFileEntry(FILE *Qfile, const struct QFileEntry *qentry)
    {
    /* This line will be useful once distributed printing is implemented. */
    fprintf(Qfile, "PPRVersion: %s\n", SHORT_VERSION);

    /* This is at the begining of the file so that it will be easy to modify. */
    fprintf(Qfile, "Status-and-Flags: %02d %04X\n", (qentry->status * -1), qentry->flags);

    /* This is so we will know when the job was submitted. */
    fprintf(Qfile, "Time: %ld\n", qentry->time);

    fprintf(Qfile, "User: %ld %s %s\n",
    	qentry->user,					/* Unix user id */
    	qentry->username ? qentry->username : "?",	/* Unix user name */
	qentry->proxy_for ? qentry->proxy_for : "");

    fprintf(Qfile, "Priority: %d\n", qentry->priority);

    fprintf(Qfile, "For: %s\n", qentry->For ? qentry->For : "???");

    if(qentry->charge_to)
	fprintf(Qfile, "Charge-To: %s\n", qentry->charge_to);

    /* It is permissible to use --title "" to make the title an empty string 
       and thereby prevent a temporary file name from becoming the title by 
       default.  That is why we must test for an empty title string.
       */
    if(qentry->Title && qentry->Title[0] != '\0')
	fprintf(Qfile, "Title: %s\n", qentry->Title);

    if(qentry->draft_notice)
	fprintf(Qfile, "DraftNotice: %s\n", qentry->draft_notice);

    if(qentry->Creator)
	fprintf(Qfile, "Creator: %s\n", qentry->Creator);

    if(qentry->Routing)
	fprintf(Qfile, "Routing: %s\n", qentry->Routing);

    if(qentry->lpqFileName)
    	fprintf(Qfile, "lpqFileName: %s\n", qentry->lpqFileName);

    fprintf(Qfile, "Banners: %d %d\n", qentry->do_banner, qentry->do_trailer);

    fprintf(Qfile, "Response: %.*s %.*s %.*s\n", MAX_RESPONSE_METHOD, qentry->responder,
	MAX_RESPONSE_ADDRESS, qentry->responder_address,
	MAX_RESPONDER_OPTIONS, qentry->responder_options ? qentry->responder_options : "");

    /* If the --commentatary switch was used, emmit a "Commentator:" line.
       If no --commentator or --commentator-address option was used, use the
       same name as the responder and the responder address.  If no
       --commentator-options switch was used, use a blank options list.
       */
    if(qentry->commentator.interests)
	{
	fprintf(Qfile, "Commentator: %d \"%s\" \"%s\" \"%s\"\n",
		qentry->commentator.interests,
		qentry->commentator.progname ? qentry->commentator.progname : qentry->responder,
		qentry->commentator.address ? qentry->commentator.address : qentry->responder_address,
		qentry->commentator.options ? qentry->commentator.options : "");
	}

    /* This big long line contains lots of information.  It will later be broken
       up into several lines. */
    fprintf(Qfile, "Attr: %d %.1f %d %d %d %d %d %d %d %d %d %ld %ld %d %d\n",
		qentry->attr.langlevel,
		qentry->attr.DSClevel,
		qentry->attr.pages,
		qentry->attr.pageorder,
		qentry->attr.prolog,
		qentry->attr.docsetup,
		qentry->attr.script,
		qentry->attr.extensions,
		qentry->attr.pagefactor,
		qentry->attr.orientation,	/* missing below */
		qentry->attr.proofmode,		/* missing below */
		qentry->attr.input_bytes,
		qentry->attr.postscript_bytes,
		qentry->attr.parts,		/* missing below */
		(int)qentry->attr.docdata	/* missing below */
		);

    /* These will replace "Attr:".  For now they are ignored. */
    fprintf(Qfile, "Attr-DSC: %.1f %d %d %d %d\n",
    		qentry->attr.DSClevel,
    		qentry->attr.prolog,
    		qentry->attr.docsetup,
    		qentry->attr.script,
    		qentry->attr.pageorder);
    fprintf(Qfile, "Attr-Langlevel: %d %d\n",
    		qentry->attr.langlevel,
    		qentry->attr.extensions);
    fprintf(Qfile, "Attr-Pages: %d %d\n",
    		qentry->attr.pages, qentry->attr.pagefactor);
    fprintf(Qfile, "Attr-ByteCounts: %ld %ld\n",
    		qentry->attr.input_bytes, qentry->attr.postscript_bytes);

    fprintf(Qfile, "Opts: %d %d %d %d %u\n",
		qentry->opts.binselect,
		qentry->opts.copies,
		qentry->opts.collate,
		qentry->opts.keep_badfeatures,
		qentry->opts.hacks);

    fprintf(Qfile, "N-Up: %d %d %d %d\n",
		qentry->N_Up.N,			/* virtual pages on each side of sheet */
    		qentry->N_Up.borders,		/* should we print borders? */
    		qentry->N_Up.sigsheets,		/* how many sheets per signature? (0=no signature printing) */
		qentry->N_Up.sigpart);		/* Fronts, backs, both */

    if(qentry->Filters)
	fprintf(Qfile, "Filters: %s\n", qentry->Filters);

    if(qentry->PassThruPDL)
    	fprintf(Qfile, "PassThruPDL: %s\n", qentry->PassThruPDL);

    fprintf(Qfile, "CachePriority: %d\n", (int)qentry->CachePriority);

    fprintf(Qfile, "StripPrinter: %d\n", (int)qentry->StripPrinter);

    if(qentry->page_list.mask)
	fprintf(Qfile, "PageList: %d %s\n", qentry->page_list.count, qentry->page_list.mask);

    return 0;
    }

/* end of file */
