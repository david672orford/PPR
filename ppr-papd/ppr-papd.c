/*
** mouse:~ppr/src/ppr-papd/ppr-papd.c
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
** Last modified 26 December 2002.
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
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "ppr-papd.h"
#include "ppr_exits.h"
#include "util_exits.h"
#include "version.h"

const char myname[] = "ppr-papd";

/* Globals related to the input line buffer. */
char line[257];         /* 255 characters plus one null plus extra for my CR */
int line_len;		/* tokenize() insists that we set this */

/* Other globals: */
struct ADV *adv = NULL; 
int name_count = 0;          		/* total advertised names */
int i_am_master = TRUE;			/* set to FALSE in child ppr-papd's */
int children = 0;			/* The number of our children living. (master daemon) */
int query_trace = 0;			/* debug level */
char *default_zone = "*";		/* for advertised names */
gu_boolean opt_foreground = FALSE;

/* The names of various files. */
static const char *log_file_name = LOGFILE;
static const char *pid_file_name = PIDFILE;

/*
** This function is called by fatal(), debug(),
** and libppr_throw() below.
*/
static void log(const char category[], const char atfunction[], const char format[], va_list va)
    {
    FILE *file;

    if(opt_foreground)
	file = stderr;
    else if((file = fopen(log_file_name, "a")) == (FILE*)NULL)
	return;

    fprintf(file, "%s: %s: ", category, datestamp());
    if(! i_am_master)
	fprintf(file, "child %ld: ", (long)getpid());
    if(atfunction)
	fprintf(file, "%s(): ", atfunction);
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
** and becuase ppr-papd_query.c uses this when query_trace
** is non-zero.
*/
void debug(const char string[], ... )
    {
    va_list va;
    va_start(va,string);
    log("DEBUG", NULL, string, va);
    va_end(va);
    } /* end of debug() */

/*
** Fatal error function.
**
** Print a message in the log file, remove the advertised names, and exit.
*/
void fatal(int rval, const char string[], ... )
    {
    va_list va;

    va_start(va, string);
    log("FATAL", NULL, string, va);
    va_end(va);

    /* Master daemon cleanup: */
    if(i_am_master)
	{
	int i;
	for(i = 0; adv[i].adv_type != ADV_LAST; i++)
	    {
	    if(adv[i].adv_type == ADV_ACTIVE)
	        remove_name(adv[i].PAPname, adv[i].fd);
	    }
	
	unlink(pid_file_name);		/* remove file with our pid in it */

	debug("Shutdown complete");	/* <-- and let those who are waiting know. */
	}

    /* Daemon's child cleanup: */
    else
	{
	printjob_abort();
	}

    /* Exit with the code we were told to exit with. */
    exit(rval);
    } /* end of fatal() */

/*
** This is for libppr exceptions.  It overrides the version
** in libppr.a.
*/
void libppr_throw(int exception_type, const char exception_function[], const char format[], ...)
    {
    va_list va;
    va_start(va, format);
    log("libppr exception", exception_function, format, va);
    va_end(va);
    exit(1);
    } /* end of libppr_throw() */

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
** This is the handler for the signals that are likely to
** be used to terminate the AppleTalk Printer Server.
**
** The first time such a signal is caught, this routine
** calls fatal().
**
** If more signals are received before the server exits,
** they have no effect.
*/
static void termination_handler(int sig)
    {
    static int count = 0;
    if(count++ > 0)
    	debug("Signal \"%s\" received, termination routine already initiated", gu_strsignal(sig));
    else
	fatal(1, "Received signal \"%s\", shutdown started", gu_strsignal(sig));
    } /* end of termination_handler() */

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

    for(x=0; x < 255; x++)	/* read up to 255 characters */
	{
	c = cli_getc(sesfd);	/* get the next character */

	if(c=='\r' || c=='\n' || c==-1)
	    {			/* If carriage return or line feed or end of file, */
	    line[x] = '\0';	/* terminate the line and stop reading. */
	    break;
	    }

	line[x] = c;		/* Store the character. */
	}

    line_len = x;		/* Store the final length. */

    if(c == -1 && x == 0)	/* If end of file and line was empty */
	return (char*)NULL;	/* return NULL pointer. */

    if(x == 255)		/* If line overflow, eat up extra characters. */
	while( ((c=cli_getc(sesfd)) != '\r') && (c != '\n') && (c != -1) );

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

    reply(sesfd,"%%[ Flushing: rest of job (to end-of-file) will be ignored ]%%\n");

    onebuffer = FALSE;				/* Flush the rest of the job */
    while(pap_getline(sesfd) != (char*)NULL);	/* by reading it and */
						/* throwing it away. */
    reply_eoj(sesfd); 			/* Acknowledge receipt of end of job. */
    close_reply(sesfd);			/* Close connexion to printer. */
    } /* end of postscript_stdin_flushfile() */

