/*
** mouse:~ppr/src/pprdrv/pprdrv_commentary.c
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
** Last modified 28 May 2004.
*/

#include "before_system.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "pprdrv.h"
#include "interface.h"
#include "commentary.h"

/*
** This module is the guts of the PPR commentator mechanism.  Commentators
** are programs which are used to send messages to someone about how
** pprdrv is progressing with printing the job.  They talk about things
** like the printer failing to accept data or having its cover open.
**
** The function commentary() is called whenever something happens which might
** be interesting to an operator.  The value of flags indicates
** the catagory of the event.  The string "message" indicates
** precisely what has occured.
**
** The behaviour of this function is defined by the "Commentator:"
** lines in the printer's configuration file.  A commentator line
** has the format:
**
** Commentator: flags name address "options"
**
** The flags field indicates what types of messages should be reported
** using this commentator.  The name indicates which commentator to use.
** If it is "file", the information is written to the file indicated by
** the "address" field.  If the commentator name is not "file", it is the
** name of a program in ~ppr/commentators which should be executed.
**
** We do not wait here for the commentator to exit.  Instead, pprdrv waits
** for all its children to exit before it exits itself.  This is done
** by calling commentator_wait().
*/

/*
** This function is called from commentary() to write the message to a file.
*/
static void commentary_file(const struct COMMENTATOR *com, int category, const char cooked[], const char raw1[], const char raw2[], int severity)
	{
	const char function[] = "commentary_file";
	FILE *f;
	time_t seconds_now;					/* For time stamping the */
	struct tm *time_detail;				/* print log file. */
	char time_str[20];

	if(!com->address)
		fatal(EXIT_PRNERR_NORETRY, "%s(): assertion failed", function);

	seconds_now = time((time_t*)NULL);
	time_detail = localtime(&seconds_now);
	strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_detail);

	if((f = fopen(com->address, "a")))
		{
		fprintf(f, "%s %s %d \"%s\" \"%s\" \"%s\" %d\n",
				time_str,
				printer.Name, category,
				cooked,
				raw1 ? raw1 : "",
				raw2 ? raw2 : "",
				severity);
		fclose(f);
		}
	} /* end of commentary_file() */

/*
** This function is called from commentary() to run a program to send the message.
*/
static void commentary_spawn(const struct COMMENTATOR *com, int category, const char cooked[], const char raw1[], const char raw2[], int severity, const char canned_message[])
	{
	const char function[] = "commentary_spawn";
	char fname[MAX_PPR_PATH];
	const char *progfile;

	if(!com->progname || !com->address)
		fatal(EXIT_PRNERR_NORETRY, "%s(): assertion failed", function);

	if(com->progname[0] == '/')
		{
		progfile = com->progname;
		}
	else
		{
		ppr_fnamef(fname, "%s/%s", COMDIR, com->progname);
		progfile = fname;
		}

	if(access(progfile, X_OK) < 0)
		{
		error("Can't execute commentator \"%s\", errno=%d (%s)", progfile, errno, gu_strerror(errno) );
		}
	else
		{
		int fd;
		char category_str[4];
		char severity_str[2];
		snprintf(category_str, sizeof(category_str), "%d", category);
		snprintf(severity_str, sizeof(severity_str), "%d", severity);

		switch( fork() )
			{
			case -1:			/* error */
				error("Fork() failed while executing commentator \"%s\", errno=%d (%s)", progfile, errno, gu_strerror(errno) );
				break;

			case 0:				/* child */
				if((fd = open("/dev/null", O_RDWR)) != -1)
					{
					if(fd != 0)
						{
						dup2(fd, 0);
						close(fd);
						}
					}
				if((fd = open(PPRDRV_LOGFILE, O_WRONLY | O_CREAT | O_APPEND, UNIX_644)) != -1)
					{
					if(fd != 1) dup2(fd, 1);
					if(fd != 2) dup2(fd, 2);
					if(fd > 2) close(fd);
					}
				execl(progfile, com->progname,
						com->address,
						com->options ? com->options : "",
						printer.Name,
						category_str,
						cooked,
						raw1 ? raw1 : "",
						raw2 ? raw2 : "",
						severity_str,
						canned_message,
						(char*)NULL);
				error("exec(\"%s\", ...) failed, errno=%d (%s)", progfile, errno, gu_strerror(errno));
				exit(242);

			default:			/* parent */
				break;
			}
		}
	} /* end of commentary_spawn() */

