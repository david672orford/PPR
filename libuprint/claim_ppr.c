/*
** mouse:~ppr/src/libuprint/claim_ppr.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 8 October 1999.
*/

#include "before_system.h"
#include "gu.h"
#include "global_defines.h"

#include "uprint.h"

/*
** Return TRUE if the destname is the name
** of a valid PPR destination.
*/
int printdest_claim_ppr(const char *destname)
    {
    char fname[MAX_PPR_PATH];
    struct stat statbuf;

    ppr_fnamef(fname, "%s/%s", ALIASCONF, destname);	/* try alias */
    if(stat(fname, &statbuf) == 0)			/* if file found, */
	return TRUE;

    ppr_fnamef(fname, "%s/%s", GRCONF, destname);	/* try group */
    if(stat(fname, &statbuf) == 0)			/* if file found, */
	return TRUE;

    ppr_fnamef(fname, "%s/%s", PRCONF, destname);	/* try printer */
    if(stat(fname, &statbuf) == 0)			/* if it exists, */
	return TRUE;

    return FALSE;
    } /* end of printdest_claim_ppr() */

/* end of file */
