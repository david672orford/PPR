/*
** mouse:~ppr/src/pprd/pprd.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 18 November 2000.
*/

/*
** A PostScript Print Spooler Daemon written by David Chappell
** at Trinity College.
*/

#include "before_system.h"
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
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
struct QEntry *queue;		/* array holding terse queue */
int queue_size;			/* number of entries for which there is room */
int queue_entries = 0;		/* entries currently used */

struct Printer *printers;	/* array of printer description structures */
int printer_count = 0;		/* how many printers do we have? */

struct Group *groups;		/* array of group structures */
int group_count = 0;		/* how many groups? */

int upgrade_countdown = UPGRADE_INTERVAL;
int active_printers = 0;	/* number of printers currently active */
int starving_printers = 0;	/* printers currently waiting for rations */

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

void reapchild(int signum)
    {
    pid_t pid;
    int wstat;			/* result of waitpid() */
    int saved_errno = errno;
    int gu_alloc_saved = gu_alloc_checkpoint_get();

    lock_level++;		/* set our flag */

    while((pid = waitpid((pid_t)-1, &wstat, WNOHANG)) > (pid_t)0)
	{
	DODEBUG_PRNSTOP(("reapchild(): child terminated, pid=%ld, exit=%i", (long)pid, WEXITSTATUS(wstat)));

	/* Give everyone a chance to claim it. */
	if(!pprdrv_child_hook(pid, wstat))
	    if(!remote_child_hook(pid, wstat))
		responder_child_hook(pid, wstat);
	}

    lock_level--;				/* clear our flag */

    DODEBUG_PRNSTOP(("reapchild(): done"));

    gu_alloc_checkpoint_put(gu_alloc_saved);
    errno = saved_errno;
    } /* end of reapchild() */

/*=======================================================================
** Timer tick routine (SIGALRM handler).
=======================================================================*/
void tick(int sig)
    {
    int saved_errno = errno;		/* save errno of interupted code */
    alarm(TICK_INTERVAL);		/* set timer for next tick */
    printer_tick();
    remote_tick();
    errno = saved_errno;
    } /* end of tick() */

/*========================================================================
** The Main Procedure
** Initialization and FIFO command dispatch routine.
========================================================================*/
int main(int argc, char *argv[])
    {
    const char function[] = "main";
    int option_foreground = FALSE;
    int FIFO;			/* First-in-first-out which feeds us requests */
    sigset_t lock_set;

    /* Initialize internation messages library. */
    #ifdef INTERNATIONAL
    setlocale(LC_MESSAGES, "");
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

    /* This may call root_create_directories() */
    adjust_ids();

    /* If the --forground switch wasn't used, then dropt into background. */
    if(! option_foreground)
	gu_daemon(PPR_UMASK);

    /* Change the home directory to the PPR home directory: */
    chdir(HOMEDIR);

    /* Create and repair directories within /var/spool/ppr. */
    create_work_directories();

    /* Create /var/spool/ppr/pprd.pid. */
    create_lock_file();

    /* Handler for terminate, child exit, and timer. */
    install_signal_handlers();

    /* move /var/spool/ppr/logs/pprd to pprd.old */
    rename_old_log_file();

    /*
    ** This code must come after adjust_ids() and gu_daemon().
    ** It makes the first log entry and tells queue-display
    ** programs that we are starting up.
    */
    debug("PPRD startup, pid=%ld", (long)getpid());
    state_update("STARTUP");

    /*
    ** Create a signal mask which will be needed by the
    ** lock() and unlock() functions.
    */
    sigemptyset(&lock_set);
    sigaddset(&lock_set, SIGALRM);
    sigaddset(&lock_set, SIGCHLD);

    /* Make sure the local node gets the node id of 0. */
    if(! nodeid_is_local_node(nodeid_assign(ppr_get_nodename())))
    	fatal(1, "%s(): line %d: assertion failed", function, __LINE__);

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

    /* Start periodic alarm going. */
    alarm(TICK_INTERVAL);

    /*
    ** Main Loop
    **
    ** This daemon terminates only if it is killed,
    ** so the condition on this loop is always TRUE.
    **
    ** Notice that
    ** SIGCHLD and SIGALRM are blocked except while
    ** we are executing read().  This is so as to be
    ** absolutely sure that they will not interupted
    ** non-reentrant C library calls.
    */
    while(TRUE)
	{
	char command[256];	/* buffer for received command + zero byte  */
	int len;		/* length of data in buffer */

	DODEBUG_MAINLOOP(("top of main loop"));

	sigprocmask(SIG_UNBLOCK, &lock_set, (sigset_t*)NULL);

	/*
	** Get a line from the FIFO.  We include lame code for
	** Cygnus-Win32 which doesn't implement mkfifo() yet.
	*/
	#ifdef HAVE_MKFIFO
	while((len = read(FIFO, command, 255)) < 0)
	    {
	    if(errno != EINTR)	/* <-- exception for OSF/1 3.2 */
		fatal(0, "%s(): read() on FIFO failed, errno=%d (%s)", function, errno, gu_strerror(errno));
	    }
	#else
	while((len = read(FIFO, command, 255)) <= 0)
	    {
	    if(len < -1)
	    	fatal(0, "%s(): read() on FIFO failed, errno=%d (%s)", function, errno, gu_strerror(errno));
	    sleep(1);
	    }
	#endif

	sigprocmask(SIG_BLOCK, &lock_set, (sigset_t*)NULL);

	if(len == 0 || command[len-1] != '\n')
	    {
	    error("ignoring misformed command from pipe");
	    continue;
	    }

	/* Remove the line feed which terminates the command. */
	command[len - 1] = '\0';

	DODEBUG_MAINLOOP(("command: %s", command));

	switch(command[0])		/* examine the first character */
	    {
	    case 'j':			/* a print job */
		queue_new_job(command);
		break;

	    case 'n':			/* Nag operator by email */
		gu_alloc_checkpoint();
	    	ppad_remind();
	    	gu_alloc_assert(0);
	    	break;

	    case 'N':			/* new printer or group config */
		if(command[1] == 'P')
		    new_printer_config(&command[3]);
		else			/* 'G' */
		    new_group_config(&command[3]);
		break;

	    default:                    /* anything else needs a reply to ppop */
		ppop_dispatch(command);
		break;
	    }
	} /* end of endless while() loop */
    } /* end of main() */

/* end of file */

