/*
** mouse:~ppr/src/libppr/reswidth.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 21 November 2000.
*/

/*
** Return the maximum line length for a specified responder.  If we don't know,
** we return 0.  The number returned by this routine is passed to gu_wordwrap().
** If this routine returns 0, gu_wordwrap() does nothing, leaving the default line
** breaks in place.
*/

#include "before_system.h"
#include <string.h>
#include "gu.h"
#include "global_defines.h"


int get_responder_width(const char *name)
    {
    if(strcmp(name, "samba") == 0)
    	return 58;

    return 0;		/* others get default */
    } /* end of get_responder_width() */

/* end of file */
