/*
** mouse:~ppr/src/libppr/enc2font.c
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
** Last modified 31 October 2003.
*/

#include "before_system.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "gu.h"
#include "global_defines.h"
#include "libppr_font.h"

/*
** This function searches fonts.conf to find a find that will provide the characters
** for a particular encoding.  The font family, weight, and slant are also
** specified.
*/
int encoding_to_font(const char encoding[], const char fontfamily[], const char fontweight[], const char fontslant[], const char fontwidth[], struct FONT_INFO *fontinfo)
	{
	const char *filename = FONTSCONF;
	FILE *file;
	char *p;
	char *f1, *f2, *f3, *f4, *f5, *f6, *f7, *f8;
	int linenum = 0;
	int retval = -1;

	if(!(file = fopen(filename, "r")))
		{
		error("Can't open \"%s\", errno=%d (%s)", filename, errno, gu_strerror(errno));
		return -1;
		}

	while(fgets(fontinfo->line, sizeof(fontinfo->line), file))
		{
		linenum++;

		if(fontinfo->line[0] == '#' || fontinfo->line[0] == ';')		/* comments */
		   continue;

		/* Trim whitespace of the end of the line.  If there is nothing left,
		 * skip this line.  This was once in a call to a gu_ fuction, but now
		 * that all the rest of the code has switched over to gu_getline(), 
		 * they didn't need it anymore.
		 */
			{
			int len = strlen(fontinfo->line);
			while(--len >= 0 && isspace(fontinfo->line[len]))
				fontinfo->line[len] = '\0';
			if(len == 0)
				continue;
			}

		/* Use gu_strsep() to split the line into eight fields separated by colons. */
		p = fontinfo->line;
		if(!(f1 = gu_strsep(&p, ":"))
				|| !(f2 = gu_strsep(&p, ":"))
				|| !(f3 = gu_strsep(&p, ":"))
				|| !(f4 = gu_strsep(&p, ":"))
				|| !(f5 = gu_strsep(&p, ":"))
				|| !(f6 = gu_strsep(&p, ":"))
				|| !(f7 = gu_strsep(&p, ":"))
				|| !(f8 = gu_strsep(&p, ":")) )
			{
			error("Not enough fields in \"%s\" line %d", filename, linenum);
			/* error("f2=%p, f3=%p, f4=%p, f5=%p, f6=%p, f7=%p, f8=%p", f2, f3, f4, f5, f6, f7, f8); */
			continue;
			}

		if(strcmp(f1, encoding) == 0
				&& strcmp(fontfamily, f2) == 0
				&& strcmp(fontweight, f3) == 0
				&& strcmp(fontslant, f4) == 0
				&& strcmp(fontwidth, f5) == 0)
			{
			fontinfo->font_family = f2;
			fontinfo->font_weight = f3;
			fontinfo->font_slant = f4;
			fontinfo->font_width = f5;
			fontinfo->font_psname = f6;
			fontinfo->font_encoding = f7;
			fontinfo->ascii_subst_font = f8[0] ? f8 : NULL;
			retval = 0;
			break;
			}
		} /* line reading loop */

	if(ferror(file))
		error("Error reading \"%s\", errno=%d (%s)", filename, errno, gu_strerror(errno));

	fclose(file);
	return retval;
	}

/* end of file */
