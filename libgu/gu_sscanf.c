/*
** mouse:~ppr/src/libgu/gu_sscanf.c
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
** Last modified 9 September 2005.
*/

/*! \file
	\brief safe sscanf()
	
This module contains a limited version of sscanf() which allows the maximum
width of string arguments to be specified as an additional argument.  It also
defines additional formats such as %Z which reads a string which extends to the end
of the input string and allocaces storage for it, and %S which reads a word 
and allocates storage for it.

*/

#include "config.h"
#include <ctype.h>
#include <string.h>
#include "gu.h"

/** a safe sscanf()

This function is similiar to sscanf().  It has additional format specifiers
which allocate memory and read quoted strings.  Since it is meant to read PPR
configuration files and queue files, it goes out of its way not to heed the
current locale.

It implements the following formats:

<dl>

<dt>%d</dt>

<dd>read a signed decimal int.  The argument should be a pointer to an int.</dd>

<dt>%ld</dt>

<dd>read a signed long decimal int.  The argument should be a pointer to a long int.</dd>

<dt>%u</dt>

<dd>read an unsigned decimal int</dd>

<dt>%x</dt>

<dd>read an unsigned hexadecimal int</dd>

<dt>%hx</td>

<dd>read an unsigned hexadecimal short int</dd>

<dt>%f</dt>

<dd>read a floating point number</dd>

<dt>%s</dt>

<dd>Read characters up to the next whitespace.  The argument should be a
pointer to a char array with enough space to hold the string and the
terminating NULL.

To prevent overruns, the size of the array must be
specified by a decimal number between the <tt>%</tt> and the <tt>s</tt> or
by a <tt>@</tt>. If the length of the array is specified with a <tt>@</tt>
then the actual length is read from gu_sscanf()'s next argument (the one
before the pointer to the char array).</dd>

Note that the size of the array defaults to zero which causes an exception
to be thrown!

<dt>%S</dt>

<dd>Read characters up to the next whitespace, allocate memory, and store
them in the allocated memory.  The argument should be a pointer to a char
pointer which will be set to the address of the allocated memory.  The
field-width is ignored</dd>

<dt>%Z</dt>

<dd>Read characters up to the end of the string, allocate storeage
for them, and copy them into that storage.  The argument should be a pointer to
a char pointer to which will be set to the address of the allocated memory.</dd>

<dt>%Q</dt>

<dd>Read the next word (posibly quoted) and allocate storate for it.</dd>

<dt>%A</dt>

<dd>If there is a quoted string, read as %Q does.  Otherwise, read as %Z does.
This is used for reading values which must be quoted if they have leading or
trailing spaces but needn't be if they have only internal spaces.</dd>

<dt>%t</dt>

<dd>Get a time_t.</dd>

<dt>%n</dt>

<dd>Store the number of characters read so far.</dd>

</dl>

*/
int gu_sscanf(const char *input, const char *format, ...)
	{
	const char function[] = "gu_sscanf";
	const char *pattern=format; /* Current position in the format. */
	const char *string=input;
	va_list va;
	int count = 0;				/* number of things extracted so far. */
	int maxextlen;				/* maximum characters to extract, including NULL */
	gu_boolean islong;			/* TRUE if ell encountered. */
	gu_boolean isshort;			/* TRUE if aich encountered. */
	gu_boolean suppress;

	va_start(va, format);

	while(*pattern && *string)					/* Work until we run out of */
		{										/* pattern or input string. */
		if(*pattern == '%')						/* If special sequence begins, */
			{
			pattern++;
			suppress = FALSE;
			maxextlen = 0;
			isshort = FALSE;
			islong = FALSE;

			if(*pattern == '*')					/* assignment suppression */
				{
				suppress = TRUE;
				pattern++;
				}
			
			if(*pattern == '@')					/* Get "field-width" from */
				{
				maxextlen = va_arg(va, int);	/* the next parameter. */
				pattern++;
				}
			else								/* nope?  any digits? */
				{
				while(gu_ascii_isdigit(*pattern))
					{
					maxextlen *= 10;
					maxextlen += gu_ascii_digit_value(*(pattern++));
					}
				}

			for( ;strchr("lh", *pattern); pattern++)
				{
				switch(*pattern)
					{
					case 'l':
						islong = TRUE;
						break;
					case 'h':
						isshort = TRUE;
						break;
					}
				}

			switch(*(pattern++))				/* Act on the type id char. */
				{
				case '%':						/* Literal "%" */
					if( *(string++) != '%' )	/* Match it */
						goto break_break;
					break;

				case '\0':						/* Nothing following '%', */
					pattern--;					/* oops! */
					break;

				case 'd':
					{
					int sign = 1;						/* 1 or -1 */
					if(*string == '-')					/* if a minus sign is found, */
						{
						sign = -1;
						string++;
						}
					if(! gu_ascii_isdigit(*string))		/* if no number present, */
						goto break_break;
					if(islong)
						{
						long int templong = 0;
						while(gu_ascii_isdigit(*string))
							{
							templong *= 10;
							templong += gu_ascii_digit_value(*(string++));
							}
						if(!suppress)
							*(va_arg(va, long int *)) = (templong * sign);
						}
					else if(isshort)
						{
						short int tempshort = 0;
						while(gu_ascii_isdigit(*string))
							{
							tempshort *= 10;
							tempshort += gu_ascii_digit_value(*(string++));
							}
						if(!suppress)
							*(va_arg(va, short int *)) = tempshort*sign; /* store it */
						}
					else
						{
						int tempint = 0;
						while(gu_ascii_isdigit(*string))
							{
							tempint *= 10;
							tempint += gu_ascii_digit_value(*(string++));
							}
						if(!suppress)
							*(va_arg(va, int *)) = (tempint * sign);
						}
					}
					count++;						/* increment count of values extracted */
					break;

				case 'u':
					if(!gu_ascii_isdigit(*string))
						goto break_break;
					if(islong)
						{
						gu_Throw("%s(): %%lu not implemented", function);
						}
					else if(isshort)
						{
						gu_Throw("%s(): %%hu not implemented", function);
						}
					else
						{
						unsigned int tempint = 0;
						while(gu_ascii_isdigit(*string))		/* convert digits */
							{
							tempint *= 10;
							tempint += gu_ascii_digit_value(*(string++));
							}
						if(!suppress)
							*(va_arg(va, unsigned int *)) = tempint;
						}
					count++;
					break;


				case 'x':
					if(islong)
						{
						unsigned long int tempint = 0;
						while(gu_ascii_isxdigit(*string))		/* convert digits */
							{
							tempint <<= 4;
							tempint |= gu_ascii_xdigit_value(*(string++));
							}
						if(!suppress)
							*(va_arg(va, unsigned long int *)) = tempint;
						}
					else
						{
						unsigned int tempint = 0;
						while(gu_ascii_isxdigit(*string))		/* convert digits */
							{
							tempint <<= 4;
							tempint |= gu_ascii_xdigit_value(*(string++));
							}
						if(!suppress)
							{
							if(isshort)
								*(va_arg(va, unsigned short int *)) = tempint;
							else
								*(va_arg(va, unsigned int *)) = tempint;
							}
						}
					count++;
					break;

				case 'f':
					{
					int sign = 1;
					int whole = 0;
					double fraction = 0.0;
					if(*string == '-')					/* if a minus sign is found, */
						{
						sign = -1;
						string++;
						}
					if(! gu_ascii_isdigit(*string))		/* if no number present, */
						goto break_break;
					while(gu_ascii_isdigit(*string))	/* convert digits to left of decimal point */
						{
						whole *= 10;
						whole += (*(string++) - '0');
						}
					if(*string == '.')
						{
						float place = 0.1;
						string++;
						while(gu_ascii_isdigit(*string))
							{
							fraction += place * (*(string++) - '0');
							place /= 10.0;
							}
						}
					if(!suppress)
						{
						if(isshort)
							gu_Throw("%s(): no such thing as short float", function);
						else if(islong)
							*(va_arg(va, double *)) = (double)sign * ((double)whole + fraction);
						else
							*(va_arg(va, float *)) = (float)sign * ((float)whole + fraction);
						}
					}
					count++;
					break;

				case 's':
					if(!suppress)
						{
						char *extptr = va_arg(va, char *);

						if(maxextlen < 1)
							gu_Throw("%s(): field width for %%s is %d!", function, maxextlen);
					
						/* Copy what will fit */
						while(*string && !gu_ascii_isspace(*string) && --maxextlen)
							*(extptr++) = *(string++);
						*extptr = '\0';
						}

					/* Eat the part that won't fit. */
					while( *string && !gu_ascii_isspace(*string) )
						string++;

					count++;			/* one more item done */
					break;

				case 'S':
					{
					int len = strcspn(string, " \t\n");
					*(va_arg(va, char **)) = gu_strndup(string, len);
					string += len;
					count++;
					}
					break;

				case 'Z':
					{
					int len = strcspn(string, "\n");
					*(va_arg(va,char **)) = gu_strndup(string, len);
					string += len;
					while(*string)
						string++;
					count++;
					}
					break;

				case 'Q':
					{
					int len;
					switch(*string)
						{
						case 042:		/* ASCII double quote */
							string++;
							len = strcspn(string, "\"");
							break;
						case 047:		/* ASCII single quote */
							string++;
							len = strcspn(string, "'");
							break;
						default:
							len = strcspn(string, " \t\n");
							break;
						}
					*(va_arg(va, char **)) = gu_strndup(string, len);
					string += len;
					if(*string == 042 || *string == 047)
						string++;
					count++;
					}
					break;

				case 'A':
					{
					int len;
					switch(*string)
						{
						case 042:		/* ASCII double quote */
							string++;
							len = strcspn(string, "\"");
							break;
						case 047:		/* ASCII single quote */
							string++;
							len = strcspn(string, "'");
							break;
						default:
							len = strlen(string);
							/* don't copy trailing space */
							while(len > 0 && gu_ascii_isspace(string[len - 1]))
								len--;
							break;
						}
					*(va_arg(va, char **)) = gu_strndup(string, len);
					string += len;
					if(*string == 042 || *string == 047)
						string++;
					count++;
					}
					break;

				case 'n':
					*(va_arg(va, int *)) = ((string - input) / sizeof(const char));
					count++;
					break;

				case 't':
					{
					time_t temptime = 0;
					while(gu_ascii_isdigit(*string))				/* convert digits */
						{
						temptime *= 10;
						temptime += (*(string++) - '0');
						}
					*(va_arg(va, time_t *)) = temptime;
					count++;
					}
					break;

				/*
				 * Catch unimplemented formats here
				 */
				default:
					gu_Throw("%s(): unrecognized format '%c' in \"%s\"", function, *(pattern - 1), format);
				}
			}

		/* Not a format specifier. */
		else
			{
			if(gu_ascii_isspace(*pattern))				/* Whitespace matches any */
				{										/* amount of whitespace. */
				pattern++;
				while(gu_ascii_isspace(*string))
					string++;
				}
			else										/* Everthing else */
				{										/* must exactly match */
				if( *(pattern++) != *(string++) )		/* characters in string. */
					break;
				}
			}
		} /* end of loop that lasts as long as pattern and string both last */
		break_break:

	va_end(va);

	return count;
	} /* end of gu_sscanf() */

