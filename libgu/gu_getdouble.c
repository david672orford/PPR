/*
** mouse:~ppr/src/libgu/gu_getdouble.c
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
** Last modified 22 November 2000.
*/

#include "before_system.h"
#include <ctype.h>
#include "gu.h"

/*
** Read text and return a double precision floating point number.
**
** We must use this because sscanf() does not recognize "1" or "42" as
** valid floating point numbers.
**
** The above doesn't seem very likely.  Maybe there is another reason.
** This function is similiar to strtod().  The difference is that this
** can only work on simply floating point values such as are used
** in PostScript DSC comments.  It is not affected by locals, which
** could be good under certain circumstances.
*/
double gu_getdouble(const char *s)
	{
	double sign=1;
	double t=0;
	double place=0.1;

	while(*s==' ' || *s=='\t')		/* eat leading space */
		s++;

	if( *s == '+' )					/* ignore plus sign */
		{
		s++;
		}
	else if( *s == '-' )			/* if minus sign, reverse sign */
		{
		sign=-1;
		s++;
		}

	while( isdigit(*s) )				/* read whole part */
		t=t*10.0 + (*(s++) - '0');

	if(*s == '.')						/* if decimal part */
		{
		s++;							/* skip decimal point */

		while( isdigit(*s) )			/* read fractional part */
			{
			t+=( place * (*(s++) - '0') );
			place/=10;
			}
		}

	return t * sign;
	} /* end of gu_getdouble() */

/* end of file */

