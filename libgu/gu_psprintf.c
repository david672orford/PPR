/*
** mouse:~ppr/src/libgu/gu_psprintf.c
** Copyright 1995--2003, Trinity College Computing Center.
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
** Last modified 6 March 2003.
*/

#include "config.h"
#include "gu.h"
#include "global_defines.h"

/** use format string to send PostScript code to stdout
 *
 * This is a special printf()-like function based on printer_printf() in 
 * ../pprdrv/pprdrv_buf.c.  It can insert strings (%s), decimal numbers (%d),
 * floating point numbers (%f), 8-bit octal numbers (%o), and characters (%c).
 * The number of digits to print after the decimal point is chosen 
 * automatically.  Now width or precision can be specified.
*/
void gu_psprintf(const char *format, ...)
	{
	const char function[] = "gu_psprintf";
	va_list va;
	const char *sptr;
	int n;
	double dn;
	char nstr[25];

	va_start(va, format);

	while(*format)
		{
		if(*format == '%')
			{
			format++;					/* move past the '%' */
			switch(*(format++))
				{
				case '%':				/* literal '%' */
					fputc('%', stdout);
					break;
				case 'c':				/* a character */
					n = va_arg(va, int);
					fputc(n, stdout);
					break;
				case 'd':				/* a decimal value */
					n = va_arg(va, int);
					snprintf(nstr, sizeof(nstr), "%d", n);
					fputs(nstr, stdout);
					break;
				case 'f':				/* a double */
					dn = va_arg(va, double);
					sptr = gu_dtostr(dn);
					fputs(sptr, stdout);
					break;
				case 'o':				/* an octal char value */
					n = va_arg(va, int);
					snprintf(nstr, sizeof(nstr), "%.3o", n);	/* (We assume three digits */
					fputs(nstr, stdout);						/* because PostScript needs 3.) */
					break;
				case 's':				/* a string */
					sptr = va_arg(va, char *);
					fputs(sptr, stdout);
					break;
				default:
					gu_Throw("%s(): illegal format spec: %s", function, format-2);
					break;
				}
			}
		else
			{
			fputc(*(format++), stdout);
			}
		}

	va_end(va);
	} /* end of gu_psprintf() */

/* end of file */
