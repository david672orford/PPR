/*
** mouse:~ppr/src/pprd/pprd_respond.c
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
** Last modified 8 March 2002.
*/

#include "before_system.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include "gu.h"
#include "global_defines.h"
#include "pprd.h"
#include "./pprd.auto_h"

/*
** This is called whenever a responder exits.
*/
void responder_child_hook(pid_t pid, int wstat)
	{
	DODEBUG_RESPOND(("(respond process?)"));
	} /* end of responder_child_hook() */

/*
** Send a response to the user by means of the designated response method.
**
** This routine is sometimes called directly, at other times it is called
** by the wrapper routine respond().
*/
void respond2(const char *destnode, const char *destname, int id, int subid, const char *homenode, int prnid, const char *prnname, int response_code)
	{
	const char function[] = "respond2";
	char jobname[256];
	char filename[MAX_PPR_PATH];
	int fd;
	pid_t pid;									/* Process id of responder */

	/* Format the job name as a string.	 This will be the 1st parameter. */
	snprintf(jobname, sizeof(jobname), "%s:%s-%d.%d(%s)", destnode, destname, id, subid, homenode);

	/* Open the queue file before the fork since it may be deleted when we return. */
	ppr_fnamef(filename, "%s/%s", QUEUEDIR, jobname);
	if((fd = open(filename, O_RDONLY)) == -1)
		{
		fprintf(stderr, "Can't open \"%s\", errno=%d (%s)\n", filename, errno, gu_strerror(errno));
		return;
		}

	if((pid = fork()) == -1)					/* if error, */
		{
		error("%s(): can't fork(), errno=%d (%s)", function, errno, gu_strerror(errno));
		}
	else if(pid == 0)							/* if child, */
		{
		char code_str[5];
		char per_duplex_str[8];
		char per_simplex_str[8];

		/* Parent may have signals blocked. */
		child_unblock_all();

		/* Move the queue file to file descriptor 3 if it isn't there already. */
		if(fd != 3)
			{
			dup2(fd, 3);
			close(fd);
			}

		/* Connect stdin to the job log file, and stdout and stderr to the pprd log file. */
		ppr_fnamef(filename, "%s/%s-log", DATADIR, jobname);
		child_stdin_stdout_stderr(filename, PPRD_LOGFILE);

		/* The 2nd parameter is the response code number. */
		snprintf(code_str, sizeof(code_str), "%d", response_code);

		/* 4th and 5th are the printer charge rates */
		per_duplex_str[0] = '\0';
		per_simplex_str[0] = '\0';
		if(prnid > -1)
			{
			snprintf(per_duplex_str, sizeof(per_duplex_str), "%d", printers[prnid].charge_per_duplex);
			snprintf(per_simplex_str, sizeof(per_simplex_str), "%d", printers[prnid].charge_per_simplex);
			}

		/* Here we go! */
		execl("lib/ppr-respond", "ppr-respond",
				"pprd",
				jobname,
				code_str,
				prnname,
				per_duplex_str,
				per_simplex_str,
				NULL);
		debug("child: %s(): execl() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
		exit(12);
		} /* end of child clause */

	/* parent drops through */
	close(fd);

	} /* end of respond2() */

/*
** This is the outer routine.  It takes a destination and a node id number and
** converts them to names before calling respond2() which does the real work.
*/
void respond(int destnode_id, int destid, int id, int subid, int homenode_id, int prnid, int response)
	{
	DODEBUG_RESPOND(("respond(destnode_id=%d, destid=%d, id=%d, subid=%d, homenode_id=%d, prnid=%d, response=%d)", destnode_id, destid, id, subid, homenode_id, prnid, response));
	respond2(nodeid_to_name(destnode_id), destid_to_name(destnode_id, destid), id, subid, nodeid_to_name(homenode_id), prnid, destid_to_name(destnode_id, prnid), response);
	}

/* end of file */
