/*
** mouse:~ppr/src/libgu/gu_throw.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 22 November 2000.
*/

#include "before_system.h"
#include <stdarg.h>
#include <stdlib.h>
#include "gu.h"

void libppr_throw(int exception_type, const char function[], const char format[], ...)
    {
    va_list va;
    fprintf(stderr, "Fatal exception of type %d in %s(): ", exception_type, function);
    va_start(va, format);
    vfprintf(stderr, format, va);
    va_end(va);
    fputc('\n', stderr);
    exit(255);
    } /* end of libppr_throw() */

/* end of file */

