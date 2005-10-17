/*
** mouse:~ppr/src/pprd/pprd.c
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
** Last modified 17 October 2005.
*/

/*
** A PostScript Print Spooler Daemon written by David Chappell
** at Trinity College.
*/

#include "config.h"
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "pprd.h"
#include "./pprd.auto_h"
#include "interface.h"

/*
** Misc global variables
*/
const char myname[] = "pprd";
time_t daemon_start_time;		/* time at which this daemon started */
struct QEntry *queue;			/* array holding terse queue */
int queue_size;					/* number of entries for which there is room */
int queue_entries = 0;			/* entries currently used */

struct Printer *printers;		/* array of printer description structures */
int printer_count = 0;			/* how many printers do we have? */

struct Group *groups;			/* array of group structures */
int group_count = 0;			/* how many groups? */

int upgrade_countdown = UPGRADE_INTERVAL;
int active_printers = 0;		/* number of printers currently active */
int starving_printers = 0;		/* printers currently waiting for rations */

/*=========================================================================
** Utility functions used by children
=========================================================================*/

/*
** We call this from the child after a fork() because pprd likely has SIGCHLD
** and possibly other signals blocked while calling fork().
*/
void child_unblock_all(void)
	{
	sigset_t sigset;
	sigemptyset(&sigset);
	sigprocmask(SIG_SETMASK, &sigset, (sigset_t*)NULL);
	} /* end of child_unblock_all() */

/*
** We calls this from the child side of fork() because pprd has no stdin,
** stdout, or stderr handles open.  It directs the child's stdin to /dev/null
** and stdout and stderr to the pprd log file.
*/
void child_stdin_stdout_stderr(const char input_file[], const char output_file[])
	{
	const char function[] = "child_stdin_stdout_stderr";
	int fd;

	/* Open the indicated file for input.  If that doesn't work, open /dev/null. */
	if((fd = open(input_file, O_RDONLY)) < 0)
		{
		if((fd = open("/dev/null", O_RDONLY)) < 0)
			fatal(1, "child: %s(): can't open \"/dev/null\", errno=%d (%s)", function, errno, gu_strerror(errno) );
		}

	/* If the handle we got was not stdin, make it stdin. */
	if(fd != 0) dup2(fd, 0);
	if(fd > 0) close(fd);

	/* Open the second file for output. */
	if((fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, UNIX_644)) < 0)
		fatal(1, "child: %s(): can't open \"%s\", errno=%d (%s)", function, output_file, errno, gu_strerror(errno));

	/* Connect the second file to stdout and stderr. */
	if(fd != 1) dup2(fd, 1);
	if(fd != 2) dup2(fd, 2);
	if(fd > 2) close(fd);
	} /* end of child_stdin_stdout_stderr() */

/*=========================================================================
** Lock and unlock those data structures which must not be simultainiously
** modified.  The one I can think of at the moment is the queue.
**
** In this version of PPR, these functions are noops.  We will
** leave them in though as they may be useful if we ever want to
** convert this daemon to multi-threaded operation.
=========================================================================*/
int lock_level = 0;

void lock(void)
	{
	lock_level++;
	} /* end of lock */

void unlock(void)
	{
	lock_level--;
	} /* end of unlock() */

/*===================================================================
** Routines for handling child exits.  These children will generally
** be either pprdrv or responders.
===================================================================*/

static volatile gu_boolean sigchld_caught = FALSE;
static struct timeval select_tv;

/* This is the signal hanlder.  It does nothing but set variables. */
void sigchld_handler(int signum)
	{
	/* The select() loop tests for this. */
	sigchld_caught = TRUE;

	/* This is in case the signal is received right after it is unblocked
	   and before select() can be called.  We set the timeout to zero so
	   that select() will return right away. */
	gu_timeval_zero(&select_tv);
	}

/* This is called from the select() loop. */
static void reapchild(void)
	{
	pid_t pid;					/* pid from waitpid() */
	int wstat;					/* status from waitpid() */

	while((pid = waitpid((pid_t)-1, &wstat, WNOHANG)) > (pid_t)0)
		{
		DODEBUG_PRNSTOP(("reapchild(): child terminated, pid=%ld, exit=%i", (long)pid, WEXITSTATUS(wstat)));

		/* Is it pprdrv? */
		if(!pprdrv_child_hook(pid, wstat))
			{
			/* Those which follow don't have complex reporting, 
			   report weird stuff here. */
			if(WIFSIGNALED(wstat))
				{
				error("Process %ld was killed by signal %d (%s)%s",
					(long)pid,
					WTERMSIG(wstat),
					gu_strsignal(WTERMSIG(wstat)),
					WCOREDUMP(wstat) ? ", (core dumped)" : "");
				}
			else if(!WIFEXITED(wstat))
				{
				error("Process %ld met a mysterious end", (long)pid);
				}

			/* Is it pprd-question? */
			if(!question_child_hook(pid, wstat))
				/* Is it a responder? */
				responder_child_hook(pid, wstat);
			}
		}

	DODEBUG_PRNSTOP(("reapchild(): done"));
	} /* end of reapchild() */

