/*
** mouse:~ppr/src/papsrv/papsrv.c
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
** AppleTalk Printer Access Protocol server.  Main module.
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

#include "papsrv.h"
#include "ppr_exits.h"
#include "util_exits.h"
#include "version.h"

const char myname[] = "papsrv";

/* The table of advertised names. */
struct ADV adv[PAPSRV_MAX_NAMES];
int name_count = 0;

/* Globals related to the input line buffer. */
char line[257];			/* 255 characters plus one null plus extra for my CR */
int line_len;			/* tokenize() insists that we set this */

/* Other globals: */
int i_am_master = TRUE;					/* set to FALSE in child papsrv's */
int children = 0;						/* The number of our children living. (master daemon) */
jmp_buf printjob_env;					/* saved environment for longjump() */
pid_t pid = (pid_t)0;					/* pid of ppr */
char *aufs_security_dir = (char*)NULL;	/* for use with CAP AUFS security */
int query_trace = 0;					/* debug level */
char *default_zone = "*";				/* for advertised names */

/* The names of various files. */
static char *log_file_name = DEFAULT_PAPSRV_LOGFILE;
static char *pid_file_name = DEFAULT_PAPSRV_PIDFILE;

/*
** This function is called by fatal(), debug(), and error().
*/
static void log(const char category[], const char atfunction[], const char format[], va_list va)
	{
	FILE *file;
	if((file = fopen(log_file_name, "a")) != (FILE*)NULL)
		{
		fprintf(file, "%s: %s: ", category, datestamp());
		if(! i_am_master)
			fprintf(file, "child %ld: ", (long)getpid());
		if(atfunction) fprintf(file, "%s(): ", atfunction);
		vfprintf(file, format, va);
		fputc('\n', file);
		fclose(file);
		}
	} /* end of log() */

/*
** Print a debug message in the log.
**
** This is compiled in even if debugging per-say is
** not compiled in because we always make some calls
** to put some very basic messages in the log file
** and becuase papsrv_query.c uses this when query_trace
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
** This function is called by libpprdb functions.
*/
void error(const char string[], ... )
	{
	va_list va;
	va_start(va, string);
	log("ERROR", NULL, string, va);
	va_end(va);
	} /* end of error() */

/*
** Fatal error function.
**
** Print a message in the log file, call the AppleTalk
** cleanup routine, and exit.
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
		appletalk_dependent_cleanup();	/* let AppleTalk remove names, etc */
		unlink(pid_file_name);			/* remove file with our pid in it */
		debug("Shutdown complete");		/* <-- and let those who are waiting know. */
		}

	/* Daemon's child cleanup: */
	else
		{
		if(pid)							/* if we have launched ppr, */
			kill(pid, SIGTERM);			/* then kill it */
		}

	/* Exit with the code we were told to exit with. */
	exit(rval);
	} /* end of fatal() */

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
		if( isprint(s[x]) )
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
** calls fatal() which will call appletalk_independent_cleanup().
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
	} /* end of cleanup() */

