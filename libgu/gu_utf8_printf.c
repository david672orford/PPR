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
** Last modified 28 September 2005.
*/

#include "config.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"

/* This function is similiar to Standard C's fputwc() */
static wchar_t gu_fputwc(wchar_t wc, FILE *f)
	{
	char conv[MB_CUR_MAX];
	int len;
	if((len = wctomb(conv, wc)) == -1)	/* if invalid wchar */
		len = wctomb(conv, (wchar_t)'?');
	/*printf("(wc=%d,len=%d)", wc, len);*/
	fwrite(conv, sizeof(char), len, f);
	return wc;
	}

/* How many character columns will a UTF-8 character consume.
 * The answer will be 0, 1, or 2.
 */
static int gu_wcwidth(wchar_t wc)
	{
	return 1;	/* real implementation needed */
	}

/* How many character columns will a UTF-8 string consume? */
static int gu_utf8_wswidth(const char *string)
	{
	wchar_t wc;
	int len = 0;
	while((wc = gu_utf8_sgetwc(&string)))
		{
		len += gu_wcwidth(wc);
		}
	return len;
	}

#define MAX_FSPECS 10

union VALUE {
	int integer;
	char *character_pointer;
	};

enum VALUE_TYPE {
	VALUE_TYPE_INTEGER,
	VALUE_TYPE_CHARACTER_POINTER
	};

struct FSPEC {
	gu_boolean flag_left_justify;
	gu_boolean flag_show_sign;
	gu_boolean flag_blank;
	gu_boolean flag_leading_zeros;
	int width;
	int precision;
	enum VALUE_TYPE value_type;
	union VALUE value;
	int format_length;
	};

