/*
** mouse:~ppr/src/libppr/qentryfile_free.c
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
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"

static void free_and_zero(void **p)
	{
	if(*p)
		{
		gu_free(*p);
		*p = (void*)NULL;
		}
	}

/** Free the storage used by a struct QEntryFile
 *
 * Frees the memory blocks used by a struct QEntryFile's data members.
*/
void qentryfile_free(struct QEntryFile *job)
	{
	free_and_zero((void**)&job->destname);

	free_and_zero((void**)&job->username);
	free_and_zero((void**)&job->proxy_for);
	free_and_zero((void**)&job->For);
	free_and_zero((void**)&job->charge_to);
	free_and_zero((void**)&job->magic_cookie);
	free_and_zero((void**)&job->responder.name);
	free_and_zero((void**)&job->responder.address);
	free_and_zero((void**)&job->responder.options);
	free_and_zero((void**)&job->lc_messages);

	free_and_zero((void**)&job->Creator);
	free_and_zero((void**)&job->Title);
	free_and_zero((void**)&job->Routing);
	free_and_zero((void**)&job->lpqFileName);
	free_and_zero((void**)&job->PassThruPDL);
	free_and_zero((void**)&job->Filters);
	free_and_zero((void**)&job->PJL);

	free_and_zero((void**)&job->page_list.mask);
	free_and_zero((void**)&job->draft_notice);
	free_and_zero((void**)&job->question);
	} /* end of qentryfile_free() */

/* end of file */

