/*
** mouse.trincoll.edu:~ppr/src/libuprint/uprint_type_ppr.c
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
#include <string.h>
#include "gu.h"
#include "global_defines.h"

#include "uprint.h"
#include "uprint_private.h"

/*
**
** Given a filled in UPRINT structure, this function 
** will return an argument for ppr's -T switch.
*/
const char *uprint_get_content_type_ppr(void *p)
    {
    struct UPRINT *upr = (struct UPRINT *)p;

    DODEBUG(("uprint_get_content_type_ppr(p=%p)", p));

    if(upr->content_type_lpr != '\0')
	{
	switch(upr->content_type_lpr)
	    {
	    case 'c':			/* cifplot(1) output */
		return "cif";
	    case 'd':			/* TeX DVI */
		return "dvi";
	    case 'f':			/* formatted */
	    	return (const char *)NULL;
	    case 'g':			/* plot(1) data */
	    	return "plot";
	    case 'l':			/* leave control codes */
		return (const char *)NULL;
	    case 'n':			/* ditroff output */
		return "troff";
	    case 'p':			/* pass thru pr(1) */
		return "pr";
	    case 't':			/* old troff output */
		return "cat4";
	    case 'v':			/* sun raster format */
		return "sunras";
	    case 'o':			/* PostScript */
	    	return "postscript";
	    }
	}

    else if(upr->content_type_lp != (char*)NULL)
	{
	if(strcmp(upr->content_type_lp, "simple") == 0)
	    return (const char *)NULL;

	if(strcmp(upr->content_type_lp, "postscript") == 0)
	    return "postscript";
	}

    return (const char *)NULL;
    } /* end of uprint_get_content_type_ppr() */

/* end of file */
