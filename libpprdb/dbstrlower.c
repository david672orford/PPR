/*
** mouse:~ppr/src/libpprdb/dbstrlower.c
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
** Last modified 19 April 2001.
*/

/*
** Make a new copy of a string which copy has been converted to
** lower case.
*/

#include "before_system.h"
#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "userdb.h"

char *dbstrlower(const char *s)
    {
    char *ns;           /* new string */
    char *ptr;          /* pointer into new string */

    ptr = ns = (char*)gu_alloc(strlen(s) + 1, sizeof(char));

    while(*s)
	*(ptr++) = tolower(*(s++));

    return ns;
    } /* end of dbstrlower() */

/* end of file */

