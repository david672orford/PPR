/*
** mouse:~ppr/src/libgu/daemon.c
** Copyright 1995--1999, Trinity College Computing Center.
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

/*
** This function is used to put the calling process into the
** background.  It does this by forking, then parent half
** to exits.  It does other things too, such as closing
** all open files and setting the session id.
*/

#include "before_system.h"
#include <signal.h>
#include <unistd.h>
#include <limits.h>
#include "gu.h"

void gu_daemon(mode_t daemon_umask)
    {
    const char function[] = "gu_daemon";

    /* Ignore group leader death.
       (I don't think the above description
       is quite acurate.)  */
    signal_interupting(SIGHUP, SIG_IGN);

    /* Override inherited umask: */
    umask(daemon_umask);

    /* If we have fork(), then drop into
       the background. */
    #ifdef HAVE_FORK
    {
    pid_t pid;

    if((pid = fork()) == -1)
	{
	fputs("FATAL:  can't fork daemon\n", stderr);
	libppr_throw(EXCEPTION_STARVED, function, "can't fork daemon");
	}

    if(pid)
	_exit(0);
    }
    #endif

    /* Dissasociate from controling terminal
       and become a process group leader.
       (Can we do that if we couldn't fork?) */
    if(setsid() == -1)
	{
	fputs("FATAL: can't set session id\n", stderr);
	libppr_throw(EXCEPTION_OTHER, function, "can't set session id");
	}

    /* Close all file descriptors. */
    {
    int fd, stopat;
    stopat = sysconf(_SC_OPEN_MAX);
    for(fd=0; fd < stopat; fd++)
	close(fd);
    }
    } /* end of daemon() */

/* end of file */

