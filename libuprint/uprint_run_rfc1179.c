/*
** mouse:~ppr/src/templates/module.c
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
** Last modified 7 May 2003.
*/

#include "before_system.h"
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
#include "uprint.h"
#include "uprint_private.h"

/*
** This function runs the setuid-root program uprint_rfc1179 which does all of our 
** priviledged socket operations.
*/
int uprint_run_rfc1179(const char exepath[], const char *const argv[])
	{
	const char function[] = "uprint_run_rfc1179";
	int pipefds[2];
	pid_t pid;

	if(pipe(pipefds) == -1)
		{
		uprint_errno = UPE_INTERNAL;
		uprint_error_callback(_("%s(): pipe() failed, errno=%d (%s)"), function, errno, gu_strerror(errno));
		return -1;
		}

	if((pid = fork()) == -1)	/* failed */
		{
		uprint_errno = UPE_FORK;
		uprint_error_callback("%s(): fork() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
		close(pipefds[0]);
		close(pipefds[1]);
		return -1;
		}
	else if(pid == 0)			/* child */
		{
		uid_t uid;

		close(pipefds[0]);
		if(pipefds[1] != 2)
			{
			dup2(pipefds[1], 2);
			close(pipefds[1]);
			}

		uid = getuid();
		if(setreuid(uid, uid) == -1)
			{
			uprint_errno = UPE_SETUID;
			uprint_error_callback(_("%s(): setreuid(%ld, %ld) failed, errno=%d (%s)"), function, (long)uid, (long)uid, errno, gu_strerror(errno));
			exit(241);
			}

		execv(exepath, (char *const *)argv);	/* <-- it's OK, execv() won't modify argv[] */

		uprint_errno = UPE_EXEC;
		uprint_error_callback(_("%s(): execv(\"%s\", ...) failed, errno=%d (%s)\n"), function, exepath, errno, gu_strerror(errno));
		exit(242);
		}
	else						/* parent */
		{
		FILE *reader;
		char *line = NULL;
		int line_available = 80;
		char *p;
		int wstatus;

		close(pipefds[1]);
		if(!(reader = fdopen(pipefds[0], "r")))
			{
			uprint_errno = UPE_INTERNAL;
			uprint_error_callback(_("%s(): fdopen(\"%d\", \"%s\") failed, errno=%d (%s)"), function, pipefds[0], "r", errno, gu_strerror(errno));
			}

		while((line = gu_getline(line, &line_available, reader)))
			{
			if((p = lmatchp(line, "uprint_error_callback:")))
				{
				uprint_error_callback(p);
				}
			else if((p = lmatchp(line, "uprint_errno:")))
				{
				uprint_errno = atoi(p);
				}
			else
				{
				fprintf(stderr, "*** %s\n", line);
				}
			}

		if(wait(&wstatus) == -1)
			{
			uprint_errno = UPE_WAIT;
			uprint_error_callback(_("%s(): wait() failed, errno=%d (%s)"), function, errno, gu_strerror(errno));
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
			if(WEXITSTATUS(wstatus) != 0)
				{
				return -1;
				}
			else
				{
				return 0;
				}
			}
		} /* parent */

	/* NOTREACHED */
	return -1;
	} /* end of uprint_run_rfc1179() */

/* end of file */
