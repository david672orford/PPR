/*
** mouse:~ppr/src/libuprint/uprint_run.c
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
** Last modified 24 June 1999.
*/

#include "before_system.h"
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"

#include "uprint.h"
#include "uprint_private.h"

/*
** Run a command.  If the command doesn't execute normally
** (such as if it can't be found or it core dumps) then
** uprint_errno is set and this function returns -1.  If
** the command runs to completion, this function will
** return the command's exit code.
*/
int uprint_run(uid_t uid, const char *exepath, const char *const argv[])
    {
    const char function[] = "uprint_run";
    pid_t pid;
    int wstatus;

    #ifdef DEBUG
    {
    int x;
    printf("DEBUG: uprint_run(exepath=\"%s\", *argv[]={", exepath);
    for(x=0; argv[x] != (const char *)NULL; x++)
	{
	if(x) printf(", ");
	printf("\"%s\"", argv[x]);
	}
    printf("})\n");
    }
    #endif

    /*
    ** Run it.
    */
    if((pid = fork()) == -1)	/* failed */
	{
	uprint_errno = UPE_FORK;
    	uprint_error_callback("%s(): fork() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
	return -1;
	}
    else if(pid == 0)		/* child */
    	{
	if(setuid(0) == -1)
	    {
	    fprintf(stderr, "%s(): setuid(0) failed, errno=%d (%s)\n", function, errno, gu_strerror(errno));
	    exit(242);
	    }
	if(setuid(uid) == -1)
	    {
	    fprintf(stderr, "%s(): seteuid(%ld) failed, errno=%d (%s)\n", function, (long)uid, errno, gu_strerror(errno));
	    exit(242);
	    }

	execv(exepath, (char *const *)argv); /* it's OK, execv() won't modify it */

	fprintf(stderr, "%s(): execv() of \"%s\" failed, errno=%d (%s)\n", function, exepath, errno, gu_strerror(errno));
	exit(242);
	}
    else			/* parent */
	{
    	if(wait(&wstatus) == -1)
    	    {
	    uprint_errno = UPE_WAIT;
    	    uprint_error_callback("%s(): wait() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
	    return -1;
    	    }
	/* If it died due to a signal, */
	if(! WIFEXITED(wstatus))
	    {
	    if(WCOREDUMP(wstatus))
		{
		uprint_errno = UPE_CORE;
		uprint_error_callback("%s(): Child \"%s\" dumped core", function, exepath);
		}
	    else
		{
		uprint_errno = UPE_KILLED;
		uprint_error_callback("%s(): Child \"%s\" was killed", function, exepath);
		}
	    return -1;
	    }

	/* If it exited deliberately, */
	else
	    {
	    /* If the special code used above, */
	    if(WEXITSTATUS(wstatus) == 242)
		{
		uprint_errno = UPE_EXEC;
		uprint_error_callback("%s(): failed to run \"%s\" under uid %ld", function, argv[0], (long)uid);
		return -1;
		}
	    /* Any other exit code, */
	    else
		{
		uprint_errno = UPE_CHILD;
		/* The program can presumable explain its own failure: */
		/* uprint_error_callback("%s exited with code %d", argv[0], WEXITSTATUS(wstatus)); */
		return WEXITSTATUS(wstatus);
		}
	    }
	} /* parent */

    /* NOTREACHED */
    return -1;
    } /* end of uprint_run() */

/* end of file */
