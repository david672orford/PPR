/*
** mouse:~ppr/src/libppr/tail_status.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 8 December 1999.
*/

#include "before_system.h"
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
	if((p = strchr(buffer, '\n')))		/* remove line-feed */
	    *p = '\0';
	if((*callback)(buffer, extra))		/* call callback routine */
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
	if( (tail_pprd && do_tail(callback, &pprd_fd, STATE_UPDATE_FILE, &pprd_file_count, extra) > 0)
		|| (tail_pprdrv && do_tail(callback, &pprdrv_fd, STATE_UPDATE_PPRDRV_FILE, &pprdrv_file_count, extra) > 0) )
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

