/*
** mouse:~ppr/src/libgu/gu_fscanf.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 24 May 2001.
*/

/*
** This module constains a very limited version of gu_fscanf().  It can only scan
** integers.
*/

#include "before_system.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "gu.h"

int gu_fscanf(FILE *input, const char *format, ...)
    {
    va_list va;
    int count = 0;				/* number of things extracted so far */
    const char *pattern = format;
    int c;					/* current character */

    va_start(va, format);

    while(*pattern && (c = fgetc(input)) != EOF)	/* Work until we run out of */
	{						/* pattern or input file. */
	if(*pattern == '%')				/* If special sequence begins, */
	    {
	    pattern++;

	    switch(*(pattern++))		/* Act on the type id char. */
		{
		case '%':			/* Literal "%" must match itself. */
		    if(c != '%')
		    	goto break_break;
		    break;

		/*
		 * Decimal integer.
		 */
		case 'd':
		case 'i':
		    {
		    int sign = 1;			/* 1 or -1 */
		    int n = 0;
		    if(c == '-')			/* if a minus sign is found, */
			{
			sign = -1;			/* set a flag */
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
		    count++;				/* increment count of values extracted */
		    break;

		/*
		 * Catch unimplemented formats here
		 */
		default:
		    libppr_throw(EXCEPTION_BADUSAGE, "gu_fscanf", "unrecognized format '%c' in \"%s\"", *(pattern - 1), format);
		}
	    }

	/* If not a format specifier, */
	else
	    {
	    if(isspace(*pattern))		/* Whitespace matches any */
		{				/* amount of whitespace. */
		while(c != EOF && isspace(c))
		    c = fgetc(input);
                if(c != EOF)			/* Push back the 1st non-whitespace character. */
                    ungetc(c, input);
		pattern++;
		}
	    else                               	/* Everthing else must exactly */
		{                              	/* match characters in string. */
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
