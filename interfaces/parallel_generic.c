/*
** mouse:~ppr/src/interfaces/parallel_generic.c
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
** use this file as a basis to create one.	Note that the os-specific module 
** is selected by the PARALLEL= line in Makefile.conf.	For example, if it says
** "PARALLEL=linux", then interface_linux.c will be used.
*/

#include "before_system.h"
#include "gu.h"
#include "global_defines.h"
#include "parallel.h"

/*
** This routine is called just after the port has been opened.	It should
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
** guess what they might mean.	That is done in parallel.c.
*/
int parallel_port_status(int fd)
	{
	return 0;
	}

/*
** This routine is called just before closing the parallel port.
** It probably doesn't have to do anything.
*/
void parallel_port_cleanup(int fd)
	{
	}

/* end of file */
