/*
** mouse:~ppr/src/libgu/gu_strerror.c
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

#include <string.h>
#include <errno.h>
#include "gu.h"
#undef strerror

/*
** There is a bug in Linux strerror() which makes it set errno
** to 2 the first time it is called.
*/
char *gu_strerror(int n)
	{
	int saved_errno = errno;
	char *s = strerror(n);
	errno = saved_errno;
	return s;
	}

/* end of file */

