/*
** mouse:~ppr/src/ipp/ipp_utils.c
** Copyright 1995--2002, Trinity College Computing Center.
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
** Last modified 18 February 2002.
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
