/*
** mouse:~ppr/src/libgu/gu_fscanf.c
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
** Last modified 2 September 2005.
*/

/*! \file
	\brief limited fscanf()

	This module constains a very limited version of gu_fscanf().  So far it
	it can only scan integers.  It deliberately ignores the current locale.
	It is intended for IPC though it could also be used for reading configuration
	files.

	If this function is expanded very much it should be integrated with
	gu_sscanf() so that they share a common 'engine'.

*/

#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "gu.h"

int gu_fscanf(FILE *input, const char *format, ...)
	{
	const char function[] = "gu_fscanf";
	va_list va;
	int count = 0;								/* number of things extracted so far */
	const char *pattern = format;
	int c;										/* current character */

	va_start(va, format);

	while(*pattern && (c = fgetc(input)) != EOF)/* Work until we run out of */
		{										/* pattern or input file. */
		if(*pattern == '%')						/* If special sequence begins, */
			{
			pattern++;

			switch(*(pattern++))				/* Act on the type id char. */
				{
				case '%':						/* Literal "%" must match itself. */
					if(c != '%')
						goto break_break;
					break;

				/*
				 * Decimal integer.
				 */
				case 'd':
				case 'i':
					{
					int sign = 1;				/* 1 or -1 */
					int n = 0;
					if(c == '-')				/* if a minus sign is found, */
						{
						sign = -1;				/* set a flag */
						c = fgetc(input);
						}
					for( ; c != EOF && isdigit(c); c = fgetc(input))
						{
						n *= 10;
						n += (c - '0');
						}
					if(c != EOF)
						ungetc(c, input);
					*(va_arg(va, int *)) = (n * sign);
					}
					count++;					/* increment count of values extracted */
					break;

				/*
				 * Catch unimplemented formats here
				 */
				default:
					gu_Throw("%s(): unrecognized format '%c' in \"%s\"", function, *(pattern - 1), format);
				}
			}

		/* If not a format specifier, */
		else
			{
			if(isspace(*pattern))				/* Whitespace matches any */
				{								/* amount of whitespace. */
				while(c != EOF && isspace(c))
					c = fgetc(input);
				if(c != EOF)					/* Push back the 1st non-whitespace character. */
					ungetc(c, input);
				pattern++;
				}
			else								/* Everthing else must exactly */
				{								/* match characters in string. */
				if(*pattern++ != c)
					{
					ungetc(c, input);
					break;
					}
				}
			}
		} /* end of loop that lasts as long as pattern and file both last */
	break_break:

	va_end(va);

	return count;
	} /* end of gu_sscanf() */

/* end of file */
