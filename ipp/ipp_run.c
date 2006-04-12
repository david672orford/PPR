/*
** mouse:~ppr/src/ipp/ipp_run.c
** Copyright 1995--2006, Trinity College Computing Center.
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
** Last modified 12 April 2006.
*/

/*
 * This is PPR's IPP (Internet Printer Protocol) server.
 * This module contains routines for running programs
 * and capturing their output.
 */

#include "config.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "ipp_constants.h"
#include "ipp_utils.h"
#include "queueinfo.h"
#include "ipp.h"

static void sigchld_handler(int sig)
	{
	}

/** Launch a program and capture its stdout.
 */
FILE *gu_popen(char *argv[], const char type[])
	{
	const char function[] = "gu_popen";
	pid_t pid;
	int fds[2];

	signal_restarting(SIGCHLD, sigchld_handler);

	if(pipe(fds) == -1)
		gu_Throw(_("%s() failed, errno=%d (%s)"), "pipe", errno, strerror(errno));

	if((pid = fork()) == -1)
		gu_Throw(_("%s() failed, errno=%d (%s)"), "fork", errno, strerror(errno));

	if(pid == 0)				/* child */
		{
		if(*type == 'r')
			{
			close(fds[0]);
			if(fds[1] != 1)
				{
				dup2(fds[1], 1);
				close(fds[1]);
				}
			}
		else if(*type == 'w')
			{
			close(fds[1]);
			if(fds[0] != 0)
				{
				dup2(fds[0], 0);
				close(fds[0]);
				}
			}
		execv(argv[0], argv);
		_exit(255);
		}

	/* parent */
	if(*type == 'r')
		{
		close(fds[1]);
		return fdopen(fds[0], type);
		}
	else if(*type == 'w')
		{
		close(fds[0]);
		return fdopen(fds[1], type);
		}
	else
		{
		gu_Throw("%s(): invalid type: %s", function, type);
		}
	} /* gu_popen() */

int gu_pclose(FILE *f)
	{
	int status;
	fclose(f);
	if(wait(&status) == -1)
		gu_Throw("%s() failed, errno=%d (%s)", "wait", errno, strerror(errno));
	if(!WIFEXITED(status))
		return -1;
	else
		return WEXITSTATUS(status);
	} /* gu_pclose() */

int run(char command[], ...)
	{
	va_list va;
	#define MAX_ARGV 16 
	char *argv[MAX_ARGV];
	int iii;
	FILE *f;
	char *line = NULL;
	int line_space = 80;

	argv[0] = command;
	fprintf(stderr, " $ %s", command);
	
	va_start(va, command);
	for(iii=1; iii < MAX_ARGV; iii++)
		{
		if(!(argv[iii] = va_arg(va, char*)))
			break;
		fprintf(stderr, " %s", argv[iii]);
		}
	va_end(va);
	fprintf(stderr, "\n");

	f = gu_popen(argv, "r");
	while((line = gu_getline(line, &line_space, f)))
		{
		fprintf(stderr, " %s\n", line);
		}

	return gu_pclose(f);	
	} /* run() */

/* end of file */
