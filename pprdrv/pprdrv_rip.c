/*
** mouse:~ppr/src/pprdrv/pprdrv_rip.c
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
** Last modified 12 March 2002.
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
#include "global_structs.h"
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
	if(WTERMSIG(rip_wait_status) == SIGSEGV)
	    {
	    alert(printer.Name, TRUE,
		_("The RIP (Ghostscript) malfunctioned while printing the job\n"
		"%s.  Presumably it can print other jobs that don't\n"
		"trigger the bug, so the spooler will arrest the job and move on.\n"),
			QueueFile
		);
	    hooked_exit(EXIT_JOBERR, "RIP segfault");
	    }
	else
	    {
	    alert(printer.Name, TRUE,
		_("The RIP (Ghostscript) was killed by signal %d (%s).\n"
		"Perhaps a system operator did it or perhaps the system\n"
		"is shutting down.\n"),
			WTERMSIG(rip_wait_status),
			gu_strsignal(WTERMSIG(rip_wait_status))
		);
	    if(WCOREDUMP(rip_wait_status))
		alert(printer.Name, FALSE, "The RIP dumped core.");
	    hooked_exit(EXIT_PRNERR, "RIP killed");
	    }
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

	/* Disconnect the dead RIP without flushing.  This code like this is also in job_end(). */
	intstdin = rip_stop(intstdin, FALSE);
	gu_nonblock(intstdin, TRUE);

	/* Close the interface without flushing. */
	interface_close(FALSE);

	/* If it core dumped or something, that is not a job error. */
	exit_code = rip_exit_screen();

	/* Did Ghostscript indicate a command line problem?
	   This is our lame solution.  It would be better to get the
	   error message into the alerts file!!! */
	if(exit_code == 1)
	    {
	    fatal(EXIT_PRNERR_NORETRY, "Possible error on Ghostscript command line.  Check options.\n"
	    				"(Use \"ppop log %s:%s-%d.%d\" to view the error message.)",
	    					job.destnode, job.destname, job.id, job.subid);
	    }

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
    const char **rip_args;
    int rip_pipe[2];

    DODEBUG_INTERFACE(("%s()", function));

    /* Get the path to the Ghostscript interpreter from ppr.conf. */
    if(!gs)
	{
	if(strcmp(printer.RIP.name, "gs") == 0)
	    {
	    if(!(gs = gu_ini_query(PPR_CONF, "ghostscript", "gs", 0, NULL)))
		fatal(EXIT_PRNERR_NORETRY, "Failed to get value \"gs\" from section [ghostscript] of \"%s\"", PPR_CONF);
	    }
	else if(strcmp(printer.RIP.name, "ppr-gs") == 0)
	    {
	    gs = HOMEDIR"/lib/ppr-gs";
	    }
	else
	    {
    	    fatal(EXIT_PRNERR_NORETRY, "Unknown RIP \"%s\".", printer.RIP.name);
    	    }
    	}

    /* Make sure the RIP exists. */
    if(access(gs, X_OK) != 0)
    	{
	fatal(EXIT_PRNERR_NORETRY, "Ghostscript program \"%s\" doesn't exist or isn't executable.", gs);
    	}

    /* Build the argument vector. */
    {
    int si, di;
    rip_args = gu_alloc(printer.RIP.options_count + 6, sizeof(const char *));
    di = 0;
    rip_args[di++] = gs;
    for(si = 0; si < printer.RIP.options_count; si++)
	{
	rip_args[di++] = printer.RIP.options[si];
	}
    rip_args[di++] = "-q";
    rip_args[di++] = "-dSAFER";
    rip_args[di++] = "-sOutputFile=| cat - >&3";
    rip_args[di++] = "-";
    rip_args[di++] = NULL;
    }
    
    /* Reset flags related to the RIP dying. */
    rip_died = FALSE;
    rip_fault_check_disable = FALSE;

    if(pipe(rip_pipe) == -1)
    	fatal(EXIT_PRNERR, "pipe() failed, errno=%d (%s)", errno, strerror(errno));

    if((rip_pid = fork()) == -1)
    	fatal(EXIT_PRNERR, "fork() failed, errno=%d (%s)", errno, strerror(errno));

    /* If child, */
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
	execv(gs, (char**)rip_args);
	_exit(1);
    	}

    close(rip_pipe[0]);
    gu_free(rip_args);
    saved_printdata_handle = printdata_handle;

    DODEBUG_INTERFACE(("%s(): done", function));
    return rip_pipe[1];
    } /* end of rip_start() */

/*
** This is called from job_end().
*/
int rip_stop(int printdata_handle2, gu_boolean flushit)
    {
    FUNCTION4DEBUG("rip_stop")
    DODEBUG_INTERFACE(("%s()", function));

    /* Flush the last of the PostScript to the RIP. */
    if(flushit)
	{
	DODEBUG_INTERFACE(("%s(): calling printer_flush()", function));
	printer_flush();
	DODEBUG_INTERFACE(("%s(): return from printer_flush()", function));
	}

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