/* gcc -Wall -DTEST -I../include -o gu_sscanf gu_sscanf.c ../libgu.a */
#ifdef TEST
#include <stdio.h>
int main(int argc, char *argv[])
	{
	int t_d;
	long int t_ld;
	short int t_hd;
	unsigned int t_u;
	unsigned int t_x;
	unsigned short int t_hx;
	float t_f;
	double t_lf;
	char t_s[10];
	char t_s2[5];
	
	printf("Read %d items, expected 9:\n",
		gu_sscanf(
			"101 -102 103	  201   1b2 1b3  4000.00042 4000.00043  smith",
			"%d %ld %hd %u %x %hx %f %lf %10s",
			&t_d,
			&t_ld,
			&t_hd,
			&t_u,
			&t_x,
			&t_hx,
			&t_f,
			&t_lf,
			t_s)
		);
	printf("\tt_d=%d\n", t_d);
	printf("\tt_ld=%ld\n", t_ld);
	printf("\tt_hd=%hd\n", t_hd);
	printf("\tt_u=%u\n", t_u);
	printf("\tt_x=%x\n", t_x);
	printf("\tt_hx=%hx\n", t_hx);
	printf("\tt_f=%f (c.f. %f)\n", t_f, (float)4000.00042);
	printf("\tt_lf=%lf\n", t_lf);
	printf("\tt_s=\"%s\"\n", t_s);

	printf("Read %d items, expected 2:\n",
		gu_sscanf(
			"123456789012 123456789012",
			"%@s %@s",
			sizeof(t_s), t_s,
			sizeof(t_s2), t_s2
			)
		);
	printf("\tt_s=\"%s\"\n", t_s);
	printf("\tt_s2=\"%s\"\n", t_s2);

	printf("Read %d items, expected 3:\n",
		gu_sscanf(
			"one two three",
			"%@s %*s %@s",
			sizeof(t_s), t_s,
			sizeof(t_s2), t_s2
			)
		);
	printf("\tt_s=\"%s\"\n", t_s);
	printf("\tt_s2=\"%s\"\n", t_s2);

	printf("Read %d items, expected 4:\n",
		gu_sscanf(
			"one 2 3three",
			"%@s %*d %*d%@s",
			sizeof(t_s), t_s,
			sizeof(t_s2), t_s2
			)
		);
	printf("\tt_s=\"%s\"\n", t_s);
	printf("\tt_s2=\"%s\"\n", t_s2);

	return 0;
	}
#endif

/* end of file */
