/*
** mouse:~ppr/src/papd/papd.c
** Copyright 1995--2005, Trinity College Computing Center.
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
** Last modified 19 October 2005.
*/

/*
** This is the main module of PPR's new server for Apple's Printer Access 
** Protocol (PAP).
*/

#include "config.h"
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
static pid_t master_pid = (pid_t)0;
static gu_boolean opt_foreground = FALSE;

/*===========================================================================
** Debugging and Logging Code
===========================================================================*/

/*
** This writes a line to the log.  It is called by debug and by the
** exception handler in main().
*/
static void write_logline(const char category[], const char message[])
	{
	int fd;
	pid_t mypid;
	char temp[256];
	
	if(master_pid != 0 && master_pid != (mypid = getpid()))
		snprintf(temp, sizeof(temp), "%s: %s: child %ld: %s\n", category, datestamp(), (long)mypid, message);
	else
		snprintf(temp, sizeof(temp), "%s: %s: %s\n", category, datestamp(), message);

	if(opt_foreground || master_pid == 0)
		{
		write(2, temp, strlen(temp));		/* to stderr */
		}
	else if((fd = open(LOGFILE, O_WRONLY | O_APPEND | O_CREAT, UNIX_644)) != -1)
		{
		write(fd, temp, strlen(temp));
		close(fd);
		}
	} /* end of log() */

/*
** Print a debug message in the log.
**
** This is compiled in even if debugging per-say is
** not compiled in because we always make some calls
** to put some very basic messages in the log file
** and because papd_query.c uses this when query_trace
** is non-zero.
*/
void debug(const char string[], ... )
	{
	va_list va;
	char temp[256];
	va_start(va,string);
	vsnprintf(temp, sizeof(temp), string, va);
	write_logline("DEBUG", temp);
	va_end(va);
	} /* end of debug() */

/* This is needed by try_fontindex() */
void error(const char string[], ... )
	{
	va_list va;
	char temp[256];
	va_start(va,string);
	vsnprintf(temp, sizeof(temp), string, va);
	write_logline("ERROR", temp);
	va_end(va);
	} /* end of debug() */

/*
** Return a copy of a string with control characters
** (non-printable characters) replaced with dots.
** This is used when representing the data stream in
** debugging output.
*/
#ifdef DEBUG
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
#endif

/*===========================================================================
** Signal Handlers
===========================================================================*/

/*
** This is the handler for the signals that are likely to be used to terminate
** papd.  This routine is left in place for the children as it should
** do no harm.
**
** The first time such a signal is caught, this routine sets keep_running to 
** FALSE. If more signals are received before the server exits, they have no
** effect other other than to provide a peevish message in the log.
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

/*
** This do-nothing signal hanlder is used to ignore SIGPIPE.
*/
static void sig_empty_handler(int sig)
	{
	}

