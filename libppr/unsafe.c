/*
** mouse:~ppr/src/libppr/unsafe.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 5 September 2001.
*/

#include "before_system.h"
#include <string.h>
#include "gu.h"
#include "global_defines.h"

/*
** Function to detect characters PostScript will object to in
** font names, medium types, and colour names.
*/
gu_boolean is_unsafe_ps_name(const char name[])
    {
    if(strspn(name, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    		"abcdefghijklmnopqrstuvwxyz"
    		"0123456789-_") != strlen(name))
    	return TRUE;

    return FALSE;
    } /* end of is_unsafe_ps_name() */

/* end of file */

