/*
** mouse:~ppr/src/libppr/signal_sh.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last revised 25 November 1999.
*/

#include <stdio.h>
#include <signal.h>

/*
** This program creates signal.sh which is an include file for
** shell scripts.
*/
int main(int argc, char *argv[])
    {
    printf(	"#\n"
		"# This file was automatically generated.\n"
    		"#\n");
    printf("SIGHUP=%d\n", SIGHUP);
    printf("SIGINT=%d\n", SIGINT);
    printf("SIGQUIT=%d\n", SIGQUIT);
    printf("SIGKILL=%d\n", SIGKILL);
    printf("SIGTERM=%d\n", SIGTERM);
    printf("SIGUSR1=%d\n", SIGUSR1);
    printf("SIGUSR2=%d\n", SIGUSR2);
    printf("SIGTTIN=%d\n", SIGTTIN);
    printf("SIGTTOU=%d\n", SIGTTOU);
    return 0;
    }

/* end of file */

