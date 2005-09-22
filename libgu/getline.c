/*
** mouse:~ppr/src/libppr/getline.c
** Copyright 1995--2005, Trinity College Computing Center.
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
** Last modified 22 September 2005.
*/

/*! \file
	\brief read lines of unlimited length from a file efficiently
*/

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include "gu.h"

/** read lines of unlimited length from a file efficiently

This function is used to read configuration file lines.  It avoids
the problem of buffers that are too small.  The parameter "line"
points to a buffer in gu_alloc()ed memory.  On the first call this
may be a NULL pointer.  If it is too small, a new, longer one will
be obtained and "space_available" will be updated.  The new buffer
is returned.  The caller should initialy set "line_available" to
a little bigger than the LIKELY line length.  Notice that on EOF
the line is automatically freed.  If you don't read until EOF you
must manually free the line.  The file will still be open.

Use the function like this:

\code
int line_available = 80;
char *line = NULL;

while((line = gu_getline(line, &line_available, stdin))
	{

	}
\endcode
*/
char *gu_getline(char *line, int *space_available, FILE *fstream)
	{
	int len;

	if(*space_available < 1)	/* sanity check */
		return NULL;

	if(!line)					/* if not allocated yet, */
		line = (char*)gu_alloc(*space_available, sizeof(char));

	if(!fgets(line, *space_available, fstream))
		{
		int e = errno;
		gu_free(line);
		errno = e;
		return NULL;
		}

	len = strlen(line);

	/* If fgets() filled the available space but the last character 
	 * is not a newline,
	 */
	while(len == (*space_available - 1) && line[len - 1] != '\n')
		{
		*space_available *= 2;		/* double the space */
		line = (char*)gu_realloc(line, *space_available, sizeof(char));
		if(!fgets((line + len), (*space_available - len), fstream))
			break;
		len = strlen(line);
		}

	/* Remove trailing newlines, carriage returns, spaces, and tabs. */
	while((--len >= 0) && gu_ascii_isspace(line[len]))
		line[len] = '\0';

	return line;
	}

/* end of file */
