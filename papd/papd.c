/*
** mouse:~ppr/src/papd/papd.c
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

/*
** This is the main module of PPR's new server for Apple's Printer Access 
** Protocol (PAP).
*/

#include "before_system.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <ctype.h>
#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "papd.h"
#include "ppr_exits.h"
#include "util_exits.h"
#include "version.h"

const char myname[] = "papd";

/* Globals related to the input line buffer. */
char line[257];
int line_len;

/* Other globals: */
pid_t master_pid = (pid_t)0;
gu_boolean opt_foreground = FALSE;
int children = 0;						/* The number of our children living. (master daemon) */
char *default_zone = "*";				/* for advertised names */

/* The names of various files. */
static const char *log_file_name = LOGFILE;
static const char *pid_file_name = PIDFILE;

/*
** This function is called by fatal() and debug().
*/
static void log(const char category[], const char format[], va_list va)
	{
	FILE *file;
	pid_t mypid;
	
	if(opt_foreground)
		file = stderr;
	else if((file = fopen(log_file_name, "a")) == (FILE*)NULL)
		return;

	fprintf(file, "%s: %s: ", category, datestamp());
	mypid = getpid();
	if(master_pid != 0 && master_pid != mypid)
		fprintf(file, "child %ld: ", (long)mypid);
	vfprintf(file, format, va);
	fputc('\n', file);

	if(!opt_foreground)
		fclose(file);
	} /* end of log() */

/*
** Print a debug message in the log.
**
** This is compiled in even if debugging per-say is
** not compiled in because we always make some calls
** to put some very basic messages in the log file
** and becuase papd_query.c uses this when query_trace
** is non-zero.
*/
void debug(const char string[], ... )
	{
	va_list va;
	va_start(va,string);
	log("DEBUG", string, va);
	va_end(va);
	} /* end of debug() */

/*
** Return a copy of a string with control characters
** (non-printable characters) replaced with dots.
** This is used when representing the data stream in
** debugging output.
*/
char *debug_string(char *s)
	{
	static char result[80];
	unsigned int x;

	for(x=0; s[x] && x < (sizeof(result)-1); x++)
		{
		if(isprint(s[x]))
			result[x] = s[x];
		else
			result[x] = '.';
		}

	result[x] = '\0';

	return result;
	} /* end of debug_string() */

/*
** This is the handler for the signals that are likely to be used to terminate
** papd.  This routine is left in place for the children as it should
** do no harm.
**
** The first time such a signal is caught, this routine calls fatal(). If more 
** signals are received before the server exits, they have no effect other
** other than to provide a peevish message in the log.
*/
static volatile gu_boolean keep_running = TRUE;
static void termination_handler(int sig)
	{
	static int count = 0;
	if(count++ > 0)
		debug("Signal \"%s\" received, but shutdown already in progress", gu_strsignal(sig));
	keep_running = FALSE;
	} /* end of termination_handler() */

/*
** This SIGHUP and SIGALRM handler tells the main loop to reload the config.
*/
static volatile gu_boolean reload_config = TRUE;
static void sighup_sigalrm_handler(int sig)
	{
	reload_config = TRUE;
	}

/*
** If this is Linux, then we will receive SIGIO every time one of the queue
** configuration directories changes.  So as to not reload many times
** during a batched series of edits, we don't actually start the reload
** but rather schedual it (or reschedual it) for five seconds into the
** future.
*/
static void sigio_handler(int sig)
	{
	alarm(5);
	}

/*========================================================================
** Get a line from the client and store it in line[],
** its length in line_len.
**
** This routine is not dependent on the type of
** AppleTalk stack in use.
========================================================================*/
char *pap_getline(int sesfd)
	{
	int x;
	int c;

	for(x=0; x < 255; x++)		/* read up to 255 characters */
		{
		c = at_getc(sesfd);		/* get the next character */

		if(c=='\r' || c=='\n' || c==-1)
			{					/* If carriage return or line feed or end of file, */
			line[x] = '\0';		/* terminate the line and stop reading. */
			break;
			}

		line[x] = c;			/* Store the character. */
		}

	line_len = x;				/* Store the final length. */

	if(c == -1 && x == 0)		/* If end of file and line was empty */
		return (char*)NULL;		/* return NULL pointer. */

	if(x == 255)				/* If line overflow, eat up extra characters. */
		while( ((c = at_getc(sesfd)) != '\r') && (c != '\n') && (c != -1) );

	return line;
	} /* end of pap_getline() */

