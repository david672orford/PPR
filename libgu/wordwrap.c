/*
** mouse:~ppr/src/libgu/wordwrap.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 22 November 2000.
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

	*di = (char)NULL;
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
