/*
** mouse:~ppr/src/misc/tcpbind.c
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
** Last modified 21 October 2005.
*/

/*! \file
	\brief bind to one or more addresses, change user id, and launch daemon

  This program creates one or more TCP sockets, binds them to specified 
  addresses, change to an specified user id, and exec()s a daemon
  passing it the sockets in the environment variable TCPBIND_SOCKETS.  The 
  deamon should detect this variable and rather than run in Inetd mode, 
  select() on the sockets, call accept(), and fork off copies of itself.

  This program is used in order to have this code, which has security 
  implications if run as root, in one place.  It is necessary to run
  it as root in order to bind to sockets below 1024.
	
*/

#include "config.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#ifndef INADDR_NONE
#define INADDR_NONE -1
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "pprd.h"
#include "pprd.auto_h"

#define MAX_LISTENERS 10
struct {
	int fd;
	const char *program;
	} listeners[MAX_LISTENERS];
int listeners_count = 0;

void listener_bind(const char bind_address_list[], const char program[])
	{
	int count;
	char *scratch, *p, *item;

	/* Process a comma-separated list of bind addresses. */
	for(count=0,p=scratch=gu_strdup(bind_address_list); (item = gu_strsep(&p, ",")); count++)
		{
		char *item2, *f1, *f2;
		int fd, port;
		struct sockaddr_in serv_addr;

		if(listeners_count == MAX_LISTENERS)
			gu_Throw("too many listing sockets");
	
		/* Each item is in the format [IP Address]:port. */
		item2 = item;
		if(!(f1 = gu_strsep(&item2, ":")) || !(f2 = gu_strsep(&item2, ":")))
			gu_Throw("syntax error in bind address list item %d %s", count+1, item);

		if(strspn(f2, "0123456789") == strlen(f2))
			{
			port = atoi(f2);
			}
		else
			{
			struct servent *service;
			if(!(service = getservbyname(f2, "tcp")))
				gu_Throw("unknown port: %s", f2);
			port = ntohs(service->s_port);
			}

		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		if(strlen(f1) != 0 && strcmp(f1, "*") != 0)
			{
			if((serv_addr.sin_addr.s_addr = inet_addr(f1)) == INADDR_NONE)
				gu_Throw("IP address \"%s\" is invalid.", f1);
			}
		else
			serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		serv_addr.sin_port = htons(port);

		if((fd = socket(AF_INET, SOCK_STREAM,0)) == -1)
			gu_Throw("%s() failed, errno=%d (%s)", "socket", errno, strerror(errno));
		
		/* Children don't need listening sockets. */
		gu_set_cloexec(fd);

		/* Make listening sockets non-blocking just in case select() indicates
		 * one is ready but accept() disagrees and wants to return EAGAIN.
		 */
		gu_nonblock(fd, TRUE);

		/* Try to avoid being locked out when restarting daemon: */
		{
		int one = 1;
		if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one)) == -1)
			gu_Throw("%s() failed, errno=%d (%s)", "setsockopt", errno, gu_strerror(errno));
		}

		if(bind(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
			{
			if(errno == EADDRINUSE)
				{
				debug("there is already a server listening on %s:%d", f1, port);
				close(fd);
				continue;
				}
			else
				gu_Throw("%s() failed, errno=%d (%s)", "bind", errno, strerror(errno));
			}

		if(listen(fd, 5) == -1)
			gu_Throw("%s() failed, errno=%d (%s)\n", "listen", errno, strerror(errno));

		listeners[listeners_count].fd = fd;
		listeners[listeners_count].program = program;
		listeners_count++;
		}

	gu_free(scratch);	
	} /* listener_bind() */

int listener_fd_set(int lastfd, fd_set *fdset)
	{
	int iii;
	for(iii=0; iii < listeners_count; iii++)
		{
		FD_SET(listeners[iii].fd, fdset);
		if(listeners[iii].fd > lastfd)
			lastfd = listeners[iii].fd;
		}
	return lastfd;
	}

/*
** This function is called by the daemon.  It never returns to main()
** in the daemon, but every time a connexion is received it forks a child,
** connects stdin, stdout, and stderr to the connexion, and returns to main()
** in the child.
*/
gu_boolean listener_hook(int selret, fd_set *fdset)
	{
	const char function[] = "listener_hook";
	int hit_count = 0;
	int iii;

	for(iii=0; iii < listeners_count && selret > 0; iii++)
		{
		int conn_fd;

		debug("listeners[%d] = {fd=%d,program[]=\"%s\"}", iii, listeners[iii].fd, listeners[iii].program);
		if(!FD_ISSET(listeners[iii].fd, fdset))
			continue;

		hit_count++;
		selret--;
		
		{
		struct sockaddr_in cli_addr;
		unsigned int clilen = sizeof(cli_addr);
		if((conn_fd = accept(listeners[iii].fd, (struct sockaddr *) &cli_addr, &clilen)) == -1)
			{
			/*if(errno != EAGAIN)*/
				debug("%s(): accept() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
			continue;
			}
		debug("%s(): connection to %s from %s", function, listeners[iii].program, inet_ntoa(cli_addr.sin_addr));
		}
	
		{
		pid_t pid;
		if((pid = fork()) == -1)		/* error, */
			{
			debug("%s(): fork() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
			}
		else if(pid == 0)				/* child */
			{
			/* Why do we have to do this? */
			/* On Linux it produces the message "Can't ignore signal CHLD, forcing to default." */
			/*signal_restarting(SIGCHLD, SIG_IGN);*/

			/* Connect connexion to stdin if it isn't already.
			 * We really on our caller to connect it to stdout too.
			 */
			if(conn_fd != 0)
				{
				dup2(conn_fd, 0);
				close(conn_fd);
				}
			dup2(0, 1);		/* stdout */
			dup2(0, 2);		/* stderr */

			execl(listeners[iii].program, listeners[iii].program, NULL);
			_exit(242);
			}
		else							/* parent */
			{
			debug("%s(): inet child %ld launched", function, (long)pid);
			}
		}

		/* parent or fork() failed */	
		close(conn_fd);
		}

	return hit_count > 0 ? TRUE : FALSE;
	} /* listener_hook() */

/* end of file */