/*
** We don't actually do anything other than logging on child termination.
*/
static void sigchld_handler(int signum)
	{
	pid_t pid;
	int wstat;

	while((pid = waitpid((pid_t)-1,&wstat,WNOHANG)) > (pid_t)0)
		{
		if(WCOREDUMP(wstat))
			{
			debug("Child (pid=%i) dumped core",pid);
			}
		else if(WIFSIGNALED(wstat))
			{
			debug("Child (pid=%i) died on signal %i", pid, WTERMSIG(wstat));
			}
		else if(WIFEXITED(wstat))
			{
			#ifndef DEBUG_REAPCHILD
			if(WEXITSTATUS(wstat))
			#endif
				debug("Child (pid=%i) terminated, exit=%i", pid, WEXITSTATUS(wstat));
			}
		else
			{
			debug("unexpected return from wait(), pid=%i",pid);
			}
		}

	} /* end of sigchld_handler() */

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
	struct USER user = {NULL, NULL};

	debug("connexion to %s (%s) from %d:%d", this_adv->PPRname, this_adv->PAPname, net, node);
	
	/* Load more queue configuration information so we can answer queries. */
	if(!(queue_config = queueinfo_new_load_config(this_adv->queue_type, this_adv->PPRname)))
		gu_Throw("%s(): can't information about queue \"%s\"", function, this_adv->PPRname);

	/* If so configured, see if there is a CAP AUFS session
	 * from the same node and if so, accept its authenticated user.
	 */
	login_aufs(net, node, &user);

	/* In this loop we process a sequences of 'jobs'.  A job is 
	 * either a series of queries to be answered or a document 
	 * to be printed.
	 */
	while(TRUE)									/* will loop until we break out */
		{
		at_reset_buffer();						/* fully reset the input buffering code */

		if(pap_getline(sesfd) == (char*)NULL)	/* If we can't get a line, */
			break;								/* the client has hung up. */

		DODEBUG_LOOP(("first line: \"%s\"", line));

		cptr = line + strcspn(line, " \t");		/* locate the 2nd word */
		cptr += strspn(cptr, " \t");

		/* If this is a query, */
		if(strncmp(line, "%!PS-Adobe-", 11) == 0 && strncmp(cptr, "Query", 5) == 0)
			{
			if(query_trace > 0)
				debug("start of query: %s", line);

			/* Note that query may be for login. */
			answer_query(sesfd, queue_config);

			if(query_trace > 0)
				debug("query processing complete");
			}

		/* Not a query, it must be a print job. */
		else
			{
			if(query_trace > 0)
				debug("start of print job: \"%s\"", line);

			/* If the user logged in using RBI (a feature of Apple's driver
			 * LaserWriter 8.6 and later), then retrieve the username.
			 */
			if(!user.username)
				login_rbi(&user);

			/* If the above didn't produce a username and one is 
			 * required (because there is a charge for printing
			 * on this printer (even 0.00 currency units), then
			 * reject the job.
			 */
			if(!user.username && queueinfo_chargeExists(queue_config))
				{
				/*at_reply(sesfd, "%%[ Error: Login required ]%%\r");*/
				at_reply(sesfd,
					"%%[ Error: SecurityError: SecurityViolation: Unknown user, incorrect password or log on is disabled ]%%\r"
					);
				postscript_stdin_flushfile(sesfd);
				exit(5);
				}

			/* Launch ppr and feed it the job from the PAP socket. */
			printjob(sesfd, this_adv, queue_config, net, node, &user, LOGFILE);

			if(query_trace > 0)
				debug("print job processing complete");
			}
		}

    queueinfo_free(queue_config);
	
	DODEBUG_LOOP(("child daemon done"));
	} /* end of connexion_callback() */

/*=====================================================================
** main() and command line parsing
=====================================================================*/

/*
** Send the specified signal to an already running instance of this daemon.
** This is used for --stop and --reload.
*/
static int signal_daemon(int signal_number)
	{
	FILE *f;
	int count;
	long int pid;

	if(!(f = fopen(PIDFILE, "r")))
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

/*
** Action routine for papd --stop.
** Send SIGTERM to the daemon and waits for the papd.pid file to disappear.
*/
static int do_stop(void)
	{
	int ret;
	struct stat statbuf;

	if((ret = signal_daemon(SIGTERM)))
		return ret;

	printf(_("Waiting while %s shuts down..."), myname);
	while(stat(PIDFILE, &statbuf) == 0)
		{
		printf(".");
		fflush(stdout);
		sleep(1);
		}
	printf("\n");

	printf(_("Shutdown complete.\n"));

	return 0;
	} /* end of do_stop() */

/*
 * Open the log file and print the end of it as tail does.
 */
static int do_tail(void)
	{
	int fd;
	char buffer[1024];
	int len = 0;
	struct stat statbuf;

	if(!(fd = open(LOGFILE, O_RDONLY)) == -1)
		gu_Throw(_("Can't open \"%s\", errno=%d (%s)"), LOGFILE, errno, gu_strerror(errno));

	fstat(fd, &statbuf);

	if(statbuf.st_size > 1024)
		{
		lseek(fd, statbuf.st_size - 1024, SEEK_SET);
		}
	
	while(len != -1)
		{	
		while((len = read(fd, buffer, sizeof(buffer))) != -1)
			{
			write(1, buffer, len);
			}
		sleep(1);
		}

	return EXIT_OK;
	}

static const char *option_chars = "";
static const struct gu_getopt_opt option_words[] =
		{
		{"version", 1000, FALSE},
		{"help", 1001, FALSE},
		{"foreground", 1002, FALSE},
		{"stop", 1003, FALSE},
		{"reload", 1004, FALSE},
		{"debug-toggle", 1005, FALSE},
		{"tail", 1006, FALSE},
		{(char*)NULL, 0, FALSE}
		} ;

static void help(FILE *out)
	{
	fprintf(out, _("Usage: %s [--help] [--version] [--foreground] [--stop] [--reload]\n"
						"\t--debug-toggle --tail\n"), myname);
	}

/*
** This routine parses the command line arguments.  Some arguments, suchas
** as --help and --reload cause a function to be called from this routine
** and then this routine returns without furthur command line parsing.
** In such cases, papd exits with the return code from this routine as 
** the exit code.
**
** If daemon startup should procede, this routine returns -1 (which is not
** a valid exit code).
*/
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

			case 1004:					/* --reload */
				return signal_daemon(SIGHUP);

			case 1005:					/* --debug-toggle */
				return signal_daemon(SIGUSR1);

			case 1006:					/* --tail */
				return do_tail();

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

	return -1;
	} /* end of parse_cmdline() */

