/*
** mouse:~ppr/src/libppr/cs2enc.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 10 July 1998.
*/

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "libppr_font.h"

/*
** This function takes a character set name and finds a PostScript
** encoding for it.  It fills in the supplied structure with the
** name of that encoding and other information about it.
*/
int charset_to_encoding(const char charset[], struct ENCODING_INFO *encinfo)
	{
	const char filename[] = CHARSETSCONF;
	FILE *file;
	int linenum = 0;
	const char *f1;

	/* Try to open a file with the mappings. */
	if(!(file = fopen(filename, "r")))
		{
		error(_("Can't open \"%s\", errno=%d (%s)"), filename, errno, gu_strerror(errno));
		return -1;
		}

	while(fgets(encinfo->line, sizeof(encinfo->line), file))
		{
		linenum++;

		/* Skip comment lines and blank lines: */
		switch(encinfo->line[0])
			{
			case '#':
			case ';':
			case '\n':
			case '\r':
				continue;
			case ' ':
			case '\t':
				if(strspn(encinfo->line, " \t\r\n") == strlen(encinfo->line))
					continue;
			}

		if((f1 = strtok(encinfo->line, ":")))
			{
			if(gu_strcasecmp(f1, charset) == 0)
				{
				char *f2, *f3;
				int answer;

				fclose(file);

				if( ! (f2 = strtok(NULL, ":")) || ! (f3 = strtok(NULL, ":")) )
					{
					error(_("Too few fields in \"%s\" line %d"), filename, linenum);
					return -1;
					}

				if((answer = gu_torf(f3)) == ANSWER_UNKNOWN)
					{
					error(_("Non-boolean value in 3rd field of \"%s\" line %d"), filename, linenum);
					return -1;
					}

				encinfo->encoding = f2;
				encinfo->encoding_ascii_compatible = answer ? TRUE : FALSE;
				return 0;
				}

			}

		}

	/* Did the loop end because of a read error? */
	if(ferror(file))
		error(_("Error reading \"%s\", errno=%d (%s)"), filename, errno, gu_strerror(errno));

	fclose(file);

	/* If we get here, there was an error of some sort. */
	return -1;
	}

/* end of file */
