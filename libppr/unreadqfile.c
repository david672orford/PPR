/*
** mouse:~ppr/src/libppr/unreadqfile.c
** Copyright 1995--2001, Trinity College Computing Center.
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
** Last modified 14 December 2001.
*/

#include "before_system.h"
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"

/* Define a version of gu_free() which can free const stuff: */
#define gu_free_const(a) gu_free((void*)a)

/*
** depopulate the structure
*/
void destroy_struct_QFileEntry(struct QFileEntry *job)
    {
    if(job->destnode)
    	gu_free_const(job->destnode);
    if(job->destname)
    	gu_free_const(job->destname);
    if(job->homenode)
    	gu_free_const(job->homenode);
    if(job->magic_cookie)
    	gu_free_const(job->magic_cookie);
    if(job->username)
    	gu_free_const(job->username);
    if(job->proxy_for)
    	gu_free_const(job->proxy_for);
    if(job->For)
	gu_free_const(job->For);
    if(job->charge_to)
	gu_free_const(job->charge_to);
    if(job->Creator)
	gu_free_const(job->Creator);
    if(job->Title)
	gu_free_const(job->Title);
    if(job->lpqFileName)
    	gu_free_const(job->lpqFileName);
    if(job->draft_notice)
	gu_free_const(job->draft_notice);
    if(job->responder)
	gu_free_const(job->responder);
    if(job->responder_address)
	gu_free_const(job->responder_address);
    if(job->responder_options)
    	gu_free_const(job->responder_options);
    if(job->PassThruPDL)
    	gu_free_const(job->PassThruPDL);
    if(job->Filters)
    	gu_free_const(job->Filters);
    if(job->commentator.progname)
    	gu_free_const(job->commentator.progname);
    if(job->commentator.address)
    	gu_free_const(job->commentator.address);
    if(job->commentator.options)
    	gu_free_const(job->commentator.options);
    if(job->page_list.mask)
    	gu_free(job->page_list.mask);
    if(job->question)
    	gu_free_const(job->question);

    /* Things not read by read_struct_QFileEntry() */
    if(job->PJL)
    	gu_free_const(job->PJL);

    } /* end of destroy_struct_QFileEntry() */

/* end of file */

