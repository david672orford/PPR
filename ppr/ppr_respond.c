/*
** mouse:~ppr/src/ppr/ppr_respond.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 23 May 2001.
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
** Given queue id information, the response number, and the extra parameter, build
** an appropriate English message to send to the user.
*/
static void respond_build_message(char *response_str, size_t space_available, const char *destnode, const char *destname, int id, int subid, const char *homenode, int response, const char *extra)
    {
    switch(response)
	{
	case RESP_CANCELED_NOCHARGEACCT:
	    snprintf(response_str, space_available,
	    _("Your new print job for \"%s\" was rejected because\n"
	    "\"%s\" does not have a charge account."),
		network_destspec(destnode, destname),
		extra
		);
	    break;
	case RESP_CANCELED_BADAUTH:
	    snprintf(response_str, space_available,
	    _("Your new print job for \"%s\" was rejected because\n"
	    "you did not enter %s's authorization code."),
		network_destspec(destnode, destname),
		extra
		);
	    break;
	case RESP_CANCELED_OVERDRAWN:
	    snprintf(response_str, space_available,
	    _("Your new print job for \"%s\" was rejected because\n"
	    "your account is overdrawn."),
		network_destspec(destnode, destname)
		);
	    break;
	case RESP_CANCELED_NONCONFORMING:
	    snprintf(response_str, space_available,
	    _("Your new print job for \"%s\" was rejected because\n"
	    "it does not contain DSC page division information."),
		network_destspec(destnode, destname)
		);
	    break;
	case RESP_NOFILTER:
	    snprintf(response_str, space_available,
	    _("Your new print job for \"%s\" has been rejected because no filter\n"
	    "is available which can convert %s to PostScript."),
		network_destspec(destnode, destname),
		extra
		);
	    break;
	case RESP_FATAL:
	    snprintf(response_str, space_available,
	    _("Your new print job for \"%s\" has been rejected by PPR because of a\n"
	    "fatal error: %s."),
		network_destspec(destnode, destname),
		extra
		);
	    break;
	case RESP_NOSPOOLER:
	    snprintf(response_str, space_available,
	    _("Your new print job for \"%s\" has been lost because PPRD is not running."),
		network_destspec(destnode, destname)
		);
	    break;
	case RESP_BADMEDIA:
	    snprintf(response_str, space_available,
	    _("Your new print job for \"%s\" has been rejected because it requires\n"
	    "a size and type of medium (paper) which is not available."),
		network_destspec(destnode, destname)
		);
	    break;
	case RESP_BADPJLLANG:
	    snprintf(response_str, space_available,
	    _("Your new print job for \"%s\" has been rejected because the\n"
	    "PJL header requests an unrecognized printer language \"%s\"."),
		network_destspec(destnode, destname),
		extra
		);
	    break;
	case RESP_FATAL_SYNTAX:
	    snprintf(response_str, space_available,
	    _("Your new print job for \"%s\" has been rejected because\n"
	    "the ppr command line contains an error:\n"
	    "%s."),
		network_destspec(destnode, destname),
		extra
		);
	    break;
	case RESP_CANCELED_NOPAGES:
	    snprintf(response_str, space_available,
	    _("Your new print job for \"%s\" has been rejected because\n"
	    "you requested printing of only selected pages but the pages\n"
	    "are not marked by DSC comments."),
	    	network_destspec(destnode, destname)
	    	);
	    break;
	case RESP_CANCELED_ACL:
	    snprintf(response_str, space_available,
	    _("Your print job for \"%s\" has been rejected because the\n"
	    "PPR access control lists do not grant \"%s\" access to\n"
	    "that destination."),
	    	network_destspec(destnode, destname),
	    	extra
	    	);
	    break;
	default:
	    snprintf(response_str, space_available,
	    "Invalid response code %d for your new print job.", response);
	}
    } /* end of respond_build_message() */

