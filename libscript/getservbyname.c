/*
** mouse:~ppr/src/libscript/getservbyname.c
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
#include <netdb.h>
#include <netinet/in.h>

int main(int argc, char *argv[])
    {
    struct servent *se;

    if(argc != 3)
    	{
	fprintf(stderr, "Usage: %s <servicename> <protocol>\n", argv[0]);
	return 10;
    	}

    if(!(se = getservbyname(argv[1], argv[2])))
    	{
    	printf("-1\n");
    	return 1;
    	}

    printf("%d\n", ntohs(se->s_port));
    return 0;
    }

/* end of file */

