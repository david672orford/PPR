/*
** mouse:~ppr/src/lprsrv/lprsrv_standalone.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 11 September 2000.
*/

#include "before_system.h"
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
#include "lprsrv.h"

/*
** This module contains functions which lprsrv needs to run in
** standalone mode (without Inetd).
**
** The standalone mode code is optional.  After we include lprsrv.h
** (which is done above) we test to see if STANDALONE is defined.
*/
#ifdef STANDALONE

/* This is set to TRUE in a child process: */
gu_boolean am_standalone_parent = FALSE;

/*
** This function is called from the command line parser.
*/
int port_name_lookup(const char *name)
    {
    struct servent *service;
    if((service = getservbyname(name, "tcp")) == (struct servent *)NULL)
	return -1;
    return ntohs(service->s_port);
    }

/*
** Create the lock file which exists mainly
** so that we can be killed:
*/
static void create_lock_file(void)
    {
    int lockfilefd;
    char temp[10];

    am_standalone_parent = TRUE;

    if((lockfilefd = open(LPRSRV_LOCKFILE, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) == -1)
	fatal(1, _("can't open \"%s\", errno=%d (%s)"), LPRSRV_LOCKFILE, errno, gu_strerror(errno));

    if(gu_lock_exclusive(lockfilefd, FALSE))
	fatal(1, _("lprsrv is already running in standalone mode"));

    sprintf(temp, "%ld\n", (long)getpid());
    write(lockfilefd, temp, strlen(temp));
    } /* end of create_lock_file() */

/*
** Create the server well known port and return a
** file descriptor for it.
*/
static int bind_server(int server_port, uid_t root_uid, uid_t safe_uid)
    {
    int sockfd;
    struct sockaddr_in serv_addr;

    seteuid(root_uid);

    if((sockfd = socket(AF_INET, SOCK_STREAM,0)) == -1)
	fatal(1, "be_server(): socket() failed, errno=%d (%s)", errno, gu_strerror(errno));

    /* We will accept from any IP address and will listen on server_port. */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(server_port);

    /* Try to avoid being locked out when restarting daemon: */
    {
    int one = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one)) == -1)
	fatal(1, "be_server(): setsockopt() failed, errno=%d (%s)", errno, gu_strerror(errno));
    }

    /* Bind to the port. */
    if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
	if(errno == EADDRINUSE)
	    fatal(1, _("there is already a server listening on TCP port %d"), server_port);
	fatal(1, "be_server(): bind() failed, errno=%d (%s)", errno, gu_strerror(errno));
	}

    seteuid(safe_uid);

    /* Set the backlog queue length. */
    if(listen(sockfd, 5) == -1)
	fatal(1, "be_server(): listen() failed, errno=%d (%s)", errno, gu_strerror(errno));

    return sockfd;
    } /* end of bind_server() */

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
static void standalone_main_loop(int sockfd)
    {
    const char function[] = "standalone_main_loop";
    int pid;
    struct sockaddr_in cli_addr;
    unsigned int clilen;		/* !!! things are changing !!! */
    int newsockfd;

    for( ; ; )				/* loop until killed */
	{
	DODEBUG_STANDALONE(("%s(): waiting for connexion", function));

	clilen = sizeof(cli_addr);
	if((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) == -1)
    	    {
	    debug("%s(): accept() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
    	    continue;
    	    }

	DODEBUG_STANDALONE(("%s(): connection request from %s", function, inet_ntoa(cli_addr.sin_addr)));

	if((pid = fork()) == -1)	/* error, */
	    {
	    debug("%s(): fork() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
	    }
	else if(pid == 0)		/* child */
	    {
	    am_standalone_parent = FALSE;
	    close(sockfd);
	    signal_restarting(SIGCHLD, SIG_IGN);
	    if(newsockfd != 0) dup2(newsockfd, 0);
	    if(newsockfd > 0) close(newsockfd);
	    return;
	    }
	else				/* parent */
	    {
	    DODEBUG_STANDALONE(("%s(): child %ld launched", function, (long)pid));
	    }

	close(newsockfd);
	}
    } /* end of main_loop() */

/*
** This ties it all together.  It is called from main().
** Since the last thing it does is call get_connexion(),
** the parent never returns but the child does numberous times.
*/
void run_standalone(int server_port, uid_t root_uid, uid_t safe_uid)
    {
    int fd;

    /*
    ** Duplicate this process and close the origional one
    ** so that the shell will stop waiting.  Also, disassociate
    ** from the controlling terminal and close all file descriptors.
    */
    gu_daemon(PPR_UMASK);

    /* By convention, PPR processes run in the PPR home
       directory. */
    chdir(HOMEDIR);

    DODEBUG_MAIN(("entering standalone mode"));
    create_lock_file();

    DODEBUG_MAIN(("starting server on port %d", server_port));
    fd = bind_server(server_port, root_uid, safe_uid);

    signal_restarting(SIGCHLD, reapchild);

    standalone_main_loop(fd);
    } /* run_standalone() */
#endif

/* end of file */

