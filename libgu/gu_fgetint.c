/*
** mouse:~ppr/src/libgu/gu_fgetint.c
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
	\brief read a decimal integer

*/

#include "config.h"
#include <stdio.h>
#include <string.h>
#include "gu.h"

int gu_fgetint(FILE *input)
	{
	int sign = 1;				/* 1 or -1 */
	int n = 0;
	int c = fgetc(input);
	if(c == '-')				/* if a minus sign is found, */
		{
		sign = -1;				/* set a flag */
		c = fgetc(input);
		}
	for( ; c != EOF && gu_ascii_isdigit(c); c = fgetc(input))
		{
		n *= 10;
		n += gu_ascii_digit_value(c);
		}
	if(c != EOF)
		ungetc(c, input);
	return sign * n;
	} /* end of gu_fgetint() */

/* end of file */
