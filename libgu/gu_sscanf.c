/*
** mouse:~ppr/src/libgu/gu_sscanf.c
** Copyright 1995--2004, Trinity College Computing Center.
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
** Last modified 27 January 2004.
*/

/*! \file
	\brief safe sscanf()
	
This module constains a limited version of sscanf() which allows the maximum
width of string arguments to be specified as an additional argument.  It also
defines additional formats such as %z which reads a string which extends to the end
of the input string, and %S which reads a word and allocates storage for it.

*/

#include "before_system.h"
#include <ctype.h>
#include <string.h>
#include "gu.h"

#define MAX_ROLLBACK 10
static int checkpoint_depth = 0;
static char *strings[MAX_ROLLBACK];
static int strings_count = 0;

/*
** Call this function to have gu_sscanf() keep a list of the memory blocks
** it allocates for return values.
*/
void gu_sscanf_checkpoint(void)
	{
	const char function[] = "gu_sscanf_checkpoint";
	if(checkpoint_depth != 0)
		gu_Throw("%s(): checkpoint_depth is %d!", function, checkpoint_depth);
	checkpoint_depth++;
	}

/*
** Call this to free all of the memory blocks that gu_sscanf() allocated
** since the last call to gu_sscanf_checkpoint().
*/
void gu_sscanf_rollback(void)
	{
	const char function[] = "gu_sscanf_checkpoint";
	if(checkpoint_depth != 1)
		gu_Throw("%s(): checkpoint_depth is %d!", function, checkpoint_depth);
	while(strings_count > 0)
		{
		strings_count--;
		gu_free(strings[strings_count]);
		}
	checkpoint_depth--;
	}

/*
** This function is used internaly to allocate space for formats such as %S. 
** We don't use gu_strndup() because it doesn't have hooks for the rollback
** stack.
*/
static char *gu_sscanf_strndup(const char *string, size_t len)
	{
	const char function[] = "gu_sscanf_strndup";
	char *p = gu_strndup(string, len);
	if(checkpoint_depth > 0)
		{
		if(strings_count >= MAX_ROLLBACK)
			gu_Throw("%s(): rollback stack overflow", function);
		strings[strings_count++] = p;
		}
	return p;
	}

/*
** This function replaces isdigit() from the C library.  The one in the C library 
** determines if c is a digit in the current locale.  We want to know if it is a
** digit in ASCII.
*/
static int gu_isdigit(int c)
	{
	if(c >= '0' && c <= '9')
		return 1;
	else
		return 0;
	}

/** safe sscanf()

This function is similiar to sscanf().  It has additional format specifiers which allocate
memory and read quoted strings.  Since it is meant to read PPR configuration files and
queue files, it does out of its way not to heed the current locale.

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
then the actuall length is read from gu_sscanf()'s next argument (the one
before the pointer to the char array).</dd>

<dt>%S</dt>

<dd>Read characters up to the next whitespace, allocate memory, and store
them in the allocated memory.  The argument should be a pointer to a char
pointer which will be set to the address of the allocated memory.</dd>

<dt>%z</dt>

<dd>Read characters up to the end of the string.  The argument should be a
char array.  To prevent overruns, the size of the char array may be
specified in the same manner as for the %s format.</dd>

<dt>%Z</dt>

<dd>Read characters up to the end of the string, allocate storeage
for them, and copy them into that storage.  The argument should be a pointer to
a pointer to a char array.</dd>

<dt>%Q</dt>

<dd>Read a quoted string and allocate storate for it.</dd>

<dt>%A</dt>

<dd>Read a quoted string, or if there isn't one, to the end of the line</dd>

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
			while(gu_isdigit(*pattern))			/* from any digits in */
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
				case '%':						/* Litteral "%" */
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
					if(! gu_isdigit(*string))				/* if no number present, */
						goto break_break;
					if(islong)
						{
						long int templong = 0;
						while(gu_isdigit(*string))			/* convert digits */
							{
							templong *= 10;
							templong += (*(string++)-'0');
							}
						*(va_arg(va, long int *)) = (templong * sign);
						}
					else if(isshort)
						{
						short int tempshort = 0;
						while(gu_isdigit(*string))			/* convert digits */
							{
							tempshort *= 10;
							tempshort += (*(string++)-'0');
							}
						*(va_arg(va, short int *)) = tempshort*sign; /* store it */
						}
					else
						{
						int tempint = 0;
						while(gu_isdigit(*string))			/* convert digits */
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
					if(!gu_isdigit(*string))
						goto break_break;
					while(gu_isdigit(*string))		/* convert digits */
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
					if(! gu_isdigit(*string))			/* if no number present, */
						goto break_break;
					while(gu_isdigit(*string))			/* convert digits to left of decimal point */
						{
						whole *= 10;
						whole += (*(string++) - '0');
						}
					if(*string == '.')
						{
						float place = 0.1;
						string++;
						while(gu_isdigit(*string))
							{
							fraction += place * (*(string++) - '0');
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
					while( *string && !isspace(*string) && --maxextlen )
						*(extptr++) = *(string++);
					*extptr = '\0';								/* terminate the string */
					while( *string && !isspace(*string) )		/* eat any extra */
						string++;
					count++;									/* one more item done */
					break;

				/*
				 * Extract a string of any length
				 * and allocate storage for it.
				 */
				case 'S':
					len = strcspn(string, " \t\n");
					*(va_arg(va, char **)) = gu_sscanf_strndup(string, len);
					string += len;
					count++;
					break;

				/*
				 * Get rest of line as a string, extracting it
				 * into the storage provided.
				 */
				case 'z':
					extptr = va_arg(va, char *);
					while(*string && --maxextlen)
						*(extptr++) = *(string++);
					*extptr = '\0';
					count++;
					break;

				/*
				 * Get the rest of the line as a string and
				 * allocate storage for it.  Notice that
				 * we pay no attention to maxextlen.
				 */
				case 'Z':
				   len = strcspn(string, "\n");
				   *(va_arg(va,char **)) = gu_sscanf_strndup(string, len);
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
					*(va_arg(va, char **)) = gu_sscanf_strndup(string, len);
					string += len;
					if(*string == 042 || *string == 047)
						string++;
					count++;
					break;

				/*
				 * Get the rest of the line or a quoted string.  If the
				 * line ends with whitespace, ommit it.
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
							while(len > 0 && isspace(string[len - 1]))
								len--;
							break;
						}
					*(va_arg(va, char **)) = gu_sscanf_strndup(string, len);
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
					while(gu_isdigit(*string))				/* convert digits */
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
			if(isspace(*pattern))				/* Whitespace matches any */
				{								/* amount of whitespace. */
				pattern++;
				while(isspace(*string))
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
