/*
** mouse:~ppr/src/libppr/qentryfile_clear.c
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
** Last modified 24 March 2005.
*/

/*! \file */

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"

/** Clear a struct QEntryFile
 *
 * This function clears a struct QEntryFile in preparation for loading information
 * into it.
 */
void qentryfile_clear(struct QEntryFile *job)
	{
	/* Clear the job id variables because we won't read them.
	   (They are encoded in the queue file name.) */
	job->destname = (char*)NULL;
	job->id = 0;
	job->subid = 0;

	/* Some more defaults */
	job->PPRVersion = 0.0;
	job->priority = 20;
	job->time = 0;
	job->commentary = 0;						/* optional */
	job->StripPrinter = TRUE;
	job->commentary = 0;

	/* More pointer defaults which we set to NULL to show they haven't 
	   been read (yet).  This order is the same as in readqfile.c so 
	   as to make it easier to compare the lists. */
	job->username = (char*)NULL;
	job->proxy_for = (char*)NULL;				/* optional */
	job->For = (char*)NULL;
	job->charge_to = (char*)NULL;				/* optional */
	job->magic_cookie = (const char *)NULL;
	job->responder.name = (const char*)NULL;
	job->responder.address = (const char*)NULL;
	job->responder.options = (const char*)NULL; /* optional */
	job->lc_messages = (char*)NULL;

	job->Creator = (char*)NULL;					/* optional */
	job->Title = (char*)NULL;					/* optional */
	job->Routing = (char*)NULL;					/* optional */
	job->lpqFileName = (char*)NULL;				/* optional */
	job->PassThruPDL = (const char *)NULL;		/* optional */
	job->Filters = (const char *)NULL;			/* optional */
	job->PJL = (const char *)NULL;				/* not read by us */

	job->page_list.mask = NULL;
	job->draft_notice = (char*)NULL;			/* optional */
	job->question = NULL;
	job->ripopts = NULL;
	}

/* end of file */
