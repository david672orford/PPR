/*
** mouse:~ppr/src/libppr/pagesize.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 10 July 1999.
*/

#include "before_system.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "gu.h"
#include "global_defines.h"


/*
** Return the length and width of a page in PostScript units
** as well as a flag indicating if it is an envelope size.
** Return -1 if we fail to find a match, return 0 if we find one.
**
** This routine is meant to interpret the arguments to the
** DSC comments which contain page size names.  This is usesful
** for deducing the media requirements of documents which do
** not contain "%%Media:" comments but do have "%%PageSize:"
** comments.
**
** This routines is also used by some of the filters to parse
** their "pagesize=" options.
**
** The "keyword" is the name to look up.  The "corrected_keyword" is the page
** size name converted to standard form and capitalization.  All of the
** others are pointers to things that should be set if they aren't
** NULL pointers.
*/
int pagesize(const char keyword[], char **corrected_keyword, double *width, double *length, gu_boolean *envelope)
	{
	const char filename[] = PAGESIZES_CONF;
	FILE *f;
	char *line = NULL;
	int line_space = 80;
	int linenum = 0;
	char *ptr;
	char *f1, *f2, *f3, *f4;
	int retval = -1;

	if(!(f = fopen(filename, "r")))
		{
		error("can't open \"%s\", errno=%d (%s)", filename, errno, gu_strerror(errno));
		return -1;
		}

	while((line = gu_getline(line, &line_space, f)))
		{
		linenum++;
		if(line[0] == '#' || line[0] == ';' || line[0] == '\0') continue;

		ptr = line;

		if(!(f1 = gu_strsep(&ptr, ":"))
				|| !(f2 = gu_strsep(&ptr, ":"))
				|| !(f3 = gu_strsep(&ptr, ":"))
				|| !(f4 = gu_strsep(&ptr, ":")))
			{
			error("too few fields in \"%s\" line %d", filename, linenum);
			continue;
			}


		if((corrected_keyword ? gu_strcasecmp(keyword, f1) : strcmp(keyword, f1)) == 0)
			{
			double f2_double, f3_double;

			if((f2_double = strtod(f2, NULL)) <= 0.0 || (f3_double = strtod(f3, NULL)) <= 0.0)
				{
				error("unreasonable width value in \"%s\" line %d", filename, linenum);
				continue;
				}

			if(corrected_keyword) *corrected_keyword = gu_strdup(f1);
			if(width) *width = f2_double;
			if(length) *length = f3_double;
			if(envelope) *envelope = atoi(f4) ? TRUE : FALSE;

			gu_free(line);
			retval = 0;
			break;
			}

		}

	if(ferror(f))
		error("error reading \"%s\", errno=%d (%s)", filename, errno, gu_strerror(errno));

	fclose(f);

	return retval;
	} /* end of pagesize() */

/* end of file */

