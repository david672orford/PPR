/*
** mouse:~ppr/src/libppr/tail_status.c
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
** Last modified 27 March 2003.
*/

#include "config.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "gu.h"
#include "global_defines.h"


static int do_tail(gu_boolean (*callback)(char *p, void *extra), FILE **fd, const char filename[], int *file_count, void *extra)
	{
	char buffer[256];
	struct stat statbuf;
	int callback_hits = 0;

	/* If the file isn't open yet, */
	if(!(*fd))
		{
		/* Open it. */
		if(!(*fd = fopen(filename, "r")))
			{
			return -1;
			}

		/* If this is the first file, skip to the end. */
		if((*file_count)++ == 0)
			{
			fseek(*fd, 0, SEEK_END);
			}
		}

	/* Read lines from the file until there are no more. */
	while(fgets(buffer, sizeof(buffer), *fd))
		{
		char *p;
		if((p = strchr(buffer, '\n')))			/* remove line-feed */
			*p = '\0';
		if((*callback)(buffer, extra))			/* call callback routine */
			callback_hits++;
		}

	/* Get the file status so we can see the user execute bit. */
	if(fstat(fileno(*fd), &statbuf) < 0)
		{
		return -1;
		}

	/* If the user execute bit is set, this file is done, close it. */
	if(statbuf.st_mode & S_IXOTH)
		{
		fclose(*fd);
		*fd = NULL;
		}

	/* Return the number of lines that the callback routines says it
	   forwarded to its client. */
	return callback_hits;
	} /* end of do_tail() */

/*
** This function never returns.  It just keeps monitoring the status files and
** feeding lines to the callback routine.  The callback routine should return
** TRUE for lines it acts on, FALSE for those it ignores.  If more than
** timeout seconds pass without the callback routine returning TRUE (perhaps
** because it is not called) then it will be called with a NULL argument
** and the timeout will be reset.  This gives the callback routine an chance
** to periodically make sure that some other process with which it is
** communicating is still alive.
*/
void tail_status(gu_boolean tail_pprd, gu_boolean tail_pprdrv, gu_boolean (*callback)(char *p, void *extra), int timeout, void *extra)
	{
	int countup = 0;
	FILE *pprd_fd = NULL;
	int pprd_file_count = 0;
	FILE *pprdrv_fd = NULL;
	int pprdrv_file_count = 0;

	while(TRUE)
		{
		if(		(tail_pprdrv && do_tail(callback, &pprdrv_fd, STATE_UPDATE_PPRDRV_FILE, &pprdrv_file_count, extra) > 0)
			 || (tail_pprd	 && do_tail(callback, &pprd_fd,	  STATE_UPDATE_FILE,		&pprd_file_count,	extra) > 0)
			)
			{
			countup = 0;
			continue;
			}

		if(countup >= timeout)
			{
			(*callback)(NULL, extra);
			countup = 0;
			}

		sleep(1);
		countup++;
		}
	}

/* end of file */

