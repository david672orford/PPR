/*
** mouse.trincoll.edu:~ppr/src/libuprint/uprint_type_lp.c
** Copyright 1997, 1998, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 24 March 1998.
*/

#include "before_system.h"
#include "gu.h"
#include "global_defines.h"

#include "uprint.h"
#include "uprint_private.h"

/*
** Get the System V lp content type of the job
** represented by the UPRINT structure.
*/
const char *uprint_get_content_type_lp(void *p)
    {
    struct UPRINT *upr = (struct UPRINT *)p;

    /* If the caller set the LP content type specifically: */
    if(upr->content_type_lp != (const char *)NULL)
    	return upr->content_type_lp;

    /* We may have to figure it out from the lpr content type: */
    if(upr->content_type_lpr != '\0')
	{
	struct LP_LPR_TYPE_XLATE *p;
	
	for(p = lp_lpr_type_xlate; p->lpname != (const char *)NULL || p->lprcode != '\0'; p++)
	    {
	    if(p->lprcode == upr->content_type_lpr)
	    	return p->lpname;
	    }	
	}

    return (const char *)NULL;
    } /* end of uprint_get_content_type_lp() */

/* end of file */
