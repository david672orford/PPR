/*
** mouse:~ppr/src/interfaces/throw.c
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
** Last modified 10 April 2001.
*/

#include "before_system.h"
#include <stdarg.h>
#include "gu.h"
#include "global_defines.h"
#include "libppr_int.h"
#include "interface.h"

#if 0
#define DODEBUG(a) int_debug a
#else
#define DODEBUG(a)
#endif

/*
** This is a special handler for library exceptions.  It overrides
** the version of libppr_throw() in ../libppr.a.
*/
void libppr_throw(int exception_type, const char exception_function[], const char format[], ...)
	{
	va_list va;
	DODEBUG(("libppr_throw(exception_type=%d, function[]=\"%s\", format[]=\"%s\", ...)", exception_type, function, format));

	alert(int_cmdline.printer, TRUE, "libppr exception in %s:", exception_function);
	va_start(va, format);
	valert(int_cmdline.printer, FALSE, format, va);
	va_end(va);

	if(exception_type == EXCEPTION_STARVED)
		int_exit(EXIT_PRNERR);
	else
		int_exit(EXIT_PRNERR_NORETRY);
	} /* end of libppr_throw() */

/* end of file */

