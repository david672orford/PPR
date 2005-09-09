/*
** mouse:~ppr/src/libgu/gu_snprintf.c
** Copyright 1995--2005, Trinity College Computing Center.
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
** Last modified 13 January 2005.
*/

/*
** This module is always compiled into the PPR library libgu.  That way,
** code which requires C99 sematics can explicitly use the gu_ versions
** of the funtions.  On systems on which snprintf(), vsnprintf(), asprintf(),
** or vasprintf() are completely missing, gu.h defines macros to equate
** them with gu_snprintf() and gu_vsnprintf(), etc.
*/

#include "config.h"

/* We have already included our config.h */
#define NO_CONFIG_H

/* C99 provides va_copy() */
#if __STDC_VERSION__ + 0 >= 199900L
#define HAVE_VA_COPY 1
#elif defined(__GNUC__)
#define HAVE___VA_COPY 1
#endif

/* Include the prototypes in order to verify consistancy. */
#include "gu.h"

/* All systems on which PPR compiles have these headers.  However,
   we will include them early to avoid problems with our redefinitions below. */
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdlib.h>
#undef HAVE_STRING_H
#undef HAVE_CTYPE_H
#undef HAVE_STDLIB_H

/* We are going to pretent that we have none of these. */
#undef HAVE_SNPRINTF
#undef HAVE_VSNPRINTF
#undef HAVE_ASPRINTF
#undef HAVE_VASPRINTF

/* Rename these functions so as not to clash with libc. */
#define smb_snprintf gu_snprintf
#define smb_vsnprintf gu_vsnprintf
#define asprintf gu_asprintf
#define vasprintf gu_vasprintf

/* Hook memory allocation. */
#define malloc(a) gu_alloc(a, sizeof(char))
#define free(a) gu_free(a)

#include "../nonppr_misc/snprintf/snprintf.c"

/* end of file */

