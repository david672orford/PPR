/*
** Copyright 1995--2003, Trinity College Computing Center.
** Written by David Chappell.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** * Redistributions of source code must retain the above copyright notice,
** this list of conditions and the following disclaimer.
**
** * Redistributions in binary form must reproduce the above copyright
** notice, this list of conditions and the following disclaimer in the
** documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
**
** Last modified 18 February 2003.
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
**
** If uid is not -1, then the child sets its user id to the
** indicated value.  This feature may be removed at some point.
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
	if(uid != -1)
	    {
	    if(setuid(0) == -1)
		{
		fprintf(stderr, "%s(): setuid(0) failed, errno=%d (%s)\n", function, errno, gu_strerror(errno));
		exit(242);
		}
	    if(setuid(uid) == -1)
		{
		fprintf(stderr, "%s(): setuid(%ld) failed, errno=%d (%s)\n", function, (long)uid, errno, gu_strerror(errno));
		exit(242);
		}
	    if(setuid(0) != -1)
	    	{
	    	fprintf(stderr, "%s(): setuid(0) didn't fail!");
	    	exit(242);
	    	}
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
