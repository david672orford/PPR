/*
** mouse:~ppr/src/templates/module.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 7 December 2000.
*/

#include "before_system.h"
#include <string.h>
#include "gu.h"

/*
** This function was inspired by Trio.	Trio's version has assertions,
** we don't have them yet.
*/
char *gu_StrAppendMax(char *target, size_t max, const char *source)
	{
	size_t len = strlen(target);
	max -= len;
	if(max > 0)
		strncpy(target + len, source, max - 1);
	target[len + max - 1] = '\0';
	return target; 
	}

/* end of file */
