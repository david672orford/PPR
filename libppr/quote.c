/*
** mouse:~ppr/src/libppr/quote.c
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
** Last modified 9 September 2005.
*/

#include "config.h"
#include <string.h>
#include "gu.h"
#include "global_defines.h"

/*
** Quote a PostScript string if necessary.
** This is generally used for inserting short resource names into DSC comments.
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
	if(strpbrk(string," \t") || strspn(string,"0123456789") == strlen(string))
		{
		snprintf(temp, sizeof(temp), "(%s)", string);
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