/*=======================================================================
** Other signal handlers.
=======================================================================*/

/*
** Handler for signals we don't expect to receive such as SIGHUP and SIGPIPE.
** The effect of this handler is to ignore such signals.
*/
static void signal_ignore(int sig)
	{
	debug("Received %d (%s)", sig, gu_strsignal(sig));
	}

/*
** This is the signal that init will send us
** at system shutdown time.
*/
volatile gu_boolean sigterm_received = FALSE;
static void sigterm_handler(int sig)
	{
	sigterm_received = TRUE;
	}

/*=======================================================================
** Timer tick routine.  This is called every TICK_INTERVAL seconds.
=======================================================================*/
static void tick(void)
	{
	printer_tick();
	question_tick();
	} /* end of tick() */

/*========================================================================
** Handle a command on the pipe.
========================================================================*/
static void do_command(int FIFO)
	{
	const char function[] = "do_command";
	char buffer[256];			/* buffer for received command */
	int len;					/* length of data in buffer */
	int count;
	char *ptr, *next;

	/*
	** Get a line from the FIFO.  We include lame code for
	** Cygnus-Win32 which doesn't implement mkfifo() yet.
	*/
	#ifdef HAVE_MKFIFO
	while((len = read(FIFO, buffer, sizeof(buffer))) < 0)
		{
		if(errno != EINTR)		/* <-- exception for OSF/1 3.2 */
			fatal(0, "%s(): read() on FIFO failed, errno=%d (%s)", function, errno, gu_strerror(errno));
		}
	#else
	while((len = read(FIFO, buffer, sizeof(buffer))) <= 0)
		{
		if(len < -1)
			fatal(0, "%s(): read() on FIFO failed, errno=%d (%s)", function, errno, gu_strerror(errno));
		sleep(1);
		}
	#endif

	if(len == 0 || buffer[len-1] != '\n')
		{
		error("ignoring malformed command(s) from pipe");
		return;
		}

	/* Remove the line feed which terminates the command. */
	buffer[len - 1] = '\0';

	/* Actually, there could be more than one command waiting. */
	for(next=buffer,count=0; (ptr = gu_strsep(&next, "\n")); count++)
		{
		DODEBUG_MAINLOOP(("command[%d]: %s", count, ptr));

		switch(ptr[0])					/* examine the first character */
			{
			case 'j':					/* a print job */
				queue_new_job(ptr);
				break;

			case 'n':					/* Nag operator by email */
				{
				int count = gu_alloc_checkpoint();
				ppad_remind();
				gu_alloc_assert(count);
				}
				break;

			case 'N':					/* new printer or group config */
				if(ptr[1] == 'P')
					new_printer_config(&ptr[3]);
				else					/* 'G' */
					new_group_config(&ptr[3]);
				break;

			case 'I':					/* Internet Printing Protocol */
				{
				int count = gu_alloc_checkpoint();
				ipp_dispatch(ptr);
				gu_alloc_assert(count);
				}
				break;

			default:					/* anything else needs a reply to ppop */
				ppop_dispatch(ptr);
				break;
			}
		}

	} /* end of do_command() */

