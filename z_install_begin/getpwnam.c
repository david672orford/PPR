/*
** mouse:~ppr/src/libscript/getpwnam.c
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
** Last modified 30 June 2000.
*/

#include <stdio.h>
#include <pwd.h>
#include <sys/types.h>

int main(int argc, char *argv[])
	{
	struct passwd *pw;

	if(argc != 2)
		{
		fprintf(stderr, "Usage: %s <username>\n", argv[0]);
		return 10;
		}

	if(!(pw = getpwnam(argv[1])))
		{
		printf("-1\n");
		return 1;
		}

	printf("%ld\n", (long)pw->pw_uid);
	return 0;
	}

/* end of file */

