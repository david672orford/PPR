/*
** mouse:~ppr/src/interfaces/parallel_linux.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 10 May 2001.
*/

#include "before_system.h"
#include <signal.h>
#include <sys/ioctl.h>
#include <linux/lp.h>
#ifndef LP_SELECD
#warning "Your linux/lp.h file is buggy, compensating!"
#define LP_POUTPA	0x20
#define LP_PSELECD	0x10
#define LP_PERRORP	0x08
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
** This routine is called just after the port has been opened.  It should
** take interface options, as stored in the supplied structure, and
** use the supplied file descriptor to set up the driver accordingly.
*/
void parallel_port_setup(int fd, const struct OPTIONS *options)
    {
    /*
       Oddly, setting it to 0 is supposed to turn this option on.
       My examination of the Linux 2.1.128 code suggests this is
       wrong.

       The careful feature requires out-of-paper, online, no-error to
       be favourable
       before sending a character.  It is obsolete in Linux 2.2.x.
    */
    /*ioctl(fd, LPCAREFUL, 1);*/

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
** value that apply.
*/
int parallel_port_status(int fd)
    {
    int raw_status = 0, status = 0;

    ioctl(fd, LPGETSTATUS, &raw_status);

    if(!(raw_status & LP_PSELECD))		/* selected input, active high */
    	status |= PARALLEL_PORT_OFFLINE;
    if(raw_status & LP_POUTPA)                  /* out-of-paper input, active high */
    	status |= PARALLEL_PORT_PAPEROUT;
    if(!(raw_status & LP_PERRORP))		/* error input, active low */
    	status |= PARALLEL_PORT_FAULT;

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

