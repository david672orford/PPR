/*
** mouse:~ppr/src/libgu/gu_snprintfcat.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 21 March 2001.
*/

#include "before_system.h"
#include <stdarg.h>
#include <string.h>
#include "gu.h"

int gu_snprintfcat(char *buffer, size_t max, const char *format, ...)
    {
    va_list va;
    size_t len = strlen(buffer);
    int ret;
    max -= len;
    buffer += len;
    va_start(va, format);
    ret = vsnprintf(buffer, max, format, va);
    va_end(va);
    return ret;
    }

/* end of file */
