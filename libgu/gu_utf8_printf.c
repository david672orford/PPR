/*
** mouse:~ppr/src/libgu/gu_utf8_printf.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 21 April 2006.
*/

/*! \file
    \brief printf() functions which accept UTF-8 input
*/

#include "config.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"

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
	long int long_integer;
	char *character_pointer;
	double floating_point;
	};

enum VALUE_TYPE {
	VALUE_TYPE_INTEGER,
	VALUE_TYPE_LONG_INTEGER,
	VALUE_TYPE_CHARACTER_POINTER,
	VALUE_TYPE_FLOATING_POINT
	};

struct FSPEC {
	gu_boolean flag_left_justify;
	gu_boolean flag_show_sign;
	gu_boolean flag_blank;
	gu_boolean flag_leading_zeros;
	int width;
	int precision;
	gu_boolean flag_l;
	enum VALUE_TYPE value_type;
	union VALUE value;
	int format_length;
	};

enum ARG_FSPEC_FIELD {
	ARG_FSPEC_FIELD_VALUE,
	ARG_FSPEC_FIELD_WIDTH,
	ARG_FSPEC_FIELD_PRECISION
	};

struct ARG {
	int fspec_index;
	enum ARG_FSPEC_FIELD fspec_field;
	};

static int get_aindex(const char **p, int default_index)
	{
	int len;
	if((len = strspn(*p, "0123456789")) > 0 && (*p)[len] == '$')
		{
		int iii = atoi(*p);
		iii--;
		(*p) += len;
		(*p)++;
		return iii;
		}
	else
		{
		return default_index;
		}
	}

static void arguments_store(struct ARG *arguments, int aindex, int findex, enum ARG_FSPEC_FIELD field)
	{
	char function[] = "gu_utf8_vfprintf_arguments_store";
	if(aindex >= MAX_FSPECS)
		gu_Throw("%s(): argument index %d is too high", function, aindex+1);
	if(arguments[aindex].fspec_index != -1)
		gu_Throw("%s(): double use of argument (by specs %d and %d)", function, arguments[aindex].fspec_index+1, findex+1);
	arguments[aindex].fspec_index = findex;
	arguments[aindex].fspec_field = field;
	}

