/*
** mouse:~ppr/src/papd/papd_printjob.c
** Copyright 1995--2004, Trinity College Computing Center.
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
** Last modified 23 January 2004.
*/

#include "before_system.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "ppr_exits.h"
#include "papd.h"

static pid_t ppr_pid = (pid_t)0;

/*===========================================================================
** Accept a print job and send it to ppr.
**
** This is called from connexion_callback() which it turn is called
** from at_service_loop().  It launches ppr and then calls 
** at_printjob_copy() to copy the printjob from the AppleTalk PAP socket
** to the pipe connected to ppr.
===========================================================================*/
void printjob(int sesfd, struct ADV *adv, void *qc, int net, int node, const char log_file_name[])
	{
	const char function[] = "printjob";
	const char *username = NULL;
	int pipefds[2];				/* a set of file descriptors for pipe to ppr */
	int wstat;					/* wait status */
	int error;
	unsigned int free_blocks, free_files;
	char netnode[10];
	char proxy_for[64];

	DODEBUG_PRINTJOB(("printjob(sesfd=%d, adv=%p, qc=%p, net=%d, node=%d, log_file_name[]=\"%s\")", sesfd, adv, qc, net, node, log_file_name));

	/*
	** Measure free space in the PPR spool area.  If it is
	** unreasonably low, refuse to accept the job.
	*/
	if(disk_space(QUEUEDIR, &free_blocks, &free_files) == -1)
		{
		debug("%s(): failed to get file system statistics", function);
		}
	else
		{
		if((free_blocks < MIN_BLOCKS) || (free_files < MIN_INODES))
			{
			at_reply(sesfd, "%%[ Error: spooler is out of disk space ]%%\n");
			postscript_stdin_flushfile(sesfd);
			gu_Throw("insufficient disk space");
			}
		}

	/* Turn the network and node numbers into a string. */
	snprintf(netnode, sizeof(netnode), "%d.%d", net, node);
	snprintf(proxy_for, sizeof(proxy_for), "%s@%s.atalk", username ? username : "?", netnode);

	/*
	** Fork and exec a copy of ppr and accept the job.
	*/
	if(pipe(pipefds))				/* pipe for sending data to ppr */
		gu_Throw("%s(): pipe() failed, errno=%d (%s)", function, errno, gu_strerror(errno));

	if((ppr_pid = fork()) == -1)	/* if we can't fork, */
		{							/* then tell the client */
		at_reply(sesfd, "%%[ Error: spooler is out of processes ]%%\n");
		postscript_stdin_flushfile(sesfd);
		gu_Throw(_("%s(): fork() failed, errno=%d (%s)"), function, errno, gu_strerror(errno));
		}

	if(ppr_pid == 0)				/* if child */
		{
		const char *argv[36];		/* 22 used by last count */
		int x;
		int fd;

		close(pipefds[1]);			/* don't need write end */
		dup2(pipefds[0],0);			/* as for read end, duplicate to stdin */
		close(pipefds[0]);			/* and close origional */

		if((fd = open(log_file_name, O_WRONLY | O_APPEND | O_CREAT,UNIX_755)) == -1)
			gu_Throw("%s(): can't open log file", function);
		if(fd != 1) dup2(fd,1);		/* stdout and */
		if(fd != 2) dup2(fd,2);		/* stderr */
		if(fd > 2) close(fd);

		/* Build the argument array. */
		x = 0;
		argv[x++] = "ppr";			/* name of program */

		/* destination printer or group */
		argv[x++] = "-d";
		argv[x++] = adv->PPRname;

		/*
		** If we have a username from "%Login", use it,
		** otherwise, tell ppr to read "%%For:" comments.
		**/
		if(username)
			{
			argv[x++] = "-f";
			argv[x++] = username;
			}
		else
			{
			argv[x++] = "-R";
			argv[x++] = "for";
			}

		/* Indicate for whom the user "ppr" is acting as proxy. */
		argv[x++] = "-X";
		argv[x++] = proxy_for;

		/* Answer for TTRasterizer query */
		if(queueinfo_ttRasterizer(qc))
			{
			argv[x++] = "-Q";
			argv[x++] = queueinfo_ttRasterizer(qc);
			}

		/* no responder */
		argv[x++] = "-m";
		argv[x++] = "pprpopup";

		/* default response address */
		argv[x++] = "-r";
		argv[x++] = netnode;

		/* default is already -w severe */
		argv[x++] = "-w";
		argv[x++] = "log";

		/*
		** Throw away truncated jobs.  This doesn't
		** work with QuickDraw GX so it is commented out.
		*/
		#if 0
		argv[x++]="-Z";
		argv[x++]="true";
		#endif

		/* LaserWriter 8.x benefits from a cache that stores stuff. */
		argv[x++] = "--cache-store=uncached";
		argv[x++] = "--cache-priority=high";
		argv[x++] = "--strip-cache=true";

		/* end of argument list */
		argv[x] = (char*)NULL;

		#ifdef DEBUG_PPR_ARGV
		{
		int y;
		for(y=0; argv[y]; y++)
			debug("argv[%d] = \"%s\"", y, argv[y]);
		}
		#endif

		execv(PPR_PATH, (char **)argv);		/* execute ppr */

		_exit(255);					/* exit if exec fails */
		}

	/*
	** Parent only from here on.  Parent clone of papd daemon, child is ppr.)
	*/
	close(pipefds[0]);			/* we don't need read end */

	/*
	** Call the AppleTalk dependent code to copy the job.
	*/
	DODEBUG_PRINTJOB(("%s(): calling at_printjob_copy()", function));
	if(! at_printjob_copy(sesfd, pipefds[1]))
		gu_Throw("%s(): print job was truncated", function);
	DODEBUG_PRINTJOB(("%s(): at_printjob_copy() returned", function));

	/* We will no longer be wanting to kill ppr, so we can forget its PID. */
	ppr_pid = (pid_t)0;

	/* Close the pipe to ppr.  (This tells ppr it has the whole job.) */
	DODEBUG_PRINTJOB(("%s(): closing pipe to ppr", function));
	close(pipefds[1]);

	/* Wait for ppr to terminate. */
	DODEBUG_PRINTJOB(("%s(): waiting for ppr to exit", function));
	wait(&wstat);
	DODEBUG_PRINTJOB(("%s(): ppr exit code: %d", function, WEXITSTATUS(wstat)));

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
			case PPREXIT_OK:					/* Normal ppr termination, */
				error = FALSE;
				break;
			case PPREXIT_NOCHARGEACCT:
				at_reply(sesfd, "%%[ Error: you don't have a charge account ]%%\n");
				break;
			case PPREXIT_OVERDRAWN:
				at_reply(sesfd, "%%[ Error: account overdrawn ]%%\n");
				break;
			case PPREXIT_TRUNCATED:
				at_reply(sesfd, "%%[ Error: input file is truncated ]%%\n");
				break;
			case PPREXIT_NONCONFORMING:
				at_reply(sesfd, "%%[ Error: insufficient DSC conformance ]%%\n");
				break;
			case PPREXIT_ACL:
				at_reply(sesfd, "%%[ Error: ACL forbids you access to selected print destination ]%%\n");
				break;
			case PPREXIT_NOSPOOLER:
				at_reply(sesfd, "%%[ Error: spooler is not running ]%%\n");
				break;
			case PPREXIT_SYNTAX:
				at_reply(sesfd, "%%[ Error: bad ppr invokation syntax ]%%\n");
				break;
			default:
				at_reply(sesfd, "%%[ Error: fatal error, see papd log ]%%\n");
				break;
			}
		}
	else if(WCOREDUMP(wstat))			/* If core dump created, */
		{
		at_reply(sesfd, "%%[ Error: papd: ppr dumped core ]%%\n");
		}
	else if(WIFSIGNALED(wstat))			/* If child caught a signal, */
		{
		at_reply(sesfd, "%%[ Error: papd: ppr killed ]%%\n");
		}
	else								/* Other return from wait(), such as stopped, */
		{
		at_reply(sesfd, "%%[ Error: papd: unexpected return from wait() ]%%\n");
		}

	/*
	** If there was an error detected above, flush the message out of
	** the buffer and flush the job and terminate this service process.
	*/
	if(error)
		{
		postscript_stdin_flushfile(sesfd);
		exit(2);
		}

	DODEBUG_PRINTJOB(("%s(): calling at_reply_eoj()", function));
	at_reply_eoj(sesfd);
	} /* end of printjob() */

/*
** This will be called from main()'s exception handler just before
** process shutdown.  It gives us a chance to kill ppr if we are
** running one.
*/
void printjob_abort(void)
	{
	if(ppr_pid)							/* If we have launched ppr, */
		kill(ppr_pid, SIGTERM);			/* then kill it. */
	ppr_pid = (pid_t)0;					/* We won't do it twice. */
	} /* end of printjob_abort() */

/* end of file */