/*
** Get a line from the client and store it in line[],
** its length in line_len.
**
** This routine is not dependent on the type of
** AppleTalk stack in use.
*/
char *pap_getline(int sesfd)
	{
	int x;
	int c;

	for(x=0; x < 255; x++)		/* read up to 255 characters */
		{
		c = cli_getc(sesfd);	/* get the next character */

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

	onebuffer = FALSE;							/* Flush the rest of the job */
	while(pap_getline(sesfd) != (char*)NULL);	/* by reading it and */
												/* throwing it away. */
	reply_eoj(sesfd);					/* Acknowledge receipt of end of job. */
	close_reply(sesfd);					/* Close connexion to printer. */
	} /* end of postscript_stdin_flushfile() */

/*===========================================================================
** Accept a print job and send it to ppr.
**
** This is called from child_main_loop() which it turn is called
** from appletalk_dependent_main_loop().
===========================================================================*/
void printjob(int sesfd, int prnid, int net, int node, const char username[], int preauthorized)
	{
	int pipefds[2];				/* a set of file descriptors for pipe to ppr */
	int wstat;					/* wait status */
	int error;
	unsigned int free_blocks, free_files;
	char netnode[10];
	char proxy_for[64];

	DODEBUG_PRINTJOB(("printjob(sesfd=%d, prnid=%d, net=%d, node=%d, preauthorized=%d)",
		sesfd, prnid, net, node, preauthorized));

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
	if(setjmp(printjob_env) == 0)		/* setjmp() returns zero when it is called, */
		{								/* non-zero when longjump() is called */
		DODEBUG_PRINTJOB(("setjmp() returned zero, spawning ppr"));

		if(pipe(pipefds))				/* pipe for sending data to ppr */
			fatal(0,"printjob(): pipe() failed, errno=%d (%s)",errno,gu_strerror(errno));

		if( (pid=fork()) == -1 )		/* if we can't fork, */
			{							/* then tell the client */
			reply(sesfd,MSG_NOPROC);
			postscript_stdin_flushfile(sesfd);
			fatal(1,"printjob(): fork() failed, errno=%d",errno);
			}

		if(pid==0)						/* if child */
			{
			const char *argv[MAX_ARGV+20];
			int x;
			int fd;

			close(pipefds[1]);			/* don't need write end */
			dup2(pipefds[0],0);			/* as for read end, duplicate to stdin */
			close(pipefds[0]);			/* and close origional */

			if( (fd=open(log_file_name, O_WRONLY | O_APPEND | O_CREAT,UNIX_755)) == -1 )
				fatal(0, "printjob(): can't open log file");
			if(fd != 1) dup2(fd,1);		/* stdout and */
			if(fd != 2) dup2(fd,2);		/* stderr */
			if(fd > 2) close(fd);

			/* Build the argument array. */
			x = 0;
			argv[x++] = "ppr";			/* name of program */

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

			/* If authorization already verified, add "-A". */
			if(preauthorized)
				argv[x++] = "-A";

			/* end of argument list */
			argv[x] = (char*)NULL;

			#ifdef DEBUG_PPR_ARGV
			{
			int y;
			for(y=0; argv[y]!=(char*)NULL; y++)
				debug("argv[%d] = \"%s\"",y,argv[y]);
			}
			#endif

			execv(PPR_PATH, (char **)argv);		/* execute ppr */

			_exit(255);					/* exit if exec fails */
			}

		/*
		** Parent only from here on.  Parent clone of papsrv daemon, child is ppr.)
		*/
		close(pipefds[0]);			/* we don't need read end */

		/*
		** Call the AppleTalk dependent code to copy the job.
		*/
		if(! appletalk_dependent_printjob(sesfd, pipefds[1]))	/* Make sure we got */
			fatal(1,"Print job was truncated.");				/* EOJ from client. */
																/* (This doesn't do much good.) */
		} /* end of if(setjump()) */

	/*--------------------------------------------------------------*/
	/* We end up here when the job is done or longjmp() is called.	*/
	/* longjmp() is called from within the SIGCHLD handler.			*/
	/*--------------------------------------------------------------*/
	DODEBUG_PRINTJOB(("after setjmp() clause"));

	/* We will no longer be wanting to kill ppr, so we can forget its PID. */
	pid = (pid_t)0;

	/* Close the pipe to ppr.  (This tells ppr it has the whole job.) */
	close(pipefds[1]);

	/* Wait for ppr to terminate. */
	wait(&wstat);

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
	else if(WCOREDUMP(wstat))			/* If core dump created, */
		{
		reply(sesfd,"%%[ Error: papsrv: ppr dumped core ]%%\n");
		}
	else if(WIFSIGNALED(wstat))			/* If child caught a signal, */
		{
		reply(sesfd, "%%[ Error: papsrv: ppr killed ]%%\n");
		}
	else								/* Other return from wait(), such as stopped, */
		{
		reply(sesfd, "%%[ Error: papsrv: unexpected return from wait() ]%%\n");
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

/*
** This will be the reapchild handler while printjob() is executing.
** If ppr exits prematurly, this breaks the loop above.
*/
void printjob_reapchild(int signum)
	{
	DODEBUG_PRINTJOB(("SIGCHLD received (ppr exited)"));
	longjmp(printjob_env, 1);
	} /* end of printjob_reapchild() */

/*
** SIGPIPE handler, used by child PAPSRVs for when PPR dies.
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

	while( (pid=waitpid((pid_t)-1,&wstat,WNOHANG)) > (pid_t)0 )
		{
		children--;				/* reduce the count of our progeny */

		if(WCOREDUMP(wstat))
			{
			debug("Child papsrv (pid=%i) dumped core",pid);
			}
		else if(WIFSIGNALED(wstat))
			{
			debug("Child papsrv (pid=%i) died on signal %i", pid, WTERMSIG(wstat));
			}
		else if(WIFEXITED(wstat))
			{
			#ifndef DEBUG_REAPCHILD
			if(WEXITSTATUS(wstat))
			#endif
			debug("Child papsrv (pid=%i) terminated, exit=%i", pid, WEXITSTATUS(wstat));
			}
		else
			{
			debug("unexpected return from wait(), pid=%i",pid);
			}
		}

	} /* end of reapchild() */

/*
** This is the main loop function for the child.  (The daemon papsrv
** forks off a child every time it accepts a connection.)  This function
** answers queries and dispatches print jobs until the client closes
** the connection.
**
** This is called from within the AppleTalk
** Dependent module.  This routine is not AppleTalk
** dependent because it deals with the network by
** calling routines in the AppleTalk Dependent module.
*/
void child_main_loop(int sesfd, int prnid, int net, int node)
	{
	char *cptr;
	const char *username = (const char*)NULL;
	int preauthorized = FALSE;	/* can we vouch for the user? */

	while(TRUE)					/* will loop until we break out */
		{
		reset_buffer(TRUE);						/* fully reset the input buffering code */

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
			answer_query(sesfd, prnid, &username, &preauthorized);

			DODEBUG_LOOP(("query processing complete"));
			}
		/* Not a query, it must be a print job. */
		else
			{
			DODEBUG_LOOP(("start of print job: \"%s\"", line));

			/* If answer_query() did not get a "%%Login:" command, */
			if( ! preauthorized )
				preauthorize(sesfd, prnid, net, node, &username, &preauthorized);

			printjob(sesfd, prnid, net, node, username, preauthorized);

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
	if(++query_trace > 2) query_trace = 0;

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
** main function
** Setup and main loop.
=====================================================================*/
static const char *option_chars = "f:l:p:X:z:";
static const struct gu_getopt_opt option_words[] =
		{
		{"help", 1000, FALSE},
		{"version", 1001, FALSE},
		{"stop", 1003, FALSE},
		{(char*)NULL, 0, FALSE}
		} ;

static void help(FILE *out)
	{
	fputs("Usage: papsrv [-f conffile] [-l logfile] [-p pidfile] [-X] [-z zone]\n", out);
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
	struct sigaction sig;
	char *conf_file_name = DEFAULT_PAPSRV_CONFFILE;
	int optchar;						/* for ppr_getopt() */
	struct gu_getopt_state getopt_state;
	int z_switch = FALSE;

	/* Initialize internation messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_PAPSRV, LOCALEDIR);
	textdomain(PACKAGE_PAPSRV);
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

	uid = getuid();				/* should be "ppr" or "root" */
	euid = geteuid();			/* should be "ppr" */
	gid = getgid();				/* should be "ppr" or "root" */
	egid = getegid();			/* should be "ppop" */

	/* If effective UID is root, either we are setuid root or not setuid and
	   being run by root.  We refuse to tolerate either! */
	if(euid == 0)
		{
		fprintf(stderr, "%s: this program must be setuid %s\n", myname, USER_PPR);
		exit(EXIT_INTERNAL);
		}

	if(uid == 0)				/* if user is root */
		{						/* then switch to "ppr" */
		setuid(0);				/* set all three group IDs to "root" */
		setgid(0);				/* set all three group IDs to "root" (Is this necessary?) */

		/* Make sure we don't keep "root" group */
		if(setregid(egid, egid) == -1)
			{
			fprintf(stderr, "%s: setregid(%ld, %ld) failed, errno=%d (%s)\n", myname, (long)egid, (long)egid, errno, gu_strerror(errno));
			exit(EXIT_INTERNAL);
			}

		/* Now we may set all three UIDs to "ppr" */
		if(setreuid(euid, euid) == -1)
			{
			fprintf(stderr, "%s: setreuid(%ld, %ld) failed, errno=%d (%s)\n", myname, (long)euid, (long)euid, errno, gu_strerror(errno));
			exit(EXIT_INTERNAL);
			}

		/* Now be paranoid. */
		if(setuid(0) != -1)
			{
			fprintf(stderr, "%s: setuid(0) didn't fail!\n", myname);
			exit(EXIT_INTERNAL);
			}
		}
	else						/* not root */
		{
		if(uid != euid)			/* if user id is not same as owner of papsrv, */
			{
			fputs("Only \"ppr\" or \"root\" may start papsrv.\n", stderr);
			exit(EXIT_DENIED);
			}
		}
	}

	/* Parse the options. */
	gu_getopt_init(&getopt_state, argc, argv, option_chars, option_words);
	while( (optchar=ppr_getopt(&getopt_state)) != -1 )
		{
		switch(optchar)
			{
			case 'f':							/* specifies papsrv configuration file name */
				conf_file_name = getopt_state.optarg;
				break;
			case 'l':							/* log file */
				log_file_name = getopt_state.optarg;
				break;
			case 'p':							/* pid file */
				pid_file_name = getopt_state.optarg;
				break;
			case 'X':							/* specifies AUFS security file name */
				aufs_security_dir = getopt_state.optarg;
				break;
			case 'z':							/* Default zone */
				if( strlen(default_zone = getopt_state.optarg) > MAX_ATALK_NAME_COMPONENT_LEN )
					{
					fputs("papsrv: -z argument is too long", stderr);
					exit(EXIT_SYNTAX);
					}
				z_switch = TRUE;
				break;

			case 1000:					/* --help */
				help(stdout);
				exit(EXIT_OK);

			case 1001:					/* --version */
				puts(VERSION);
				puts(COPYRIGHT);
				puts(AUTHOR);
				exit(EXIT_OK);

			case 1003:					/* --stop */
				exit(do_stop());
				break;

			case '?':					/* help or unrecognized switch */
				fprintf(stderr, "Unrecognized switch: %s\n\n", getopt_state.name);
				help(stderr);
				exit(EXIT_SYNTAX);

			case ':':					/* argument required */
				fprintf(stderr, "The %s option requires an argument.\n", getopt_state.name);
				exit(EXIT_SYNTAX);

			case '!':					/* bad aggreation */
				fprintf(stderr, "Switches, such as %s, which take an argument must stand alone.\n", getopt_state.name);
				exit(EXIT_SYNTAX);

			case '-':					/* spurious argument */
				fprintf(stderr, "The %s switch does not take an argument.\n", getopt_state.name);
				exit(EXIT_SYNTAX);

			default:					/* missing case */
				fprintf(stderr, "Missing case %d in switch dispatch switch()\n", optchar);
				exit(EXIT_INTERNAL);
				break;
			}
		}

	/*
	** If no -z switch was used, try to read a default zone out of
	** /etc/ppr/papsrv_default_zone
	*/
	if( ! z_switch )
		{
		FILE *zf;
		int len;

		if( (zf=fopen(PAPSRV_DEFAULT_ZONE_FILE,"r")) != (FILE*)NULL )
			{
			char line[MAX_ATALK_NAME_COMPONENT_LEN+2];

			if( fgets(line,sizeof(line),zf) != (char*)NULL )
				{
				if( (len=strcspn(line,"\r\n")) > MAX_ATALK_NAME_COMPONENT_LEN )
					{
					fputs("papsrv: default zone in \"%s\" is too long\n",stderr);
					exit(EXIT_SYNTAX);
					}

				line[len] = '\0';
				default_zone = gu_strdup(line);
				}

			fclose(zf);
			}
		}

	/*
	** For safety, set some environment variables.
	** We do this dispite the fact that we don't
	** execute anything but ppr.
	*/
	set_ppr_env();

	/* Remove some unnecessary and misleading
	   things from the environment: */
	prune_env();

	/* Go into background. */
	gu_daemon(PPR_UMASK);

	/* Chang into the PPR home directory: */
	chdir(HOMEDIR);

	/* Remove any old log file. */
	unlink(log_file_name);

	/* Tell interested humans and programs our pid. */
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
	** Arrange for child termination to be noted.
	** (Since this is of only passing interest to the daemon,
	** we want interupted system calls restarted.)
	*/
	sig.sa_handler = reapchild;			/* call reapchild() on SIGCHLD */
	sigemptyset(&sig.sa_mask);			/* block no additional sigs */
	#ifdef SA_RESTART
	sig.sa_flags = SA_RESTART;			/* restart interupted sys calls */
	#else
	sig.sa_flags = 0;
	#endif
	sigaction(SIGCHLD, &sig, NULL);

	/*
	** Install a signal handler to catch fatal signals
	** so that AppleTalk may be shut down cleanly.
	*/
	signal(SIGHUP, termination_handler);
	signal(SIGINT, termination_handler);
	signal(SIGTERM, termination_handler);
	signal(SIGUSR1, sigusr1_handler);

	/*
	** Read the configuration file and advertise
	** the printer names on the AppleTalk.
	*/
	read_conf(conf_file_name);

	/*
	** Call the Appletalk dependent main loop which will fork off a
	** child daemon to handle each incoming connection.
	** This function contains the main loop.  It never returns.
	*/
	debug("Entering main loop");
	appletalk_dependent_daemon_main_loop();

	return 0;							/* <-- this makes GNU-C happy */
	} /* end of main() */

/* end of file */
