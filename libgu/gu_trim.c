/*
** mouse:~ppr/src/libgu/gu_trim.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 30 March 2001.
*/

#include "before_system.h"
#include <string.h>
#include <ctype.h>
#include "gu.h"
#include "global_defines.h"

void gu_trim_whitespace_right(char *p)
	{
	int len = strlen(p);
	while(--len >= 0 && isspace(p[len]))
		p[len] = '\0';
	}

/* end of file */
