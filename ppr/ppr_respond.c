/*
** mouse:~ppr/src/ppr/ppr_respond.c
** Copyright 1995--2002, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 20 March 2002.
*/

/*
** Routines by means of which the PPR job submission program
** tells users why their jobs didn't go thru.  (Yes, responders
** are used for other purposes, such as telling the user that
** his job _did_ go thru, but this code is never called uppon
** to perform any such pleasant task.)
*/

#include "before_system.h"
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "ppr.h"
#include "respond.h"
#include "ppr_exits.h"

/*
** This do-nothing signal handler is intalled before
** launching the responder.  If we left the normal
** SIGCHLD handler in place, it would report that
** a filter had failed.
**
** Since we fully expect to exit after sending the response,
** we will not re-install the normal signal handler.
*/
static void empty_reapchild(int sig)
    {
    } /* end of empty_reapchild() */

/*
** Send a response to the user.
**
** The caller should call exit() immediatly after calling
** this function.
**
** It is worth noting that when this function is called, qentry.subid will
** always be zero and qentry.homenode will always be the value returned
** from ppr_get_nodename().
**
** This function returns 0 if it succedest, -1 if it fails.  The return
** code is used by ppr_infile.c.  If there is no filter to convert the file
** to PostScript and calling this function returns -1, it will use the
** hexdump filter to report the error.
*/
int respond(int response_code, const char extra[])
    {
    const char function[] = "respond";
    pid_t pid;			/* Process id of responder */
    int wstat;			/* wait() status */
    int wret;			/* wait() return code */

    if(strcmp(qentry.responder, "none") == 0)
    	return -1;

    /* Set a harmless SIGCHLD handler. */
    signal(SIGCHLD, empty_reapchild);

    /* Change to /usr/lib/ppr so we can find responders and responders can find stuff. */
    if(chdir(HOMEDIR) == -1)
    	fprintf(stderr, "%s(): chdir(\"%s\") failed, errno=%d (%s)\n", function, HOMEDIR, errno, gu_strerror(errno));

    /* Fork and exec a responder. */
    if((pid = fork()) == -1)
	{
	fprintf(stderr, "%s(): can't fork(), errno=%d (%s)\n", function, errno, gu_strerror(errno));
	return -1;
	}

    if(pid == 0)               /* if child */
	{
	int fd;
	char response_code_str[6];

	/* Convert the response code to a string. */
	snprintf(response_code_str, sizeof(response_code_str), "%d", response_code);

	/* Make sure the responder has a nice, safe stdin. */
	if((fd = open("/dev/null", O_RDONLY)) != -1)
	    {
	    if(fd != 0) dup2(fd, 0);
	    if(fd > 0) close(fd);
	    }

	/*
	** Set the effective user id back to the user's.  This is
	** important for the xwin responder because XWindows programs
	** will not work unless access() suceeds on .Xauthority.
	*/
	seteuid(user_uid);
	setegid(user_gid);

	/* Execute the responder wrapper. */
	execl("lib/ppr-respond", "ppr_respond",
		"ppr",
		qentry.destname,
		response_code_str,
		extra ? extra : "",
		qentry.responder,
		qentry.responder_address,
		qentry.responder_options ? qentry.responder_options : "",
		qentry.For ? qentry.For : "",
		qentry.Title ? qentry.Title : "",
		qentry.lc_messages ? qentry.lc_messages : "",
	    	(char*)NULL);
	    _exit(242);
	}

    /*
    ** Wait for the responder to finish.  If we detect that it
    ** finished abnormally, don't call fatal(), as that would
    ** call respond() which could have nasty results.
    */
    while((wret = wait(&wstat)) != pid)
	{
	if(wret == -1 && errno != EINTR)
	    {
	    fprintf(stderr, "%s(): wait() failed, errno=%d (%s)\n", function, errno, gu_strerror(errno) );
	    return -1;
	    }
	}

    if(WIFEXITED(wstat))
	{
	if(WEXITSTATUS(wstat) != 0)
	    {
	    error("ppr-respond exited with code %d", (int)WEXITSTATUS(wstat));
	    return -1;
	    }
	}
    else if(WIFSIGNALED(wstat))
	{
	error("ppr-respond killed by signal %d (%s)%s",
		WTERMSIG(wstat),
		gu_strsignal(WTERMSIG(wstat)),
		WCOREDUMP(wstat) ? ", core dumped" : ""
		);
	return -1;
	}
    else
	{
	error("ppr-respond suffered some really bizzar accident");
	return -1;
	}

    return 0;
    } /* end of respond() */

/* end of file */

