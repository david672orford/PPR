/*
** mouse:~ppr/src/libppr/foptions.c
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
** Last modified 10 February 1999.
*/

#include "before_system.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"


/*
** This routine complements those in options.c.  It is used
** when parsing filter options.
**
** When a filter option error is detected, this routine is used
** to, print up to 70 characters of the
** error in context with a caret underneath to show exactly
** where the error is.  Then print the error message under that.
**
** We adopt some elaborate methods because a filter options
** string will very often exceed the terminal line width.
** We print the segement which options_get_one() has
** indicated by setting options_error_context_index.  If that
** index is not zero we print "... " first.  If the part of
** the string after the point indicated by the index is longer than
** 70 characters we print just the first 70 and append " ...".
*/
void filter_options_error(int exit_code, struct OPTIONS_STATE *o, const char *format, ...)
	{
	int caret_indent;
	va_list va;

	fputc('\n', stderr);
	fputs(_("Error in filter options:\n"), stderr);

	/* print a segment of the erronious filter options string */
	fprintf(stderr, "%s%.70s%s\n",
				o->index_of_prev_name > 0 ? "... " : "",
				o->options + o->index_of_prev_name,
				(strlen(o->options + o->index_of_prev_name) > 70 ? " ..." : "") );

	/* print the caret under it at the right place */
	caret_indent = 0;
	if(o->index_of_prev_name > 0) caret_indent += 4;
	caret_indent += (o->index - o->index_of_prev_name);
	while(caret_indent--)
		fputc(' ', stderr);
	fputs("^\n", stderr);

	/* print the error message */
	va_start(va, format);
	vfprintf(stderr, format, va);
	va_end(va);

	fputs("\n\n", stderr);

	/* Exit, using the code we were told to use. */
	exit(exit_code);
	} /* end of filter_options_error() */

/* end of file */
