/*
** mouse:~ppr/src/libppr/cs2enc.c
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
** Last modified 1 March 2005.
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

				fclose(file);

				if( !(f2 = strtok(NULL, ":")) || !(f3 = strtok(NULL, ":")) )
					{
					error(_("Too few fields in \"%s\" line %d"), filename, linenum);
					return -1;
					}

				if(gu_torf_setBOOL(&(encinfo->encoding_ascii_compatible),f3) == -1)
					{
					error(_("Non-boolean value in 3rd field of \"%s\" line %d"), filename, linenum);
					return -1;
					}

				encinfo->encoding = f2;
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
