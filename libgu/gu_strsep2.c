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
** Last modified 11 May 2001.
*/

#include "before_system.h"
#include <string.h>
#include "gu.h"
#include "global_defines.h"

/*
** Like gu_strsep(), but is returns a double-quoted string as a single word.
** A double quote may be escaped with a backslash.  There is also a 
** parameter "discard" which describes junk that we should swallow if it
** is outside the quotes.
*/
char *gu_strsep_quoted(char **stringp, const char *delim, const char *discard)
    {
    char *start, *si, *di;

    start = di = si = *stringp;

    if(discard)
    	si += strspn(si, discard);

    if(*si != '\"')				/* If not a quoted string, */
    	return gu_strsep(stringp, delim);	/* fall back to regular strsep(). */

    si++;					/* move past opening quote */
    while(*si && *si != '\"')			/* until closing quote or end of string, */
    	{
	if(*si == '\\')				/* if character is a backslash */
	    {
	    si++;				/* don't copy the backslash, copy what is after it */
	    if(!*si) break;			/* and stop if there is nothing after it */
	    }
	*(di++) = *(si++);			/* copy this character */
    	}

    if(*(si++) != '\"')				/* If it didn't end with a quote */
    	return NULL;				/* we failed. */

    *di = '\0';					/* terminate copy */

    if(discard)
    	si += strspn(si, discard);

    if(*si && strchr(delim, *si))		/* If the next character is a delimiter, */
	si++;					/* move past it. */

    *stringp = si;				/* update caller's progress pointer */

    return start;				/* return start of string with quoting removed */
    }

/* end of file */
