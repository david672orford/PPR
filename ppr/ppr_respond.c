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
** Last modified 7 March 2002.
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
    char jobname[128];
    int fd;
    pid_t pid;			/* Process id of responder */
    int wstat;			/* wait() status */
    int wret;			/* wait() return code */

    /* Bail out of here if we can't do anything. */
    if(!(ppr_respond_by & PPR_RESPOND_BY_RESPONDER) && !(ppr_respond_by & PPR_RESPOND_BY_STDERR))
    	return -1;
    if(strcmp(qentry.responder, "none") == 0)
    	return -1;

    /* Build a string containing all of the queue id elements separated by spaces. */
    snprintf(jobname, sizeof(jobname), "%s:%s",
		qentry.destnode != (char*)NULL ? qentry.destnode : ppr_get_nodename(),
		qentry.destname);

    /* Create a temporary file that looks enough like a queue file to
       keep ppr-respond happy. */
    {
    char filename[MAX_PPR_PATH];
    char buffer[1024];
    ppr_fnamef(filename, "%s/ppr-respond-XXXXXX", TEMPDIR);
    if((fd = gu_mkstemp(filename)) == -1)
	{
	fprintf(stderr, "%s(): gu_mkstemp(\"%s\") failed, errno=%d (%s)\n", function, filename, errno, gu_strerror(errno));
	return -1;
	}
    unlink(filename);
    snprintf(buffer, sizeof(buffer),
	"For: %s\n"
	"Title: %s\n"
	"Response: %.*s %.*s %.*s\n",
		qentry.For ? qentry.For : "???",
		qentry.Title ? qentry.Title : "",
		MAX_RESPONSE_METHOD, qentry.responder,
		MAX_RESPONSE_ADDRESS, qentry.responder_address,
		MAX_RESPONDER_OPTIONS, qentry.responder_options ? qentry.responder_options : ""
    	);
    write(fd, buffer, strlen(buffer));
    lseek(fd, SEEK_SET, 0);
    }

    /* Set a harmless SIGCHLD handler. */
    signal(SIGCHLD, empty_reapchild);

    /* Change to /usr/lib/ppr so we can find responders and responders can find stuff. */
    if(chdir(HOMEDIR) == -1)
    	fprintf(stderr, "respond(): chdir(\"%s\") failed, errno=%d (%s)\n", HOMEDIR, errno, gu_strerror(errno));

    /* Fork and exec a responder. */
    if((pid = fork()) == -1)
	{
	fprintf(stderr, "%s(): can't fork(), errno=%d (%s)\n", function, errno, gu_strerror(errno));
	return -1;
	}

    if(pid == 0)               /* if child */
	{
	char response_code_str[6];

	/* Convert the response code to a string. */
	snprintf(response_code_str, sizeof(response_code_str), "%d", response_code);

	/* Move the queue file to file descriptor 3 if it isn't there already. */
	if(fd != 3)
	    {
	    dup2(fd, 3);
	    close(fd);
	    }

	/* Make sure the responder has a nice, safe stdin. */
	if((fd = open("/dev/null", O_RDONLY)) < 0)
	    {
	    fprintf(stderr, "Warning: can't open \"/dev/null\", errno=%d (%s)\n", errno, gu_strerror(errno));
	    }
	else
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
		(ppr_respond_by & PPR_RESPOND_BY_STDERR) ? "--stderr" : "--no-stderr",
		(ppr_respond_by & PPR_RESPOND_BY_RESPONDER) ? "--responder" : "--no-responder",
		jobname,
		response_code_str,
		extra ? extra : "",
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
	    fprintf(stderr, "%s(): ppr-respond exited with code %d.\n", function, (int)WEXITSTATUS(wstat));
	    return -1;
	    }
	}
    else if(WIFSIGNALED(wstat))
	{
	fprintf(stderr, "%s(): ppr-respond killed by signal %d (%s).\n", function, WTERMSIG(wstat), gu_strsignal(WTERMSIG(wstat)));
	if(WCOREDUMP(wstat))
	    fprintf(stderr, "Core dumped\n");
	return -1;
	}
    else
	{
	fprintf(stderr, "%s(): ppr-respond suffered some really bizzar accident.\n", function);
	return -1;
	}

    return 0;
    } /* end of respond() */

/* end of file */

