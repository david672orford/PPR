/*
** mouse:~ppr/src/libgu/gu_utf8_printf.c
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
** Last modified 1 September 2005.
*/

#include "config.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"

/* This function is similiar to fputwc() */
static wchar_t gu_fputwc(wchar_t wc, FILE *f)
	{
	char conv[MB_CUR_MAX];
	int len;
	if((len = wctomb(conv, wc)) == -1)	/* if invalid wchar */
		len = wctomb(conv, (wchar_t)'?');
	fwrite(conv, sizeof(char), len, f);
	return wc;
	}

int gu_utf8_vfprintf(FILE *f, const char *format, va_list args)
	{
	const char function[] = "gu_utf8_vfprintf";
	int char_count = 0;
	wchar_t wc;
	const char *formatp;
	for(formatp = format; (wc = gu_utf8_sgetwc(&formatp)); )
		{
		if(wc == '%')
			{
			gu_boolean flag_left_justify = FALSE;
			gu_boolean flag_show_sign = FALSE;
			gu_boolean flag_space_pad = FALSE;
			gu_boolean flag_hash = FALSE;
			gu_boolean flag_leading_zeros = FALSE;
			gu_boolean flag_h = FALSE;
			gu_boolean flag_l = FALSE;
			gu_boolean flag_L = FALSE;
			int width = 0;
			int precision = 0;

			while((wc = gu_utf8_sgetwc(&formatp)) && strchr("-+ #0", wc))
				{
				switch(wc)
					{
					case '-':
						flag_left_justify = TRUE;
						break;
					case '+':
						flag_show_sign = TRUE;
						break;
					case ' ':
						flag_space_pad = TRUE;
						break;
					case '#':
						flag_hash = TRUE;
						break;
					case '0':
						flag_leading_zeros = TRUE;
						break;
					}
				}
		
			while(gu_ascii_isdigit(wc))
				{
				width *= 10;
				width += gu_ascii_digit_value(wc);		
				wc = gu_utf8_sgetwc(&formatp);
				}

			if(wc == '.')
				{
				wc = gu_utf8_sgetwc(&formatp);
				while(gu_ascii_isdigit(wc))
					{
					precision *= 10;
					precision += gu_ascii_digit_value(wc);		
					wc = gu_utf8_sgetwc(&formatp);
					}
				}

			while(strchr("hlL", wc))
				{
				switch(wc)
					{
					case 'h':
						flag_h = TRUE;
						break;
					case 'l':
						flag_l = TRUE;
						break;
					case 'L':
						flag_L = TRUE;
						break;
					}
				wc = gu_utf8_sgetwc(&formatp);
				}
			
			/* this must be the type field */	
			switch(wc)
				{
				case '\0':		/* ran off end of format string! */
					break;
				case '%':
					gu_fputwc('%', f);
					break;
				case 'd':
					break;
				case 's':
					break;
				default:
					gu_Throw("%s(): unrecognized format '%c' in \"%s\"", function, wc, format);
					break;
				}
			}
		else
			{
			gu_fputwc(wc, f);
			}
		}
	return char_count;
	} /* gu_utf8_vfprintf() */

int gu_utf8_printf(const char *format, ...)
	{
	va_list ap;
	int ret;
	va_start(ap, format);
	ret = gu_utf8_vfprintf(stdout, format, ap);
	va_end(ap);
	return ret;
	}

int gu_utf8_fprintf(FILE *f, const char *format, ...)
	{
	va_list ap;
	int ret;
	va_start(ap, format);
	ret = gu_utf8_vfprintf(f, format, ap);
	va_end(ap);
	return ret;
	}

/* gcc -DTEST -I../include -Wall -o gu_utf8_printf gu_utf8_printf.c ../libgu.a */
#ifdef TEST
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
int main(int argc, char *argv[])
	{
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	gu_utf8_printf("Hello, world!\n");
	gu_utf8_printf("Hello, %s!\n", 1, "David");
	return 0;
	}
#endif

/* end of file */
