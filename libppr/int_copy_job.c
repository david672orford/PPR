/*
** mouse:~ppr/src/libppr/int_copy_job.c
** Copyright 1995--2006, Trinity College Computing Center.
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
** Last modified 16 February 2006.
*/

/*! \file
	\brief PPR interface copy routine
*/

#include "config.h"
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#ifdef USE_SHUTDOWN
#include <sys/socket.h>
#endif
#ifdef INTERNATIONAL
#include <libintl.h>
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

/* This the the size of the read and write buffers.	 8192 is bigger than any pipe. */
#define BUFFER_SIZE 8192

/* There is one of these for each data flow direction. */
enum COPYSTATE {COPYSTATE_WRITING, COPYSTATE_READING};

/** bidirectional copy routine for PPR printer interface programs
*
* This function copies data from stdin to the printer (portfd) and from the 
* printer to stdout.  It uses select() to allow it to copy in both directions
* simultaniously.
*
* If the argument idle_status_interval is non-zero, then int_copy_job() will 
* send a control-T to the printer if the send buffer is empty and nothing has
* been sent for idle_status_interval seconds or more.
*
* If a system call fails in an unexpected way, then the function pointed to
* by fatal_prn_err is called.
*
* If the argument send_eof is not a NULL pointer, then the function it points 
* to is called with the printer file descriptor as its lone argument once the
* last data block has been written.
*
* If the argument status_function is not a NULL pointer, then the function it
* points to is called every status_interval seconds.  It is passed the 
* pointer status_address.
*
* Peter Benie <Peter.Benie@mvhi.com> has provided valuable advice concerning the
* use of select() and non-blocking file descriptors.  He says that write()
* and possible even read() can fail with errno set to EAGAIN even if select()
* has stated that the file descriptor is open for writing.  In a private e-mail 
* to PPR's author, he cited three of the possible reasons:
*
* >a) select can't tell how big the next write is going to be
* >
* >  With some file types (eg. pipes) select will mark the fd as ready
* >  if one byte could be written, however, write(2) guarantees that
* >  small writes ( <= PIPE_BUF, typically 512 bytes) are written
* >  atomically, so write has to do the _entire_ write or return EAGAIN.
* >
* >  On Linux, this doesn't result in a loop because the fd is then marked
* >  as not-ready until the state of the pipe changes for some other
* >  reason, such as more space becoming available. I don't know how
* >  other Unix systems handle this condition.
* >
* >b) select may not be able to tell if the device is ready
* >
* >  It may not be possible to determine whether a device is ready
* >  without writing to it. For such devices, select will _always_ return
* >  ready at least once when the device isn't ready. 
* >
* >c) select(2) and write(2) are separate system calls and are therefore not
* >  atomic; the condition of the device may change between the two system
* >  calls
* >
* >  Suppose that several devices share a common buffer area for writes.
* >  When that buffer has space available, all devices can legitimately
* >  return ready from select. By the time that write is called, that
* >  buffer space may no longer be available. 
* >
* >  I doubt you'll ever see this condition from a printer, but you may
* >  see it with network I/O under very high load.
*
* Most man pages for select(2) do a pretty poor job of describing its 
* behavior and say almost nothing about proper use.  The vagueness of the 
* origional 4.2BSD man page is probably why various implementations of 
* select() display subtle differences in behavior.  Some of these differences
* are described in the Linux select(2) man page and by W. Richard Stevens in
* _Advanced_Programming_in_the_Unix Environment_ (ISBN 0-201-56317-7) pages 
* 399-400.  More information can be found in Linux's select_tut(2) manpage.
*/
void int_copy_job(int portfd,
		int idle_status_interval,
		void (*prn_err)(const char syscall[], int fd, int err),
		void (*send_eoj_funct)(int fd), 
		void (*status_function)(void * status_address), void *status_address, int status_interval,
		const char *init_string
		)
	{
	char xmit_buffer[BUFFER_SIZE];		/* data going to printer */
	char *xmit_ptr = xmit_buffer;
	int xmit_len = 0;
	char recv_buffer[BUFFER_SIZE];		/* data coming from printer */
	char *recv_ptr = xmit_buffer;
	int recv_len = 0;
	gu_boolean recv_eoj = FALSE;		/* Has the printer closed its end? */
	enum COPYSTATE xmit_state = COPYSTATE_READING;
	enum COPYSTATE recv_state = COPYSTATE_READING;
	fd_set rfds, wfds;
	int last_stdin_read = 1;			/* how many bytes from stdin last time? */
	int selret;
	time_t time_next_control_t = 0;		/* time of next schedualed control-T (if not postponed) */
	time_t time_next_status = 0;		/* time of next schedualed status function call */
	struct timeval *timeout, timeout_workspace;
	int select_write_wrong = 0;			/* how many times in a row did select() mislead us about write() readiness? */
	int select_read_wrong = 0;			/* same for read() readiness */

	DODEBUG(("int_copy_job(portfd=%d, idle_status_interval=%d)", portfd, idle_status_interval));

	/*
	** If the interface's feedback option is true, set the printer port to
	** O_NONBLOCK.  This is important because we don't want to block if it
	** can't yet accept BUFFER_SIZE bytes.
	**
	** We could set stdin and stdout to O_NONBLOCK too, but they are much less
	** likely to block for an appreciatable period of time and we aren't
	** sure if the code that prints %%[ ... ]%% messages can deal with a
	** non-blocking stdout.
	*/
	gu_nonblock(portfd, int_cmdline.feedback);

	/*
	** Initialize these timers to their first-fire times.  If we didn't, they
	** would fire as soon as we enter the loop.
	*/
	if(idle_status_interval > 0)
		time_next_control_t = (time(NULL) + idle_status_interval);
	if(status_interval > 0)
		time_next_status = (time(NULL) + status_interval);

	/*
	 * If there is an initialization string, copy it into the transmit buffer.
	 */
	if(init_string)
		{
		xmit_len = last_stdin_read = (unsigned char)init_string[0];
		memcpy(xmit_buffer, init_string+1, xmit_len);
		xmit_ptr = xmit_buffer;
		xmit_state = COPYSTATE_WRITING;
		}
	
	/*
	** Copy stdin to the printer and from the printer to stdout.  Continue
	** to do so for as long as read() on stdin doesn't return 0 and we
	** have data in the receive buffer from the printer that we haven't
	** sent to pprdrv yet.
	**
	** If send_eoj_funct is defined, then also wait for a definite end-
	** of-job indication from the printer.
	*/
	while(last_stdin_read || recv_len > 0 
				|| (send_eoj_funct && !recv_eoj)
				)
		{
		if(select_write_wrong > 10 || select_read_wrong > 10)
			{
			alert(int_cmdline.printer, TRUE,
					_("Device driver for printer port \"%s\" is defective.  It may\n"
					"operate with reduced function if you turn off two-way operation with the\n"
				    "command \"ppad feedback %s false\".\n"),
				int_cmdline.address,
				int_cmdline.printer
				);
			exit(EXIT_PRNERR_NORETRY);
			}
			
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);

		/* Which file descriptor in the path from pprdrv to printer? */
		switch(xmit_state)
			{
			case COPYSTATE_READING:
				if(last_stdin_read)						/* if read() on fd 0 hasn't reported zero bytes yet, */
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
		if(time_next_control_t > 0 || time_next_status > 0)
			{
			time_t next_schedualed = time_next_control_t > 0 && (time_next_control_t < time_next_status || time_next_status <= 0) ? time_next_control_t : time_next_status;
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
			if(errno != EINTR)
				(*prn_err)("select", portfd, errno);
			continue;		/* if prn_err() didn't abort, restart loop */
			}

		DODEBUG(("select() returned %d", selret));

		/* Paranoid */
		if((FD_ISSET(0, &rfds) && xmit_state != COPYSTATE_READING)
				|| (FD_ISSET(portfd, &wfds) && xmit_state != COPYSTATE_WRITING)
				|| (FD_ISSET(portfd, &rfds) && recv_state != COPYSTATE_READING)
				|| (FD_ISSET(1, &wfds) && recv_state != COPYSTATE_WRITING))
			{
			alert(int_cmdline.printer, TRUE, "%s interface: line %d: assertion failed", int_cmdline.int_basename, __LINE__);
			exit(EXIT_PRNERR_NORETRY);
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
					exit(EXIT_PRNERR_NORETRY);
					}
				xmit_ptr = "\24";
				xmit_len = 1;
				xmit_state = COPYSTATE_WRITING;
				time_next_control_t = 0;				/* don't schedual another yet */
				}
			if(time_next_status > 0 && time_now >= time_next_status)
				{
				(*status_function)(status_address);
				time_next_status = (time_now + status_interval);
				}
			continue;
			}

		/* If pprdrv has sent us data, */
		if(FD_ISSET(0, &rfds))
			{
			DODEBUG(("data available on stdin"));
			while((xmit_len = last_stdin_read = read(0, xmit_ptr = xmit_buffer, sizeof(xmit_buffer))) < 0)
				{
				DODEBUG(("read() set errno to %d (%s)", errno, strerror(errno)));
				if(errno == EINTR)
					continue;
				if(errno != EAGAIN)		/* EAGAIN may be possible under wacko circumstances */
					{					/* this is ok, last_stdin_read will keep the loop alive */
					alert(int_cmdline.printer, TRUE, "%s interface: stdin read() failed, errno=%d (%s)", int_cmdline.int_basename, errno, gu_strerror(errno));
					exit(EXIT_PRNERR);
					}
				xmit_len = 0;
				break;
				}

			DODEBUG(("read %d byte%s from stdin", xmit_len, xmit_len != 1 ? "s" : ""));

			if(xmit_len > 0)						/* if we got something, */
				{									/* switch to write-to-printer mode */
				xmit_state = COPYSTATE_WRITING;
				time_next_control_t = 0;			/* cancel control-T */
				}
			else if(send_eoj_funct)
				{
				(*send_eoj_funct)(portfd);
				}
			}

		/* If the printer is ready to receive the data from pprdrv which
		 * we already have buffered, */
		else if(FD_ISSET(portfd, &wfds))
			{
			int len;
			DODEBUG(("space available on printer"));
			while((len = write(portfd, xmit_ptr, xmit_len)) < 0)
				{
				DODEBUG(("write() failed, errno=%d (%s)", errno, gu_strerror(errno)));
				if(errno == EINTR)
					continue;
				if(errno == EAGAIN)
					select_write_wrong++;	/* demerit for being wrong */
				else
					(*prn_err)("write", portfd, errno);
				len = 0;
				break;
				}

			if(len > 0)
				select_write_wrong = 0;		/* remember, this is consecutive wrongness */
			
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

		/* If we have received "feedback" data from the printer, */
		if(FD_ISSET(portfd, &rfds))
			{
			DODEBUG(("data available on printer"));
			while((recv_len = read(portfd, recv_ptr = recv_buffer, sizeof(recv_buffer))) < 0)
				{
				if(errno == EINTR)
					continue;
				DODEBUG(("read() failed, errno=%d (%s)", errno, gu_strerror(errno)));
				if(errno == EAGAIN)
					select_read_wrong++;	/* demerit for being wrong */
				else
					(*prn_err)("read", portfd, errno);
				recv_len = 0;
				break;
				}

			DODEBUG(("read %d byte%s from printer", recv_len, recv_len != 1 ? "s" : ""));
			DODEBUG(("--->\"%.*s\"<---", recv_len, recv_ptr));

			if(recv_len > 0)
				{
				recv_state = COPYSTATE_WRITING;
				select_read_wrong = 0;		/* string of wrongness broken */
				}
			else
				{
				recv_eoj = TRUE;
				}
			}

		/* If the pipe to pprdrv can accept "feedback" data which we
		 * have buffered, */
		else if(FD_ISSET(1, &wfds))
			{
			int len;
			DODEBUG(("space available on stdout"));
			while((len = write(1, recv_ptr, recv_len)) < 0)
				{
				if(errno == EINTR)
					continue;
				/* EAGAIN if available space is less than PIPE_BUF bytes, */
				if(errno != EAGAIN)
					{
					alert(int_cmdline.printer, TRUE, "%s interface: stdout write() failed, errno=%d (%s)", int_cmdline.int_basename, errno, gu_strerror(errno));
					exit(EXIT_PRNERR);
					}
				len = 0;
				break;
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
			exit(EXIT_PRNERR_NORETRY);
			}
		}

	} /* int_copy_job() */

/* end of file */

