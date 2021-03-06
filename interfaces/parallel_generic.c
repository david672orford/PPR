/*
** mouse:~ppr/src/interfaces/parallel_generic.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 27 September 2006.
*/

/*
** Since there are major differences in the way parallel ports are controled
** on various Unix systems, the parallel interface is linked with an
** operating-system-specific module.  That module contains functions 
** which are called from parallel.c to do such things as toggle the reset
** line in the parallel cable and read the state of the printer status
** lines.  This file, parallel_generic.c, is a dummy implementation of
** the operating-system-specific module which contains only stub functions.
** It is used for those operating systems for which an operatining-system-
** specific module has not been written.
**
** If your operating system does not yet have its own module, then you should
** use this file as a basis to create one.  Note that the os-specific module 
** is selected by the PARALLEL= line in Makefile.conf.  For example, if it says
** "PARALLEL=linux", then interface_linux.c will be used.
*/

#include "config.h"
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "libppr_int.h"
#include "interface.h"
#include "parallel.h"

/*
** This routine is called just after the port has been opened.  It should
** take interface options, as stored in the supplied structure, and
** use the supplied file descriptor to set up the driver accordingly.
*/
void parallel_port_setup(int fd, const struct OPTIONS *options)
	{
	}

/*
** This routine is called to reset the printer by the use of
** the reset line in the parallel cable.
*/
void parallel_port_reset(int fd)
	{
	}

/*
** This routine reports on the state of the ONLINE, PAPEROUT
** and FAULT lines in the parallel cable.  It should return
** an integer which is the total of all the PARALLEL_PORT_*
** value that apply.
**
** PARALLEL_PORT_OFFLINE
** PARALLEL_PORT_PAPEROUT
** PARALLEL_PORT_FAULT
** PARALLEL_PORT_BUSY
**
** The raw parallel port signals should be returned.  Don't try
** guess what they might mean.  That is done in parallel.c.
*/
int parallel_port_status(int fd)
	{
	return 0;
	}

/*
 * This is the routine which is called on parallel port errors.  Customized 
 * versions for particular operating systems should attempt to deduce the
 * actual cause of the error from syscall[] and error_number.  The fd 
 * parameter is provided in case it is desirable to perform tests
 * on the printer port in order to determine the cause of the error.
 */
void parallel_port_error(const char syscall[], int fd, int error_number)
	{
	alert(int_cmdline.printer, TRUE, _("Parallel port communication failed during %s(), errno=%d (%s)."), syscall, error_number, gu_strerror(error_number));
	exit(EXIT_PRNERR);
	}

/*
** This routine is called just before closing the parallel port.
** It probably doesn't have to do anything.
*/
void parallel_port_cleanup(int fd)
	{
	}

/*
** This is the routine which implements --probe.  Its exit code is the one
** which the parallel interface program passes to exit().  The meaning of
** the possible return codes is a follows:
**
** EXIT_PRINTED
**		no error
**
** EXIT_PRNERR_NORETRY 
**		parallel port probing not implemented for this OS
**
** EXIT_PRNERR_NORETRY_NO_SUCH_ADDRESS
**		probing not implemented for specified port
**
** EXIT_PRNERR_NO_SUCH_ADDRESS
**		port not found
**
** EXIT_PRNERR
**		other failure	
*/
int parallel_port_probe(const char address[])
	{
	return EXIT_PRNERR_NORETRY;
	}

/* end of file */