/*==============================================================
** Emulate the behavior of the standard PostScript error
** handler in performing a "flushfile" on stdin.
** This routine is AppleTalk independent.
==============================================================*/
void postscript_stdin_flushfile(int sesfd)
	{
	#ifdef DEBUG
	debug("postscript_stdin_flushfile(sesfd=%d)",sesfd);
	#endif

	at_reply(sesfd,"%%[ Flushing: rest of job (to end-of-file) will be ignored ]%%\n");

	while(pap_getline(sesfd) != (char*)NULL)	/* by reading it and */
		{
												/* throwing it away. */
		}
	at_reply_eoj(sesfd);						/* Acknowledge receipt of end of job. */
	at_close_reply(sesfd);						/* Close connexion to printer. */
	} /* end of postscript_stdin_flushfile() */

/*===========================================================
** SIGCHLD handler
===========================================================*/
static void reapchild(int signum)
	{
	pid_t pid;
	int wstat;

	while((pid = waitpid((pid_t)-1,&wstat,WNOHANG)) > (pid_t)0)
		{
		children--;				/* reduce the count of our progeny */

		if(WCOREDUMP(wstat))
			{
			debug("Child papd (pid=%i) dumped core",pid);
			}
		else if(WIFSIGNALED(wstat))
			{
			debug("Child papd (pid=%i) died on signal %i", pid, WTERMSIG(wstat));
			}
		else if(WIFEXITED(wstat))
			{
			#ifndef DEBUG_REAPCHILD
			if(WEXITSTATUS(wstat))
			#endif
			debug("Child papd (pid=%i) terminated, exit=%i", pid, WEXITSTATUS(wstat));
			}
		else
			{
			debug("unexpected return from wait(), pid=%i",pid);
			}
		}

	} /* end of reapchild() */

/*=========================================================================
** This is the main loop function for the child processes which handle
** client connexions.  (The daemon papd forks off a child every time 
** it accepts a connection.)  This function answers queries and dispatches 
** print jobs until the client closes the connection.
**
** This is a callback function for at_service().  This routine is not
** AppleTalk-implementation-dependent because it deals with the network
** exclusively through routines in the AppleTalk-implementation-dependent
** module.
=========================================================================*/
void connexion_callback(int sesfd, struct ADV *this_adv, int net, int node)
	{
	const char function[] = "connexion_callback";
	char *cptr;
	void *queue_config;

	/* Load more queue configuration information so we can answer queries. */
	if(!(queue_config = queueinfo_new(this_adv->queue_type, this_adv->PPRname)))
		gu_Throw("%s(): can't information about queue \"%s\"", function, this_adv->PPRname);

	/* We don't want to use our parent's SIGCHLD handler. */
	signal_restarting(SIGCHLD, printjob_reapchild);

	/* We want to be prepared for the possibility that ppr will die. */
	signal_interupting(SIGPIPE, printjob_sigpipe);

	while(TRUE)									/* will loop until we break out */
		{
		at_reset_buffer();						/* fully reset the input buffering code */

		if(pap_getline(sesfd) == (char*)NULL)	/* If we can't get a line, */
			break;								/* the client has hung up. */

		DODEBUG_LOOP(("first line: \"%s\"", line));

		cptr = line+strcspn(line, " \t");		/* locate the 2nd word */
		cptr += strspn(cptr, " \t");

		/* If this is a query, */
		if(strncmp(line, "%!PS-Adobe-", 11) == 0 && strncmp(cptr, "Query", 5) == 0)
			{
			DODEBUG_LOOP(("start of query: %s", line));

			/* Note that query may be for login. */
			answer_query(sesfd, queue_config);

			DODEBUG_LOOP(("query processing complete"));
			}
		/* Not a query, it must be a print job. */
		else
			{
			DODEBUG_LOOP(("start of print job: \"%s\"", line));

			printjob(sesfd, this_adv, &queue_config, net, node, log_file_name);

			DODEBUG_LOOP(("print job processing complete"));
			}
		}

    queueinfo_delete(queue_config);
	
	DODEBUG_LOOP(("child daemon done"));
	} /* end of connexion_callback() */

/*=====================================================================
** main() and command line parsing
=====================================================================*/

