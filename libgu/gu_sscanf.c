/*
** mouse:~ppr/src/libgu/gu_sscanf.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 4 May 2001.
*/

/*
** This module constains a limited version of sscanf() which allows the
** maximum width of string arguments to be specified as an additional
** argument.  It also defines a format called %z which is a string which
** extends to the end of the string which is fed for gu_sscanf().
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
    if(checkpoint_depth != 0)
    	libppr_throw(EXCEPTION_OVERFLOW, "gu_sscanf_checkpoint", "checkpoint_depth is %d!", checkpoint_depth);
    checkpoint_depth++;
    }

/*
** Call this to free all of the memory blocks that gu_sscanf() allocated
** since the last call to gu_sscanf_checkpoint().
*/
void gu_sscanf_rollback(void)
    {
    if(checkpoint_depth != 1)
    	libppr_throw(EXCEPTION_OVERFLOW, "gu_sscanf_checkpoint", "checkpoint_depth is %d!", checkpoint_depth);
    while(strings_count > 0)
    	{
	strings_count--;
	gu_free(strings[strings_count]);
    	}
    checkpoint_depth--;
    }

/*
** This function is used internaly to allocate space for formats such
** as %S.
*/
static char *gu_sscanf_strndup(const char *string, size_t len)
    {
    char *p = gu_strndup(string, len);
    if(checkpoint_depth > 0)
    	{
	if(strings_count >= MAX_ROLLBACK)
	    libppr_throw(EXCEPTION_OVERFLOW, "gu_sscanf_strndup", "rollback stack overflow");
	strings[strings_count++] = p;
    	}
    return p;
    }

/*
** Here is the title function.
*/
int gu_sscanf(const char *input, const char *format, ...)
    {
    va_list va;
    int count = 0;		/* Number of things extracted so far. */
    int maxextlen;		/* Maximum characters to extract, including NULL. */
    int len;			/* Actual length */
    int sign;			/* 1 or -1 */
    const char *pattern=format;	/* Current position in the format. */
    const char *string=input;
    int islong;			/* True if ell encountered. */
    int isshort;		/* True if aich encountered. */
    char *extptr;		/* Pointer to string we are extracting into. */

    va_start(va, format);

    while(*pattern && *string)			/* Work until we run out of */
	{					/* pattern or input string. */
	if(*pattern == '%')			/* If special sequence begins, */
	    {
	    pattern++;

	    maxextlen = 0;			/* Get "precision" */
	    while(isdigit(*pattern))		/* from any digits in */
		{				/* the format. */
		maxextlen *= 10;
		maxextlen += ( *(pattern++) - '0' );
		}
	    if(*pattern == '#')			/* Get "precision" from */
		{
		maxextlen = va_arg(va, int);	/* the next parameter. */
		pattern++;
		}

	    if(*pattern == 'l')			/* Will this be a long? */
	    	{ islong = TRUE; pattern++; }
	    else
	    	{ islong = FALSE; }

	    if(*pattern=='h')			/* Will it be short? */
	    	{ isshort = TRUE; pattern++; }
	    else
	    	{ isshort = FALSE; }

	    switch(*(pattern++))		/* Act on the type id char. */
		{
		case '%':			/* Litteral "%" */
		    if( *(string++) != '%' )	/* Match it */
		    	goto break_break;
		    break;

		case '\0':			/* Nothing following '%', */
		    pattern--;			/* oops! */
		    break;

		/*
		 * Decimal integer.
		 */
		case 'd':
		case 'i':
		    if(*string == '-')		    /* if a minus sign is found, */
			{
			string++;
			sign = -1;		    /* set a flag */
			}
		    else
		    	{
		    	sign = 1;
		    	}
		    if(! isdigit(*string))		/* if no number present, */
		    	goto break_break;
		    if(islong)
		    	{
			long int templong = 0;
		    	while(isdigit(*string))		/* convert digits */
			    {
			    templong *= 10;
			    templong += (*(string++)-'0');
			    }
			*(va_arg(va, long int *)) = (templong * sign);
			}
		    else if(isshort)
			{
			short int tempshort = 0;
		    	while(isdigit(*string))		/* convert digits */
			    {
			    tempshort *= 10;
			    tempshort += (*(string++)-'0');
			    }
			*(va_arg(va, short int *)) = tempshort*sign; /* store it */
			}
		    else
		    	{
			int tempint = 0;
		    	while(isdigit(*string))		/* convert digits */
			    {
			    tempint *= 10;
			    tempint += (*(string++) - '0');
			    }
			*(va_arg(va, int *)) = (tempint*sign);
			}
		    count++;			    /* increment count of values extracted */
		    break;
		/*
		 * Unsigned int
		 */
		case 'u':
		    {
		    unsigned int tempint = 0;
		    if(!isdigit(*string))
		    	goto break_break;
		    while(isdigit(*string))     /* convert digits */
			{
			tempint *= 10;
			tempint += (*(string++) - '0');
			}
		    *(va_arg(va, unsigned int *)) = tempint;
		    }
		    count++;
		    break;
		/*
		 * Extract a string into the storage provided.
		 */
		case 's':
		    extptr = va_arg(va, char *);		/* get pointer string to extract to */
		    while( *string && !isspace(*string) && --maxextlen )
			*(extptr++) = *(string++);
		    *extptr = '\0';				/* terminate the string */
		    while( *string && !isspace(*string) )	/* eat any extra */
			string++;
		    count++;					/* one more item done */
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
			case 042:	/* ASCII double quote */
			    string++;
			    len = strcspn(string, "\"");
			    break;
			case 047:	/* ASCII single quote */
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
			case 042:	/* ASCII double quote */
			    string++;
			    len = strcspn(string, "\"");
			    break;
			case 047:	/* ASCII single quote */
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
		    while(isdigit(*string))		/* convert digits */
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
		    libppr_throw(EXCEPTION_BADUSAGE, "gu_sscanf", "unrecognized format '%c' in \"%s\"", *(pattern - 1), format);
		}
	    }

	else                                    /* Ordinary characters in */
	    {                                   /* pattern must match themselves. */
	    if(isspace(*pattern))               /* Whitespace */
		{                               /* matches any amount of */
		pattern++;                      /* whitespace. */
		while(isspace(*string))
		    string++;
		}
	    else                                /* everthing else */
		{                               /* must exactly match */
		if( *(pattern++) != *(string++) )   /* characters in string */
		    break;
		}
	    }
	} /* end of loop that lasts as long as pattern and string both last */
	break_break:

    va_end(va);

    return count;
    } /* end of gu_sscanf() */

/* end of file */