/*
** Send a response to the user.
**
** The caller should call exit() immediatly after calling
** this function.
**
** Return 0 if we send the message, -1 if we don't.
**
** It is worth noting that when this function is called, qentry.subid will
** always be zero and qentry.homenode will always be the value returned
** from ppr_get_nodename().
*/
int respond(int response, const char *extra)
    {
    char response_str[512];		/* for building our message */
    int retval = -1;			/* will set to 0 if we send the message */

    /* Change to /usr/lib/ppr so we can find responders and responders can find stuff. */
    if(chdir(HOMEDIR) == -1)
    	fprintf(stderr, "respond(): chdir(\"%s\") failed, errno=%d (%s)\n", HOMEDIR, errno, gu_strerror(errno));

    /* Construct an English message. */
    respond_build_message(response_str, sizeof(response_str), qentry.destnode, qentry.destname, qentry.id, qentry.subid, qentry.homenode, response, extra);

    /*
    ** If we should respond by stderr, do it now.
    ** Notice that output to stderr is never re-wrapped.
    **
    ** Notice that we don't return after doing this.  It may
    ** be that we are supposed to use both stderr and the responder.
    */
    if(ppr_respond_by & PPR_RESPOND_BY_STDERR)
	{
    	fprintf(stderr, "%s\n", response_str);
    	retval = 0;		/* that counts as sending the message */
    	}

    /*
    ** If we should use the responder and the responder name
    ** is not set to "none", then launch it.
    */
    if((ppr_respond_by & PPR_RESPOND_BY_RESPONDER) && strcmp(qentry.responder, "none"))
	{
	char resfname[MAX_PPR_PATH];	/* for responder file name */
	pid_t pid;			/* Process id of responder */
	int wstat;			/* wait() status */
	int wret;			/* wait() return code */
	struct stat statbuf;

	/* Re-wrap the response string.  This modifies response_str. */
	gu_wordwrap(response_str, get_responder_width(qentry.responder));

	/* Set a harmless SIGCHLD handler. */
	signal(SIGCHLD, empty_reapchild);

	/* Construct the full path to the responder from HOMEDIR: */
	ppr_fnamef(resfname, "%s/%s", RESPONDERDIR, qentry.responder);

	/*
	** Get file information about the responder.  This will tell us
	** if it exists but it will also allow us to determine if the
	** setuid bit is set on the responder.  If it is not we will use
	** setuid() to set the effective uid to that of the person
	** who invoked this program.
	*/
	if(stat(resfname, &statbuf) == -1)
	    {
	    fprintf(stderr, "can't stat() \"%s\", errno=%d (%s)\n", resfname, errno, gu_strerror(errno));
	    return -1;
	    }

    	/* Fork and exec a responder. */
	if((pid = fork()) == -1)
	    {
	    fprintf(stderr, "can't fork() in respond(), errno=%d (%s)\n", errno, gu_strerror(errno) );
	    return -1;
	    }

	if(pid == 0)               /* if child */
	    {
	    char response_code_number[3];
	    char time_number[16];
	    char queue_id[256];
	    int fd;

	    /* Convert the response code to a string. */
	    snprintf(response_code_number, sizeof(response_code_number), "%d", response);

	    /* Convert the submission time to a string. */
	    snprintf(time_number, sizeof(time_number), "%ld", qentry.time);

	    /* Build a string containing all of the queue id elements separated by spaces. */
	    snprintf(queue_id, sizeof(queue_id), "%s %s %d %d %s",
	    	qentry.destnode != (char*)NULL ? qentry.destnode : ppr_get_nodename(),
	    	qentry.destname, qentry.id, qentry.subid, qentry.homenode);

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
	    ** Unless the setuid bit is set, set the effective
	    ** user id back to the user's.  This is important
	    ** for the xwin responder because XWindows programs
	    ** will not work unless access() suceeds on
	    ** .Xauthority.
	    */
	    if((statbuf.st_mode & S_ISUID) == 0)
	    	seteuid(user_uid);
	    if((statbuf.st_mode & S_ISGID) == 0)
	    	setegid(user_gid);

	    /* Execute the responder: */
	    execl(resfname, qentry.responder,
	    	qentry.For ? qentry.For : qentry.username, /* !!! */
	    	qentry.responder_address,				/* address to send response to */
	    	response_str,						/* suggested message text */
		"",							/* second response string */
		qentry.responder_options ? qentry.responder_options : "",
		response_code_number,					/* response type number */
		queue_id,						/* complete job queue id */
		extra ? extra : "",					/* normally printer name */
		qentry.Title ? qentry.Title : "",			/* job title */
		time_number,						/* time job was submitted */
		"",							/* reason last arrested */
		"?",							/* number of pages */
	    	(char*)NULL);
	    _exit(242);
	    }

	retval = 0;		/* we sent it */

	/*
	** Wait for the responder to finish.  If we detect that it
	** finished abnormally, don't call fatal(), as that would
	** call respond() which could have nasty results.
	*/
	while((wret = wait(&wstat)) != pid)
	    {
	    if(wret == -1 && errno != EINTR)
	    	{
	    	fprintf(stderr, "ppr_respond.c: respond(): wait() failed, errno=%d (%s)\n", errno, gu_strerror(errno) );
		exit(PPREXIT_OTHERERR);
		}
	    }

	if(WIFEXITED(wstat))
	    {
	    if( WEXITSTATUS(wstat) != 0 )
	    	{
	    	fprintf(stderr, "Responder exited with code %d.\n", (int)WEXITSTATUS(wstat));
	    	exit(PPREXIT_OTHERERR);
	    	}
	    }
	else if(WIFSIGNALED(wstat))
	    {
	    fprintf(stderr, "Responder killed by signal %d (%s).\n", WTERMSIG(wstat), gu_strsignal(WTERMSIG(wstat)));
	    if(WCOREDUMP(wstat))
	    	fprintf(stderr, "Core dumped\n");
	    exit(PPREXIT_OTHERERR);
	    }
	else
	    {
	    fprintf(stderr, "ppr_respond.c: respond(): The responder suffered some really bizzar accident.\n");
	    exit(PPREXIT_OTHERERR);
	    }
	} /* If we should exec() a responder */

    /*
    ** When respond() returns, its caller will immediately
    ** call exit(), so we should remove any files ppr has created.
    */
    file_cleanup();

    return retval;				/* Say if we did anything */
    } /* end of respond() */

/* end of file */

