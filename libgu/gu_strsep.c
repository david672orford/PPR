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

/*! \file
	\brief string parser

*/

#include "config.h"
#include <string.h>
#include "gu.h"
#include "global_defines.h"

/** extract fields from a string

This is intended as a clone of BSD strsep(). BSD strsep() differs from ANSI
strtok() in that it can handle empty fields and in that it has no static
storage, so it is thread safe.

p = line;
if(!(f1 = gu_strsep(&p, ":")) || !(f2 = gu_strsep(&p, ":")))
	{
	error(_("Not enough fields in \"%s\" line %d"), filename, linenum);
	return -1;
	}

*/
char *gu_strsep(char **stringp, const char *delim)
	{
	char *start;
	size_t len;

	start = *stringp;					/* first token starts immediately */

	if(!start)							/* if we reached the end last time, no token */
		return NULL;

	len = strcspn(start, delim);		/* token length is length of run without delimiters */

	if(start[len])						/* if terminated by delimiter rather than end of string, */
		{
		start[len] = '\0';				/* insert a ASCIIz string terminator */
		/* don't remove parentheses from next two lines */
		(*stringp) += len;				/* for next time, move past the token */
		(*stringp)++;					/* and move past the delimiter for next time */
		}
	else
		{
		*stringp = NULL;
		}
	
	return start;						/* return the token we found */
	}

/* end of file */

