/*
** mouse:~ppr/src/libppr/datestamp.c
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
** This  routine is used for log file data stamps.
*/

#include "before_system.h"
#include <time.h>
#include "gu.h"
#include "global_defines.h"


static int old_tm_year = 0;	/* year and day of last datestamp */
static int old_tm_yday = 0;	/* initial value works out to 1 Jan 1970 */
static char tempdate[33];

char *datestamp(void)
    {
    time_t seconds_now;
    int len;
    struct tm *time_detail;

    seconds_now = time((time_t*)0);		/* get seconds since 1/1/70 */
    time_detail = localtime(&seconds_now);	/* conv into days, hours, etc */

    if(time_detail->tm_year == old_tm_year && time_detail->tm_yday == old_tm_yday)
	{        /* if not new day, */
	len = strftime(tempdate, sizeof(tempdate), "%I:%M:%S%p", time_detail);
	}
    else
	{        /* if new day, */
	len = strftime(tempdate, sizeof(tempdate), "%d %b %Y, %I:%M:%S%p", time_detail);
	old_tm_year = time_detail->tm_year;
	old_tm_yday = time_detail->tm_yday;
	}

    if(len)                 /* if strftime() worked, */
	return tempdate;    /* return pointer to result */
    else                    /* otherwise, */
	return "<date missing>"; 
    } /* end of datestamp() */

/* end of file */
