/*
** mouse.trincoll.edu:~ppr/src/libuprint/uprint_type_lpr.c
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
** Last modified 12 May 1998.
*/

#include "before_system.h"
#include <string.h>
#include "gu.h"
#include "global_defines.h"

#include "uprint.h"
#include "uprint_private.h"

/*
** This function returns the switch which should be used
** to indicate the content type when invoking lpr.
** It will return a NULL character if none should be
** used.
*/
char uprint_get_content_type_lpr(void *p)
    {
    struct UPRINT *upr = (struct UPRINT *)p;

    /* If it was set directly, */
    if(upr->content_type_lpr != (char)NULL)
    	return upr->content_type_lpr;

    /* If we can convert an lp content type spec, */
    if(upr->content_type_lp != (char*)NULL)
	{
	struct LP_LPR_TYPE_XLATE *p;

	for(p = lp_lpr_type_xlate; p->lpname != (const char *)NULL || p->lprcode != '\0'; p++)
	    {
	    if(strcmp(p->lpname, upr->content_type_lp) == 0)
	    	return p->lprcode;
	    }
	}

    return '\0';
    } /* end of uprint_get_content_type_lpr() */

/* end of file */
