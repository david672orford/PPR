/*
** mouse:~ppr/src/libppr/int_copy_job.c
** Copyright 1995--2002, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 19 April 2002.
*/

#include "before_system.h"
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#ifdef USE_SHUTDOWN
#include <sys/socket.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "libppr_int.h"
#include "interface.h"

#if 0
#define DODEBUG(a) int_debug a
#else
#define DODEBUG(a)
#endif

/* This the the size of the read and write buffers.  8192 is bigger than any pipe. */
#define BUFFER_SIZE 8192

/* There is one of these for each data flow direction. */
enum COPYSTATE {COPYSTATE_WRITING, COPYSTATE_READING};

/*
** This function copies data from stdin to the printer (portfd) and from the 
** printer to stdout.
**
** If idle_status_interval is non-zero, then it sends a control-T if the send 
** buffer is empty and nothing has been sent for idle_status_interval seconds.
**
** If a system call fails in an unexpected way, then the function pointed to
** by fatal_prn_err is called.
**
** If snmp_function is not NULL, then it is called every snmp_status_interval
** seconds.  It is passed the pointer snmp_address.
**
** The USE_SHUTDOWN code was an experiment.  HP JetDirect cards don't seem to
** handle shutdown properly, so it was a bust.  However it may still be the
** right thing to do, so it is still here but #ifdefed out.
*/
void int_copy_job(int portfd, int idle_status_interval, void (*fatal_prn_err)(int err), void (*snmp_function)(void * snmp_address), void *snmp_address, int snmp_status_interval)
    {
    char xmit_buffer[BUFFER_SIZE];	/* data going to printer */
    char *xmit_ptr = xmit_buffer;
    int xmit_len = 0;
    char recv_buffer[BUFFER_SIZE]; 	/* data coming from printer */
    char *recv_ptr = xmit_buffer;
    int recv_len = 0;
    #ifdef USE_SHUTDOWN
    gu_boolean recv_zero_read = FALSE;
    #endif
    enum COPYSTATE xmit_state = COPYSTATE_READING;
    enum COPYSTATE recv_state = COPYSTATE_READING;
    fd_set rfds, wfds;
    int last_stdin_read = 1;		/* how many bytes from stdin last time? */
    int selret;
    time_t time_next_control_t = 0;	/* time of next schedualed control-T (if not postponed) */
    time_t time_next_snmp = 0;		/* time of next schedualed SNMP query */
    struct timeval *timeout, timeout_workspace;

    DODEBUG(("int_copy_job(portfd=%d, idle_status_interval=%d)", portfd, idle_status_interval));

    /* Set the printer port to O_NONBLOCK.  This is important because we don't
       want to block if it can't accept BUFFER_SIZE bytes. */
    gu_nonblock(portfd, TRUE);

    /* Initialize these to the current time to avoid premature triggering. */
    if(idle_status_interval > 0)
	time_next_control_t = (time(NULL) + idle_status_interval);
    if(snmp_status_interval > 0)
        time_next_snmp = (time(NULL) + snmp_status_interval);

    /*
    ** Copy stdin to the printer and from the printer to stdout.  Continue
    ** to do so for as long as read() on stdin doesn't return 0 and we
    ** have data in the receive buffer from the printer that we haven't
    ** sent to pprdrv yet.
    */
    while(last_stdin_read || recv_len > 0 
		#ifdef USE_SHUTDOWN
    		|| !recv_zero_read
		#endif
    		)
    	{
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);

	/* Which file descriptor in the path from pprdrv to printer? */
	switch(xmit_state)
	    {
	    case COPYSTATE_READING:
		if(last_stdin_read)			/* if read() on fd 0 hasn't reported zero bytes yet, */
		    {
		    DODEBUG(("wish to read stdin"));
		    FD_SET(0, &rfds);
		    }
		break;
	    case COPYSTATE_WRITING:
		DODEBUG(("wish to write to printer"));
		FD_SET(portfd, &wfds);
		break;
	    }

	/* Which file descriptor in the path from printer to pprdrv? */
	if(int_cmdline.feedback)
	    {
	    switch(recv_state)
		{
		case COPYSTATE_READING:
		    DODEBUG(("wish to read from printer"));
		    FD_SET(portfd, &rfds);
		    break;
		case COPYSTATE_WRITING:
		    DODEBUG(("wish to write to stdout"));
		    FD_SET(1, &wfds);
		    break;
		}
	    }

	/* If we don't have data to write and have not already queued a 
	   control-T, determine how long select() can block without 
	   violating the idle_status_interval setting. */
	if(time_next_control_t > 0 || time_next_snmp > 0)
	    {
	    time_t next_schedualed = time_next_control_t > 0 && (time_next_control_t < time_next_snmp || time_next_snmp <= 0) ? time_next_control_t : time_next_snmp;
	    next_schedualed -= time(NULL);
	    DODEBUG(("remaining time before next schedualed action: %ld", (long)next_schedualed));
	    if(next_schedualed < 0)
		next_schedualed = 0;
	    timeout_workspace.tv_sec = next_schedualed;
	    timeout_workspace.tv_usec = 0;
	    timeout = &timeout_workspace;
	    }
	else
	    {
	    timeout = NULL;
	    }

	/* Wait until the there is date to read or write or
	   the timeout expires. */
	if((selret = select(portfd + 1, &rfds, &wfds, NULL, timeout)) < 0)
	    {
	    DODEBUG(("select() failed, errno=%d (%s)", errno, gu_strerror(errno)));
	    (*fatal_prn_err)(errno);
	    }

	DODEBUG(("select() returned %d", selret));

	/* Paranoid */
	if((FD_ISSET(0, &rfds) && xmit_state != COPYSTATE_READING)
		|| (FD_ISSET(portfd, &wfds) && xmit_state != COPYSTATE_WRITING)
		|| (FD_ISSET(portfd, &rfds) && recv_state != COPYSTATE_READING)
		|| (FD_ISSET(1, &wfds) && recv_state != COPYSTATE_WRITING))
	    {
	    alert(int_cmdline.printer, TRUE, "%s interface: line %d: assertion failed", int_cmdline.int_basename, __LINE__);
	    int_exit(EXIT_PRNERR_NORETRY);
	    }

	/* If select() timed out, then it is either time to put a control-T 
	   in the transmit buffer or to send an SNMP query. */
	if(selret == 0)
	    {
	    time_t time_now;
	    time(&time_now);
	    if(time_next_control_t > 0 && time_now >= time_next_control_t)
		{
		DODEBUG(("time for ^T"));
		if(xmit_state != COPYSTATE_READING)
		    {
		    alert(int_cmdline.printer, TRUE, "%s interface: line %d: assertion failed", int_cmdline.int_basename, __LINE__);
		    int_exit(EXIT_PRNERR_NORETRY);
		    }
		xmit_ptr = "\24";
		xmit_len = 1;
		xmit_state = COPYSTATE_WRITING;
		time_next_control_t = 0;		/* don't schedual another yet */
		}
	    if(time_next_snmp > 0 && time_now >= time_next_snmp)
		{
		(*snmp_function)(snmp_address);
		time_next_snmp = (time_now + snmp_status_interval);
		}
	    continue;
	    }

	if(FD_ISSET(0, &rfds))
	    {
	    DODEBUG(("data available on stdin"));
	    if((xmit_len = last_stdin_read = read(0, xmit_ptr = xmit_buffer, sizeof(xmit_buffer))) < 0)
	    	{
		alert(int_cmdline.printer, TRUE, "%s interface: stdin read() failed, errno=%d (%s)", int_cmdline.int_basename, errno, gu_strerror(errno));
		int_exit(EXIT_PRNERR);
	    	}

	    DODEBUG(("read %d byte%s from stdin", xmit_len, xmit_len != 1 ? "s" : ""));

	    if(xmit_len > 0)
		{
	    	xmit_state = COPYSTATE_WRITING;
	    	time_next_control_t = 0;		/* cancel control-T */
	    	}
	    #ifdef USE_SHUTDOWN
	    else
	   	{
		shutdown(portfd, SHUT_WR);
	   	}
	    #endif
	    }

	else if(FD_ISSET(portfd, &wfds))
	    {
	    int len;
	    DODEBUG(("space available on printer"));
	    if((len = write(portfd, xmit_ptr, xmit_len)) < 0)
	    	{
	    	DODEBUG(("write() failed, errno=%d (%s)", errno, gu_strerror(errno)));
		if(errno == EAGAIN)
		    len = 0;
		else
		    (*fatal_prn_err)(errno);
		}

	    DODEBUG(("wrote %d byte%s to printer", len, len != 1 ? "s" : ""));
	    DODEBUG(("--->\"%.*s\"<---", len, xmit_ptr));

	    xmit_ptr += len;
	    xmit_len -= len;

	    if(xmit_len == 0)
		{
	    	xmit_state = COPYSTATE_READING;

		if(idle_status_interval)
		    time_next_control_t = (time(NULL) + idle_status_interval);
		}
	    }

	if(FD_ISSET(portfd, &rfds))
	    {
	    DODEBUG(("data available on printer"));
	    if((recv_len = read(portfd, recv_ptr = recv_buffer, sizeof(recv_buffer))) < 0)
	    	{
	    	DODEBUG(("read() failed, errno=%d (%s)", errno, gu_strerror(errno)));
		if(errno == EAGAIN)
		    recv_len = 0;
		else
		    (*fatal_prn_err)(errno);
		}

	    DODEBUG(("read %d byte%s from printer", recv_len, recv_len != 1 ? "s" : ""));
	    DODEBUG(("--->\"%.*s\"<---", recv_len, recv_ptr));

	    if(recv_len > 0)
		recv_state = COPYSTATE_WRITING;
	    #ifdef USE_SHUTDOWN
	    else
	    	recv_zero_read = TRUE;
	    #endif
	    }

	else if(FD_ISSET(1, &wfds))
	    {
	    int len;
	    DODEBUG(("space available on stdout"));
	    if((len = write(1, recv_ptr, recv_len)) < 0)
	    	{
		alert(int_cmdline.printer, TRUE, "%s interface: stdout write() failed, errno=%d (%s)", int_cmdline.int_basename, errno, gu_strerror(errno));
		int_exit(EXIT_PRNERR);
	    	}

	    DODEBUG(("wrote %d byte%s to stdout", len, len != 1 ? "s" : ""));

	    recv_ptr += len;
	    recv_len -= len;
	    if(recv_len == 0)
		recv_state = COPYSTATE_READING;
	    }

	/* Paranoid */
	if(xmit_len < 0 || recv_len < 0)
	    {
	    alert(int_cmdline.printer, TRUE, "%s interface: line %d: assertion failed", int_cmdline.int_basename, __LINE__);
	    int_exit(EXIT_PRNERR_NORETRY);
	    }
    	}

    } /* end of int_copy_job() */

/* end of file */