static const char *option_chars = "";
static const struct gu_getopt_opt option_words[] =
		{
		{"version", 1000, FALSE},
		{"help", 1001, FALSE},
		{"foreground", 1002, FALSE},
		{"stop", 1003, FALSE},
		{"reload", 1004, FALSE},
		{(char*)NULL, 0, FALSE}
		} ;

static void help(FILE *out)
	{
	fprintf(out, _("Usage: %s [--help] [--version] [--foreground] [--stop] [--reload]\n"), myname);
	}

/* Send the specified signal to an already running instance of this daemon. */
static int do_signal(int signal_number)
	{
	FILE *f;
	int count;
	long int pid;

	if(!(f = fopen(pid_file_name, "r")))
		{
		fprintf(stderr, _("%s: not running\n"), myname);
		return 1;
		}

	count = fscanf(f, "%ld", &pid);

	fclose(f);

	if(count != 1)
		{
		fprintf(stderr, _("%s: failed to read PID from lock file"), myname);
		return 2;
		}

	printf(_("Sending %s to %s (PID=%ld).\n"), gu_strsignal(signal_number), myname, pid);

	if(kill((pid_t)pid, signal_number) < 0)
		{
		fprintf(stderr, "%s: kill(%ld, %s) failed, errno=%d (%s)\n", myname, pid, gu_strsignal(signal_number), errno, gu_strerror(errno));
		return 3;
		}

	return 0;
	}

/* Send SIGTERM to the daemon and waits for the papd.pid file to disappear. */
static int do_stop(void)
	{
	int ret;
	struct stat statbuf;

	if((ret = do_signal(SIGTERM)))
		return ret;

	printf(_("Waiting while %s shuts down..."), myname);
	while(stat(pid_file_name, &statbuf) == 0)
		{
		printf(".");
		fflush(stdout);
		sleep(1);
		}
	printf("\n");

	printf(_("Shutdown complete.\n"));

	return 0;
	}

/*
** If this program was invoked with excessive permissions (i.e. as root),
** renounce them now.  This program should be setuid "ppr" and setgid "ppr".
**
** We use fputs() and return here because fatal() would put the
** message into the log file which is not where we want it.  Besides,
** it could create a log file with wrong permissions.
*/
static int drop_privs(void)
	{
	uid_t uid, euid;
	gid_t gid, egid;

	uid = getuid();				/* should be "ppr" or "root" */
	euid = geteuid();			/* should be "ppr" */
	gid = getgid();				/* should be "ppr" or "root" */
	egid = getegid();			/* should be "ppop" */

	/* If effective UID is root, either we are setuid root or not setuid and
	   being run by root.  We refuse to tolerate either! */
	if(euid == 0)
		{
		fprintf(stderr, "%s: this program must be setuid %s\n", myname, USER_PPR);
		return(EXIT_INTERNAL);
		}

	if(uid == 0)				/* if user is root */
		{						/* then switch to "ppr" */
		setuid(0);				/* set all three group IDs to "root" */
		setgid(0);				/* set all three group IDs to "root" (Is this necessary?) */

		/* Make sure we don't keep "root" group. */
		if(setregid(egid, egid) == -1)
			{
			fprintf(stderr, "%s: setregid(%ld, %ld) failed, errno=%d (%s)\n", myname, (long)egid, (long)egid, errno, gu_strerror(errno));
			return EXIT_INTERNAL;
			}

		/* Now we may set all three UIDs to "ppr". */
		if(setreuid(euid, euid) == -1)
			{
			fprintf(stderr, "%s: setreuid(%ld, %ld) failed, errno=%d (%s)\n", myname, (long)euid, (long)euid, errno, gu_strerror(errno));
			return EXIT_INTERNAL;
			}

		/* Now be paranoid. */
		if(setuid(0) != -1)
			{
			fprintf(stderr, "%s: setuid(0) didn't fail!\n", myname);
			return EXIT_INTERNAL;
			}
		}
	else						/* not root */
		{
		if(uid != euid)			/* if real user id is not same as owner of papd (euid), */
			{
			fputs("Only \"ppr\" or \"root\" may start papd.\n", stderr);
			return EXIT_DENIED;
			}
		}

	return 0;
	}


