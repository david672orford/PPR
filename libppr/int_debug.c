/*
** mouse:~ppr/src/libppr_int/int_debug.c
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
** Last modified 24 June 1999.
*/

#include "before_system.h"
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include "gu.h"
#include "global_defines.h"

#include "libppr_int.h"

/*
** Write lines to the debug file.
*/
void int_debug(const char format[], ... )
    {
    char fname[MAX_PPR_PATH];
    va_list va;
    FILE *file;

    ppr_fnamef(fname, "%s/interface_%s", LOGDIR, int_cmdline.int_basename);
    if((file = fopen(fname, "a")) != NULL)
	{
	fprintf(file, "DEBUG: (%ld) ", (long)getpid());
	va_start(va, format);
	vfprintf(file, format, va);
	va_end(va);
	fprintf(file, "\n");
	fclose(file);
	}
    } /* end of int_debug() */

/* end of file */

