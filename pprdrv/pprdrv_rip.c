/*
** mouse:~ppr/src/pprdrv/pprdrv_rip.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 11 May 2001.
*/

#include "before_system.h"
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "pprdrv.h"
#include "interface.h"

static int saved_printdata_handle;
static volatile pid_t rip_pid = 0;
static gu_boolean rip_died = FALSE;
static volatile int rip_wait_status;
static gu_boolean rip_fault_check_disable;

/*
** This is called from pprdrv.c:sigchld_handler() whenever a child exits.
** We claim the child as our own by returning TRUE (if we return FALSE,
** then sigchild_handler will go on looking for the parent.
**
** Since this is called from signal handler, we may only call reentrant
** functions.  In other words, set a flag and get out.
*/
gu_boolean rip_sigchld_hook(pid_t pid, int wait_status)
    {
    FUNCTION4DEBUG("rip_sigchld_hook")
    if(pid == rip_pid)
    	{
	#ifdef DEBUG_INTERFACE
	signal_debug("%s(): RIP terminated", function);
	#endif
	rip_died = TRUE;
	rip_wait_status = wait_status;
	rip_pid = 0;
    	return TRUE;
    	}
    return FALSE;
    } /* end of rip_sigchld_hook() */

/*
** We call this function after the RIP exits.  It handles conditions that
** are considered fatal.
*/
static int rip_exit_screen(void)
    {
    if(WIFSIGNALED(rip_wait_status))
	{
	alert(printer.Name, TRUE,
		_("The RIP terminated abruptly after receiving signal %d (%s)\n"
		"from a non-spooler process (possibly the OS kernel)."),
		WTERMSIG(rip_wait_status),
		gu_strsignal(WTERMSIG(rip_wait_status))
		);
	if(WCOREDUMP(rip_wait_status))
	    {
	    alert(printer.Name, FALSE, "The RIP dumped core.");
	    hooked_exit(EXIT_PRNERR_NORETRY, "RIP core dump");
	    }
	hooked_exit(EXIT_PRNERR, "RIP killed");
	}
    if(WIFSTOPPED(rip_wait_status))
	fatal(EXIT_PRNERR_NORETRY, _("RIP stopped by signal \"%s\"."), gu_strsignal(WSTOPSIG(rip_wait_status)));
    if(! WIFEXITED(rip_wait_status))
	fatal(EXIT_PRNERR_NORETRY, _("Bizaar RIP malfunction: SIGCHLD w/out signal or exit."));
    return WEXITSTATUS(rip_wait_status);
    } /* end of rip_exit_screen() */

/*
** This is called from pprdrv.c:fault_check().
*/
void rip_fault_check(void)
    {
    FUNCTION4DEBUG("rip_fault_check")
    DODEBUG_INTERFACE(("%s()", function));
    if(rip_died && !rip_fault_check_disable)
    	{
	int exit_code;
	DODEBUG_INTERFACE(("%s(): rip_wait_status=0x%04x", function, rip_wait_status));

	/* Try to get the interface to claim blame. */
	rip_died = FALSE;
	sleep(1);
	fault_check();

	/* Check for unpleasant stuff such as core dumps. */
	exit_code = rip_exit_screen();

	/* It still shouldn't have exited. */
	fatal(EXIT_JOBERR, "The RIP exited with code %d before accepting all of the data.", exit_code);
    	}
    DODEBUG_INTERFACE(("%s(): done, no fault", function));
    } /* end of rip_fault_check() */

