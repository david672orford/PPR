/*
** mouse.trincoll.edu:~ppr/src/libgu/gu_dtostr.c
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
#include "gu.h"

/*
** Convert a double procision floating point number to a string.
**
** There will be between 1 and 4 digits after the decimal point.
** This function is used primarily for generating PostScript
** code.
**
** There is probably a way to do this with snprintf().
*/
const char *gu_dtostr(double n)
    {
    int whole;
    int frac;
    int neg = 0;
    static char stat_str[16];
    char *s = &stat_str[15];
    int x,y,z;

    if( n < 0 )		/* if n is negative */
	{
	neg = -1;	/* make a not of that fact */
	n *= -1;	/* and make n possitive */
	}

    /*
    ** Break the floating point value into integers which
    ** represent the whole and fractional parts.
    */
    whole= (int)n;
    frac = (int)( ((n-(double)whole) + 0.00005) * 10000.0 );

    if(whole > 999999999 )
	return "<overflow>";

    *s--=(char)NULL;        /* terminate the string */

    for(x=0,z=0;x<4;x++)    /* print the fractional part */
	{
	y=frac % 10;
	if(y || z)          /* if digit non-zero or prev non-zero */
	    {
	    *s-- = y + '0';
	    z=1;
	    }
	frac/=10;
	}

    if(z)               /* if any place was non-zero, */
	*s-- = '.';     /* store a decimal point */

    do                  /* do the whole part, digit by digit */
	{
	*s-- = (whole % 10) + '0';
	whole/=10;
	} while(whole);

    if(neg)             /* if negative, add a sign */
	*s-- = '-';

    return s+1;
    } /* end of gu_dtostr() */

/* end of file */

