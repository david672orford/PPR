/*
** mouse:~ppr/src/libgu/daemon.c
** Copyright 1995--2004, Trinity College Computing Center.
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
** Last modified 23 January 2004.
*/

/*! \file
	\brief become a daemon
*/

#include "before_system.h"
#include <signal.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include "gu.h"

/** become a daemon

This function is used to put the calling process into the background.  It
does this by forking, then parent half to exits.  It does other things too,
such as closing all open files and setting the session id.

*/
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
		gu_Throw("%s(): fork() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
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
		gu_Throw("%s(): setsid() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
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

