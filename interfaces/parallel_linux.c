/*
** mouse:~ppr/src/interfaces/parallel_linux.c
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
** Last modified 10 January 2003.
*/

/*
** This is parallel_generic.c with the functions filled in with
** Linux-specific code.	 The necessary information was found
** in the lp(4) man page.
*/

#include "before_system.h"
#include <signal.h>
#include <sys/ioctl.h>
#include <linux/lp.h>
#include <unistd.h>
#ifndef LP_PSELECD
#warning "Your linux/lp.h file is buggy, compensating!"
#define LP_PERRORP		0x08
#define LP_PSELECD		0x10
#define LP_POUTPA		0x20
#define LP_PBUSY		0x80
#endif
#include "gu.h"
#include "global_defines.h"
#include "interface.h"
#include "parallel.h"

/*
** These are used to implement the
** "reset_on_cancel=" option.
*/
static int printer_fd;
static void sigterm_handler(int sig)
	{
	parallel_port_reset(printer_fd);
	_exit(EXIT_SIGNAL);
	}

/*
** This routine is called just after the port has been opened.	It should
** take interface options, as stored in the supplied structure, and
** use the supplied file descriptor to set up the driver accordingly.
*/
void parallel_port_setup(int fd, const struct OPTIONS *options)
	{
	if(options->reset_on_cancel)
		{
		printer_fd = fd;
		signal_interupting(SIGTERM, sigterm_handler);
		}

	}

/*
** This routine is called to reset the printer by the use of
** the reset line in the parallel cable.
*/
void parallel_port_reset(int fd)
	{
	ioctl(fd, LPRESET);
	}

/*
** This routine reports on the state of the ONLINE, PAPEROUT
** and FAULT lines in the parallel cable.  It should return
** an integer which is the total of all the PARALLEL_PORT_*
** value that apply:
**
** PARALLEL_PORT_OFFLINE
** PARALLEL_PORT_PAPEROUT
** PARALLEL_PORT_FAULT
** PARALLEL_PORT_BUSY
**
** The raw parallel port signals should be returned.  Don't try
** guess what they might mean.	That is done in parallel.c.
*/
int parallel_port_status(int fd)
	{
	int raw_status = 0, status = 0;

	ioctl(fd, LPGETSTATUS, &raw_status);

	if(!(raw_status & LP_PSELECD))				/* selected input, active high */
		status |= PARALLEL_PORT_OFFLINE;
	if(raw_status & LP_POUTPA)					/* out-of-paper input, active high */
		status |= PARALLEL_PORT_PAPEROUT;
	if(!(raw_status & LP_PERRORP))				/* error input, active low */
		status |= PARALLEL_PORT_FAULT;
	if(!(raw_status & LP_PBUSY))				/* helps to detect turned off printers */
		status |= PARALLEL_PORT_BUSY;

	return status;
	}

/*
** This routine is called just before closing the parallel port.
** It probably doesn't have to do anything.
*/
void parallel_port_cleanup(int fd)
	{
	/* Cancel the reset printer on cancel handler. */
	signal_interupting(SIGTERM, SIG_DFL);
	}

/* end of file */
