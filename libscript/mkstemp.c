/*
** mouse:~ppr/src/libscripts/mkstemp.c
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
** Last modified 1 May 2001.
*/

#include "config.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "gu.h"
#include "global_defines.h"


int main(int argc, char *argv[])
	{
	if(argc != 2)
		{
		fprintf(stderr, "usage: mkstemp /tmp/fileXXXXXX\n");
		return 10;
		}

	if(mkstemp(argv[1]) == -1)
		{
		fprintf(stderr, "mkstemp: mkstemp(\"%s\") failed, errno=%d, (%s)\n", argv[1], errno, gu_strerror(errno));
		return 1;
		}
		
	printf("%s\n", argv[1]);
	return 0;
	}

/* end of file */
