/*
** mouse:~ppr/src/misc/tail_status.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 13 November 2001.
*/

#include "before_system.h"
#include <stdio.h>
#include "gu.h"
#include "global_defines.h"
#include "version.h"

gu_boolean print_function(char *p, void *extra)
    {
    if(!p)
    	{
    	printf("\n");
    	}
    else
    	{
    	printf("%s\n", p);
    	}
    fflush(stdout);
    return TRUE;
    }

int main(int argc, char *argv[])
    {
    printf("VERSION %s\n", SHORT_VERSION);
    tail_status(TRUE, TRUE, print_function, 60, (void*)NULL);
    return 0;
    } /* end of main */

/* end of file */

