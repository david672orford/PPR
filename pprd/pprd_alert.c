/*
** mouse:~ppr/src/pprd/pprd_alert.c
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
** Last modified 12 April 2001.
*/

/*
** This module contains two functions, alert_post() which is called every
** time a printer fails, and alert_printer_working() which is called
** whenever a printer finishes a job (even if the job is arrested).
*/

#include "config.h"
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>				/* so we can include pprd.auto_h */
#include "gu.h"
#include "global_defines.h"

#include "pprd.h"
#include "./pprd.auto_h"		/* for definition of debug() */

/*
** This is called whenever a printer is placed in either fault-retry mode
** or fault-no-retry mode.
*/
void alert_printer_failed(char *prn, int frequency, char *method, char *address, int n)
	{
	const char function[] = "alert_printer_failed";

	DODEBUG_ALERTS(("%s(prn=\"%s\", frequency=%d, method=\"%s\", address=\"%s\", n=%d)", function, prn, frequency, method, address, n));

	/* If no alert posting for this printer, do nothing. */
	if(frequency==0)
		return;

	/*
	** If no alert method, do nothing.	(ppad will set the method to
	** "none" and the address to "nobody" if the user sets the alert
	** frequency without setting the method and address.)
	*/
	if(strcmp(method, "none") == 0)
		return;

	/*
	** Send them only if it is the proper time, that is, if in
	** no-retry-fault mode or every frequency retries.  If
	** frequency is negative, alert only once at the prescribed point.
	*/
	if( n == 0													/* no retry */
			|| (frequency < 0 && n == (frequency * -1))			/* negative */
			|| (frequency > 0 && (n % frequency) == 0)			/* positive */
			)
		{
		int fd;					/* for opening /dev/null */
		char fname[MAX_PPR_PATH];		/* path to log file */
		int _stdin[2];			/* pipe to sendmail */
		FILE *sendmail;			/* stream to sendmail */
		FILE *logfile;			/* stream from log file */
		int c;					/* character from log file */

		DODEBUG_ALERTS(("%s(): sending alert", function));

		if(pipe(_stdin))		 /* open a pipe to sendmail */
			{
			error("%s(): pipe() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
			return;
			}

		switch(fork())
			{
			case -1:							/* case: error */
				error("%s(): fork() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
				return;

			case 0:								/* case: child */
				close(_stdin[1]);				/* close our copy of write end of pipe */

				if(_stdin[0] != 0)
					{
					dup2(_stdin[0], 0);
					close(_stdin[0]);
					}

				fd = open("/dev/null", O_WRONLY);		/* open /dev/null */
				if(fd!=1) dup2(fd, 1);					/* as stdout */
				if(fd!=2) dup2(fd, 2);					/* and stderr */
				if(fd>2) close(fd);
				execl(SENDMAIL_PATH, "sendmail", address, (char*)NULL);
				_exit(255);

			default:							/* case: we are parent */
				close(_stdin[0]);				/* close our copy of read end */

				if( (sendmail=fdopen(_stdin[1],"w")) == (FILE*)NULL )
					{
					error("%s(): fdopen() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
					return;
					}

				/*
				** Try to get Sendmail to accept a "From:" line which
				** includes the system name.
				*/
				fprintf(sendmail, "From: %s PPR Spooler <%s>\n", ppr_get_nodename(), USER_PPR);

				/*
				** We _must_ supply a "To:" line or Sendmail will
				** put in an ugly "Appearently-To:" line.
				*/
				fprintf(sendmail, "To: %s\n", address);

				/*
				** The subject varies according to what type
				** of alerts have been requested.
				*/
				if(frequency > 0)
					fprintf(sendmail, "Subject: Faults on %s\n", prn);
				else
					fprintf(sendmail, "Subject: Faults on %s (first and final notice)\n",prn);

				/* A blank line indicates the end of the header. */
				fputc('\n', sendmail);

				/* Append the alerts file as the body. */
				ppr_fnamef(fname, "%s/%s", ALERTDIR, prn);		  /* open */
				if((logfile = fopen(fname, "r")) != (FILE*)NULL)
					{
					while((c = fgetc(logfile)) != -1)
						fputc(c, sendmail);
					fclose(logfile);
					}

				/* Close the pipe thru which we are feeding Sendmail. */
				fclose(sendmail);
				break;
			} /* end of switch */

		} /* end of if proper time */

	} /* end of alert_printer_failed() */

/*
** This is called whenever a job is successfully sent
** to a printer.  If the alert frequency is negative,
** we must inform the user that the printer has
** recovered if we informed the user of the failure.
*/
void alert_printer_working(char *prn, int frequency, char *method, char *address, int n)
	{
	const char function[] = "alert_printer_working";
	int _stdin[2];				/* pipe to sendmail */
	int fd;						/* used to open /dev/null in child */
	FILE *sendmail;

	DODEBUG_ALERTS(("%s(prn=\"%s\", frequency=%d, method=\"%s\", address=\"%s\", n=%d)", function, prn, frequency, method, address, n));

	/* If frequency is non-negative, do nothing. */
	if(frequency >= 0)
		return;

	/* If alert not sent, do nothing */
	if( n < (frequency * -1) )
		return;

	/* if no alert method, do nothing */
	if(strcmp(method, "none") == 0)
		return;

	DODEBUG_ALERTS(("%s(): sending alert cancel message", function));

	if(pipe(_stdin))		 /* open a pipe to sendmail */
		{
		error("%s(): pipe() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
		return;
		}

	switch(fork())		 /* duplicate this process */
		{
		case -1:						/* case: fork failed */
			error("%s(): fork() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
			break;

		case 0:							/* case: we are child */
			close(_stdin[1]);
			if(_stdin[1] != 0)
				{
				dup2(_stdin[0],0);
				close(_stdin[0]);
				}

			fd = open("/dev/null", O_WRONLY);	/* open /dev/null */
			if(fd!=1) dup2(fd,1);				/* as stdout */
			if(fd!=2) dup2(fd,2);				/* and stderr */
			if(fd>2) close(fd);
			execl(SENDMAIL_PATH, "sendmail", address, (char*)NULL);
			_exit(242);

		default:						/* case: we are parent */
			close(_stdin[0]);			/* close our copy of read end */
			if((sendmail = fdopen(_stdin[1], "w")) == (FILE*)NULL)
				{
				error("%s(): fdopen() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
				return;
				}

			fprintf(sendmail, "From: %s PPR Spooler <%s>\n", ppr_get_nodename(), USER_PPR);
			fprintf(sendmail, "To: %s\n", address);
			fprintf(sendmail, "Subject: Recovery of %s\n", prn);
			fprintf(sendmail, "\n");
			fprintf(sendmail, "The printer \"%s\" which you were previously notified had\n",prn);
			fprintf(sendmail, "failed, has sucessfully printed a job.  Presumably it has recovered.\n");
			fprintf(sendmail, "If there are further difficulties, you will be informed.\n\n");

			if(n == 1)
				fprintf(sendmail, "By the way, it only failed once before it printed the job.\n");
			else
				fprintf(sendmail, "By the way, it failed %d times before it finally printed the job.\n", n);

			fclose(sendmail);
			break;
		} /* end of switch */

	} /* end of alert_printer_working() */

/* end of file */

