/*
** mouse:~ppr/src/libppr/enc2font.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 30 March 2001.
*/

#include "before_system.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "gu.h"
#include "global_defines.h"
#include "libppr_font.h"

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

	if(fontinfo->line[0] == '#' || fontinfo->line[0] == ';')	/* comments */
	   continue;

	gu_trim_whitespace_right(fontinfo->line);

	if(strlen(fontinfo->line) == 0)					/* blank lines */
	   continue;

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
