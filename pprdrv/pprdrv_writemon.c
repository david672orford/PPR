/*
** mouse:~ppr/src/pprdrv/pprdrv_writemon.c
** Copyright 1995--2002, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 15 January 2002.
*/

#include "before_system.h"
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "pprdrv.h"
#include "interface.h"
#include "commentary.h"

/*
** The purpose of this module is to detect when the printer
** is stalled.  A printer may become stalled if it is out
** of paper, if the PostScript code is taking a very long time
** to execute, or if the PostScript code is in an infinite loop.
*/

/* The stall monitoring variables: */
static struct
	{
	const char *remembered_operation;		/* what pprdrv is doing */
	struct timeval start_time;			/* when pprdrv begin doing it */
	gu_boolean start_time_set;			/* has start_time been recorded yet? */
	struct timeval stall_time;			/* how long was it stalled? */
	int stall_time_minutes_announced;		/* what was the number of minutes in the last announcement? */
	} stack[10];
static int stackp;

/* The page stopwatch: */
static gu_boolean online_online_valid;			/* do we have on-line/off-line info? */
static gu_boolean online_online;			/* is printer on-line? */
static struct timeval online_unaccumulated_start_time;	/* when did it go online or finish previous page? */
static struct timeval online_accumulator;		/* total of previous online periods */

/*=========================================================================
** This is called from start_interface() to set this module
** to its initial state.
=========================================================================*/
void writemon_init(void)
    {
    stackp = -1;
    gu_timeval_zero(&online_accumulator);	/* we haven't measured any time online */
    online_online_valid = FALSE;		/* we don't know if the printer is online */
    }

/*==========================================================================
** These routines are used for keeping track of how long operations
** associated with communicating with the printer are taking.
==========================================================================*/

static void writemon_stalled(const struct timeval *time_stalled);
static int compute_severity(int minutes);

/*
** This is called at the start of an operation that might stall.
** The variable operation is a string such as "WRITE", "QUERY",
** "WAIT_CONTROL-D", "WAIT_PJL", "WAIT_SIGNAL", or "CLOSE".
*/
void writemon_start(const char operation[])
    {
    const char function[] = "writemon_start";
    DODEBUG_INTERFACE(("%s(operation=\"%s\")", function, operation));

    stackp++;
    if(stackp >= 10)
    	fatal(EXIT_PRNERR_NORETRY, "%s(): assertion failed: stackp >= 10", function);

    stack[stackp].remembered_operation = operation;
    stack[stackp].start_time_set = FALSE;
    stack[stackp].stall_time_minutes_announced = 0;

    /* Let the "ppop status" code know. */
    ppop_status_writemon(operation, 0);
    }