/*
** This is called from job_start().  It interposes a Raster Image Processor
** such as Ghostscript between pprdrv and the interface program.  It also
** connects stdout and stderr of the RIP to the write end of the pipe
** leading back to pprdrv.
*/
int rip_start(int printdata_handle, int stdout_handle)
    {
    FUNCTION4DEBUG("rip_start")
    static const char *gs = NULL;
    int rip_pipe[2];
    char driver_option[32];

    DODEBUG_INTERFACE(("%s()", function));

    /* Make sure gs is what the user wants. */
    if(strcmp(printer.RIP.name, "gs"))
	fatal(EXIT_PRNERR_NORETRY, "Ghostscript (gs) is the only RIP supported at this time.");

    /* Get the path to the Ghostscript interpreter from ppr.conf. */
    if(!gs)
	{
	if(!(gs = gu_ini_query(PPR_CONF, "ghostscript", "gs", 0, NULL)))
	    fatal(EXIT_PRNERR_NORETRY, "Failed to get value \"gs\" from section [ghostscript] of \"%s\"", PPR_CONF);
	}

    if(strlen(printer.RIP.driver) > 4 && strcmp(&printer.RIP.driver[strlen(printer.RIP.driver) - 4], ".upp") == 0)
	snprintf(driver_option, sizeof(driver_option), "@%s", printer.RIP.driver);
    else
	snprintf(driver_option, sizeof(driver_option), "-sDEVICE=%s", printer.RIP.driver);

    rip_died = FALSE;
    rip_fault_check_disable = FALSE;

    if(pipe(rip_pipe) == -1)
    	fatal(EXIT_PRNERR, "pipe() failed, errno=%d (%s)", errno, strerror(errno));

    if((rip_pid = fork()) == -1)
    	fatal(EXIT_PRNERR, "fork() failed, errno=%d (%s)", errno, strerror(errno));

    if(rip_pid == 0)
    	{
	/* See pprdrv_interface.c:start_interface() for comments on this paranoid code. */
	int new_stdin, new_stdout, new_printdata;
	umask(PPR_INTERFACE_UMASK);
	setpgid(0, 0);
	new_stdin=dup(rip_pipe[0]);
	new_stdout=dup(stdout_handle);
	new_printdata=dup(printdata_handle);
	close(rip_pipe[0]);
	close(rip_pipe[1]);
	close(stdout_handle);
	close(printdata_handle);
	dup2(new_stdin,0);
	dup2(new_stdout,1);
	dup2(new_stdout,2);
	dup2(new_printdata,3);
	close(new_stdin);
	close(new_stdout);
	close(new_printdata);

	/* Launch Ghostscript. */
	execl(gs, gs, driver_option, "-q", "-dSAFER", "-sOutputFile=| cat - >&3", "-", NULL);
	_exit(1);
    	}

    close(rip_pipe[0]);

    saved_printdata_handle = printdata_handle;

    DODEBUG_INTERFACE(("%s(): done", function));
    return rip_pipe[1];
    } /* end of rip_start() */

/*
** This is called from job_end().
*/
int rip_stop(int printdata_handle2)
    {
    FUNCTION4DEBUG("rip_stop")
    DODEBUG_INTERFACE(("%s()", function));

    /* Flush the last of the PostScript to the RIP. */
    DODEBUG_INTERFACE(("%s(): calling printer_flush()", function));
    printer_flush();
    DODEBUG_INTERFACE(("%s(): return from printer_flush()", function));

    /* We can handle it from here. */
    rip_fault_check_disable = TRUE;

    /* Let the RIP know that that is EOF. */
    close(printdata_handle2);

    DODEBUG_INTERFACE(("%s(): waiting for RIP to exit", function));
    {
    int t;
    writemon_start("RIP_CLOSE");
    for(t=0; !rip_died; t += 1)
    	{
	feedback_wait(t, TRUE);
	}
    writemon_unstalled("RIP_CLOSE");
    }
    DODEBUG_INTERFACE(("%s(): rip_wait_status=0x%04x", function, rip_wait_status));

    /* If we don't clear this, rip_fault_check() will scream! */
    rip_died = FALSE;

    /* Give the interface a last chance to complain. */
    fault_check();

    /* Check to see if the RIP core dumped or something like that. */
    rip_exit_screen();

    DODEBUG_INTERFACE(("%s(): done", function));
    return saved_printdata_handle;
    } /* end of rip_stop() */

/*
** This is called from kill_interface().
*/
void rip_cancel(void)
    {
    FUNCTION4DEBUG("rip_cancel")
    DODEBUG_INTERFACE(("%s()", function));

    if(rip_pid > 0)
	{
	DODEBUG_INTERFACE(("%s(): killing RIP", function));
	rip_fault_check_disable = TRUE;
	kill(rip_pid * -1, SIGTERM);
	}

    DODEBUG_INTERFACE(("%s(): done", function));
    }

/* end of file */