/*
** This is the function which is called by other parts of pprdrv when
** there is something interesting to report.
*/
void commentary(int category, const char cooked[], const char raw1[], const char raw2[], int severity)
	{
	FUNCTION4DEBUG("commentary")
	static int commentary_seq_number = 0;

	struct COMMENTATOR *com;
	char canned_message[1024];

	DODEBUG_COMMENTARY(("%s(category=%d, cooked=\"%s\", raw1=\"%s\", raw2=\"%s\", severity=%d)",
		function, category, cooked, raw1 ? raw1 : "", raw2 ? raw2 : "", severity));

	commentary_seq_number++;

	/*
	** Filter out EXIT_ENGAGED messages that follow the printer
	** error "off line" since they would be annoyingly redundant.
	*/
	{
	static char prev_pmsg[15] = {'\0'};

	if(category & COM_EXIT && raw1 && strcmp(raw1, "EXIT_ENGAGED") == 0
				&& strcmp(prev_pmsg, "off line") == 0)
		{
		DODEBUG_COMMENTARY(("%s(): skipping redundant message", function));
		return;
		}

	if(category & COM_PRINTER_ERROR)
		{
		strncpy(prev_pmsg, cooked, sizeof(prev_pmsg));
		prev_pmsg[sizeof(prev_pmsg) - 1] = '\0';
		}
	}

	/*
	** If in test mode, we don't actually send the commentary since we are
	** not actually printing.
	*/
	if(test_mode)
		{
		fprintf(stderr, "commentary: category=%d, cooked=\"%s\", raw1=\"%s\", raw2=\"%s\"\n",
				category, cooked, raw1 ? raw1 : "", raw2 ? raw2 : "");
		return;
		}

	/*
	** Prepare the canned message.
	*/
	{
	const char *job_title = job.Title ? job.Title : job.lpqFileName ? job.lpqFileName : "untitled";
	int duration = raw2 ? atoi(raw2) : 0;
	canned_message[0] = '\0';

	if(category & COM_PRINTER_ERROR)
		{
		snprintf(canned_message, sizeof(canned_message),
				severity > 5
					?
					commentary_seq_number == 1
						?
						_("The printer \"%s\", which could print your job \"%s-%d\" (%s), "
						"reports the error \"%s%s%s%s\".  This condition must be "
						"corrected before your job can start.")
						:
						_("The printer \"%s\", which is printing your job \"%s-%d\" (%s), "
						"reports the error \"%s%s%s%s\".  This condition must be "
						"corrected before your job can continue.")
					:
					_("The printer \"%s\", which is printing your job \"%s-%d\" (%s), "
					"reports the error \"%s%s%s%s\".  It may be to your benefit "
					"to correct this condition."),
				printer.Name,
				job.destname, job.id,
				job_title,
				cooked,
				raw1 ? " (" : "",
				raw1 ? raw1 : "",
				raw1 ? ")" : ""
				);
		if(duration > 1)
			{
			gu_snprintfcat(canned_message, sizeof(canned_message),
				_("  This condition has persisted for %d minutes."),
				duration);
			}
		}

	if(category & COM_EXIT && severity > 5)
		{
		snprintf(canned_message, sizeof(canned_message),
				_("The printer \"%s\" cannot start your job \"%s-%d\" (%s) "
				"because of the error condition \"%s\"."),
			printer.Name,
			job.destname, job.id,
			job_title,
			cooked
			);
		}

	if(category & COM_STALL)
		{
		snprintf(canned_message, sizeof(canned_message),
				_("The printer \"%s\", which is printing your job \"%s-%d\" (%s), "
				"%s."),
			printer.Name,
			job.destname, job.id,
			job_title,
			cooked
			);
		}

	gu_wordwrap(canned_message, 75);
	}

	/*
	** Send the message to every commentator that is interested
	** in events of this category.
	*/
	for(com = printer.Commentators; com; com=com->next)
		{
		DODEBUG_COMMENTARY(("%s(): com->interests = %d", function, com->interests));
		if((category & com->interests) == category)		/* if interested, */
			{
			DODEBUG_COMMENTARY(("commentary(): someone is interested!"));

			if(strcmp(com->progname, "file") == 0)
				commentary_file(com, category, cooked, raw1, raw2, severity);
			else
				commentary_spawn(com, category, cooked, raw1, raw2, severity, canned_message);
			}
		} /* end of loop which goes thru all the commentators */

	/*
	** Here is where we handle the ppr --commentary option.
	**
	** Notice that the "file" commentator won't work.  We consider it bad to
	** allow users to write to files of their choosing!  We also don't allow
	** commentator names which contain slashes.  This is to limit users to
	** the commentators in the commentators/ directory.
	*/
	DODEBUG_COMMENTARY(("%s(): job.commentary = %d", function, job.commentary));
	if((category & job.commentary) == category)
		{
		struct COMMENTATOR temp;
		DODEBUG_COMMENTARY(("commentary(): job submitter is interested"));
		temp.progname = job.responder;
		temp.address = job.responder_address;
		temp.options = job.responder_options;
		commentary_spawn(&temp, category, cooked, raw1, raw2, severity, canned_message);
		}

	/*
	** Allow the WWW interface and things like that to see the message.
	*/
	{
	char buffer[256];
	snprintf(buffer, sizeof(buffer), "COMMENTARY %s %d \"%s\" \"%s\" \"%s\" %d\n",
		printer.Name, category, cooked, raw1 ? raw1 : "", raw2 ? raw2 : "", severity);
	state_update_pprdrv_puts(buffer);
	}

	} /* end of commentary() */