/*
** SIGPIPE handler, used by child ppr-papd processes.
*/
void sigpipe_handler(int sig)
    {
    fatal(1, "SIGPIPE received");
    } /* end of sigpipe_handler() */

/*===========================================================
** Note child termination.
** This handler is used only by the main daemon.
===========================================================*/
static void reapchild(int signum)
    {
    pid_t pid;
    int wstat;

    if(!i_am_master)
	{
	DODEBUG_PRINTJOB(("SIGCHLD received (ppr exited)"));
	printjob_reapchild();
	return;
	}
	
    while((pid = waitpid((pid_t)-1,&wstat,WNOHANG)) > (pid_t)0)
	{
	children--;             /* reduce the count of our progeny */

	if(WCOREDUMP(wstat))
	    {
	    debug("Child ppr-papd (pid=%i) dumped core",pid);
	    }
	else if(WIFSIGNALED(wstat))
	    {
	    debug("Child ppr-papd (pid=%i) died on signal %i", pid, WTERMSIG(wstat));
	    }
	else if(WIFEXITED(wstat))
	    {
	    #ifndef DEBUG_REAPCHILD
	    if(WEXITSTATUS(wstat))
	    #endif
	    debug("Child ppr-papd (pid=%i) terminated, exit=%i", pid, WEXITSTATUS(wstat));
	    }
	else
	    {
	    debug("unexpected return from wait(), pid=%i",pid);
	    }
	}

    } /* end of reapchild() */

/*=========================================================================
** This is the main loop function for the child.  (The daemon ppr-papd
** forks off a child every time it accepts a connection.)  This function
** answers queries and dispatches print jobs until the client closes
** the connection.
**
** This is called from within the AppleTalk
** Dependent module.  This routine is not AppleTalk
** dependent because it deals with the network by
** calling routines in the AppleTalk Dependent module.
=========================================================================*/
void child_main_loop(int sesfd, int prnid, int net, int node)
    {
    char *cptr;
    struct QUEUE_CONFIG queue_config;

    conf_load_queue_config(&adv[prnid], &queue_config);

    while(TRUE)			/* will loop until we break out */
	{
	reset_buffer(TRUE);			/* fully reset the input buffering code */

	if(pap_getline(sesfd) == (char*)NULL)	/* If we can't get a line, */
	    break;				/* the client has hung up. */

	DODEBUG_LOOP(("first line: \"%s\"", line));

	cptr = line+strcspn(line, " \t");	/* locate the 2nd word */
	cptr += strspn(cptr, " \t");

	/* If this is a query, */
	if(strncmp(line, "%!PS-Adobe-", 11) == 0 && strncmp(cptr, "Query", 5) == 0)
	    {
	    DODEBUG_LOOP(("start of query: %s", line));

	    /* Note that query may be for login. */
	    answer_query(sesfd, &queue_config);

	    DODEBUG_LOOP(("query processing complete"));
	    }
	/* Not a query, it must be a print job. */
	else
	    {
	    DODEBUG_LOOP(("start of print job: \"%s\"", line));

	    printjob(sesfd, &adv[prnid], &queue_config, net, node, log_file_name);

	    DODEBUG_LOOP(("print job processing complete"));
	    }
	}
    DODEBUG_LOOP(("child daemon done"));
    } /* end of child_main_loop() */

/*=====================================================================
** Turn query tracing on or off.
=====================================================================*/
static void sigusr1_handler(int sig)
    {
    if(++query_trace > 2)
	query_trace = 0;

    switch(query_trace)
	{
	case 0:
	    debug("Query tracing turned off");
	    break;
	default:
	    debug("Query tracing set to level %d",query_trace);
	    break;
	}
    } /* end of sigusr1_handler() */

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
	{(char*)NULL, 0, FALSE}
	} ;

static void help(FILE *out)
    {
    fprintf(out, _("Usage: %s [--help] [--version] [--foreground] [--stop]\n"), myname);
    }