int gu_utf8_vfprintf(FILE *f, const char *format, va_list args)
	{
	const char function[] = "gu_utf8_vfprintf";
	int char_count = 0;
	wchar_t wc;
	const char *formatp;
	int iii;
	struct FSPEC fspecs[MAX_FSPECS];
	int arguments[MAX_FSPECS];

	for(iii = 0; iii < MAX_FSPECS; iii++)
		arguments[iii] = -1;

	/* First pass, parse all of the format specifiers and note which 
	 * argument each requires.
	 */
	for(formatp = format, iii = 0; (wc = gu_utf8_sgetwc(&formatp)); )
		{
		if(wc == '%')
			{
			const char *formatp_save = formatp;
			int position;
			fspecs[iii].flag_left_justify = FALSE;
			fspecs[iii].flag_show_sign = FALSE;
			fspecs[iii].flag_blank = FALSE;
			fspecs[iii].flag_leading_zeros = FALSE;
			fspecs[iii].width = 0;
			fspecs[iii].precision = -1;		/* unspecified */
			position = iii;

			/* Is the argument index specified? */
			{
			int len;
			if((len = strspn(formatp, "0123456789")) > 0 && formatp[len] == '$')
				{
				position = atoi(formatp);
				position--;		/* convert from 1 based to 0 based */
				len++;
				formatp += len;
				}
			}
		
			if(position >= MAX_FSPECS)
				gu_Throw("%s(): argument index too high", function);
			if(arguments[position] != -1)
				gu_Throw("%s(): double use of argument (by specs %d and %d)", function, arguments[position]+1, iii+1);
			arguments[position] = iii;

			/* Look for formatting modifiers. */			
			while((wc = gu_utf8_sgetwc(&formatp)) && strchr("-+ #0", wc))
				{
				switch(wc)
					{
					case '-':
						fspecs[iii].flag_left_justify = TRUE;
						break;
					case '+':
						fspecs[iii].flag_show_sign = TRUE;
						break;
					case ' ':
						fspecs[iii].flag_blank = TRUE;
						break;
					case '0':
						fspecs[iii].flag_leading_zeros = TRUE;
						break;
					}
				}
	
			/* Look for field width */	
			while(gu_ascii_isdigit(wc))
				{
				fspecs[iii].width *= 10;
				fspecs[iii].width += gu_ascii_digit_value(wc);		
				wc = gu_utf8_sgetwc(&formatp);
				}

			/* Look for precision */
			if(wc == '.')
				{
				fspecs[iii].precision = 0;
				wc = gu_utf8_sgetwc(&formatp);
				while(gu_ascii_isdigit(wc))
					{
					fspecs[iii].precision *= 10;
					fspecs[iii].precision += gu_ascii_digit_value(wc);		
					wc = gu_utf8_sgetwc(&formatp);
					}
				}

			/* Look for parameter size modifiers. */
			#if 0
			while(strchr("hlL", wc))
				{
				switch(wc)
					{
					case 'h':
						fspecs[iii].flag_h = TRUE;
						break;
					case 'l':
						fspecs[iii].flag_l = TRUE;
						break;
					case 'L':
						fspecs[iii].flag_L = TRUE;
						break;
					}
				wc = gu_utf8_sgetwc(&formatp);
				}
			#endif
			
			/* this must be the type field */	
			switch(wc)
				{
				case '\0':		/* ran off end of format string! */
					break;
				case '%':
					break;
				case 'd':
					fspecs[iii].value_type = VALUE_TYPE_INTEGER;
					break;
				case 's':
					fspecs[iii].value_type = VALUE_TYPE_CHARACTER_POINTER;
					break;
				default:
					gu_Throw("%s(): unrecognized format '%c' in \"%s\"", function, wc, format);
					break;
				}

			/* How many characters does the format specifier consume? */
			fspecs[iii].format_length = (formatp - formatp_save);

			iii++;
			}
		}

	/* Load the required arguments. */
	{
	int param_count = iii;
	for(iii = 0; iii < param_count; iii++)
		{
		if(arguments[iii] == -1)
			gu_Throw("%s(): parameter gap", function);
		switch(fspecs[arguments[iii]].value_type)
			{
			case VALUE_TYPE_INTEGER:
				fspecs[arguments[iii]].value.integer = va_arg(args, int);
				break;
			case VALUE_TYPE_CHARACTER_POINTER:
				fspecs[arguments[iii]].value.character_pointer = va_arg(args, char*);
				break;
			}
		}
	}

	/* Second pass, produce output. */	
	for(formatp = format, iii = 0; (wc = gu_utf8_sgetwc(&formatp)); )
		{
		if(wc == '%')
			{
			/* Skip up to the type field */
			{
			int y;
			for(y = 0; y < fspecs[iii].format_length; y++)
				 wc = gu_utf8_sgetwc(&formatp);
			}

			switch(wc)
				{
				case '%':
					gu_fputwc('%', f);
					break;

				/* Decimal integer */
				case 'd':
					{
					char buffer[32];
					int bi;
					int n = fspecs[iii].value.integer;
					gu_boolean negative = FALSE;
					int width_left;

					/* Separate sign and absolute value */
					if(n < 0)
						{
						negative = TRUE;
						n *= -1;
						}

					/* Convert the absolute value to decimal ASCII */
					for(bi=0; (n > 0 || bi == 0) && bi < sizeof(buffer); bi++)
						{
						buffer[bi] = (n % 10) + '0';
						n /= 10;
						}

					/* If width is specified, subtract width value including its sign. */
					if((width_left = fspecs[iii].width) > 0)
						{
						width_left -= bi;
						if(negative || fspecs[iii].flag_show_sign || fspecs[iii].flag_blank)
							width_left--;
						}

					/* If spaces will serve as padding on the left side, insert them now. */
					if(width_left > 0 && !fspecs[iii].flag_left_justify && !fspecs[iii].flag_leading_zeros)
						{
						while(width_left-- > 0)
							{
							gu_fputwc(' ', f);
							char_count++;
							}
						}

					/* Print the sign or a blank space for it. */
					if(negative)
						gu_fputwc('-', f);
					else if(fspecs[iii].flag_show_sign)
						gu_fputwc('+', f);
					else if(fspecs[iii].flag_blank)
						gu_fputwc(' ', f);

					/* If there is leftover space and it should be filled
					 * with zeros, do so now.  However, left justify
					 * overrides this.
					 */
					if(width_left > 0 && fspecs[iii].flag_leading_zeros && !fspecs[iii].flag_left_justify)
						{
						while(width_left-- > 0)
							gu_fputwc('0', f);
						}

					/* Now come the digits. */
					while(bi-- > 0)
						{
						gu_fputwc(buffer[bi], f);
						}

					/* If there is still field width left, fill it
					 * with spaces.
					 */
					while(width_left-- > 0)
						{
						gu_fputwc(' ', f);
						char_count++;
						}
					}
					break;

				/* UTF-8 String */
				case 's':
					{
					const char *s = fspecs[iii].value.character_pointer;
					int width_left, limit;
					wchar_t wc;

					/* If a width was specified, substract the width of the
					 * string to be printed.
					 */ 
					if((width_left = fspecs[iii].width) > 0)
						width_left -= gu_utf8_wswidth(s);

					/* If that leaves something, and the string is to be
					 * right-justified in its field, then print spaces
					 * as left-hand-side padding.
					 */
					if(width_left > 0 && !fspecs[iii].flag_left_justify)
						{
						while(width_left-- > 0)
							{
							gu_fputwc(' ', f);
							char_count++;
							}
						}

					/* Read and output wide characters until end of string
					 * or countdown reaches 0, whichever comes first.  Note
					 * that if the precision was not specified, then countdown 
					 * will start at -1 and thus will not reach zero.
					 */
				   	limit = fspecs[iii].precision;
					while((wc = gu_utf8_sgetwc(&s)) && limit-- != 0)
						{
						gu_fputwc(wc, f);
						char_count++;
						}

					/* If there is still field width left, fill it
					 * with spaces.
					 */
					while(width_left-- > 0)
						{
						gu_fputwc(' ', f);
						char_count++;
						}
					}
					break;
				}

			iii++;
			}
		else
			{
			gu_fputwc(wc, f);
			char_count++;
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

int gu_utf8_puts(const char *string)
	{
	return gu_utf8_fputs(string, stdout);
	}

int gu_utf8_putline(const char *string)
	{
	int count = gu_utf8_fputs(string, stdout);
	gu_fputwc('\n', stdout);
	count++;
	return count;
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
	gu_utf8_printf("Hello, %s!\n", "David");
	gu_utf8_printf("%3$s %s %1$s\n", "one", "two", "three");
	gu_utf8_printf("[%10s]\n", "right");
	gu_utf8_printf("[%-10s]\n", "left");
	gu_utf8_printf("[%-10.10s]\n", "left too long");
	gu_utf8_printf("right: [%10d]\n", 1000);
	gu_utf8_printf("left:  [%-10d]\n", 1000);
	gu_utf8_printf("zeros: [%010d]\n", 1000);
	gu_utf8_printf("plus:  [%+10d]\n", 1000);
	gu_utf8_printf("space: [% 10d]\n", 1000);
	gu_utf8_printf("neg:   [% 10d]\n", -1000);
	return 0;
	}
#endif

/* end of file */
