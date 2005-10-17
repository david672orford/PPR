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
** Last modified 2 September 2005.
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
#include "gu.h"
#include "global_defines.h"

const char myname[] = "tcpbind";

int main(int argc, char *argv[])
	{
	char *bind_addresses, *username;
	int count;
	void *list = gu_pcs_new_cstr("TCPBIND_SOCKETS=");

	if(getuid() != geteuid())
		{
		fprintf(stderr, "%s: should not be setuid!\n", myname);
		return 1;
		}

	if(argc < 4)
		{
		fprintf(stderr, "%s: Usage: tcpbind <address> <username> <daemon> ...\n", myname);
		return 1;
		}
	bind_addresses = argv[1];
	username = argv[2];

	{
	char *p, *item;
	for(count=0, p=bind_addresses; (item = gu_strsep(&p, ",")); count++)
		{
		char *f1, *f2;
		int port, fd;
		struct sockaddr_in serv_addr;
		if(!(f1 = gu_strsep(&item, ":")) || !(f2 = gu_strsep(&item, ":")))
			{
			fprintf(stderr, "%s: can't parse address %d\n", myname, count+1);
			return 1;
			}

		if((port = atoi(f2)) == 0)
			{
			fprintf(stderr, "%s: invalid port: %s\n", myname, f2);
			return 1;
			}

		if((fd = socket(AF_INET, SOCK_STREAM,0)) == -1)
			{
			fprintf(stderr, "%s: socket() failed, errno=%d (%s)\n", myname, errno, strerror(errno));
			return 1;
			}
		
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		serv_addr.sin_port = htons(port);

		/* Try to avoid being locked out when restarting daemon: */
		{
		int one = 1;
		if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one)) == -1)
			fprintf(stderr, "%s: setsockopt() failed, errno=%d (%s)\n", myname, errno, gu_strerror(errno));
		}

		if(bind(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
			{
			if(errno == EADDRINUSE)
				fprintf(stderr, "%s: there is already a server listening on %s:%d\n", myname, f1, port);
			else
				fprintf(stderr, "%s: bind() failed, errno=%d (%s)\n", myname, errno, strerror(errno));
			return 1;
			}

		if(count > 0)
			gu_pcs_append_char(list, ',');
		gu_pcs_append_sprintf(list, "%d:%d", port, fd);
		}	
	}
	
	{
	int ret;
	if((ret = renounce_root_privs(myname, username, NULL)) != 0)
		return ret;
	}

	execv(argv[3], &argv[3]);
	fprintf(stderr, "%s: execv() failed, errno=%d (%s)\n", myname, errno, strerror(errno));
	return 1;
	} /* end of main() */

/* end of file */
