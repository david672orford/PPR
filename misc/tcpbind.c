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
** Last modified 20 October 2005.
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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifndef INADDR_NONE
#define INADDR_NONE -1
#endif
#include "gu.h"
#include "global_defines.h"

const char myname[] = "tcpbind";

int main(int argc, char *argv[])
	{
	char *bind_addresses, *username, *pidname;
	gu_boolean option_foreground = FALSE;

	if(getuid() != geteuid())
		{
		fprintf(stderr, "%s: should not be setuid!\n", myname);
		exit(1);
		}

	if(argc >= 2 && strcmp(argv[1], "--foreground") == 0)
		{
		option_foreground = TRUE;
		putenv("TCPBIND_FOREGROUND=1");
		argv++;
		argc--;
		}
	
	if(argc < 5)
		{
		fprintf(stderr, "%s: Usage: tcpbind [--foreground] <addresses>:<port>,... <username> <pidname> <daemon> ...\n", myname);
		exit(1);
		}
	bind_addresses = argv[1];
	username = argv[2];
	pidname = argv[3];

	/* We must close file descriptors 3 and up now because we can't later as we 
	 * would clobber our listening sockets.
	 */
	gu_daemon_close_fds();
	
	/* Process a comma-separated list of bind addresses. */
	{
	int count;
	char *p, *item;
	void *list = gu_pcs_new_cstr("TCPBIND_SOCKETS=");
	for(count=0, p=bind_addresses; (item = gu_strsep(&p, ",")); count++)
		{
		char *item2, *f1, *f2;
		int fd, port;
		struct sockaddr_in serv_addr;

		/* Each item is in the format [IP Address:]port. */
		item2 = item;
		if(!(f1 = gu_strsep(&item2, ":")) || !(f2 = gu_strsep(&item2, ":")))
			{
			fprintf(stderr, "%s: syntax error in address:port list item %d %s: \n", myname, count+1, item);
			exit(1);
			}

		if((port = atoi(f2)) == 0)
			{
			fprintf(stderr, "%s: invalid port: %s\n", myname, f2);
			exit(1);
			}

		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		if(strlen(f1) != 0 && strcmp(f1, "*") != 0)
			{
			if((serv_addr.sin_addr.s_addr = inet_addr(f1)) == INADDR_NONE)
				{
				fprintf(stderr, "%s: IP address \"%s\" is invalid.\n", myname, f1);
				exit(1);
				}
			}
		else
			serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		serv_addr.sin_port = htons(port);

		if((fd = socket(AF_INET, SOCK_STREAM,0)) == -1)
			{
			fprintf(stderr, "%s: socket() failed, errno=%d (%s)\n", myname, errno, strerror(errno));
			exit(1);
			}
		
		/* Try to avoid being locked out when restarting daemon: */
		{
		int one = 1;
		if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one)) == -1)
			{
			fprintf(stderr, "%s: setsockopt() failed, errno=%d (%s)\n", myname, errno, gu_strerror(errno));
			exit(1);
			}
		}

		if(bind(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
			{
			if(errno == EADDRINUSE)
				fprintf(stderr, "%s: there is already a server listening on %s:%d\n", myname, f1, port);
			else
				fprintf(stderr, "%s: bind() failed, errno=%d (%s)\n", myname, errno, strerror(errno));
			exit(1);
			}

		if(listen(fd, 5) == -1)
			{
			fprintf(stderr, "%s: listen() failed, errno=%d (%s)\n", myname, errno, strerror(errno));
			exit(1);
			}

		if(count > 0)
			gu_pcs_append_char(&list, ',');
		gu_pcs_append_sprintf(&list, "%d=%d", fd, port);
		}	
	putenv(gu_pcs_free_keep_cstr(&list));	
	}

	/* Become the specified user if not already. */
	{
	int ret;
	if((ret = renounce_root_privs(argv[4], username, NULL)) != 0)
		return ret;
	}

	/* Build the name of our PID file, store it in the environment,
	 * and fork off the daemon process. */
	{
	char *p;
	gu_asprintf(&p, "TCPBIND_PIDFILE=%s/%s.pid", RUNDIR, pidname);
	putenv(p);
	gu_daemon(myname, option_foreground, UNIX_002, p + strlen("TCPBIND_PIDFILE="));
	}

	execv(argv[4], &argv[4]);
	fprintf(stderr, "%s: execv() failed, errno=%d (%s)\n", myname, errno, strerror(errno));
	exit(1);
	} /* end of main() */

/* end of file */
