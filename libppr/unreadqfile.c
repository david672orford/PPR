/*
** mouse:~ppr/src/libppr/unreadqfile.c
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
** Last modified 30 March 2001.
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

    /* Things not read by read_struct_QFileEntry() */
    if(job->PJL)
    	gu_free_const(job->PJL);

    } /* end of destroy_struct_QFileEntry() */

/* end of file */

