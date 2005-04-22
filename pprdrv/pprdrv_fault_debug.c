/*
** mouse:~ppr/src/pprdrv/pprdrv_fault_debug.c
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
** Last modified 22 April 2005.
*/

#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
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
	DODEBUG_MAIN(("hooked_exit(): calling exit(%d)", rval));
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

	/* Because fatal() is not used for things that should happen
	   during normal operation, log the problem in the pprdrv log. */
	va_start(va, string);
	pprdrv_log_vprintf("FATAL", string, va);
	va_end(va);

	va_start(va, string);
	if(rval == EXIT_JOBERR)				/* problem with print job */
		{
		job_log_vprintf("FATAL", string, va);
		}
	else								/* problem with printer */
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
	if((fd = open(PPRDRV_LOGFILE, O_WRONLY | O_APPEND | O_CREAT, UNIX_644)) >= 0)
		{
		char tempstr[256];
		write(fd, "FATAL: ", 7);
		va_start(va, format);
		vsnprintf(tempstr, sizeof(tempstr), format, va);
		va_end(va);
		write(fd, tempstr, strlen(tempstr));
		write(fd, "\n", 1);
		close(fd);
		}
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

/* This wraps the main() function in order to install a special 
 * exception handler which writes the exception to the printer's alerts log.
 */
int main(int argc, char *argv[])
	{
	gu_Try {
		return real_main(argc, argv);
		}
	gu_Catch {
		pprdrv_log_vprintf("FATAL", "%s", gu_exception);
		job_log_vprintf("FATAL", "%s", gu_exception);

		if(strstr(gu_exception, "alloc()") || strstr(gu_exception, "fork()"))
			hooked_exit(EXIT_STARVED, NULL);
		else
			hooked_exit(EXIT_PRNERR_NORETRY, NULL);
		}
	/* NOREACHED */
	return 255;
	} /* end of main() */

/* end of file */

