/*
** mouse:~ppr/src/libgu/gu_strcasecmp.c
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
** Last modified 30 June 2003.
*/

#include "before_system.h"
#include <stdio.h>
#include <ctype.h>
#include "gu.h"

/*
** Perform a case insensitive comparison.
** If the strings differ, return non-zero.
*/
int gu_strcasecmp(const char *s1, const char *s2)
	{
	int val;
	while((val = toupper(*s1) - toupper(*s2)) == 0)
		{
		if(*s1 == '\0' && *s2 == '\0')
			break;
		s1++; s2++;
		}
	return val;
	} /* end of gu_strcasecmp() */

/*
** Perform a case insensitive comparison for up to n characters.
** If the strings differ, return non-zero.
*/
int gu_strncasecmp(const char *s1, const char *s2, int n)
	{
	int val = 0;
	while(n-- > 0 && (val = toupper(*s1) - toupper(*s2)) == 0)
		{
		if(*s1 == '\0' && *s2 == '\0')
			break;
		s1++; s2++;
		}
	return val;
	} /* end of gu_strncasecmp() */

/* end of file */
