/*
** mouse:~ppr/src/libscript/getgrnam.c
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
#include <grp.h>
#include <sys/types.h>

int main(int argc, char *argv[])
    {
    struct group *gr;

    if(argc != 2)
    	{
	fprintf(stderr, "Usage: %s <username>\n", argv[0]);
	return 10;
    	}

    if(!(gr = getgrnam(argv[1])))
    	{
    	printf("-1\n");
    	return 1;
    	}

    printf("%ld\n", (long)gr->gr_gid);
    return 0;
    }

/* end of file */

