/*
** mouse:~ppr/src/libppr/alert.c
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
** Last modified 30 January 2003.
*/

/*
** This module contains routines for posting printer alerts.  
**
** Normally the alert is posted to the printer's alert log file, but if
** the printer name is "-" then the alert message is sent to stderr.
**
** There is probably no longer any good reason for it, but these routines
** avoid the use of stdio.  They format the message in a fixed length
** buffer, so there is a limit on how long the alert message can be.
*/

#include "before_system.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include "gu.h"
#include "global_defines.h"
#include "libppr_int.h"

void valert(const char printer[], int dateflag, const char string[], va_list args)
    {
    int afile;
    time_t now;

    /* Bail out if called as a result of a fatal error which occured
       before the caller has determined the printer name. */
    if(printer == (char*)NULL)
    	return;

    /* We will be comparing the current time to the alert file modification time. 
       Not only that, but we might use it for a time stamp. */
    time(&now);

    /* If the printer name is the special value "-", use stderr. */
    if(strcmp(printer, "-") == 0)
	{
    	afile = 2;
	}

    /* Otherwise, use the printer's alerts file. */
    else
	{
	char fname[MAX_PPR_PATH];
	struct stat statbuf;

	ppr_fnamef(fname, "%s/%s", ALERTDIR, printer);

	/* If can't stat() the alert log or it is older than one hour, truncate it. */
	if( stat(fname, &statbuf) != 0 || (difftime(now, statbuf.st_mtime) > 3600) )
	    afile = open(fname, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, UNIX_644);

	/* If younger than one hour, append. */
	else
	    afile = open(fname, O_WRONLY | O_CREAT | O_APPEND, UNIX_644);
	}

    /* If we have a file we can use, */
    if(afile != -1)
	{
	char templine[512];
	int len = 0;
	templine[0] = '\0';

	/* If the caller has indicated that this is the first line of the alert
	   message, then leave a blank line and print a datestamp. */
	if(dateflag && strcmp(printer, "-"))
	    {
	    snprintf(templine, sizeof(templine), "\n%s", ctime(&now));
	    len = strlen(templine);
	    }

	/* Add the message. */
	vsnprintf(templine + len, sizeof(templine) - len, string, args);
	len += strlen(templine + len);

	/* Add a line-feed. */
	snprintf(templine + len, sizeof(templine) - len, "\n");
	len += strlen(templine + len);

	write(afile, templine, len);

	if(strcmp(printer, "-"))
	    close(afile);
	}

    } /* end of valert() */

void alert(const char printer[], int dateflag, const char string[], ...)
    {
    va_list va;

    va_start(va,string);
    valert(printer, dateflag, string, va);
    va_end(va);
    } /* end of alert() */

/* end of file */

