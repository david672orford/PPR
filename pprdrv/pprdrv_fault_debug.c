/*
** mouse:~ppr/src/pprdrv/pprdrv_fault_debug.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 3 May 2001.
*/

#include "before_system.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>
#include "gu.h"
#include "global_defines.h"
#include "pprdrv.h"
#include "interface.h"

/*
** This module contains exiting, fatal error, and debug message
** printing code.
*/

/*
** Get the current time with microsecond resolution
** and compute the difference between the current time
** and the time that was recorded when we started.
*/
static void get_elapsed_time(struct timeval *time_elapsed)
    {
    struct timeval time_now;
    gettimeofday(&time_now, (struct timezone *)NULL);
    time_elapsed->tv_sec = time_now.tv_sec - start_time.tv_sec;
    time_elapsed->tv_usec = time_now.tv_usec - start_time.tv_usec;
    if(time_elapsed->tv_usec < 0) {time_elapsed->tv_usec += 1000000; time_elapsed->tv_sec--;}
    }

/*
** If possible, write a line to pprdrv's log file.
*/
static void pprdrv_log_vprintf(const char msgclass[], const char format[], va_list va)
    {
    FILE *f;
    if(test_mode)
	f = stderr;
    else
	f = fopen(PPRDRV_LOGFILE, "a");

    if(f)
	{
        struct timeval time_elapsed;
        get_elapsed_time(&time_elapsed);
        fprintf(f, "%s (%ld %s %ld.%02d): ", msgclass, (long)getpid(), datestamp(), (long)time_elapsed.tv_sec, (int)(time_elapsed.tv_usec / 10000) );
	vfprintf(f, format, va);
	fputc('\n', f);
	}

    if(!test_mode && f)
    	fclose(f);
    } /* end of pprdrv_log_printf() */

/*
** If possible, write a piece of text to the job's log file.
*/
static void job_log_vprintf(const char msgclass[], const char format[], va_list va)
    {
    FILE *f;
    if(test_mode)
	{
	f = stderr;
	}
    else
	{
        char fname[MAX_PPR_PATH];
        ppr_fnamef(fname, "%s/%s-log", DATADIR, QueueFile);
	f = fopen(fname, "a");
	}

    if(f)
	{
	fprintf(f, "%s: ", msgclass);
	vfprintf(f, format, va);
	fputc('\n', f);
	}

    if(!test_mode && f)
    	fclose(f);
    } /* end of job_log_vprintf() */

/*
** With the exception of the case where pprdrv is invoked with the wrong number
** of arguments and sucessful printing (which calls most of the functions
** that this function calls) this function is called instead of exit().
**
** fatal() calls this function as its last act.
*/
void hooked_exit(int rval, const char *explain)
    {
    /* Let the commentator know we will be exiting almost immediately so that
       it can announce the fact. */
    commentary_exit_hook(rval, explain);

    /* If the interface program is running, stop it. */
    kill_interface();

    /* Let the "ppop status" code clear the auxiliary status messages. */
    ppop_status_exit_hook(rval);

    /* Wait for the commentators to finish. */
    commentator_wait();

    /* Clear "waiting for commentators". */
    ppop_status_shutdown();

    /* Now we can do a real exit. */
    exit(rval);
    } /* end of hooked_exit() */

/*
** This function adds an error message to pprdrv's log file, and if not in
** test mode, to either the printer's or the job's log file.  It then calls
** hooked_exit().
**
** Not all fatal errors pass through this function.  In some cases, alert()
** is called and then hooked_exit().  Doing it that way can produce
** prettyer messages.  Theirfor this function is used for reporting the most
** unlikely faults.
**
** This function ought to be ok to call from within
** signal handlers.
*/
void fatal(int rval, const char string[], ...)
    {
    va_list va;
    va_start(va, string);

    /* Because fatal() is not used for things that should happen
       during normal operation, log the problem in the pprdrv log. */
    pprdrv_log_vprintf("FATAL", string, va);

    if(rval == EXIT_JOBERR)		/* problem with print job */
	{
	job_log_vprintf("FATAL", string, va);
	}
    else 				/* problem with printer */
	{
	if(!test_mode)
	    valert(printer.Name, TRUE, string, va);
	}

    va_end(va);

    hooked_exit(rval, (char*)NULL);
    } /* end of fatal() */

/*
** This stript-down version of fatal() is safe to call
** from within signal handlers.  It does a lot less.
*/
void signal_fatal(int rval, const char format[], ...)
    {
    va_list va;
    int fd;
    va_start(va, format);
    if((fd = open(PPRDRV_LOGFILE, O_WRONLY | O_APPEND | O_CREAT, UNIX_644)) >= 0)
    	{
	char tempstr[256];
	write(fd, "FATAL: ", 7);
	vsnprintf(tempstr, sizeof(tempstr), format, va);
	write(fd, tempstr, strlen(tempstr));
	write(fd, "\n", 1);
	close(fd);
    	}
    va_end(va);
    _exit(rval);
    } /* end of signal_fatal() */

/*
** This routine is generally used only for unusual error
** conditions, internal errors.  It is not used to report
** routine conditions such as printer faults.  It is used
** for debug messages which we still want to see when
** debugging is not compiled in.
*/
void error(const char *string, ... )
    {
    va_list va;
    va_start(va, string);
    pprdrv_log_vprintf("ERROR", string, va);
    va_end(va);
    } /* end of error() */

/*
** Print a debug line in the pprdrv log file.
**
** This code is not included in production versions.  If you want
** the debugging message to be included in the production version,
** use error() instead.
*/
#ifdef DEBUG
void debug(const char string[], ...)
    {
    va_list va;
    va_start(va, string);
    pprdrv_log_vprintf("DEBUG", string, va);
    va_end(va);
    } /* end of debug() */

/* This one is for signal handlers. */
void signal_debug(const char format[], ...)
    {
    va_list va;
    int fd;
    va_start(va, format);
    if((fd = open(PPRDRV_LOGFILE, O_WRONLY | O_APPEND | O_CREAT, UNIX_644)) >= 0)
    	{
	char tempstr[256];
	write(fd, "DEBUG: ", 7);
	vsnprintf(tempstr, sizeof(tempstr), format, va);
	write(fd, tempstr, strlen(tempstr));
	write(fd, "\n", 1);
	close(fd);
    	}
    va_end(va);
    }
#endif

/*
** This function is called from within certain libppr functions
** when they encounter truly exceptional conditions.
** This function overrides the default libppr_throw() in libppr.a.
*/
void libppr_throw(int exception_type, const char exception_function[], const char format[], ...)
    {
    va_list va;
    char tempstr[256];

    va_start(va, format);
    snprintf(tempstr, sizeof(tempstr), "%s() failed: %s", exception_function, format);
    va_end(va);

    pprdrv_log_vprintf("FATAL", tempstr, va);
    job_log_vprintf("FATAL", tempstr, va);

    if(exception_type == EXCEPTION_STARVED)
    	hooked_exit(EXIT_STARVED, NULL);
    else
        hooked_exit(EXIT_PRNERR_NORETRY, NULL);
    } /* end of libppr_throw() */

/* end of file */

