/*
** mouse:~ppr/src/libgu/daemon.c
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
** Last modified 23 February 2006.
*/

/*! \file
	\brief become a daemon
*/

#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include "gu.h"

/** close all file descriptors beyond stderr (which is 2).
 */
void gu_daemon_close_fds(void)
	{
	int fd, stopat;
	stopat = sysconf(_SC_OPEN_MAX);
	for(fd=3; fd < stopat; fd++)
		close(fd);
	}

/** become a daemon

This function is used to put the calling process into the background.  It
does this by forking, then the parent half exits.  It does other things too,
such as closing all open files and setting the session id.

*/
void gu_daemon(const char progname[], gu_boolean standalone, mode_t daemon_umask, const char lockfile[])
	{
	int pid_fd;

	/* Override inherited umask: */
	umask(daemon_umask);

	/* We open the lock file without O_EXCL because we use locks to determine
	 * if another daemon still has it open and locked.
	 */
	if((pid_fd = open(lockfile, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR)) == -1)
		{
		fprintf(stderr, "%s: can't create lock file \"%s\", errno=%d (%s)\n", progname, lockfile, errno, strerror(errno));
		exit(1);
		}

	/* This lock is temporary.  The child must create its own. */
	if(gu_lock_exclusive(pid_fd, FALSE))
		{
		fprintf(stderr, "%s: daemon already running\n", progname);
		exit(1);
		}

	if(!standalone)
		{
		pid_t pid;

		/* Ignore group leader death.
		   (I don't think the above description
		   is quite accurate.)  */
		signal_interupting(SIGHUP, SIG_IGN);
	
		/* If we have fork(), then drop into
		   the background. */
		#ifdef HAVE_FORK
		if((pid = fork()) == -1)
			{
			fprintf(stderr, "%s: can't fork daemon\n", progname);
			exit(1);
			}
	
		if(pid)			/* if parent, */
			{
			sleep(1);	/* time for child to request lock */
			_exit(0);
			}
		#endif
		}

	/* child from here on */

	/* Write the child PID into the previously opened lock file. */
	{
	char temp[16];
	gu_snprintf(temp, sizeof(temp), "%ld\n", (long)getpid());
	write(pid_fd, temp, strlen(temp));
	/* Don't pass the lock file descriptor onto our children. */
	gu_set_cloexec(pid_fd);
	/* Grab the lock as soon as the parent exits. */
	gu_lock_exclusive(pid_fd, TRUE);
	}

	if(!standalone)
		{	
		/* Dissasociate from controling terminal and become a process 
		 * group leader. */
		if(setsid() == -1)
			{
			fprintf(stderr, "%s: can't set session id\n", progname);
			exit(1);
			}
	
		/* Close stdin, stdout, and stderr.  Be careful
		 * not to close the lock file. */
		{
		int fd;
		for(fd=0; fd < 3; fd++)
			{
			if(fd != pid_fd)
				close(fd);
			}
		}
		}
	} /* end of gu_daemon() */

/* end of file */

