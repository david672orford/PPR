/*
** mouse:~ppr/src/pprdrv/pprdrv_interface.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 7 February 2001.
*/

/*
** This module contains routines for running the interface
** and setting up the printer.
**
** The external interface to this module is:
**	job_start()		called before each PS job (banner, body, etc.)
**	job_end()		called after each PS job
**	job_nomore()		called after last time job_end() called
*/

#include "before_system.h"
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

#include "global_structs.h"
#include "pprdrv.h"
#include "interface.h"

int intstdin = -1;				/* pipe to interface program */
int intstdout;					/* pipe from interface program */
static pid_t intpid = 0;			/* interface process id */
static int interface_wait_status;
static volatile gu_boolean sigusr1_caught;
static volatile gu_boolean sigalrm_caught;
static volatile gu_boolean interface_died;
extern volatile gu_boolean sigterm_caught;		/* set by handler in pprdrv.c */

/*
** Catch child death signals.  This handler is in placed during those
** time when we don't expect the interface to terminate.  We ignore
** the termination of things other than the interface, such as
** commentators and custom hooks.
*/
static void sigchld_handler(int sig)
    {
    const char function[] = "sigchld_handler";
    pid_t waitid;
    int wait_status;

    /*
    ** In this loop we retrieve process exit status until there
    ** are no more children whose exit status is available.  When
    ** there are no more, we return.  If the child is the interface,
    ** we drop out of the loop and let the code below handle it.
    **
    ** Notice that it is important to test for a return value of
    ** 0 before we test for a return value that is equal to intpid
    ** since if there is no interface intpid will be 0.
    */
    while(TRUE)
	{
	waitid = waitpid((pid_t)-1, &wait_status, WNOHANG);

	/* If no more children have exited or no more children exist, */
	if(waitid == 0)
	    break;

	if(waitid == -1)
	    {
	    if(errno == ECHILD)
		break;
	    if(errno == EINTR)
	    	continue;
	    signal_fatal(EXIT_PRNERR, "%s(): waitpid() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
	    }

	/* If the child that died was the interface, */
	if(waitid == intpid)
	    {
	    #ifdef DEBUG_INTERFACE
	    signal_debug("%s(): interface terminated", function);
	    #endif
	    interface_wait_status = wait_status;
	    interface_died  = TRUE;
	    alarm(0);
	    kill(getpid(), SIGALRM);	/* <-- interupt system calls */
	    continue;
	    }

	/* If we get this far, a commentator terminated. */
	#ifdef DEBUG_INTERFACE
	signal_debug("%s(): process %ld terminated", function, (long int)waitid);
	#endif
    	}
    } /* end of sigchld_handler() */

/*
** We use this rather than setting the action to SIG_IGN because
** POSIX does not specify what happens when SIGCHLD is ignored.
** It might work to set it to SIG_DFL, but I haven't tried that.
*/
static void empty_sigchld_handler(int sig)
    {
    }

/*
** Handler for signal from interface (user signal 1).
** If we are using the JobBreak method "signal" or "signal/pjl",
** the interface will send us this signal as soon as it has
** set-up its SIGUSR1 handler.  It will also send us that signal
** to acknowledge any USR1 signals we send it.
*/
static void sigusr1_handler(int sig)
    {
    #ifdef DEBUG_INTERFACE
    signal_debug("SIGUSR1 caught");
    #endif
    sigusr1_caught = TRUE;
    } /* end of sigusr1_handler() */

static void sigalrm_handler(int sig)
    {
    #ifdef DEBUG_INTERFACE
    signal_debug("SIGALRM caught");
    #endif
    sigalrm_caught = TRUE;
    }

static void sigpipe_handler(int sig)
    {
    #ifdef DEBUG_INTERFACE
    signal_debug("SIGPIPE caught");
    #endif
    }

/*
** We call this no matter when the interface exits.  If this function
** finds that the interface didn't exit in a well-controlled fashion
** (i. e., clearly indicating sucess or failure), it treats the
** circumstance as a fatal error.
*/
static void interface_exit_screen(void)
    {
    /*
    ** See if interface just died when it got a signal.
    */
    if(WIFSIGNALED(interface_wait_status))
	{
	/*
	** Since we did not send the signal, it is a fatal error,
	** however we will not call fatal() because we must
	** issue a rather complicated message.  We will make calls
	** to alert(), and hooked_exit().
	*/
	alert(printer.Name, TRUE, "Interface terminated abruptly after receiving signal %d (%s)", WTERMSIG(interface_wait_status), gu_strsignal(WTERMSIG(interface_wait_status)));
	alert(printer.Name, FALSE, "from a non-spooler process (possibly the OS kernel).");

	/* If the signal caused the interface to dump core, say so. */
	if( WCOREDUMP(interface_wait_status) )
	    {
	    alert(printer.Name, FALSE, "Interface dumped core.");
	    hooked_exit(EXIT_PRNERR_NORETRY, "interface core dump");
	    }

	hooked_exit(EXIT_SIGNAL, "interface program killed");
	} /* end of if interface died due to the receipt of a signal */

    /*
    ** This is similiar to the section just above.
    **
    ** See if the interface exited with a status code which indicates that
    ** it caught a signal and then exited volunarily.
    */
    if(WIFEXITED(interface_wait_status) && WEXITSTATUS(interface_wait_status) == EXIT_SIGNAL)
    	{
    	alert(printer.Name, TRUE, _("The interface performed a controlled shutdown because a non-spooler"
				"process (possibly the OS kernel) sent it a signal."));
	hooked_exit(EXIT_SIGNAL, "interface program killed");
    	} /* end of if interface caught a signal */

    /*
    ** It is possible that the interface did not exit.  I don't know
    ** why that might be, but it is possible.
    */
    if(WIFSTOPPED(interface_wait_status))
	fatal(EXIT_PRNERR_NORETRY, _("Interface stopped by signal \"%s\"."), gu_strsignal(WSTOPSIG(interface_wait_status)));
    if(! WIFEXITED(interface_wait_status))
	fatal(EXIT_PRNERR_NORETRY, _("Bizaar interface program malfunction: SIGCHLD w/out signal or exit."));

    /*
    ** If the exit code is a legal one for an interface, use it;
    ** if not, assume something really bad has happened and
    ** ask to have the printer put in no-retry-fault-mode.
    **
    ** Oops!  There is one more exception!  Ghostscript will exit
    ** immediatly if a Postscript error occurs.  We must be ready
    ** for that.
    */
    if(WEXITSTATUS(interface_wait_status) > EXIT_INTMAX)
	fatal(EXIT_PRNERR_NORETRY, _("Interface program malfunction: it returned invalid exit code %d,"), WEXITSTATUS(interface_wait_status));
    } /* end of interface_exit_screen() */

/*
** This is called by this module's clients whenever they want to assure
** themselves that the interface program hasn't died.
*/
void interface_fault_check(void)
    {
    FUNCTION4DEBUG("interface_fault_check")
    DODEBUG_INTERFACE(("%s()", function));

    /* Not strictly an interface fault, but we catch it here. */
    if(sigterm_caught)
	{
	DODEBUG_INTERFACE(("%s(): exiting because sigterm_caught is TRUE", function));
	hooked_exit(EXIT_SIGNAL, "printing halted");
	}

    if(interface_died)
    	{
	DODEBUG_INTERFACE(("%s(): interface terminated", function));

        /* Make a note of the fact that the interface is dead. */
        intpid = 0;

	/* If we don't turn this off, then feedback_drain() won't work! */
	interface_died = FALSE;

	/* Check for really wierd stuff */
        interface_exit_screen();

	/* It is now safe to say we have a real exit code. */
	DODEBUG_INTERFACE(("%s(): interface exit code is %d", function, WEXITSTATUS(interface_wait_status)));

        /* Read final output from interface: */
        feedback_drain();

        /*
        ** Test for a very strange termination.  Notice that normal
        ** termination would have been caught by the clause above.
        */
        if(WEXITSTATUS(interface_wait_status) == EXIT_PRINTED)
            {
            fatal(EXIT_PRNERR_NORETRY, _("Interface program malfunction: it quit before accepting\n"
                    "\tall of the job but didn't return an error code."));
            }

        /*
        ** If a PostScript error message came from the interface
        ** before it exited, assume that it was talking to a broken
        ** AppleTalk implementation which doesn't implement the
        ** flush correctly.
        **
        ** See the similiar code toward the end of main().
        */
        if(feedback_posterror())
            hooked_exit(EXIT_JOBERR, "postscript error");

        /*
        ** Interface terminated with error code before it
        ** had accepted all of the data, pass the code back to pprd.
        */
        hooked_exit(WEXITSTATUS(interface_wait_status), (char*)NULL);
        }

    DODEBUG_INTERFACE(("%s(): done", function));
    } /* end of interface_fault_check() */

/*
** This routine launches the interface program.
**
** This routine sets the global variables intstdout,
** intstdin, and intpid.
*/
static void start_interface(const char *BarBarPDL)
    {
    const char *function = "start_interface";
    int _stdin[2];           		/* for opening pipe to interface */
    int _stdout[2];			/* for pipe from interface */
    int new_stdin, new_stdout;
    char fname[MAX_PPR_PATH];		/* possibly holds path of interface program */
    char *fname_ptr;
    char jobbreak_str[2];		/* jobbreak setting converted to ASCII */
    char feedback_str[2];		/* feedback setting converted to ASCII */
    char codes_str[2];

    DODEBUG_INTERFACE(("%s(\"%s\")", function, BarBarPDL ? BarBarPDL : ""));

    /* Install our signal handlers. */
    signal_restarting(SIGCHLD, sigchld_handler);
    signal_interupting(SIGALRM, sigalrm_handler);
    signal_restarting(SIGUSR1, sigusr1_handler);
    signal_interupting(SIGPIPE, sigpipe_handler);
    interface_died = FALSE;

    /* Initialize output routines. */
    printer_bufinit();
    writemon_init();

    /* If running in test mode, don't really start an interface. */
    if(test_mode)
    	{
	intstdin = 1;
	intstdout = 0;
	feedback_setup(intstdout);
    	return;
    	}

    if(*printer.Interface == '/')	/* if absolute path, */
	{				/* use it */
	fname_ptr = printer.Interface;
	}
    else				/* Otherwise, prepend the interfaces */
	{				/* directory name */
	ppr_fnamef(fname, "%s/%s", INTDIR, printer.Interface);
	fname_ptr = fname;
	}

    /* Make sure we can execute the interface program. */
    if(access(fname_ptr, X_OK) == -1)
	fatal(EXIT_PRNERR_NORETRY, _("Interface \"%s\" does not exist or is not executable, errno=%d (%s)."), printer.Interface, errno, gu_strerror(errno));

    /* Open a pipe to the interface. */
    if(pipe(_stdin) < 0)
	fatal(EXIT_PRNERR, "%s(): pipe() failed, errno=%d (%s)", function, errno, gu_strerror(errno));

    /* Open a pipe from the interface. */
    if(pipe(_stdout) < 0)
	fatal(EXIT_PRNERR, "%s(): pipe() failed, errno=%d (%s)", function, errno, gu_strerror(errno));

    /* Block SIGCHLD until we can get the
       PID recorded in intpid. */
    {
    sigset_t nset;
    sigemptyset(&nset);
    sigaddset(&nset, SIGCHLD);
    sigprocmask(SIG_BLOCK, &nset, NULL);
    }

    /* get ready for a SIGUSR1 */
    sigusr1_caught = FALSE;

    /* duplicate this process */
    if((intpid = fork()) < 0)
	{
	/* Fork failed.  We don't want this information building up
	       in the pprdrv log file, so we will not
	       use fatal(). */
	DODEBUG_INTERFACE(("%s(): fork() failed, errno=%d (%s)", function, errno, gu_strerror(errno)));
	hooked_exit(EXIT_STARVED, (char*)NULL);
	}

    /* Child */
    if(intpid == 0)
	{
	umask(PPR_INTERFACE_UMASK);	/* for temporary file security */
	setpgid(0, 0);			/* for easier killing */
	new_stdin=dup(_stdin[0]);	/* read end of stdin is for us */
	new_stdout=dup(_stdout[1]);	/* write end of stdout is for us */
	close(_stdin[0]);		/* we don't need these any more */
	close(_stdin[1]);		/* we must clear them away now */
	close(_stdout[0]);		/* as some of them are shurly */
	close(_stdout[1]);		/* file descritors 0, 1, and 2 */
	dup2(new_stdin,0);		/* copy the temporaries to the real */
	dup2(new_stdout,1);		/* things */
	dup2(new_stdout,2);
	close(new_stdin);		/* close the temporaries */
	close(new_stdout);

	/* Convert two of the parameters from integers to strings. */
	snprintf(jobbreak_str, sizeof(jobbreak_str), "%1.1d", printer.Jobbreak);
	snprintf(feedback_str, sizeof(feedback_str), "%1.1d", printer.Feedback);
	snprintf(codes_str, sizeof(codes_str), "%1.1d", (int)printer.Codes);

	/*
	** Our child should not start out with signals blocked,
	** even if we have some blocked, so clear the signal mask.
	** For example, SIGCHLD will have been blocked.
	*/
	{
	sigset_t sigset;
	sigemptyset(&sigset);
	sigprocmask(SIG_SETMASK, &sigset, (sigset_t*)NULL);
	}

	/* We will now overlay this child process with the interface. */
	execl(fname_ptr, printer.Interface, /* replace child pprdrv with */
	    printer.Name,                   /* the interface */
	    printer.Address,
	    printer.Options,
	    jobbreak_str, feedback_str, codes_str,
	    QueueFile,
	    job.Routing ? job.Routing : "",
	    job.For ? job.For : "",
	    BarBarPDL ? BarBarPDL : "",
	    (char*)NULL);

	/*
	** Does it mean anything if we exit with EXIT_PRNERR_NORETRY?
	** Yes it does!  We _ARE_ the interface.  We can do anything
	** an interface program can do!  Hah!  Hah!  Haah!
	*/
	fatal(EXIT_PRNERR_NORETRY, "%s(): execl() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
	}

    /* Parent */
    setpgid(intpid, 0);         /* in case we kill before it runs (race condition) */
    close(_stdin[0]);           /* close our copy of read end */
    close(_stdout[1]);          /* close unneeded write end */
    intstdin = _stdin[1];
    intstdout = _stdout[0];

    /* Unblock SIGCHLD which was blocked before the fork(): */
    {
    sigset_t nset;
    sigemptyset(&nset);
    sigaddset(&nset, SIGCHLD);
    sigprocmask(SIG_UNBLOCK, &nset, NULL);
    }

    /*
    ** Make the two file descriptors which we use to communicate with the
    ** interface non-blocking.  We will be using select() to figure out
    ** when they are ready.
    */
    {
    int flags;

    flags = fcntl(intstdin, F_GETFL);
    flags |= O_NONBLOCK;
    if(fcntl(intstdin, F_SETFL, flags) < 0)
    	fatal(EXIT_PRNERR, "%s(): fcntl() failed (on intstdin), errno=%d (%s)", function, errno, gu_strerror(errno));

    flags = fcntl(intstdout, F_GETFL);
    flags |= O_NONBLOCK;
    if(fcntl(intstdout, F_SETFL, flags) < 0)
    	fatal(EXIT_PRNERR, "%s(): fcntl() failed (on intstdout), errno=%d (%s)", function, errno, gu_strerror(errno));
    }

    /* Tell the feedback reader which fd to read on.  This must be done
       before there is any chance that anyone will call feedback_wait();
       or interface_fault_check(). */
    feedback_setup(intstdout);

    /*
    ** If SIGUSR1 protocol is being used, wait for the
    ** child to inform us that it has installed _its_
    ** SIGUSR1 handler by sending _us_ SIGUSR1.
    **
    ** We use the stall warning mechanism in this code
    ** even though it is not quite right for the job.
    ** Our intent is to detect an interface which does
    ** not start up properly or does not support
    ** JOBBREAK_SIGNAL correctly.
    */
    if(printer.Jobbreak == JOBBREAK_SIGNAL || printer.Jobbreak == JOBBREAK_SIGNAL_PJL)
	{
	sigset_t nset, oset;            /* newset, oldset */

	DODEBUG_INTERFACE(("%s(): waiting for SIGUSR1", function));

	sigemptyset(&nset);             /* Block SIGUSR1. */
	sigaddset(&nset, SIGUSR1);
	sigaddset(&nset, SIGUSR1);
	sigprocmask(SIG_BLOCK, &nset, &oset);

	sigalrm_caught = FALSE;
	alarm(30);

	while(! sigusr1_caught && ! sigalrm_caught)
	    sigsuspend(&oset);

	alarm(0);

	sigprocmask(SIG_SETMASK, &oset, (sigset_t*)NULL);	/* Unblock SIGUSR1. */

	if(!sigusr1_caught)
	    {
	    interface_fault_check();
	    fatal(EXIT_PRNERR_NORETRY, "%s(): timeout during SIGUSR1 handshaking startup", function);
	    }
	}

    DODEBUG_INTERFACE(("%s(): done", function));
    } /* end of start_interface() */

/*
** This routine sends an end of file to the interface process.
**
** If the interface called exit(), we will return the code,
** otherwise, we return the code it should have returned.
*/
static int close_interface(void)
    {
    const char function[] = "close_interface";
    pid_t pid;
    int exit_code;

    DODEBUG_INTERFACE(("%s()", function));

    /* Flush out the last of the printer data. */
    printer_flush();

    /* If we are running in test mode there is nothing more to do. */
    if(test_mode)
    	return EXIT_PRINTED;

    /* We will handle reaping from here. */
    signal_restarting(SIGCHLD, empty_sigchld_handler);

    /* Close the pipe to the interface. */
    close(intstdin);

    /* Wait last output and for the interface to terminate: */
    feedback_drain();
    close(intstdout);

    /* Get the interface's exit code. */
    while((pid = wait(&interface_wait_status)) != intpid)
	{
	if(pid == -1)
	    {
	    if(errno == EINTR)
	    	continue;
	    fatal(EXIT_PRNERR, "%s(): wait() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
	    }
	}

    /* Look for really wierd stuff. */
    interface_exit_screen();

    /* Get the exit code of the interface. */
    exit_code = WEXITSTATUS(interface_wait_status);

    DODEBUG_INTERFACE(("%s(): returning %d", function, exit_code));
    return exit_code;
    } /* end of close_interface() */

/*
** If the interface has been launched, kill it.
*/
void kill_interface(void)
    {
    FUNCTION4DEBUG("kill_interface")
    DODEBUG_INTERFACE(("%s()", function));

    /* Disable interface failure detection. */
    signal_restarting(SIGCHLD, empty_sigchld_handler);

    if(intpid)
	{
	DODEBUG_INTERFACE(("%s(): killing interface, intpid=%d", function, intpid));
	kill((intpid*(-1)), SIGTERM);	/* kill the interface */
	}                               /* and its children */
    #ifdef DEBUG_INTERFACE
    else
    	{
	debug("%s(): nothing to kill", function);
    	}
    #endif

    DODEBUG_INTERFACE(("%s(): done", function));
    } /* end of kill_interface() */

/*
** Routine to send SIGUSR1 job breaks.  This is only used with
** interfaces which use the "signal" or "signal/pjl" jobbreak
** methods.
*/
static void signal_jobbreak(void)
    {
    FUNCTION4DEBUG("signal_jobbreak")
    int timeout;

    DODEBUG_INTERFACE(("%s()", function));

    /* Force buffered data into pipe. */
    DODEBUG_INTERFACE(("%s(): calling printer_flush()", function));
    printer_flush();

    /* get ready to catch signal */
    sigusr1_caught = FALSE;

    /* Send the signal to the interface */
    DODEBUG_INTERFACE(("%s(): kill(%ld, SIGUSR1)", function, (long int)intpid));
    kill(intpid, SIGUSR1);

    /* and wait for the response */
    writemon_start("WAIT_SIGNAL");
    for(timeout=1; !sigusr1_caught; timeout++)
	{
	DODEBUG_INTERFACE(("%s(): timeout=%d", function, timeout));
	feedback_wait(timeout, TRUE);
	}
    writemon_unstalled("WAIT_SIGNAL");

    DODEBUG_INTERFACE(("%s(): done", function));
    } /* end of signal_jobbreak() */

/*
** Send the PJL Universal Exit Language command to the printer.
*/
void printer_universal_exit_language(void)
    {
    printer_puts("\33%-12345X");
    }

/*
** Use a PJL command to set the display message on a printer.
**
** This code is a little more complicated than would seem necessary
** because printer_printf() does not support "%.*".
*/
#define PJL_DISPLAY_LEN 32 /* 16 */
void printer_display_printf(const char message[], ...)
    {
    char temp[PJL_DISPLAY_LEN + 1];
    va_list va;
    va_start(va, message);
    vsnprintf(temp, sizeof(temp), message, va);
    va_end(va);
    printer_printf("@PJL RDYMSG DISPLAY = \"%s\"\n", temp);
    }

/*
** Send setup strings to the printer.  We use the information
** from the "JobBreak: " line to decide what to send.
*/
static const char *setup_BarBarPDL;
static void printer_setup(const char *BarBarPDL, int iteration)
    {
    DODEBUG_INTERFACE(("printer_setup(\"%s\")", BarBarPDL ? BarBarPDL : ""));

    setup_BarBarPDL = BarBarPDL;

    /*
    ** Now, act according to jobbreak method.
    */
    switch(printer.Jobbreak)
    	{
     	case JOBBREAK_CONTROL_D:		/* <-- `Serial' control-d protocol */
	    if(iteration == 1)			/* if first time called, */
		printer_putc(4);		/* send a control-d to clear the printer */
    	    break;

    	case JOBBREAK_PJL:			/* <-- PJL without signals */
	case JOBBREAK_SIGNAL_PJL:		/* <-- PJL with signals */
	    printer_universal_exit_language();

	    /* If this is a non-PostScript job with a PJL header,
	       emmit the PJL header stuff here. */
	    if(BarBarPDL && job.PJL)
		printer_puts(job.PJL);

	    /* Set message on printer's LCD.  We know of no reason why job.For 
	       should ever be NULL, but we are paranoid. */
	    printer_display_printf("%s", job.For ? job.For : "???");
	    /* printer_puts("@PJL RDYMSG2 DISPLAY = \"xxxx\"\n"); */

	    /* Ask for the current status of the printer. */
	    printer_puts("@PJL INFO STATUS\n");

	    /* Ask printer to send a message at the end of each page,
	       at the end of the job, and every time the printer's
	       status changes. */
	    printer_puts("@PJL USTATUSOFF\n");
	    printer_puts("@PJL USTATUS PAGE = ON\n");
	    printer_puts("@PJL USTATUS JOB = ON\n");
	    printer_puts("@PJL USTATUS DEVICE = ON\n");

	    /* Tell the printer to tell us a job has started! */
	    printer_puts("@PJL JOB\n");

	    /* Manually select PostScript personality. */
	    printer_printf("@PJL ENTER LANGUAGE = %s\n", BarBarPDL ? BarBarPDL : "postscript");

	    /* If this is PostScript and TBCP is called for, turn it on. */
	    if(!BarBarPDL && printer.Codes == CODES_TBCP)
		{
		printer_puts("\1M");
		printer_TBCP_on();
		}

	    break;

	case JOBBREAK_SAVE_RESTORE:		/* <-- silly, optimistic method */
	    printer_puts("save\n");
	    break;

	default:				/* <-- all others, do nothing */
	    break;
	}

    DODEBUG_INTERFACE(("printer_setup(): done"));
    } /* end of printer_setup() */

/*
** Printer cleanup.  Undo what we did or stuff like that.
** We use the information from the "JobBreak: " line.
*/
static void printer_cleanup(void)
    {
    FUNCTION4DEBUG("printer_cleanup")
    DODEBUG_INTERFACE(("%s()", function));

    switch(printer.Jobbreak)
    	{
    	case JOBBREAK_CONTROL_D:		/* <-- simple ``serial'' control-D protocol */
	    printer_putc(4);
	    printer_flush();
            if(printer.Feedback)
                {
		writemon_start("WAIT_CONTROL-D");
                while(control_d_count > 0)	/* continue until all matched */
                    {
                    DODEBUG_INTERFACE(("%s(): control_d_count = %d", function, control_d_count));
                    feedback_wait(0, FALSE);	/* wait for data from printer */
                    }
		writemon_unstalled("WAIT_CONTROL-D");
                }
    	    break;

    	case JOBBREAK_PJL:			/* <-- PJL with control-D */
    	case JOBBREAK_SIGNAL_PJL:		/* <-- PJL with signals */
	    if(!setup_BarBarPDL)		/* If PostScript, */
		{
		if(printer.Codes == CODES_TBCP)
		    printer_TBCP_off();

		if(printer.Jobbreak == JOBBREAK_SIGNAL_PJL)
		    signal_jobbreak();
		else
		    printer_putc(4);		/* <-- serial/parallel/JetDirect end of job */
		}

	    /* Send "Universal Exit Language" command to place printer
	       in PJL mode.  (Unfortunately, this drives an HP 4M nuts
	       if it is on AppleTalk!  I haven't found a solution yet.) */
	    printer_universal_exit_language();

	    /* Use PJL to tell the printer that the job is over.
	       It will give us a response when the print engine is done. */
	    printer_puts("@PJL EOJ\n");

	    /* Since we will be waiting for a response, we better make
	       sure the commands aren't stuck in the output buffer! */
	    printer_flush();

	    /* Would you believe, we must not transmit the command to turn
	       this stuff off until we have received all the notifications?
	       That is why we call feedback_pjl_wait() to pause until the
	       printer responds to the "@PJL EOJ".
	    */
	    feedback_pjl_wait();

	    /* Just to be safe, we will send the "Universal Exit Language" command
	       again.  Then we will clear the display and try to generally
	       reset the printer's PJL features. */
	    printer_universal_exit_language();
	    printer_display_printf("");
	    #ifdef DEBUG_INTERFACE
	    printer_display_printf("Sucessful!");
	    #endif
	    printer_puts("@PJL RESET\n");
	    printer_puts("@PJL USTATUSOFF\n");
	    printer_universal_exit_language();

	    break;

	case JOBBREAK_SAVE_RESTORE:		/* <-- that silly optimistic method */
	    printer_puts("\nrestore\n");
	    break;

	default:				/* <-- all other methods */
	    break;
	}

    /* Flush now so bytes sent get credited to correct job. */
    printer_flush();

    DODEBUG_INTERFACE(("%s(): done", function));
    } /* end of printer_cleanup() */

/*
** This function is the one called from outside this
** module whenever a job (including banner and trailer
** pages and query jobs) is started.
**
** The parameter "jobtype" is one of the following:
**
** JOBTYPE_QUERY
**	This is a PostScript query job.
**
** JOBTYPE_FLAG
**	This is a PostScript flag page.
**
** JOBTYPE_THEJOB
**	This is the main PostScript job.
**
** JOBTYPE_THEJOB_TRANSPARENT
**	This is the main job in the mode invoked with
**	"ppr -H transparent".
**
** JOBTYPE_THEJOB_BARBAR
**	This is the main job in some non-PostScript language.
*/
static enum JOBTYPE start_jobtype;
void job_start(enum JOBTYPE jobtype)
    {
    const char *BarBarPDL;
    static int iteration = 0;

    DODEBUG_INTERFACE(("job_start(jobtype=%d)", jobtype));

    /* Save the jobtype for job_end(). */
    start_jobtype = jobtype;

    /* If this is the main job and the main job is going to
       be in some PDL other than PostScript, then set BarBarPDL
       to the string which identifies that PDL. */
    switch(jobtype)
    	{
    	case JOBTYPE_THEJOB_BARBAR:
    	    BarBarPDL = job.PassThruPDL;
	    break;
	case JOBTYPE_THEJOB_TRANSPARENT:
	    BarBarPDL = job.Filters;
	    break;
	default:
	    BarBarPDL = NULL;
	    break;
	}

    /* Set a global flag so that the feedback reading routines
       will know whether pages count toward the job's page count.
       (Banner and trailer pages don't.)
       */
    doing_primary_job = (jobtype==JOBTYPE_THEJOB
		|| jobtype==JOBTYPE_THEJOB_BARBAR
		|| jobtype==JOBTYPE_THEJOB_TRANSPARENT)
    	? TRUE : FALSE;

    if(intstdin == -1)
    	{
    	start_interface(BarBarPDL);
    	}
    else if(printer.Jobbreak == JOBBREAK_SIGNAL)
    	{
    	signal_jobbreak();
    	}
    else if(printer.Jobbreak == JOBBREAK_NEWINTERFACE)
    	{
	close_interface();          /* Error code ignored. */
	start_interface(BarBarPDL);
	}

    iteration++;
    if(jobtype != JOBTYPE_THEJOB_TRANSPARENT)
	printer_setup(BarBarPDL, iteration);

    DODEBUG_INTERFACE(("job_start(): done"));
    } /* end of job_start() */

/*
** This is called, from outside this module whenever
** the end of a job is reached.
*/
void job_end(void)
    {
    DODEBUG_INTERFACE(("job_end()"));

    if(start_jobtype != JOBTYPE_THEJOB_TRANSPARENT)
	printer_cleanup();

    DODEBUG_INTERFACE(("job_end(): done"));
    } /* end of job_end() */

/*
** This is called after the last time job_end() is called.
*/
int job_nomore(void)
    {
    int ret;
    DODEBUG_INTERFACE(("job_nomore()"));
    ret = close_interface();
    intstdin = -1;
    DODEBUG_INTERFACE(("job_nomore(): done"));
    return ret;
    } /* end of job_nomore() */

/* end of file */