static int do_stop(void)
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

    printf(_("Sending SIGTERM to %s (PID=%ld).\n"), myname, pid);

    if(kill((pid_t)pid, SIGTERM) < 0)
    	{
	fprintf(stderr, "%s: kill(%ld, SIGTERM) failed, errno=%d (%s)\n", myname, pid, errno, gu_strerror(errno));
	return 3;
    	}

    {
    struct stat statbuf;
    printf(_("Waiting while %s shuts down..."), myname);
    while(stat(pid_file_name, &statbuf) == 0)
    	{
	printf(".");
	fflush(stdout);
	sleep(1);
    	}
    printf("\n");
    }

    printf(_("Shutdown complete.\n"));

    return 0;
    }

int main(int argc, char *argv[])
    {
    int optchar;
    struct gu_getopt_state getopt_state;

    /* Initialize internation messages library. */
    #ifdef INTERNATIONAL
    setlocale(LC_MESSAGES, "");
    bindtextdomain(PACKAGE_PPR_PAPD, LOCALEDIR);
    textdomain(PACKAGE_PPR_PAPD);
    #endif

    /*
    ** If this program was invoked with excessive permissions (i.e. as root),
    ** renounce them now.  This program should be setuid "ppr" and setgid "ppop".
    **
    ** We use fputs() and exit() here because fatal() would put the
    ** message into the log file which is not where we want it.  Besides,
    ** it could create a log file with wrong permissions.
    */
    {
    uid_t uid, euid;
    gid_t gid, egid;

    uid = getuid();		/* should be "ppr" or "root" */
    euid = geteuid();		/* should be "ppr" */
    gid = getgid();		/* should be "ppr" or "root" */
    egid = getegid();		/* should be "ppop" */

    if(uid == 0)		/* if user is root */
	{			/* then switch to "ppr" */
	setuid(0);		/* set all three group IDs to "root" */
	setgid(0);		/* set all three group IDs to "root" (Is this necessary?) */
	setuid(euid);		/* now we may set all three UIDs to "ppr" */
	setgid(egid);		/* Make sure we don't keep "root" group */
	}
    else			/* not root */
	{
	if(uid != euid)		/* if user id is not same as owner of ppr-papd, */
	    {
	    fputs("Only \"ppr\" or \"root\" may start ppr-papd.\n", stderr);
	    exit(EXIT_DENIED);
	    }
	}
    }

    /* Parse the options. */
    gu_getopt_init(&getopt_state, argc, argv, option_chars, option_words);
    while((optchar = ppr_getopt(&getopt_state)) != -1)
    	{
	switch(optchar)
	    {
	    case 1000:			/* --version */
	    	puts(VERSION);
	    	puts(COPYRIGHT);
	    	puts(AUTHOR);
	    	exit(EXIT_OK);

	    case 1001:			/* --help */
	    	help(stdout);
	    	exit(EXIT_OK);

	    case 1002:			/* --foreground */
		opt_foreground = TRUE;
	    	break;

	    case 1003:			/* --stop */
		exit(do_stop());
	    	break;

	    case '?':			/* help or unrecognized switch */
		fprintf(stderr, "Unrecognized switch: %s\n\n", getopt_state.name);
		help(stderr);
		exit(EXIT_SYNTAX);

	    case ':':			/* argument required */
	    	fprintf(stderr, "The %s option requires an argument.\n", getopt_state.name);
		exit(EXIT_SYNTAX);

	    case '!':			/* bad aggreation */
	    	fprintf(stderr, "Switches, such as %s, which take an argument must stand alone.\n", getopt_state.name);
	    	exit(EXIT_SYNTAX);

	    case '-':			/* spurious argument */
	    	fprintf(stderr, "The %s switch does not take an argument.\n", getopt_state.name);
	    	exit(EXIT_SYNTAX);

	    default:			/* missing case */
	    	fprintf(stderr, "Missing case %d in switch dispatch switch()\n", optchar);
	    	exit(EXIT_INTERNAL);
		break;
	    }
    	}

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
    pid_t pid;

    pid = getpid();
    debug("Daemon starting, pid=%ld", (long)pid);

    if((f = fopen(pid_file_name, "w")) != (FILE*)NULL)
    	{
    	fprintf(f, "%ld\n", (long)pid);
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
    signal_interupting(SIGPIPE, sigpipe_handler);

    /*
    ** Read the configuration file and advertise
    ** the printer names on the AppleTalk.
    */
    adv = conf_load(adv);

    /*
    ** Call the Appletalk dependent main loop which will fork off a
    ** child daemon to handle each incoming connection.
    ** This function contains the main loop.  It never returns.
    */
    debug("Entering main loop");
    appletalk_dependent_daemon_main_loop(adv);

    return 0;				/* <-- this makes GNU-C happy */
    } /* end of main() */

/* end of file */
