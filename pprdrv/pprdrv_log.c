/*
** mouse:~ppr/src/templates/module.c
** Copyright 1995--2003, Trinity College Computing Center.
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
** Last modified 24 February 2003.
*/

/*===========================================================================
** This module contains the functions that open and write to the jobs log 
** file or open it for reading by the caller.
===========================================================================*/

#include "before_system.h"
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "pprdrv.h"
#include "interface.h"

static FILE *log = NULL;

/* This opens the job log for write.  This is called from some of the functions below. */
static void log_open(void)
    {
    /* Open the log file for write. */
    if(test_mode)
	{
	int dfd;
	if((dfd = dup(2)) == -1)
	    fatal(EXIT_PRNERR, X_("dup(2) failed, errno=%d, (%s)"), errno, gu_strerror(errno));
	if(!(log = fdopen(dfd, "w")))
	    fatal(EXIT_PRNERR, X_("fdopen(%d, \"w\") failed, errno=%d (%s)"), dfd, errno, gu_strerror(errno));
	}
    else
	{
        char fname[MAX_PPR_PATH];
        ppr_fnamef(fname, "%s/%s-log", DATADIR, QueueFile);
        if((log = fopen(fname, "a")) == (FILE*)NULL)
	    fatal(EXIT_PRNERR, _("Can't open \"%s\", errno=%d (%s)"), fname, errno, gu_strerror(errno));
	}

    /* Be paranoid. */
    gu_set_cloexec(fileno(log));
    }

/* This is called by the banner page printing code in pprdrv_flag.c. */
FILE *log_reader(void)
    {
    char fname[MAX_PPR_PATH];
    ppr_fnamef(fname, "%s/%s-log", DATADIR, QueueFile);
    return fopen(fname, "r");
    }

void log_putc(int c)
    {
    if(!log)
    	log_open();
    fputc(c, log);
    }

void log_puts(const char data[])
    {
    if(!log)
    	log_open();
    fputs(data, log);
    }

void log_vprintf(const char format[], va_list va)
    {
    if(!log)
    	log_open();
    vfprintf(log, format, va);
    }

void log_printf(const char format[], ...)
    {
    va_list va;
    va_start(va, format);
    log_vprintf(format, va);
    va_end(va);
    }

void log_flush(void)
    {
    if(log)
    	fflush(log);
    }

void log_close(void)
    {
    if(log)
    	fclose(log);
    log = NULL;
    }

/* end of file */
