/*
** mouse:~ppr/src/pprdrv/pprdrv_rip.c
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
** Last modified 17 January 2005.
*/

#include "config.h"
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
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
			give_reason("RIP malfunction");
			alert(printer.Name, TRUE,
				_("The RIP (Ghostscript) malfunctioned (caught SIGSEGV) while printing the job\n"
				"%s.  Presumably it can print other jobs that don't\n"
				"trigger the bug, so the spooler will arrest the job and move on."),
						QueueFile
				);
			hooked_exit(EXIT_JOBERR, "RIP segfault");
			}
		else if(WTERMSIG(rip_wait_status) == SIGPIPE)
			{
			if(!feedback_ghosterror())		/* only if not already explained */
				{
				alert(printer.Name, TRUE,
					_("The RIP (Ghostscript) died due to a broken pipe.  Presumably that pipe led to\n"
					"a post-processing filter which died.  Look in the RIP options to find out what\n"
					"post-processing filter is being used."));
				}
			hooked_exit(EXIT_PRNERR, "RIP broken pipe");
			}
		else
			{
			alert(printer.Name, TRUE,
				_("The RIP (Ghostscript) was killed by signal %d (%s).\n"
				"Perhaps a system operator did this or perhaps the system\n"
				"is shutting down."),
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
** This is called from pprdrv.c:fault_check().  It is supposed to check if
** the RIP is still alive.  If the RIP is dead, this function should
** call fatal().
*/
void rip_fault_check(void)
	{
	FUNCTION4DEBUG("rip_fault_check")
	DODEBUG_INTERFACE(("%s()", function));
	if(rip_died && !rip_fault_check_disable)
		{
		int exit_code;
		DODEBUG_INTERFACE(("%s(): rip_wait_status=0x%04x", function, rip_wait_status));

		/* Disconnect the dead RIP without flushing.  Code like this is also in job_end(). */
		intstdin = rip_stop(intstdin, FALSE);
		gu_nonblock(intstdin, TRUE);

		/* Close the interface without flushing. */
		interface_close(FALSE);

		/* If it core dumped or something, that is not a job error. */
		exit_code = rip_exit_screen();

		/* Did Ghostscript indicate a command line problem?
		   */
		if(exit_code == 1 && feedback_ghosterror())
			{
			hooked_exit(EXIT_PRNERR_NORETRY, "Ghostscript RIP aborted.");
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
**
** printdata_handle is the file descriptor that the RIP should write 
**		the printer control language data to.
** stdout_handle is the file descriptor to which the RIP should write
**		the stdout and stderr of the PostScript program.
*/
int rip_start(int printdata_handle, int stdout_handle)
	{
	const char function[] = "rip_start";
	static const char *rip_exe = NULL;
	static gu_boolean rip_is_unwrapped_ghostscript = FALSE;
	const char **rip_args;
	int rip_pipe[2];

	DODEBUG_INTERFACE(("%s()", function));

	/* If we haven't figured out where the RIP is yet and whether it is Ghostscript
	  or now, do so now. */
	if(!rip_exe)
		{
		/* If the rip name ends in "/gs", assume it is just plain, unwrapped 
		   Ghostscript.  Note that this means that if you want to use unwrapped
		   Ghostscript, you have to specify the path.  For example:

				$ ppad myprn rip /usr/bin/gs -sDEVICE=hpjet

		   Note that a RIP name specified in a PPD file is not allowed to
		   contain slashes.  This is to make sure that people don't write
		   PPD files that apply only to their own computers!

		   */
		if(rmatch(printer.RIP.name, "/gs"))
			{
			rip_exe = printer.RIP.name;
			rip_is_unwrapped_ghostscript = TRUE;
			}

		/* Since the program isn't named "gs", assume that it is some
		   sort of Ghostscript wrapper in $LIBDIR, probably
		   our own ppr-gs.
		   */
		else
			{
			char *p;
			gu_asprintf(&p, "%s/%s", LIBDIR, printer.RIP.name);
			rip_exe = p;
			}
		}

	/* Make sure the RIP exists. */
	if(access(rip_exe, X_OK) != 0)
		{
		fatal(EXIT_PRNERR_NORETRY, "RIP program \"%s\" doesn't exist or isn't executable.", rip_exe);
		}

	/* Build the argument vector. */
	{
	gu_boolean saw_OutputFile = FALSE;
	int si, di;
	rip_args = gu_alloc(printer.RIP.options_count + 6, sizeof(const char *));
	di = 0;
	rip_args[di++] = rip_exe;							/* argv[0] is program name */
	for(si = 0; si < printer.RIP.options_count; si++)	/* copy user supplied arguments */
		{
		rip_args[di++] = printer.RIP.options[si];
		if(gu_strncasecmp(printer.RIP.options[si], "-sOutputFile=", 13) == 0)
			saw_OutputFile = TRUE;
		}
	if(rip_is_unwrapped_ghostscript)					/* Ghostscript gets extra arguments */
		{
		rip_args[di++] = "-q";
		rip_args[di++] = "-dSAFER";
		if(!saw_OutputFile)
			rip_args[di++] = "-sOutputFile=| cat - >&3";
		rip_args[di++] = "-";
		}
	rip_args[di++] = NULL;								/* terminate the arguments list */
	}

	/* Reset flags related to the RIP dying. */
	rip_died = FALSE;
	rip_fault_check_disable = FALSE;

	if(pipe(rip_pipe) == -1)
		fatal(EXIT_PRNERR, "%s(): pipe() failed, errno=%d (%s)", function, errno, strerror(errno));

	if((rip_pid = fork()) == -1)
		fatal(EXIT_PRNERR, "%s(): fork() failed, errno=%d (%s)", function, errno, strerror(errno));

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

		/* Some RIPs need the PPD file to set themselves up. */
		{
		char *ppd_equals;		/* <-- don't free this! */
		gu_asprintf(&ppd_equals, "PPD=%s", printer.PPDFile);
		putenv(ppd_equals);
		}

		/* Foomatic likes its own special options. */
		if(job.ripopts)
			{
			char *ppr_ripopts_equals;	/* <-- don't free this either! */
			gu_asprintf(&ppr_ripopts_equals, "PPR_RIPOPTS=%s", job.ripopts);
			putenv(ppr_ripopts_equals);
			}
		else
			{
			putenv("PPR_RIPOPTS=");
			}

		/* Launch Ghostscript. */
		execv(rip_exe, (char**)rip_args);
		_exit(1);
		}

	DODEBUG_INTERFACE(("%s(): rip_args[0] = \"%s\", rip_pid=%ld", function, rip_args[0], (long)rip_pid));

	close(rip_pipe[0]);			/* read end */

	gu_free(rip_args);

	/* We will stash this away so that rip_stop() can return it.  Before then,
	   the file descriptor which we return will take its place. */
	saved_printdata_handle = printdata_handle;

	/* We will be using select() to read and write simultaniously. */
	gu_nonblock(rip_pipe[1], TRUE);

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