/*
** This function will be called before we exit.
*/
void commentary_exit_hook(int rval, const char explain[])
	{
	int com_code = COM_EXIT;
	const char *cooked;
	const char *raw;
	int severity;

	/*
	** Allocate space for the composite string,
	** adding space for string if it turned out to
	** be non-NULL.
	*/
	switch(rval)
		{
		case EXIT_PRINTED:
			cooked = "has printed a job";
			raw = "EXIT_PRINTED";
			severity = 1;
			break;
		case EXIT_PRNERR:
			cooked = "printer error";
			raw = "EXIT_PRNERR";
			severity = 7;
			break;
		case EXIT_PRNERR_NORETRY:
			cooked = "printer error, no retry";
			raw = "EXIT_PRNERR_NORETRY";
			severity = 10;
			break;
		case EXIT_JOBERR:
			cooked = "job error";
			raw = "EXIT_JOBERR";
			severity = 3;
			break;
		case EXIT_SIGNAL:
			cooked = "interface program killed";
			raw = "EXIT_SIGNAL";
			severity = 10;
			break;
		case EXIT_ENGAGED:
			cooked = "otherwise engaged or off-line";
			raw = "EXIT_ENGAGED";
			severity = 6;
			break;
		case EXIT_STARVED:
			cooked = "starved for system resources";
			raw = "EXIT_STARVED";
			severity = 7;
			break;
		case EXIT_INCAPABLE:
			cooked = "incapable of printing this job";
			raw = "EXIT_INCAPABLE";
			severity = 2;
			break;
		default:
			cooked = "invalid exit code";
			raw = "EXIT_?";
			severity = 10;
			break;
		}

	/*
	** If the user supplied an explaination, use it as the cooked
	** argument for commentary() in stead of the default one.
	*/
	if(explain)
		cooked = explain;

	/* This is for ppr-panel. */
	{
	char buffer[256];
	snprintf(buffer, sizeof(buffer), "PEXIT %s %s\n", printer.Name, raw);
	state_update_pprdrv_puts(buffer);
	}
	
	/*
	** Feed the whole thing to the commentary function.
	** Then, wait for the commentators to exit.
	*/
	commentary(com_code, cooked, raw, NULL, severity);
	} /* end of commentary_exit_hook() */

/*
** This is an empty signal handler for commentator_wait().
** It catches SIGCHLD and SIGALRM because someone has to.
*/
static void empty_handler(int sig)
	{
	}

/*
** Wait for any remaining commentators (actually, any remaining
** child processes) to exit.  If they don't exit within 60 seconds,
** send SIGHUP to our entire process group (pprdrv and all its
** children).  Feel free to disable this code if you don't like it.
**
** This function is called just before pprdrv exits.  It is called at
** the end of main() and from commentator_wait().  Among other things,
** this implies it is called when fatal() is called.
*/
void commentator_wait(void)
	{
	int saved_errno;

	signal_restarting(SIGCHLD, empty_handler);
	signal_interupting(SIGALRM, empty_handler);

	alarm(60);
	while(wait((int*)NULL) >= 0) ;
	saved_errno = errno;
	alarm(0);

	/* If the timeout expired, kill any remaining child processes. */
	if(saved_errno == EINTR)
		{
		alert(printer.Name, FALSE, "Timeout after 60 seconds waiting for commentator(s) to exit.");
		signal_restarting(SIGHUP, SIG_IGN);
		kill(-1 * getpid(), SIGHUP);
		}

	} /* end of commentator_wait() */

/* end of file */

