/*
** mouse:~ppr/src/ipp/ipp_utils.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 21 November 2000.
*/

#include "before_system.h"
#include <stdio.h>
#include <stdarg.h>
#include <netinet/in.h>
#include <stdint.h>
#include "gu.h"
#include "global_defines.h"

#include "ipp_utils.h"

/* Get an IPP signed byte. */
int ipp_gsb(void *p)
    {
    return (int)*(int8_t *)p;
    }

/* Get an IPP signed short. */
int ipp_gss(void *p)
    {
    return ntohs(*(int16_t *)p);
    }

/* Get an IPP signed integer. */
int ipp_gsi(void *p)
    {
    return ntohl(*(int32_t *)p);
    }

/* Set an IPP signed byte. */
void ipp_ssb(void *p, int val)
    {
    *(int8_t *)p = val;
    }

/* Set an IPP signed short. */
void ipp_sss(void *p, int val)
    {
    *(int16_t *)p = htons(val);
    }

/* Set an IPP signed integer. */
void ipp_ssi(void *p, int val)
    {
    *(int32_t *)p = htonl(val);
    }

/* Send a debug message to the HTTP server's error log. */
void debug(const char message[], ...)
    {
    va_list va;
    va_start(va, message);
    fputs("ipp: ", stderr);
    vfprintf(stderr, message, va);
    fputc('\n', stderr);
    va_end(va);
    } /* end of debug() */

/* end of file */
