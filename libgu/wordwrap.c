/*
** mouse:~ppr/src/libgu/wordwrap.c
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
** Last modified 14 May 2003.
*/

/*
** This function will re-line-wrap a string so that it has lines whose
** length do not exceed a certain limit.
*/

#include "before_system.h"
#include <string.h>
#include "gu.h"

void gu_wordwrap(char *string, int width)
	{
	const char *si;
	char *di;
	int curlen;									/* length of the current line */
	int len;
	int spacelen;								/* number of spaces at end of line */

	if(width <= 0)
		return;

	for(si=di=string,spacelen=curlen=0; *si; spacelen=0)
		{
		while( *si == ' ' || *si == '\n' )		/* Copy spaces to the destination */
			{									/* if they are not at the */
			if( curlen > 0 )					/* begining of the line. */
				{
				*(di++) = ' ';
				curlen++;
				spacelen++;
				}
			si++;
			}

		len = strcspn(si, " \n");				/* length of next word */

		if( (curlen+len) > width )				/* If it won't fit, */
			{
			di -= spacelen;						/* take back the preceeding spaces, */
			*(di++) = '\n';						/* and replace them with a line feed. */
			curlen = 0;
			}

		strncpy(di, si, len);					/* copy the word */
		si += len;
		di += len;
		curlen += len;
		}

	*di = '\0';
	} /* end of gu_wordwrap() */

#ifdef TEST
int main(int argc, char *argv[])
		{
		char old[512];
		char new[512];

		strcpy(old,"Now is the time for all good men to come to the aid of the party.\n");
		strcat(old,"The quick brown fox jumped over the lazy yellow dogs.");

		gu_wordwrap(new,old,20);

		printf("%s\n",new);

		return 0;
		}
#endif

/* end of file */
