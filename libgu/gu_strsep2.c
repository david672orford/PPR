/*
** mouse:~ppr/src/libgu/gu_strsep.c
** Copyright 1995--2003, Trinity College Computing Center.
** Written by David Chappell.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
** 
** * Redistributions of source code must retain the above copyright notice,
** this list of conditions and the following disclaimer.
** 
** * Redistributions in binary form must reproduce the above copyright
** notice, this list of conditions and the following disclaimer in the
** documentation and/or other materials provided with the distribution.
** 
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE 
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
** POSSIBILITY OF SUCH DAMAGE.
**
** Last modified 14 November 2003.
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

	if(!start)									/* Did we reach the end last time? */
		return NULL;
	
	if(discard)									/* If there is a discard list, ignore those characters. */
		si += strspn(si, discard);

	if(*si != '\"')								/* If not a quoted string, */
		return gu_strsep(stringp, delim);		/* fall back to regular strsep(). */

	si++;										/* move past opening quote */
	while(*si && *si != '\"')					/* until closing quote or end of string, */
		{
		if(*si == '\\')							/* if character is a backslash */
			{
			si++;								/* don't copy the backslash, copy what is after it */
			if(!*si) break;						/* and stop if there is nothing after it */
			}
		*(di++) = *(si++);						/* copy this character */
		}

	if(*(si++) != '\"')							/* If it didn't end with a quote */
		return NULL;							/* we failed. */

	*di = '\0';									/* terminate copy */

	if(discard)
		si += strspn(si, discard);

	if(*si && strchr(delim, *si))				/* If the next character is a delimiter, */
		{
		si++;									/* move past it. */
		*stringp = si;							/* update caller's progress pointer */
		}
	else										/* otherwise we go no furthur */
		{
		*stringp = NULL;
		}

	return start;								/* return start of string with quoting removed */
	}

/* end of file */
