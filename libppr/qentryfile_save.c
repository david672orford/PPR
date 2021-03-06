/*
** mouse:~ppr/src/libppr/qentryfile_save.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 21 April 2006.
*/

/*! \file */

#include "config.h"
#include <stdio.h>
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "version.h"

/** write a string QEntryFile to a file
 *
 */
int qentryfile_save(const struct QEntryFile *qentry, FILE *Qfile)
	{
	/* This line must remain at the begining of the file where
	 * pprd can easily modify it. */
	if(fprintf(Qfile, "PPRD: %02X %08X %02X %04X                                      \n",
			qentry->spool_state.priority,
			qentry->spool_state.sequence_number,
			(qentry->spool_state.status * -1),
			qentry->spool_state.flags) != 64)
		gu_Throw("PPRD line is not 64 bytes long!");

	/* Keep this one at the begining too */
	fprintf(Qfile, "User: %s\n", qentry->user);

	/* We don't really use this, but it could solve arguments. */
	fprintf(Qfile, "PPRVersion: %s\n", SHORT_VERSION);
	
	/* Human language in which to communicate with the submitter
	 * about this job. */
	if(qentry->lc_messages)
		fprintf(Qfile, "LC_MESSAGES: %s\n", qentry->lc_messages);

	/* This is so we will know when the job was submitted. */
	fprintf(Qfile, "Time: %ld\n", qentry->time);

	fprintf(Qfile, "MagicCookie: %s\n", qentry->magic_cookie);

	/* more submitter identity */
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

	fprintf(Qfile, "Response: %s %s %s\n",
		qentry->responder.name,
		qentry->responder.address,
		qentry->responder.options ? qentry->responder.options : ""
		);

	/* If the --commentatary switch was used, emmit a "Commentary:" line. */
	fprintf(Qfile, "Commentary: %d\n", qentry->commentary);

	fprintf(Qfile, "Attr-DSC: %s %s %d %d %d\n",
				gu_dtostr(qentry->attr.DSClevel),
				qentry->attr.DSC_job_type ? qentry->attr.DSC_job_type : "NULL",
				qentry->attr.orientation,
				qentry->attr.proofmode,
		   (int)qentry->attr.docdata);
	fprintf(Qfile, "Attr-DSC-Sections: %d %d %d\n",
				qentry->attr.prolog,
				qentry->attr.docsetup,
				qentry->attr.script);
	fprintf(Qfile, "Attr-LangLevel: %d %d\n",
				qentry->attr.langlevel,
				qentry->attr.extensions);
	fprintf(Qfile, "Attr-Pages: %d %d %d\n",
				qentry->attr.pages,
				qentry->attr.pageorder,
				qentry->attr.pagefactor);
	fprintf(Qfile, "Attr-ByteCounts: %ld %ld\n",
				qentry->attr.input_bytes,
				qentry->attr.postscript_bytes);
	fprintf(Qfile, "Attr-Parts: %d\n",
				qentry->attr.parts);

	fprintf(Qfile, "Opts: %d %d %d %d %u\n",
				qentry->opts.binselect,
				qentry->opts.copies,
				qentry->opts.collate,
				qentry->opts.keep_badfeatures,
				qentry->opts.hacks
				);

	fprintf(Qfile, "N-Up: %d %d %d %d %d\n",
				qentry->N_Up.N,					/* virtual pages on each side of sheet */
				qentry->N_Up.borders,			/* should we print borders? */
				qentry->N_Up.sigsheets,			/* how many sheets per signature? (0=no signature printing) */
				qentry->N_Up.sigpart,			/* Fronts, backs, both */
				qentry->N_Up.job_does_n_up
				);	   

	if(qentry->Filters)
		fprintf(Qfile, "Filters: %s\n", qentry->Filters);

	if(qentry->PassThruPDL)
		fprintf(Qfile, "PassThruPDL: %s\n", qentry->PassThruPDL);

	fprintf(Qfile, "StripPrinter: %d\n", (int)qentry->StripPrinter);

	if(qentry->page_list.mask)
		fprintf(Qfile, "PageList: %d %s\n", qentry->page_list.count, qentry->page_list.mask);

	if(qentry->question)
		fprintf(Qfile, "Question: %s\n", qentry->question);

	if(qentry->ripopts)
		fprintf(Qfile, "RIPopts: %s\n", qentry->ripopts);

	fprintf(Qfile, "EndMisc\n");

	/* see RFC 2911 4.3.6 */
	fprintf(Qfile, "job-originating-user-name %s\n", qentry->user ? qentry->user : "?");

	/* see RFC 2911 4.3.5 */
	if(qentry->Title && qentry->Title[0] != '\0')
		fprintf(Qfile, "job-name %s\n", qentry->Title);
	else if(qentry->lpqFileName)
		fprintf(Qfile, "job-name %s\n", qentry->lpqFileName);

	/* see RFC 2911 4.3.17.1 (copies not included) */
	if(qentry->PassThruPDL)
		fprintf(Qfile, "job-k-octets %ld\n", (qentry->attr.input_bytes + 512) / 1024);
	else
		fprintf(Qfile, "job-k-octets %ld\n", (qentry->attr.postscript_bytes + 512) / 1024);

	/* see RFC 2911 4.3.17.2 (we assume this means sides without copies) */
	fprintf(Qfile, "job-impressions %d\n", 
		(int)((qentry->attr.pages + qentry->N_Up.N - 1) / qentry->N_Up.N)
		);

	/* see RFC 2911 4.3.17.3 (this means sheets without copies) */
	fprintf(Qfile, "job-media-sheets %d\n",
		(int)((qentry->attr.pages + qentry->attr.pagefactor - 1) / qentry->attr.pagefactor)
			*
		qentry->opts.copies
		);

	fprintf(Qfile, "EndIPP\n");
	return 0;
	}

/* end of file */
