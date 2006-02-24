/*
** mouse:~ppr/src/libgu/gu_utf8_put.c
** Copyright 1995--2006, Trinity College Computing Center.
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
** Last modified 23 February 2006.
*/

/*! \file
    \brief fputs() and friends which accept UTF-8 input
*/

#include "config.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"

/** Write a wchar_t to a file as a multibyte sequence in the current locale
 * This function is similiar to Standard C's fputwc() 
 */
wchar_t gu_fputwc(wchar_t wc, FILE *f)
	{
	char conv[MB_CUR_MAX];
	int len;
	if((len = wctomb(conv, wc)) == -1)	/* if invalid wchar */
		len = wctomb(conv, (wchar_t)'?');
	/*printf("(wc=%d,len=%d)", wc, len);*/
	fwrite(conv, sizeof(char), len, f);
	return wc;
	}

/** Write a UTF-8 string to a file as a multibyte string in the current locale
 */
int gu_utf8_fputs(const char *string, FILE *f)
	{
	const char *p = string;
	wchar_t wc;
	int count = 0;
	while((wc = gu_utf8_sgetwc(&p)))
		{
		gu_fputwc(wc, f);
		count++;
		}
	return count;
	}

/** Write a wchar_t to stdout as a multibyte sequence
 */
wchar_t gu_putwc(wchar_t wc)
	{
	return gu_fputwc(wc, stdout);
	}

/** Write a UTF-8 string to stdout as a multibyte sequence in the current locale
 */
int gu_utf8_puts(const char *string)
	{
	return gu_utf8_fputs(string, stdout);
	}

/** Write a UTF-8 string to stdout and add a newline
 */
int gu_utf8_putline(const char *string)
	{
	int count = gu_utf8_fputs(string, stdout);
	gu_fputwc('\n', stdout);
	count++;
	return count;
	}

/* end of file */
