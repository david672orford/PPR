/*
** mouse:~ppr/src/pprd/pprd_log.c
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
** Last modified 23 September 2005.
*/

#include "config.h"
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "pprd.h"
#include "./pprd.auto_h"

gu_boolean lockfile_created = FALSE;

/*
** This is the core function which all of those below call.
*/
static void log(const char category[], const char function[], const char message[], va_list va)
	{
	FILE *file;
	if((file = fopen(PPRD_LOGFILE, "a")) != (FILE*)NULL)
		{
		fputs(category, file);
		if(function)
			fprintf(file, " %s()", function);
		fprintf(file, ": (%s) ", datestamp() );
		vfprintf(file, message, va);
		fputc('\n', file);
		fclose(file);
		}
	} /* end of log() */

/*
** Print an error message and abort.
*/
void fatal(int exitval, const char message[], ... )
	{
	/*
	** Free at least one file handle.
	** If we don't do this then we will not be able
	** to print an error message is some operation
	** fails because we have run out of file
	** handles.
	*/
	close(3);

	/* Add the line to the log. */
	{
	va_list va;
	va_start(va, message);
	log("FATAL", NULL, message, va);
	va_end(va);
	}

	/* Remove the lock file which also has our PID in it. */
	if(lockfile_created)
		unlink(PPRD_LOCKFILE);

	/*
	** This code responds to special fatal() calls which may
	** have been inserted for debugging purposes.  If we have
	** been passed the magic exit value ERROR_COREDUMP, then try to
	** dump core.  We do two kill()s because for some mysterious
	** reason SunOS 5.5.1 cron launches processes which can't
	** receive SIGQUIT.  Possibly it is because they don't
	** have a terminal.
	*/
	if(exitval == ERROR_DUMPCORE)
		{
		va_list va;
		kill(getpid(), SIGQUIT);
		kill(getpid(), SIGABRT);

		va_start(va, message);	/* silly */
		log("SIGQUIT", "fatal", "Suicide failed, no core file created", va);
		va_end(va);
		}

	exit(exitval);
	} /* end of fatal() */

/*
** Print a log line classifying it as an error.
*/
void error(const char message[], ...)
	{
	va_list va;
	va_start(va, message);
	log("ERROR", NULL, message, va);
	va_end(va);
	} /* end of error() */

/*
** Print a log line, classifying it as a debug line.
*/
void debug(const char message[], ...)
	{
	va_list va;
	va_start(va, message);
	log("DEBUG", NULL, message, va);
	va_end(va);
	} /* end of debug() */

/*
** Override the default gu_throw() so that we can get the
** message into our log file.
*/
void gu_throw(int exception_type, const char exception_function[], const char format[], ...)
	{
	va_list va;
	va_start(va, format);
	log("libgu exception", exception_function, format, va);
	va_end(va);
	fatal(1, "above was fatal");
	} /* end of gu_throw() */

/* end of file */

