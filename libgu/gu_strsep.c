/*
** mouse:~ppr/src/libgu/gu_strsep.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 4 May 2001.
*/

#include "before_system.h"
#include <string.h>
#include "gu.h"
#include "global_defines.h"

/*
** This is intended as a clone of BSD strsep().
**
** BSD strsep() differs from ANSI strtok() in that it can handle
** empty fields and in that it has no static storage, so it is
** thread safe.
*/
char *gu_strsep(char **stringp, const char *delim)
    {
    char *start;
    size_t len;

    start = *stringp;			/* first token starts immediately */

    if(!*start)				/* if no more line left, no token */
    	return NULL;

    len = strcspn(start, delim);	/* token length is length of run without delimiters */

    *stringp += len;			/* for next time, move past the token */

    if(start[len])			/* if terminated by delimiter rather than end of string, */
    	{
    	start[len] = '\0';		/* insert a ASCIIz string terminator */
    	(*stringp)++;			/* and move past the delimiter for next time */
    	}

    return start;			/* return the token we found */
    }

/* end of file */