/*
** How long the the writemon code want pprdrv to sleep before
** calling it if nothing happens?
**
** The actual stall messages are dispatched from inside this routine.
*/
int writemon_sleep_time(struct timeval *sleep_time, int timeout)
    {
    const char function[] = "writemon_sleep_time";
    struct timeval now_time, my_next_time, your_next_time;

    DODEBUG_INTERFACE(("%s(sleep_time=%d.%06d (old), timeout=%d)",
	function,
    	(int)sleep_time->tv_sec, (int)sleep_time->tv_usec,
    	timeout));

    if(stackp < 0)
        fatal(EXIT_PRNERR_NORETRY, "%s(): assertion failed: writemon_start() wasn't called", function);

    gettimeofday(&now_time, NULL);

    /* If first call to writemon_sleep_time() after writemon_start(), */
    if(!stack[stackp].start_time_set)
	{
	gu_timeval_cpy(&stack[stackp].start_time, &now_time);
	stack[stackp].start_time_set = TRUE;
	}

    if(timeout > 0)
    	{
	/* Figure out when the caller's timeout will expire. */
    	gu_timeval_cpy(&your_next_time, &stack[stackp].start_time);
	your_next_time.tv_sec += timeout;

	/* If the callers timeout has expired, we are done, tell the caller. */
	if(gu_timeval_cmp(&your_next_time, &now_time) <= 0)
	    return FALSE;
	}

    while(TRUE)
	{
        /* Figure out when we next want to make a commentary announcement. */
        gu_timeval_cpy(&my_next_time, &stack[stackp].start_time);
        my_next_time.tv_sec += ((stack[stackp].stall_time_minutes_announced + 1) * 60);

        DODEBUG_INTERFACE(("%s(): my_next_time=%d.%06d, now_time=%d.%06d, start_time=%d.%06d",
            function,
            (int)my_next_time.tv_sec, (int)my_next_time.tv_usec,
            (int)now_time.tv_sec, (int)now_time.tv_usec,
            (int)stack[stackp].start_time.tv_sec, (int)stack[stackp].start_time.tv_usec
            ));

        /* If the next commentary announcement should have been
           made already, make it now and go back to compute
           a new time for the next one. */
        if(gu_timeval_cmp(&my_next_time, &now_time) <= 0)
            {
            struct timeval temp;
            gu_timeval_cpy(&temp, &now_time);
            gu_timeval_sub(&temp, &stack[stackp].start_time);
            writemon_stalled(&temp);
	    continue;
            }
	break;
	}

    /* Use the soonest one as the time to interupt select. */
    if(timeout < 1 || gu_timeval_cmp(&my_next_time, &your_next_time) < 0)
    	gu_timeval_cpy(sleep_time, &my_next_time);
    else
    	gu_timeval_cpy(sleep_time, &your_next_time);

    /* Here is where we observe the limit set by "ppad pagetimelimit".
       First we must test that there is a limit and that we have been getting
       valid information about the printer's on-line state and that the printer
       is on-line (since the PageTimeLimit can't expire while the printer
       is off-line). */
    if(printer.PageTimeLimit > 0 && online_online_valid && online_online)
    	{
	struct timeval time_of_expiration;

	/* Algebra provides the following simple calculation derived from the obvious
	   complicated one in the folling manner:
		time_of_expiration = now + limit - accumulator - (now - unaccumulated_start_time)
		time_of_expiration = now + limit - accumulator - now + unaccumulated_start_time
		time_of_expiration = limit - accumulator + unaccumulated_start_time
		time_of_expiration = limit + unaccumulated_start_time - accumulator
	*/
	time_of_expiration.tv_sec = printer.PageTimeLimit;
	time_of_expiration.tv_usec = 0;
	gu_timeval_add(&time_of_expiration, &online_unaccumulated_start_time);
	gu_timeval_sub(&time_of_expiration, &online_accumulator);

	/* Has the time limit expired? */
	if(gu_timeval_cmp(&now_time, &time_of_expiration) > 0)
	    {
	    give_reason("PageTimeLimit exceeded");
	    hooked_exit(EXIT_JOBERR, "PageTimeLimit exceeded");
	    }

	/* If the PageTimeLimit expiration time is before the time we had
	   planed on sleeping until, change sleep_time to the PageTimeLimit
	   expiration time. */
	if(gu_timeval_cmp(&time_of_expiration, sleep_time) < 0)
	    gu_timeval_cpy(sleep_time, &time_of_expiration);
    	}

    /* Turn it into a realative time. */
    gu_timeval_sub(sleep_time, &now_time);

    if(sleep_time->tv_sec < 0 || (sleep_time->tv_sec == 0 && sleep_time->tv_usec < 0))
    	fatal(EXIT_PRNERR_NORETRY, "%s(): assertion failed, sleep_time is negative", function);

    return TRUE;
    } /* end of writemon_sleep_time() */

/*
** Check and see if we sent a commentator message indicatating that
** printing was stalled.  If we did, issue another message saying
** that the stall has cleared up.
**
** The first raw field is the opposite of the number of seconds it
** was stalled (with millisecond resolution, the second is set by
** writemon_start() and indicates what we were doing when it stalled.
*/
void writemon_unstalled(const char operation[])
    {
    const char function[] = "writemon_unstalled";

    DODEBUG_INTERFACE(("%s(\"%s\")", function, operation ? operation : ""));

    if(stackp < 0)
        fatal(EXIT_PRNERR_NORETRY, "%s(): assertion failed: writemon_start() wasn't called", function);

    if(strcmp(operation, stack[stackp].remembered_operation) != 0)
    	fatal(EXIT_PRNERR_NORETRY, "%s(): assertion failed: operation=\"%s\", remembered_operation=\"%s\"", function, operation, stack[stackp].remembered_operation);

    /* If we previously announced that it was stalled, */
    if(stack[stackp].stall_time_minutes_announced > 0)
    	{
        char num_str[10];
        int severity;

        snprintf(num_str, sizeof(num_str), "-%d.%03d", (int)stack[stackp].stall_time.tv_sec, (int)(stack[stackp].stall_time.tv_usec / 1000));
	severity = compute_severity(stack[stackp].stall_time_minutes_announced);

        /*
        ** Send the appropriate message.  The message differs according
        ** to how many minutes it was stalled.
        */
        switch(stack[stackp].stall_time_minutes_announced)
            {
            case 1:
            case 2:
            case 3:
            case 4:
                commentary(COM_STALL, "wasn't stalled", stack[stackp].remembered_operation, num_str, severity);
                break;
            default:
                commentary(COM_STALL, "is no longer stalled", stack[stackp].remembered_operation, num_str, severity);
                break;
            }
        }

    stackp--;

    /* If we are returning to a previous operation, tell the "ppop status" code. */
    if(stackp > -1)
	ppop_status_writemon(stack[stackp].remembered_operation, 0);

    } /* end of writemon_unstalled() */

