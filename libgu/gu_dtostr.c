/*
** mouse:~ppr/src/libgu/gu_dtostr.c
** Copyright 1995--2003, Trinity College Computing Center.
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
** Last modified 14 May 2003.
*/

#include "config.h"
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

	if( n < 0 )			/* if n is negative */
		{
		neg = -1;		/* make a not of that fact */
		n *= -1;		/* and make n possitive */
		}

	/*
	** Break the floating point value into integers which
	** represent the whole and fractional parts.
	*/
	whole= (int)n;
	frac = (int)( ((n-(double)whole) + 0.00005) * 10000.0 );

	if(whole > 999999999 )
		return "<overflow>";

	*s-- = '\0';			/* terminate the string */

	for(x=0,z=0;x<4;x++)	/* print the fractional part */
		{
		y=frac % 10;
		if(y || z)			/* if digit non-zero or prev non-zero */
			{
			*s-- = y + '0';
			z=1;
			}
		frac/=10;
		}

	if(z)				/* if any place was non-zero, */
		*s-- = '.';		/* store a decimal point */

	do					/* do the whole part, digit by digit */
		{
		*s-- = (whole % 10) + '0';
		whole/=10;
		} while(whole);

	if(neg)				/* if negative, add a sign */
		*s-- = '-';

	return s+1;
	} /* end of gu_dtostr() */

/* end of file */
