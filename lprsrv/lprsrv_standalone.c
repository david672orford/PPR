/*
** mouse:~ppr/src/lprsrv/lprsrv_standalone.c
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
** Last modified 20 October 2005.
*/

#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <netdb.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "rfc1179.h"
#include "lprsrv.h"

/*
** This module contains functions which lprsrv needs to run in
** standalone mode (without Inetd).
*/

static char *tcpbind_pidfile = NULL;

/*
** This function is called in the daemon whenever one of the
** children launched to service a connexion exits.
*/
static void reapchild(int sig)
	{
	int pid, stat;

	while((pid = waitpid((pid_t)-1, &stat, WNOHANG)) > (pid_t)0)
		{
		DODEBUG_STANDALONE(("child %ld terminated", (long)pid));
		}
	} /* end of reapchild() */

/*
** This function is called by the daemon.  It never returns to main()
** in the daemon, but every time a connexion is received it forks a child,
** connects stdin, stdout, and stderr to the connexion, and returns to main()
** in the child.
*/
void standalone_accept(char *tcpbind_sockets, char *pidfile)
	{
	const char function[] = "standalone_accept";
	#define MAX_FDS 32 
	int fds[MAX_FDS];
	int fdcount;
	int maxfd = -1;

	signal_restarting(SIGCHLD, reapchild);
	tcpbind_pidfile = pidfile;

	{
	char *p, *item;
	for(fdcount=0, p=tcpbind_sockets; (item = gu_strsep(&p, ",")); fdcount++)
		{
		char *f1, *f2;

		if(fdcount >= MAX_FDS)
			gu_Throw("listening on too many ports");

		/* Each item is in the format fd=socket. */
		if(!(f1 = gu_strsep(&item, "=")) || !(f2 = gu_strsep(&item, "=")))
			gu_Throw(X_("can't parse TCPBIND_SOCKETS item %d"), fdcount+1);
		if((fds[fdcount] = atoi(f1)) <= 0)
			gu_Throw(X_("can't parse socket number: %s"), f1);

		/* If this is the highest numbered FD yet, note it for first argument of select(). */
		if(fds[fdcount] > maxfd)
			maxfd = fds[fdcount];

		/* Children don't need listening sockets. */
		gu_set_cloexec(fds[fdcount]);

		/* Make listening sockets non-blocking just in case select() indicates
		 * one is ready but accept() disagrees and wants to return EAGAIN.
		 */
		gu_nonblock(fds[fdcount], TRUE);
		}
	}

	for( ; ; )							/* loop until killed */
		{
		int conn_fd;
		pid_t pid;
		fd_set fdset;
		int iii;
		int selret;

		FD_ZERO(&fdset);
		for(iii=0; iii < fdcount; iii++)
			FD_SET(fds[iii], &fdset);
	
		DODEBUG_STANDALONE(("%s(): waiting for connexion", function));
		while((selret = select(maxfd+1, &fdset, NULL, NULL, NULL)) == -1)
			{
			if(errno == EINTR)
				continue;
			else
				gu_Throw(_("%s(): %s() failed, errno=%d (%s)"), function, "select", errno, strerror(errno));
			}
		for(iii=0; selret--; )
			{
			if(!FD_ISSET(fds[iii], &fdset))
				continue;

			{
			struct sockaddr_in cli_addr;
			unsigned int clilen = sizeof(cli_addr);
			if((conn_fd = accept(fds[iii], (struct sockaddr *) &cli_addr, &clilen)) == -1)
				{
				/*if(errno != EAGAIN)*/
					debug("%s(): accept() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
				continue;
				}
			DODEBUG_STANDALONE(("%s(): connection request from %s", function, inet_ntoa(cli_addr.sin_addr)));
			}
	
			if((pid = fork()) == -1)		/* error, */
				{
				debug("%s(): fork() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
				}
			else if(pid == 0)				/* child */
				{
				/* Child shouldn't delete .pid file if an exception is thrown. */
				tcpbind_pidfile = NULL;

				/* Why do we have to do this? */
				signal_restarting(SIGCHLD, SIG_IGN);

				/* Child does not need the listening FD's. */
				for(iii=0; iii < fdcount; iii++)
					close(fds[iii]);

				/* Connect connexion to stdin if it isn't already.
				 * We really on our caller to connect it to stdout too.
				*/
				if(conn_fd != 0)
					{
					dup2(conn_fd, 0);
					close(conn_fd);
					}
				return;
				}
			else							/* parent */
				{
				DODEBUG_STANDALONE(("%s(): child %ld launched", function, (long)pid));
				}
	
			close(conn_fd);
			}
		}
	} /* end of main_loop() */

void standalone_shutdown(void)
	{
	if(tcpbind_pidfile)
		unlink(tcpbind_pidfile);
	}

/* end of file */