/* This is startup code called from main(). */
static int parse_cmdline(int argc, char *argv[])
	{
	int optchar;
	struct gu_getopt_state getopt_state;

	/* Parse the options. */
	gu_getopt_init(&getopt_state, argc, argv, option_chars, option_words);
	while((optchar = ppr_getopt(&getopt_state)) != -1)
		{
		switch(optchar)
			{
			case 1000:					/* --version */
				puts(VERSION);
				puts(COPYRIGHT);
				puts(AUTHOR);
				return EXIT_OK;

			case 1001:					/* --help */
				help(stdout);
				return EXIT_OK;

			case 1002:					/* --foreground */
				opt_foreground = TRUE;
				break;

			case 1003:					/* --stop */
				return do_stop();
				break;

			case 1004:					/* --reload */
				return do_signal(SIGHUP);
				break;

			case '?':					/* help or unrecognized switch */
				fprintf(stderr, "Unrecognized switch: %s\n\n", getopt_state.name);
				help(stderr);
				return EXIT_SYNTAX;

			case ':':					/* argument required */
				fprintf(stderr, "The %s option requires an argument.\n", getopt_state.name);
				return EXIT_SYNTAX;

			case '!':					/* bad aggreation */
				fprintf(stderr, "Switches, such as %s, which take an argument must stand alone.\n", getopt_state.name);
				return EXIT_SYNTAX;

			case '-':					/* spurious argument */
				fprintf(stderr, "The %s switch does not take an argument.\n", getopt_state.name);
				return EXIT_SYNTAX;

			default:					/* missing case */
				fprintf(stderr, "Missing case %d in switch dispatch switch()\n", optchar);
				return EXIT_INTERNAL;
				break;
			}
		}

	return EXIT_OK;
	}

static void daemon_mode(void)
	{
	/* Set environment variables such as PATH and PPR_VERSION. */
	set_ppr_env();

	/* Remove some unnecessary and misleading things from the environment. */
	prune_env();

	/* Go into background. */
	if(!opt_foreground)
		gu_daemon(PPR_UMASK);

	/* Change to the PPR home directory. */
	chdir(HOMEDIR);

	/* Remove any old log file. */
	unlink(log_file_name);

	/* Stash our PID in a file so that it is easier to find the daemon
	   to shut it down.  This file also serves as a lock file. */
	{
	FILE *f;

	master_pid = getpid();
	debug("Daemon starting, master_pid=%ld", (long)master_pid);

	if((f = fopen(pid_file_name, "w")) != (FILE*)NULL)
		{
		fprintf(f, "%ld\n", (long)master_pid);
		fclose(f);
		}
	}

	/*
	** Install signal handlers for child termination, shutdown, log
	** level bumping, and broken pipe.
	*/
	signal_restarting(SIGCHLD, reapchild);
	signal_interupting(SIGHUP, termination_handler);
	signal_interupting(SIGINT, termination_handler);
	signal_interupting(SIGTERM, termination_handler);
	signal_restarting(SIGUSR1, sigusr1_handler);
	signal_restarting(SIGHUP, sighup_sigalrm_handler);
	signal_restarting(SIGALRM, sighup_sigalrm_handler);
	signal_restarting(SIGIO, sigio_handler);
	}

/*
** The main loop alternates between calling at_service() (which 
** contains AppleTalk-implementation-dependent code) and calling
** conf_load().
*/
static int main_loop(void)
	{
	struct ADV *adv = NULL;
	debug("Entering main loop");
	gu_Try {
		while(keep_running)
			{
			if(reload_config)
				{
				adv = conf_load(adv);
				reload_config = FALSE;
				}
			at_service(adv);
			}
		}
	gu_Final {
		if(adv)
			{
			int i;
			debug("Removing advertised names");
			for(i = 0; adv[i].adv_type != ADV_LAST; i++)
				{
				if(adv[i].adv_type == ADV_ACTIVE)
					at_remove_name(adv[i].PAPname, adv[i].fd);
				}
			}
		}
	gu_Catch {
		gu_ReThrow();
		}
	debug("Shutdown complete");
	return 0;
	}

int main(int argc, char *argv[])
	{
	int ret;

	/* Initialize internation messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_PAPD, LOCALEDIR);
	textdomain(PACKAGE_PAPD);
	#endif

	if((ret = drop_privs()))
		return ret;
	
	gu_Try {
		if((ret = parse_cmdline(argc, argv)) != 0)
			return ret;
	
		daemon_mode();

		ret = main_loop();
		}
	gu_Catch {
		log("FATAL", "%s", gu_exception);

		/* This is for children which might have a copy of ppr running. */
		printjob_abort();
		}

	/* remove file with our pid in it */
	unlink(pid_file_name);
	
	return ret;
	} /* end of main() */

/* end of file */
