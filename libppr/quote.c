/*
** mouse.trincoll.edu:~ppr/src/libppr/quote.c
** Copyright 1995, 1996, 1997, 1998, Trinity College Computing Center.
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


/*
** Quote a PostScript string if necessary.
*/
const char *quote(const char *string)
	{
	static char temp[256];

	/*
	** If the string contains spaces or tabs or it is all
	** digits and so looks like a number, quote it.
	** The second clause accidentally causes empty strings
	** to be quoted which is what we want anyway.
	*/
	if( (strpbrk(string," \t") != (char *)NULL)
			|| (strspn(string,"0123456789")==strlen(string)) )
		{
		temp[0]='(';
		strcpy(&temp[1],string);
		strcat(temp,")");

		return temp;
		}

	/*
	** Otherwise, just return a pointer to it
	** so that the user may print it.
	*/
	else
		{
		return string;
		}

	} /* end of quote() */

/* end of file */
