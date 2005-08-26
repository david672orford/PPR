/*
** mouse:~ppr/src/lprsrv/uprint_run.c
** Copyright 1995--2005, Trinity College Computing Center.
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
** Last modified 9 August 2005.
*/

#include "config.h"
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "rfc1179.h"
#include "lprsrv.h"

/*
** Run a command.  If the command doesn't execute normally
** (such as if it can't be found or it core dumps) then
** uprint_errno is set and this function returns -1.  If
** the command runs to completion, this function will
** return the command's exit code.
**
** If uid is not -1, then the child sets its user id to the
** indicated value.  The same goes for gid.
*/
int uprint_run(const char *exepath, const char *const argv[])
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
		uprint_error_callback(_("%s(): %s() failed, errno=%d (%s)"), function, "fork", errno, gu_strerror(errno));
		return -1;
		}
	else if(pid == 0)			/* child */
		{
		execv(exepath, (char *const *)argv); /* it's OK, execv() won't modify it */

		fprintf(stderr, _("%s(): %s(\"%s\", ...) failed, errno=%d (%s)\n"), function, "execv", exepath, errno, gu_strerror(errno));
		fputc('\n',stderr);
		exit(242);
		}

	else						/* parent */
		{
		if(wait(&wstatus) == -1)
			{
			uprint_errno = UPE_WAIT;
			uprint_error_callback(_("%s(): %s() failed, errno=%d (%s)"), function, "wait", errno, gu_strerror(errno));
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
			uprint_errno = UPE_CHILD;
			/* The program has presumably already explained its failure.  We needn't, so this
			   is commented out. */
			/* uprint_error_callback("%s exited with code %d", argv[0], WEXITSTATUS(wstatus)); */
			return WEXITSTATUS(wstatus);
			}
		} /* parent */

	/* NOTREACHED */
	return -1;
	} /* end of uprint_run() */

/* end of file */
