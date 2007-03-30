/*
** mouse:~ppr/src/ipp/ippd_run.c
** Copyright 1995--2007, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 30 March 2007.
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
#include "ippd.h"

static void sigchld_handler(int sig)
	{
	}

/** Launch a program and capture its stdout.
 *
 * If this fails it throws an exception, so there is no need to test the
 * returned FILE object.
 */
FILE *gu_popen(const char *argv[], const char type[])
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
		execv(argv[0], (char**)argv);
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

int runv(const char command[], const char *argv[])
	{
	FILE *f;
	char *line = NULL;
	int line_space = 80;
	f = gu_popen(argv, "r");
	while((line = gu_getline(line, &line_space, f)))
		fprintf(stderr, " %s\n", line);
	return gu_pclose(f);	
	} /* runv() */

int runl(const char command[], ...)
	{
	va_list va;
	#define MAX_ARGV 16 
	const char *argv[MAX_ARGV];
	int iii;

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

	return runv(command, argv);	
	}

/* end of file */
