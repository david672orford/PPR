/*
** mouse:~ppr/src/libgu/gu_strcasecmp.c
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
#include <stdio.h>
#include <ctype.h>
#include "gu.h"

/*
** Perform a case insensitive comparison.
** If the strings differ, return non-zero.
*/
int gu_strcasecmp(const char *s1, const char *s2)
	{
	while( (toupper(*s1) == toupper(*s2)) )
		{
		if(*s1 == '\0' && *s2 == '\0')
			return 0;
		else
			{ s1++; s2++; }
		}
	return -1;
	} /* end of gu_strcasecmp() */

/*
** Perform a case insensitive comparison for up to n characters.
** If the strings differ, return non-zero.
*/
int gu_strncasecmp(const char *s1, const char *s2, int n)
	{
	while( n > 0 && (toupper(*(s1++)) == toupper(*(s2++))) )
		n--;
	return n;
	} /* end of gu_strncasecmp() */

/* end of file */