/*========================================================================
** The Main Procedure
** Initialization and FIFO command dispatch routine.
========================================================================*/
static int real_main(int argc, char *argv[])
	{
	const char function[] = "real_main";
	int option_foreground = FALSE;
	int FIFO;					/* First-in-first-out which feeds us requests */
	sigset_t lock_set;
	struct timeval next_tick;

	time(&daemon_start_time);

	/* Initialize internation messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_PPRD, LOCALEDIR);
	textdomain(PACKAGE_PPRD);
	#endif

	/*
	** Set some environment variables, (PATH, IFS, and
	** SHELL) for safety and for the convenience of the
	** programs we launch (HOME, and PPR_VERSION).
	** Remove unnecessary and potentially misleading
	** variables.
	*/
	set_ppr_env();
	prune_env();

	parse_command_line(argc, argv, &option_foreground);

	/* Switch all UIDs to USER_PPR, all GIDS to GROUP_PPR;
	 * set supplemental group IDs. */
	{
	int ret;
	if((ret = renounce_root_privs(myname, USER_PPR, GROUP_PPR)) != 0)
		return ret;
	}

	/* If the --forground switch wasn't used, then dropt into background. */
	if(! option_foreground)
		gu_daemon(PPR_PPRD_UMASK);
	else
		umask(PPR_PPRD_UMASK);

	/* Change the home directory to the PPR home directory: */
	chdir(LIBDIR);

	/* Create /var/spool/ppr/pprd.pid. */
	create_lock_file();

	/* Signal handlers for silly stuff. */
	signal_restarting(SIGPIPE, signal_ignore);
	signal_restarting(SIGHUP, signal_ignore);

	/* Signal handler for shutdown request. */
	signal_interupting(SIGTERM, sigterm_handler);

	/* Arrange for child termination to be noted. */
	signal_restarting(SIGCHLD, sigchld_handler);

	/* Move /var/spool/ppr/logs/pprd to pprd.old before we call debug()
	   for the first time (below). */
	rename_old_log_file();

	/*
	** This code must come after adjust_ids() and gu_daemon().
	** It makes the first log entry and tells queue-display
	** programs that we are starting up.
	*/
	debug("PPRD startup, pid=%ld", (long)getpid());
	state_update("STARTUP");

	/* Initialize other subsystems. */
	question_init();

	/* Load the printers database. */
	DODEBUG_STARTUP(("loading printers database"));
	load_printers();

	/* Load the groups database. */
	DODEBUG_STARTUP(("loading groups database"));
	load_groups();

	/* Set up the FIFO. */
	DODEBUG_STARTUP(("opening FIFO"));
	FIFO = open_fifo();

	/* Initialize the queue.  This is likely to start printers. */
	DODEBUG_STARTUP(("initializing the queue"));
	initialize_queue();

	/* Schedule the first timer tick. */
	gettimeofday(&next_tick, NULL);
	next_tick.tv_sec += TICK_INTERVAL;

	/*
	** Create a signal block set which will be used to block SIGCHLD except
	** when we are calling select().
	*/
	sigemptyset(&lock_set);
	sigaddset(&lock_set, SIGCHLD);
	sigprocmask(SIG_BLOCK, &lock_set, (sigset_t*)NULL);

	/*
	** This is the Main Loop.  It runs until the sigterm_handler
	** sets sigterm_received.
	*/
	while(!sigterm_received)
		{
		int readyfds;					/* return value from select() */
		fd_set rfds;					/* list of file descriptors for select() to watch */
		struct timeval time_now;		/* the current time */

		DODEBUG_MAINLOOP(("top of main loop"));

		gettimeofday(&time_now, NULL);

		/* If it is time for or past time for the next tick, */
		if(gu_timeval_cmp(&time_now, &next_tick) >= 0)
			{
			readyfds = 0;
			}

		/* If it is not time for the next tick yet, */
		else
			{
			/* Set the select() timeout so that it will return in time for the
			   next tick(). */
			gu_timeval_cpy(&select_tv, &next_tick);
			gu_timeval_sub(&select_tv, &time_now);

			/* Create a file descriptor list which contains only the descriptor
			   of the FIFO. */
			FD_ZERO(&rfds);
			FD_SET(FIFO, &rfds);

			/* Call select() with SIGCHLD unblocked. */
			sigprocmask(SIG_UNBLOCK, &lock_set, (sigset_t*)NULL);
			readyfds = select(FIFO + 1, &rfds, NULL, NULL, &select_tv);
			sigprocmask(SIG_BLOCK, &lock_set, (sigset_t*)NULL);
			}

		/* If there is something to read, */
		if(readyfds > 0)
			{
			if(!FD_ISSET(FIFO, &rfds))
				fatal(0, "%s(): assertion failed: select() returned but FIFO not ready", function);
			do_command(FIFO);
			continue;
			}

		/* If the SIGCHLD handler set the flag, handle child termination.  Once
		   we have done that, we must go back to the top of the loop because
		   we don't really know if it is time for a tick() call yet. */
		if(sigchld_caught)
			{
			sigchld_caught = FALSE;
			reapchild();
			continue;
			}

		/* If there was no error and no file descriptors are ready, then the 
		   timeout must have expired.  Call tick(). */
		if(readyfds == 0)
			{
			tick();
			next_tick.tv_sec += TICK_INTERVAL;
			continue;
			}

		/* If interupted by a system call, restart it. */
		if(errno == EINTR)
			continue;

		/* If we get this far, there was an error. */
		fatal(0, "%s(): select() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
		} /* end of endless while() loop */

	state_update("SHUTDOWN");
	fatal(0, "Received SIGTERM, exiting");
	} /* end of real_main() */

int main(int argc, char *argv[])
	{
	gu_Try
		{
		return real_main(argc, argv);
		}
	gu_Catch {
		fatal(0, "Caught exception: %s", gu_exception);
		}
	/* NOTREACHED */
	return 255;
	}

/* end of file */

