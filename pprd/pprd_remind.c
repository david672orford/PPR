/*
** mouse:~ppr/src/pprd/pprd_remind.c
** Copyright 1995--2006, Trinity College Computing Center.
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
** Last modified 7 April 2006.
*/

#include "config.h"
#include <stdio.h>
#include <signal.h>				/* for pprd.auto_h */
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>				/* for abs() */
#include <string.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "pprd.h"
#include "./pprd.auto_h"

/*
** This routine is called every time pprd receives a command
** from "ppad remind".
*/
void ppad_remind(void)
	{
	const char function[] = "ppad_remind";
	int _stdin[2];				/* pipe to sendmail */
	int fd;						/* used to open /dev/null in child */
	FILE *sendmail;
	int x;
	int status, next_error_retry, next_engaged_retry, interval;

	/* Quickly scan the list to see if there is anything we should nag about. */
	for(x=0; x < printer_count; x++)
		{
		status = printers[x].spool_state.status;
		next_error_retry = printers[x].spool_state.next_error_retry;
		next_engaged_retry = printers[x].spool_state.next_engaged_retry;
		interval = abs(printers[x].alert.interval);

		if( (status == PRNSTATUS_FAULT && (next_engaged_retry > interval || next_error_retry == 0))
				|| (status == PRNSTATUS_ENGAGED && (next_engaged_retry * ENGAGED_RETRY / 60) > 20) )
			break;
		}

	/* If we reached the end of the list, there is nothing to nag about,
	   so bail out of this function. */
	if(x == printer_count)
		return;

	if(pipe(_stdin))					/* open a pipe to sendmail */
		{
		error("%s(): pipe() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
		return;
		}

	switch(fork())						/* duplicate this process */
		{
		case -1:						/* case: fork failed */
			error("%s(): fork() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
			break;

		case 0:							/* case: we are child */
			close(_stdin[1]);			/* close our copy or write end of pipe */

			if(_stdin[0] != 0)					/* Connect the output end of the */
				{								/* pipe to stdin if it isn't already connected. */
				dup2(_stdin[0], 0);
				close(_stdin[0]);
				}

			if((fd = open("/dev/null", O_WRONLY)) != -1)		/* open /dev/null */
				{
				if(fd!=1) dup2(fd, 1);							/* as stdout */
				if(fd!=2) dup2(fd, 2);							/* and stderr */
				if(fd>2) close(fd);
				}

			execl(SENDMAIL_PATH, "sendmail", USER_PPR, (char*)NULL);
			error("Failed exec \"%s\", errno=%d (%s)", SENDMAIL_PATH, errno, gu_strerror(errno));
			exit(242);

		default:						/* case: we are parent */
			close(_stdin[0]);			/* close our copy of read end */
			if((sendmail = fdopen(_stdin[1], "w")) == (FILE*)NULL)
				{
				error("%s(): fdopen() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
				return;
				}

			fprintf(sendmail, "To: %s\n", USER_PPR);
			fprintf(sendmail, "Subject: %s\n", _("Remaining printer problems"));
			fprintf(sendmail, "\n");

			for(x=0; x < printer_count; x++)
				{
				status = printers[x].spool_state.status;
				next_error_retry = printers[x].spool_state.next_error_retry;
				next_engaged_retry = printers[x].spool_state.next_engaged_retry;
				interval = abs(printers[x].alert.interval);

				/*
				** If it is a fault which has persisted long enough to have
				** merited a notice to an operator,
				*/
				if(status == PRNSTATUS_FAULT && (next_error_retry == 0 || next_error_retry > interval))
					{
					if(next_error_retry > 0)
						{
						fprintf(sendmail, _("The printer \"%s\" has suffered %d faults.\n"),
								printers[x].name,
								next_error_retry);
						}
					else
						{
						fprintf(sendmail, _("The printer \"%s\" has suffered a fault from\n"
								"which it can not recover on its own.\n"),
								printers[x].name);
						}
					fputc('\n', sendmail);
					}
				/*
				** If it has been otherwise engaged or off line for at least
				** ENGAGED_NAG_TIME minutes,
				*/
				else if( status == PRNSTATUS_ENGAGED
						&& ((next_engaged_retry*ENGAGED_RETRY/60) > ENGAGED_NAG_TIME) )
					{
					fprintf(sendmail, _("The printer \"%s\" has been otherwise engaged or off line for %d minutes.\n"),
						printers[x].name,
						next_engaged_retry * ENGAGED_RETRY / 60);
					fputc('\n', sendmail);
					}

				} /* end of second loop thru printer list */

			fclose(sendmail);
			break;
		} /* end of switch */
	} /* end of nag() */

/* end of file */