/*
 * This function takes care of initialization tasks such as setting
 * environment variables, becoming a background process, creating
 * the PID file, and setting signal handlers.  It is called once
 * from main().
 */
static void init(void)
	{
	/* Set environment variables such as PATH and PPR_VERSION. */
	set_ppr_env();

	/* Remove some unnecessary and misleading things from the environment. */
	prune_env();

	/* Change to the PPR home directory. */
	chdir(LIBDIR);

	/* Go into background. */
	gu_daemon(myname, opt_foreground, PPR_UMASK, PIDFILE);

	/* Remove any old log file. */
	unlink(LOGFILE);

	/*
	 * Stash our PID in a file so that it is easier to find the daemon
	 * to shut it down.  This file also serves as a lock file.  If we
	 * can't create this file for some reason, we just skip it.
	 */ 
	master_pid = getpid();
	debug("Daemon starting, master_pid=%ld", (long)master_pid);

	/*
	** Install signal handlers for child termination, shutdown, log
	** level bumping, and broken pipe.
	*/
	signal_restarting(SIGCHLD, sigchld_handler);
	signal_interupting(SIGHUP, termination_handler);
	signal_interupting(SIGINT, termination_handler);
	signal_interupting(SIGTERM, termination_handler);
	signal_restarting(SIGUSR1, sigusr1_handler);			/* debug step */
	signal_restarting(SIGHUP, sighup_sigalrm_handler);		/* reload */
	signal_restarting(SIGALRM, sighup_sigalrm_handler);		/* reload */
	signal_restarting(SIGIO, sigio_handler);				/* reload in 5 seconds */
	signal_interupting(SIGPIPE, sig_empty_handler);
	} /* end of init() */

int main(int argc, char *argv[])
	{
	struct ADV *adv = NULL;
	int ret = 0;

	/* Initialize internation messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_PAPD, LOCALEDIR);
	textdomain(PACKAGE_PAPD);
	#endif

	/* If we are currently running as root, become USER_PPR or die! */
	if((ret = renounce_root_privs(myname, USER_PPR, NULL)))
		return ret;
	
	gu_Try {
		if((ret = parse_cmdline(argc, argv)) >= 0)
			return ret;
	
		init();

		/*
		** The main loop alternates between calling at_service() (which 
		** contains AppleTalk-implementation-dependent code) and calling
		** conf_load().
		*/
		debug("Starting up...");
		while(keep_running)
			{
			if(reload_config)
				{
				debug("Loading configuration...");
				adv = conf_load(adv);
				reload_config = FALSE;
				debug("Waiting for connexions...");
				}
			at_service(adv);
			}
		}
	gu_Catch {
		write_logline("FATAL", gu_exception);
		
		/* This is for children which might have a copy of ppr running. */
		printjob_abort();

		ret = 1;
		}

	/*
	 * If this is the master process, unadvertise any names which may be 
	 * advertised and remove the PID file.
	 */
	if(master_pid && master_pid == getpid())
		{
		debug("Shutting down...");
		if(adv)
			{
			int i;
			debug("Removing advertised names...");
			for(i = 0; adv[i].adv_type != ADV_LAST; i++)
				{
				if(adv[i].adv_type == ADV_ACTIVE)
					{
					debug("%s: unbinding PAP name \"%s\"", adv[i].PPRname, adv[i].PAPname);
					at_remove_name(adv[i].PAPname, adv[i].fd);
					}
				}
			}

		unlink(PIDFILE);
		debug("Shutdown complete");
		}
	
	return ret;
	} /* end of main() */

/* end of file */
