/*
** mouse:~ppr/src/templates/gu_run.c
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
** Last modified 10 March 2003.
*/

/** \file */

#include "before_system.h"
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdarg.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

/** Run a command

This function provides a simple way to run a command and wait for it to
complete.  This is useful for commands which need to run some other command
to do a part of the work.  It isn't hard to write such code, but with proper
testing of status returned by wait() it can get a little long.	Also,
centralizing it in this function will allow us to use spawn() in the future.

*/
int gu_runl(const char myname[], FILE *errors, const char *progname, ...)
	{
	pid_t pid;
	int wstat;

	if((pid = fork()) == -1)
		{
		fprintf(errors, "%s: fork() failed, errno=%d (%s)\n", myname, errno, gu_strerror(errno));
		return -1;
		}
	else if(pid == 0)	/* child */
		{
		va_list va;
		const char **args;
		int args_space = 10;
		int i = 0;
		char *p;

		args = gu_alloc(args_space, sizeof(const char*));
		args[i++] = progname;

		va_start(va, progname);
		do	{
			p = va_arg(va, char*);
			if((i + 1) > args_space)
				{
				args_space += 10;
				args = gu_realloc(args, args_space, sizeof(const char*));
				}
			args[i++] = p;
			} while(p);
		va_end(va);

		execv(progname, (char**)args);
		fprintf(errors, "%s: execv(\"%s\", *args) failed, errno=%d (%s)\n", myname, progname, errno, gu_strerror(errno));
		_exit(242);
		}

	if(wait(&wstat) == -1)
		{
		fprintf(errors, "%s: wait() failed, errno=%d (%s)\n", myname, errno, gu_strerror(errno));
		return -1;
		}

	if(!WIFEXITED(wstat) || WEXITSTATUS(wstat) != 0)
		{
		fprintf(errors, "%s: %s failed.\n", myname, progname);
		return -1;
		}

	return 0;
	}

/* end of file */
