/*
** mouse:~ppr/src/libgu/wordwrap.c
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

/*! \file
	\brief word wrap lines
*/

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include "gu.h"

/** word wrap string
 *
 * This function will re-line-wrap a string so that it has lines whose
 * length do not exceed a certain limit.  The original string is
 * modified.
 */
int gu_wordwrap(char *string, int width)
	{
	const char *si;
	char *di;
	int curlen;			/* length of the current line */
	int wordlen;
	int spacelen;		/* number of spaces at end of line */

	if(width <= 0)		/* if width is impossible, leave string unchanged */
		return 0;

	for(si=di=string,spacelen=curlen=0; *si; spacelen=0)
		{
		while(*si == ' ' || *si == '\n')	/* Copy spaces to the destination */
			{								/* if they are not at the */
			if(curlen > 0)					/* begining of the line. */
				{
				*(di++) = ' ';
				curlen++;
				spacelen++;
				}
			si++;
			}

		wordlen = strcspn(si, " \n");	/* length of next word */

		/* If it won't fit, take back the space(s) and replace 
		 * it/them with a linefeed.
		 */
		if(curlen > 0 && (curlen+wordlen) > width)
			{
			di -= spacelen;
			*(di++) = '\n';
			curlen = spacelen = 0;
			}

		/* copy the word */
		memmove(di, si, wordlen);
		si += wordlen;
		di += wordlen;
		curlen += wordlen;
		}

	if(di > string && *(di-1) == ' ')
		*(di-1) = '\n';

	*di = '\0';
	return (di - string);
	} /* end of gu_wordwrap() */

static int gu_wrap_vfprintf(FILE *file, const char format[], va_list ap)
	{
	int ret;
	char *ptr;
	int width;

	if((ptr = getenv("COLUMNS")))
		width = atoi(ptr);
	else
		width = 80;

	ret = gu_vsnprintf(NULL, 0, format, ap);
	ptr = (char*)gu_alloc(ret+1, sizeof(char));
	gu_vsnprintf(ptr, ret+1, format, ap);

	ret = gu_wordwrap(ptr, width);
	fputs(ptr, file);
	
	gu_free(ptr);

	return ret;
	} /* gu_wrap_printf() */

/** Print a word-wrapped line on stdout.
*/
int gu_wrap_printf(const char format[], ...)
	{
	va_list ap;
	int ret;
	va_start(ap, format);
	ret = gu_wrap_vfprintf(stdout, format, ap);
	va_end(ap);
	return ret;
	}

/** Print a word-wrapped line on stderr.
 */
int gu_wrap_eprintf(const char format[], ...)
	{
	va_list ap;
	int ret;
	va_start(ap, format);
	ret = gu_wrap_vfprintf(stderr, format, ap);
	va_end(ap);
	return ret;
	}

/* gcc -Wall -I../include -DTEST -o wordwrap wordwrap.c ../libgu.a */
#ifdef TEST
int main(int argc, char *argv[])
		{
		char old[512];

		strlcpy(old, "Now is the time for    all good men to come to the aid of the party.\n", sizeof(old));
		strlcat(old, "The quick brown fox jumped over the lazy yellow dogs.", sizeof(old));

		gu_wordwrap(old, 20);

		printf("%s\n", old);

		return 0;
		}
#endif

/* end of file */
