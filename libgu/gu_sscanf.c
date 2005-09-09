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
** Last modified 8 September 2005.
*/

/*! \file
	\brief safe sscanf()
	
This module constains a limited version of sscanf() which allows the maximum
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

<dd>read an int.  The argument should be a pointer to an int.</dd>

<dt>%ld</dt>

<dd>read a long int.  The argument should be a pointer to a long int.</dd>

<dt>%hd</dt>

<dd>read a short int.  The argument should be a pointer to a short int.</dd>

<dt>%u</dt>

<dd>read an unsigned int</dd>

<dt>%f</dt>

<dd>read a floating point number</dd>

<dt>%s</dt>

<dd>Read characters up to the next whitespace.  The argument should be a
pointer to a char array with enough space to hold the string and the
terminating NULL.  To prevent overruns, the size of the array may be
specified by a decimal number between the <tt>%</tt> and the <tt>s</tt> or
by a <tt>#</tt>. If the length of the array is specified with a <tt>#</tt>
then the actual length is read from gu_sscanf()'s next argument (the one
before the pointer to the char array).</dd>

<dt>%S</dt>

<dd>Read characters up to the next whitespace, allocate memory, and store
them in the allocated memory.  The argument should be a pointer to a char
pointer which will be set to the address of the allocated memory.</dd>

<dt>%Z</dt>

<dd>Read characters up to the end of the string, allocate storeage
for them, and copy them into that storage.  The argument should be a pointer to
a pointer to a char array.</dd>

<dt>%Q</dt>

<dd>Read a quoted string and allocate storate for it.</dd>

<dt>%A</dt>

<dd>If there is a quoted string, read to the closing quote.  Otherwise,
read to the end of the line.  This is used for reading values which 
must be quoted if they have leading or trailing spaces but needn't if
they just have internal spaces.</dd>

<dt>%t</dt>

<dd>Get a time_t.</dd>

<dt>%n</dt>

<dd>Store the number of characters read so far.</dd>

</dl>

*/
int gu_sscanf(const char *input, const char *format, ...)
	{
	const char function[] = "gu_sscanf";
	va_list va;
	int count = 0;				/* number of things extracted so far. */
	int maxextlen;				/* maximum characters to extract, including NULL */
	int len;					/* actual length */
	const char *pattern=format; /* Current position in the format. */
	const char *string=input;
	int islong;					/* TRUE if ell encountered. */
	int isshort;				/* TRUE if aich encountered. */
	char *extptr;				/* pointer to string we are extracting into */

	va_start(va, format);

	while(*pattern && *string)					/* Work until we run out of */
		{										/* pattern or input string. */
		if(*pattern == '%')						/* If special sequence begins, */
			{
			pattern++;

			maxextlen = 0;						/* Get "precision" */
			while(gu_ascii_isdigit(*pattern))	/* from any digits in */
				{								/* the format. */
				maxextlen *= 10;
				maxextlen += ( *(pattern++) - '0' );
				}
			if(*pattern == '#')					/* Get "precision" from */
				{
				maxextlen = va_arg(va, int);	/* the next parameter. */
				pattern++;
				}

			if(*pattern == 'l')					/* Will this be a long? */
				{ islong = TRUE; pattern++; }
			else
				{ islong = FALSE; }

			if(*pattern=='h')					/* Will it be short? */
				{ isshort = TRUE; pattern++; }
			else
				{ isshort = FALSE; }

			switch(*(pattern++))				/* Act on the type id char. */
				{
				case '%':						/* Literal "%" */
					if( *(string++) != '%' )	/* Match it */
						goto break_break;
					break;

				case '\0':						/* Nothing following '%', */
					pattern--;					/* oops! */
					break;

				/*
				 * Signed decimal int
				 */
				case 'd':
				case 'i':
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
						while(gu_ascii_isdigit(*string))			/* convert digits */
							{
							templong *= 10;
							templong += (*(string++)-'0');
							}
						*(va_arg(va, long int *)) = (templong * sign);
						}
					else if(isshort)
						{
						short int tempshort = 0;
						while(gu_ascii_isdigit(*string))			/* convert digits */
							{
							tempshort *= 10;
							tempshort += (*(string++)-'0');
							}
						*(va_arg(va, short int *)) = tempshort*sign; /* store it */
						}
					else
						{
						int tempint = 0;
						while(gu_ascii_isdigit(*string))			/* convert digits */
							{
							tempint *= 10;
							tempint += (*(string++) - '0');
							}
						*(va_arg(va, int *)) = (tempint * sign);
						}
					}
					count++;						/* increment count of values extracted */
					break;

				/*
				 * Unsigned decimal int
				 */
				case 'u':
					{
					unsigned int tempint = 0;
					if(!gu_ascii_isdigit(*string))
						goto break_break;
					while(gu_ascii_isdigit(*string))		/* convert digits */
						{
						tempint *= 10;
						tempint += (*(string++) - '0');
						}
					*(va_arg(va, unsigned int *)) = tempint;
					}
					count++;
					break;

				/*
				 * Decimal float, probably a version number.
				 */
				case 'f':
					{
					int sign = 1;
					int whole = 0;
					float fraction = 0.0;
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
					*(va_arg(va, float *)) = (float)sign * ((float)whole + fraction);
					}
					count++;
					break;

				/*
				 * Extract a string into the storage provided.
				 */
				case 's':
					extptr = va_arg(va, char *);				/* get pointer string to extract to */
					while( *string && !gu_ascii_isspace(*string) && --maxextlen )
						*(extptr++) = *(string++);
					*extptr = '\0';								/* terminate the string */
					while( *string && !gu_ascii_isspace(*string) )		/* eat any extra */
						string++;
					count++;									/* one more item done */
					break;

				/*
				 * Extract a string of any length
				 * and allocate storage for it.
				 */
				case 'S':
					len = strcspn(string, " \t\n");
					*(va_arg(va, char **)) = gu_strndup(string, len);
					string += len;
					count++;
					break;

				/*
				 * Get the rest of the line as a string and
				 * allocate storage for it.  Notice that
				 * we pay no attention to maxextlen.
				 */
				case 'Z':
				   len = strcspn(string, "\n");
				   *(va_arg(va,char **)) = gu_strndup(string, len);
				   string += len;
				   while(*string)
						string++;
				   count++;
				   break;

				/*
				 * Extract a possibly quoted string of any length
				 * and allocate storage for it.
				 */
				case 'Q':
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
					break;

				/*
				 * Get the rest of the line or a quoted string.  If the
				 * line ends with whitespace, omit the whitspace.
				 */
				case 'A':
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
							while(len > 0 && gu_ascii_isspace(string[len - 1]))
								len--;
							break;
						}
					*(va_arg(va, char **)) = gu_strndup(string, len);
					string += len;
					if(*string == 042 || *string == 047)
						string++;
					count++;
					break;

				/*
				 * Store the number of characters read so far.
				 */
				case 'n':
					*(va_arg(va, int *)) = ((string - input) / sizeof(const char));
					count++;
					break;

				/*
				 * Read a time_t.
				 */
				case 't':
					{
					time_t temptime = 0;
					while(gu_ascii_isdigit(*string))				/* convert digits */
						{
						temptime *= 10;
						temptime += (*(string++) - '0');
						}
					*(va_arg(va, time_t *)) = temptime;
					}
					count++;
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

/* end of file */