int gu_utf8_vfprintf(FILE *f, const char *format, va_list args)
	{
	const char function[] = "gu_utf8_vfprintf";
	int char_count = 0;
	wchar_t wc;
	const char *formatp;
	int findex;				/* format specifier index */
	int aindex;				/* argument index */
	struct FSPEC fspecs[MAX_FSPECS];
	struct ARG arguments[MAX_FSPECS*3];
	int len;					/* for general use */

	for(aindex = 0; aindex < MAX_FSPECS; aindex++)
		arguments[aindex].fspec_index = -1;

	/* First pass, parse all of the format specifiers and note which 
	 * argument each requires.
	 */
	for(formatp = format, findex = aindex = 0; (wc = gu_utf8_sgetwc(&formatp)); )
		{
		if(wc == '%')
			{
			const char *formatp_save = formatp;
			int position;
			fspecs[findex].flag_left_justify = FALSE;
			fspecs[findex].flag_show_sign = FALSE;
			fspecs[findex].flag_blank = FALSE;
			fspecs[findex].flag_leading_zeros = FALSE;
			fspecs[findex].width = 0;
			fspecs[findex].precision = -1;		/* unspecified */
			fspecs[findex].flag_l = FALSE;
			position = -1;

			/* Is the argument index specified? */
			if((len = strspn(formatp, "0123456789")) > 0 && formatp[len] == '$')
				{
				position = atoi(formatp);
				position--;		/* convert from 1 based to 0 based */
				formatp += (len+1);
				}
	
			/* Look for formatting modifiers. */			
			while((wc = gu_utf8_sgetwc(&formatp)) && strchr("-+ #0", wc))
				{
				switch(wc)
					{
					case '-':
						fspecs[findex].flag_left_justify = TRUE;
						break;
					case '+':
						fspecs[findex].flag_show_sign = TRUE;
						break;
					case ' ':
						fspecs[findex].flag_blank = TRUE;
						break;
					case '0':
						fspecs[findex].flag_leading_zeros = TRUE;
						break;
					}
				}
	
			/* Look for field width */
			if(wc == '*')
				{
				int i = get_aindex(&formatp, aindex);
				arguments_store(arguments, i, findex, ARG_FSPEC_FIELD_WIDTH);
				aindex++;
				wc = gu_utf8_sgetwc(&formatp);
				}
			else
				{
				while(gu_ascii_isdigit(wc))
					{
					fspecs[findex].width *= 10;
					fspecs[findex].width += gu_ascii_digit_value(wc);		
					wc = gu_utf8_sgetwc(&formatp);
					}
				}

			/* Look for precision */
			if(wc == '.')
				{
				fspecs[findex].precision = 0;
				wc = gu_utf8_sgetwc(&formatp);
				if(wc == '*')
					{
					int i = get_aindex(&formatp, aindex);
					arguments_store(arguments, i, findex, ARG_FSPEC_FIELD_PRECISION);
					aindex++;
					wc = gu_utf8_sgetwc(&formatp);
					}
				else
					{
					while(gu_ascii_isdigit(wc))
						{
						fspecs[findex].precision *= 10;
						fspecs[findex].precision += gu_ascii_digit_value(wc);		
						wc = gu_utf8_sgetwc(&formatp);
						}
					}
				}

			if(position == -1)
				position = aindex;	
			arguments_store(arguments, position, findex, ARG_FSPEC_FIELD_VALUE);

			/* Look for parameter size modifiers. */
			while(strchr("hl", wc))
				{
				switch(wc)
					{
					case 'l':
						fspecs[findex].flag_l = TRUE;
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
					break;
				case 'c':
				case 'd':
					if(fspecs[findex].flag_l)
						fspecs[findex].value_type = VALUE_TYPE_LONG_INTEGER;
					else
						fspecs[findex].value_type = VALUE_TYPE_INTEGER;
					aindex++;
					break;
				case 's':
					fspecs[findex].value_type = VALUE_TYPE_CHARACTER_POINTER;
					aindex++;
					break;
				case 'f':
					fspecs[findex].value_type = VALUE_TYPE_FLOATING_POINT;
					aindex++;
					break;
				default:
					gu_Throw("%s(): unrecognized format '%c' in \"%s\"", function, (char)wc, format);
					break;
				}

			/* How many characters does the format specifier consume?
			 * This is not really correct since it treats bytes and
			 * characters as equivelent, but this should not be a problem
			 * since all format specifiers are ASCII digits. */
			fspecs[findex].format_length = (formatp - formatp_save);

			findex++;
			}
		}

	/* Load the required arguments. */
	{
	int param_count = aindex;
	for(aindex = 0; aindex < param_count; aindex++)
		{
		if(arguments[aindex].fspec_index == -1)
			gu_Throw("%s(): parameter gap at position %d", function, aindex+1);
		findex = arguments[aindex].fspec_index;
		switch(arguments[aindex].fspec_field)
			{
			case ARG_FSPEC_FIELD_VALUE:
				switch(fspecs[findex].value_type)
					{
					case VALUE_TYPE_INTEGER:
						fspecs[findex].value.integer = va_arg(args, int);
						break;
					case VALUE_TYPE_LONG_INTEGER:
						fspecs[findex].value.long_integer = va_arg(args, long int);
						break;
					case VALUE_TYPE_CHARACTER_POINTER:
						fspecs[findex].value.character_pointer = va_arg(args, char*);
						break;
					case VALUE_TYPE_FLOATING_POINT:
						fspecs[findex].value.floating_point = va_arg(args, double);
						break;
					}
				break;
			case ARG_FSPEC_FIELD_WIDTH:
				fspecs[findex].width = va_arg(args, int);
				break;
			case ARG_FSPEC_FIELD_PRECISION:
				fspecs[findex].precision = va_arg(args, int);
				break;
			}
		}
	}

	/* Second pass, produce output. */	
	for(formatp = format, findex = 0; (wc = gu_utf8_sgetwc(&formatp)); )
		{
		if(wc == '%')
			{
			/*printf("width=%d precision=%d\n", fspecs[findex].width, fspecs[findex].precision);*/

			/* Skip up to the type field */
			{
			int y;
			for(y = 0; y < fspecs[findex].format_length; y++)
				 wc = gu_utf8_sgetwc(&formatp);
			}

			switch(wc)
				{
				case '%':
					gu_fputwc('%', f);
					break;

				/* 8-bit character */
				case 'c':
					gu_fputwc(fspecs[findex].value.integer, f);
					break;	

				/* Decimal integer */
				case 'd':
					{
					char buffer[32];
					int bi;
					long int n;
					gu_boolean negative = FALSE;
					int width_left;

					if(fspecs[findex].flag_l)
						n = fspecs[findex].value.long_integer;
					else
						n = fspecs[findex].value.integer;
					
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
					if((width_left = fspecs[findex].width) > 0)
						{
						width_left -= bi;
						if(negative || fspecs[findex].flag_show_sign || fspecs[findex].flag_blank)
							width_left--;
						}

					/* If spaces will serve as padding on the left side, insert them now. */
					if(width_left > 0 && !fspecs[findex].flag_left_justify && !fspecs[findex].flag_leading_zeros)
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
					else if(fspecs[findex].flag_show_sign)
						gu_fputwc('+', f);
					else if(fspecs[findex].flag_blank)
						gu_fputwc(' ', f);

					/* If there is leftover space and it should be filled
					 * with zeros, do so now.  However, left justify
					 * overrides this.
					 */
					if(width_left > 0 && fspecs[findex].flag_leading_zeros && !fspecs[findex].flag_left_justify)
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
					const char *s;
					int width_left, limit;
					wchar_t wc;

				   	if(!(s = fspecs[findex].value.character_pointer))
						s = "(NULL)";

					/* If a width was specified, substract the width of the
					 * string to be printed keeping in mind that the
					 * precision may limit how much of the string is printed.
					 */ 
					if((width_left = fspecs[findex].width) > 0)
						{
						size_t width = gu_utf8_wswidth(s);
						if(width <= fspecs[findex].precision)
							width_left -= width;
						else
							width_left -= fspecs[findex].precision;
						}

					/* If that leaves something, and the string is to be
					 * right-justified in its field, then print spaces
					 * as left-hand-side padding.
					 */
					if(width_left > 0 && !fspecs[findex].flag_left_justify)
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
				   	limit = fspecs[findex].precision;
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

				/* floating point */
				case 'f':
					{
					char format[32] = {'%', 0};
					gu_snprintfcat(format, sizeof(format), "%s%s%s%s",
						fspecs[findex].flag_left_justify ? "-" : "",
						fspecs[findex].flag_show_sign ? "+" : "",
						fspecs[findex].flag_blank ? " " : "",
						fspecs[findex].flag_leading_zeros ? "0" : ""
						);
					if(fspecs[findex].width > 0)
						gu_snprintfcat(format, sizeof(format), "%d", fspecs[findex].width);
					if(fspecs[findex].precision != -1)
						gu_snprintfcat(format, sizeof(format), ".%d", fspecs[findex].precision);
					gu_snprintfcat(format, sizeof(format), "f");
					/* Since fprintf() will generate output in the proper output encoding,
					 * we don't need an intermediate conversion to UTF-8. */
					/*printf(">%s<", format);*/
					fprintf(f, format, fspecs[findex].value.floating_point);
					}
					break;
				}

			findex++;
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
	gu_utf8_printf("[%-*.*s]\n", 10, 10, "left too long");
	gu_utf8_printf("[%-15.10s]\n", "left too long");
	gu_utf8_printf("[%1$-*2$.*3$s]\n", "left too long", 15, 10);
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