/*
** This routine is called periodically while printing is stalled.
** The parameter indicates how long printing has been stalled.
**
** When it calls commentary(), the first raw field is the number
** of minutes it has been stalled, the second is our "operation"
** parameter and indicates what operation is stalled.
*/
static void writemon_stalled(const struct timeval *time_stalled)
    {
    int minutes;

    DODEBUG_INTERFACE(("writemon_stalled(%d.%06d)", (int)time_stalled->tv_sec, (int)time_stalled->tv_usec));

    memcpy(&stack[stackp].stall_time, time_stalled, sizeof(struct timeval));
    minutes = (int)(time_stalled->tv_sec / 60);

    if(minutes > stack[stackp].stall_time_minutes_announced)
	{
        char num_str[10];
        char temp[40];
        int severity;

	/* Format the seconds and milliseconds stalled as an ASCIIZ string. */
        snprintf(num_str, sizeof(num_str), "%d.%03d", (int)time_stalled->tv_sec, (int)time_stalled->tv_usec / 1000);

	/* Rank the severity of this problem on a scale from 1 to 10. */
	severity = compute_severity(minutes);

	/* Call the commentator with a message depending on the length of the stall. */
        switch(minutes)
            {
            case 1:
            case 2:
                commentary(COM_STALL, "may be stalled", stack[stackp].remembered_operation, num_str, severity);
                break;
            case 3:
            case 4:
                commentary(COM_STALL, "is probably stalled", stack[stackp].remembered_operation, num_str, severity);
                break;
            default:
                snprintf(temp, sizeof(temp), "has been stalled for %d minutes", minutes);
                commentary(COM_STALL, temp, stack[stackp].remembered_operation, num_str, severity);
                break;
            }

	stack[stackp].stall_time_minutes_announced = minutes;

	/* Let the "ppop status" code know. */
	ppop_status_writemon(stack[stackp].remembered_operation, minutes);
    	}

    } /* end of writemon_stalled() */

/*
** Compute the severity of a stall message according to how long the printer
** was stalled.  This formula yields a value of 5 after 15 minutes and 10 after
** 30 minutes.
*/
static int compute_severity(int minutes)
    {
    int severity = (int)(minutes / 3);
    if(severity < 1)
    	severity = 1;
    if(severity > 10)
	severity = 10;
    return severity;
    }

/*==========================================================================
** These routines keep track of how much time the printer has been on-line
** while attempting to print the current page.
==========================================================================*/

/*
** This is called whenever the printer informs use that it has dropt a page
** into the output tray.
*/
void writemon_pagedrop(void)
    {
    DODEBUG_INTERFACE(("writemon_pagedrop()"));
    gu_timeval_zero(&online_accumulator);
    gettimeofday(&online_unaccumulated_start_time, NULL);

    if(online_online)		/* if on-line, the clock is ticking */
    	{			/* print 0 and the 'as of' time */
	char temp[32];
	snprintf(temp, sizeof(temp), "0 %d", (int)online_unaccumulated_start_time.tv_sec);
	ppop_status_pagemon(temp);
    	}
    else			/* if off-line */
    	{			/* just print zero */
	ppop_status_pagemon("0");
    	}
    }

/*
** This function is intended to be called by the feedback reader (in
** pprdrv_feedback.c) every time the printer's on-line/off-line state is known.
** We must determine if this new state is different from the previous state.
*/
void writemon_online(gu_boolean online)
    {
    DODEBUG_INTERFACE(("writemon_online(online=%s)", online ? "TRUE" : "FALSE"));

    /* If this state is different from the previous state, */
    if(online != online_online || !online_online_valid)
	{
	char temp[32];

        if(online)  		/* went from unknown or off-line to on-line */
            {
            gettimeofday(&online_unaccumulated_start_time, NULL);
            snprintf(temp, sizeof(temp), "%d %d", (int)online_accumulator.tv_sec, (int)online_unaccumulated_start_time.tv_sec);
            }
        else			/* went from unknown or on-line to off-line */
            {
	    if(online_online_valid)	/* went from on-line to off-line */
		{
		struct timeval t;
		gettimeofday(&t, NULL);
		gu_timeval_sub(&t, &online_unaccumulated_start_time);
		gu_timeval_add(&online_accumulator, &t);
		}
            snprintf(temp, sizeof(temp), "%d", (int)online_accumulator.tv_sec);
            }

	ppop_status_pagemon(temp);
	online_online_valid = TRUE;
        online_online = online;
        }
    }

/* end of file */

