/*
** mouse:~ppr/src/ppr-papd/ppr-papd_printjob.c
** Copyright 1995--2002, Trinity College Computing Center.
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
** Last modified 27 December 2002.
*/

#include "before_system.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <setjmp.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "ppr_exits.h"
#include "ppr-papd.h"

static pid_t pid = (pid_t)0;			/* pid of ppr */
static jmp_buf printjob_env;

/*===========================================================================
** Accept a print job and send it to ppr.
**
** This is called from child_main_loop() which it turn is called
** from appletalk_dependent_main_loop().
===========================================================================*/
void printjob(int sesfd, int prnid, int net, int node, const char log_file_name[])
    {
    const char function[] = "printjob";
    const char *username = "?";
    int pipefds[2];		/* a set of file descriptors for pipe to ppr */
    int wstat;			/* wait status */
    int error;
    unsigned int free_blocks, free_files;
    char netnode[10];
    char proxy_for[64];

    DODEBUG_PRINTJOB(("printjob(sesfd=%d, prnid=%d, net=%d, node=%d)",
	sesfd, prnid, net, node));

    /*
    ** Measure free space in the PPR spool area.  If it is
    ** unreasonably low, refuse to accept the job.
    */
    if(disk_space(QUEUEDIR,&free_blocks,&free_files) == -1)
    	{
    	debug("failed to get file system statistics");
    	}
    else
    	{
	if( (free_blocks < MIN_BLOCKS) || (free_files < MIN_INODES) )
	    {
	    reply(sesfd,MSG_NODISK);
	    postscript_stdin_flushfile(sesfd);
	    fatal(1,"insufficient disk space");
	    }
    	}

    /* Turn the network and node numbers into a string. */
    snprintf(netnode, sizeof(netnode), "%d.%d", net, node);
    snprintf(proxy_for, sizeof(proxy_for), "%s@%s.atalk", username ? username : "?", netnode);

    /*
    ** Fork and exec a copy of ppr and accept the job.
    */
    if(setjmp(printjob_env) == 0) 	/* setjmp() returns zero when it is called, */
	{				/* non-zero when longjump() is called */
	DODEBUG_PRINTJOB(("setjmp() returned zero, spawning ppr"));

	if(pipe(pipefds))		/* pipe for sending data to ppr */
	    fatal(0,"printjob(): pipe() failed, errno=%d (%s)",errno,gu_strerror(errno));

	if( (pid=fork()) == -1 )	/* if we can't fork, */
	    {				/* then tell the client */
	    reply(sesfd,MSG_NOPROC);
	    postscript_stdin_flushfile(sesfd);
	    fatal(1,"printjob(): fork() failed, errno=%d",errno);
	    }

	if(pid==0)			/* if child */
	    {
	    const char *argv[MAX_ARGV+20];
	    int x;
	    int fd;

	    close(pipefds[1]);		/* don't need write end */
	    dup2(pipefds[0],0);		/* as for read end, duplicate to stdin */
	    close(pipefds[0]);		/* and close origional */

	    if((fd = open(log_file_name, O_WRONLY | O_APPEND | O_CREAT,UNIX_755)) == -1)
	    	fatal(0, "printjob(): can't open log file");
	    if(fd != 1) dup2(fd,1);	/* stdout and */
	    if(fd != 2) dup2(fd,2);	/* stderr */
	    if(fd > 2) close(fd);

	    /* Build the argument array. */
	    x = 0;
	    argv[x++] = "ppr";		/* name of program */

	    /* destination printer or group */
	    argv[x++] = "-d";
	    argv[x++] = adv[prnid].PPRname;

	    /*
	    ** If we have a username from "%Login", use it,
	    ** otherwise, tell ppr to read "%%For:" comments.
	    **/
	    if(username)
	    	{ argv[x++] = "-f"; argv[x++] = username; }
	    else
	    	{ argv[x++] = "-R"; argv[x++] = "for"; }

	    /* Indicate for whom the user "ppr" is acting as proxy. */
	    argv[x++] = "-X"; argv[x++] = proxy_for;

	    /* Answer for TTRasterizer query */
	    if(adv[prnid].TTRasterizer)
	    	{ argv[x++] = "-Q"; argv[x++] = adv[prnid].TTRasterizer; }

	    /* no responder */
	    argv[x++] = "-m"; argv[x++] = "pprpopup";

	    /* default response address */
	    argv[x++] = "-r"; argv[x++] = netnode;

	    /* default is already -w severe */
	    argv[x++] = "-w"; argv[x++] = "log";

	    /*
	    ** Throw away truncated jobs.  This doesn't
	    ** work with QuickDraw GX so it is commented out.
	    */
	    /* argv[x++]="-Z"; argv[x++]="true"; */

	    /* LaserWriter 8.x benefits from a cache that stores stuff. */
	    argv[x++] = "--cache-store=uncached";
	    argv[x++] = "--cache-priority=high";
	    argv[x++] = "--strip-cache=true";

	    /*
	    ** Copy user supplied arguments.  These may
	    ** override some above.
	    */
	    {
	    int y;
	    struct ADV *a = &adv[prnid];
	    for(y=0; (argv[x] = a->argv[y]) != (char*)NULL; x++,y++);
	    }

	    /* end of argument list */
	    argv[x] = (char*)NULL;

	    #ifdef DEBUG_PPR_ARGV
	    {
	    int y;
	    for(y=0; argv[y]!=(char*)NULL; y++)
	    	debug("argv[%d] = \"%s\"",y,argv[y]);
	    }
	    #endif

	    execv(PPR_PATH, (char **)argv);	/* execute ppr */

	    _exit(255);			/* exit if exec fails */
	    }

	/*
	** Parent only from here on.  Parent clone of ppr-papd daemon, child is ppr.)
	*/
	close(pipefds[0]);          /* we don't need read end */

	/*
	** Call the AppleTalk dependent code to copy the job.
	*/
	DODEBUG_PRINTJOB(("%s(): calling appletalk_dependent_printjob()", function));
	if(! appletalk_dependent_printjob(sesfd, pipefds[1]))
	    fatal(1,"Print job was truncated.");
	DODEBUG_PRINTJOB(("%s(): appletalk_dependent_printjob() returned", function));

	} /* end of if(setjump()) */

    /*--------------------------------------------------------------*/
    /* We end up here when the job is done or longjmp() is called.  */
    /* longjmp() is called from within the SIGCHLD handler.         */
    /*--------------------------------------------------------------*/
    DODEBUG_PRINTJOB(("%s(): after setjmp() clause", function));

    /* We will no longer be wanting to kill ppr, so we can forget its PID. */
    pid = (pid_t)0;

    /* Close the pipe to ppr.  (This tells ppr it has the whole job.) */
    DODEBUG_PRINTJOB(("%s(): closing pipe to ppr", function));
    close(pipefds[1]);

    /* Wait for ppr to terminate. */
    DODEBUG_PRINTJOB(("%s(): waiting for PPR to exit", function));
    wait(&wstat);
    DODEBUG_PRINTJOB(("%s(): PPR exit code: %d", function, WEXITSTATUS(wstat)));

    /*
    ** If the return value from ppr is non-zero, then send an
    ** appropriate error message back to the client.
    */
    error = TRUE;
    if(WIFEXITED(wstat))
	{
	DODEBUG_REAPCHILD(("ppr exited with code %d", WEXITSTATUS(wstat)));

	switch(WEXITSTATUS(wstat))
	    {
	    case PPREXIT_OK:			/* Normal ppr termination, */
		error=FALSE;
		break;
	    case PPREXIT_NOCHARGEACCT:
		reply(sesfd,MSG_NOCHARGEACCT);
		break;
	    case PPREXIT_BADAUTH:
		reply(sesfd,MSG_BADAUTH);
		break;
	    case PPREXIT_OVERDRAWN:
		reply(sesfd,MSG_OVERDRAWN);
		break;
	    case PPREXIT_TRUNCATED:
	    	reply(sesfd,MSG_TRUNCATED);
	    	break;
	    case PPREXIT_NONCONFORMING:
		reply(sesfd,MSG_NONCONFORMING);
		break;
	    case PPREXIT_ACL:
	    	reply(sesfd,MSG_ACL);
	    	break;
	    case PPREXIT_NOSPOOLER:
		reply(sesfd,MSG_NOSPOOLER);
		break;
	    case PPREXIT_SYNTAX:
		reply(sesfd,MSG_SYNTAX);
		break;
	    default:
		reply(sesfd,MSG_FATALPPR);
		break;
	    }
	}
    else if(WCOREDUMP(wstat))		/* If core dump created, */
	{
	reply(sesfd,"%%[ Error: ppr-papd: ppr dumped core ]%%\n");
	}
    else if(WIFSIGNALED(wstat))		/* If child caught a signal, */
	{
	reply(sesfd, "%%[ Error: ppr-papd: ppr killed ]%%\n");
	}
    else				/* Other return from wait(), such as stopped, */
	{
	reply(sesfd, "%%[ Error: ppr-papd: unexpected return from wait() ]%%\n");
	}

    /*
    ** If there was an error detected above, flush the
    ** message out of the buffer and flush the job.
    */
    if(error)
	{
	postscript_stdin_flushfile(sesfd);
	exit(2);
	}

    DODEBUG_PRINTJOB(("printjob(): calling reply_eoj()"));
    reply_eoj(sesfd);
    } /* end of printjob() */

void printjob_abort(void)
    {
    if(pid)				/* If we have launched ppr, */
	kill(pid, SIGTERM);		/* then kill it. */
    pid = (pid_t)0;
    } /* end of printjob_abort() */

void printjob_reapchild(void)
    {
    longjmp(printjob_env, 1);
    }

/* end of file */