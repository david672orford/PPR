/*
** mouse:~ppr/src/libppr/writeqfile.c
** Copyright 1995--2002, Trinity College Computing Center.
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
** Last modified 18 Novemer 2002.
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

	fprintf(Qfile, "MagicCookie: %s\n", qentry->magic_cookie);

	fprintf(Qfile, "User: %ld %s %s\n",
		qentry->user,									/* Unix user id */
		qentry->username ? qentry->username : "?",		/* Unix user name */
		qentry->proxy_for ? qentry->proxy_for : "");

	if(qentry->lc_messages)
		fprintf(Qfile, "LC_MESSAGES: %s\n", qentry->lc_messages ? qentry->lc_messages : "");

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

	fprintf(Qfile, "Response: %s %s %s\n", qentry->responder,
		qentry->responder_address,
		qentry->responder_options ? qentry->responder_options : "");

	/* If the --commentatary switch was used, emmit a "Commentary:" line.
	   */
	fprintf(Qfile, "Commentary: %d\n", qentry->commentary);

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
				qentry->attr.orientation,		/* missing below */
				qentry->attr.proofmode,			/* missing below */
				qentry->attr.input_bytes,
				qentry->attr.postscript_bytes,
				qentry->attr.parts,				/* missing below */
				(int)qentry->attr.docdata		/* missing below */
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
				qentry->N_Up.N,					/* virtual pages on each side of sheet */
				qentry->N_Up.borders,			/* should we print borders? */
				qentry->N_Up.sigsheets,			/* how many sheets per signature? (0=no signature printing) */
				qentry->N_Up.sigpart);			/* Fronts, backs, both */

	if(qentry->Filters)
		fprintf(Qfile, "Filters: %s\n", qentry->Filters);

	if(qentry->PassThruPDL)
		fprintf(Qfile, "PassThruPDL: %s\n", qentry->PassThruPDL);

	fprintf(Qfile, "CachePriority: %d\n", (int)qentry->CachePriority);

	fprintf(Qfile, "StripPrinter: %d\n", (int)qentry->StripPrinter);

	if(qentry->page_list.mask)
		fprintf(Qfile, "PageList: %d %s\n", qentry->page_list.count, qentry->page_list.mask);

	if(qentry->question)
		fprintf(Qfile, "Question: %s\n", qentry->question);

	if(qentry->ripopts)
		fprintf(Qfile, "RIPopts: %s\n", qentry->ripopts);

	return 0;
	}

/* end of file */
