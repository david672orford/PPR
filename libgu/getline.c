/*
** mouse:~ppr/src/libppr/getline.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 22 November 2000.
*/

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include "gu.h"

/*
** This function is used to read configuration file lines.  It avoids
** the problem of buffers that are too small.  The parameter "line"
** points to a buffer in gu_alloc()ed memory.  On the first call this
** may be a NULL pointer.  If it is too small, a new, longer one will
** be obtained and "space_available" will be updated.  The new buffer
** is returned.  The caller should initialy set "line_available" to
** a little bigger than the LIKELY line length.  Notice that on EOF
** the line is automatically freed.  If you don't read to EOF you
** must manually free the line.  The file is not closed.
**
** Use the function like this:
**
** {
** int line_available = 80;
** char *line = NULL;
**
** while((line = gu_getline(line, &line_available, stdin))
**   {
**
**   }
** }
*/
char *gu_getline(char *line, int *space_available, FILE *fstream)
    {
    int len;

    if(*space_available < 1)	/* sanity check */
    	return NULL;

    if(!line)			/* if not allocated yet, */
    	line = (char*)gu_alloc(*space_available, sizeof(char));

    if(!fgets(line, *space_available, fstream))
    	{
	int e = errno;
    	gu_free(line);
    	errno = e;
    	return NULL;
    	}

    len = strlen(line);

    while(len == (*space_available - 1) && line[len - 1] != '\n')
	{
	*space_available *= 2;
	line = (char*)ppr_realloc(line, *space_available, sizeof(char));
	if(!fgets((line + len), (*space_available - len), fstream)) break;
	len = strlen(line);
	}

    /* Remove trailing newlines, carriage returns, spaces, and tabs. */
    while((--len >= 0) && isspace(line[len]))
    	line[len] = '\0';

    return line;
    }

/* end of file */

