/*
** mouse:~ppr/src/include/interface.h
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
** Last modified 21 October 2003.
*/

/*
** These values are used in communication between the
** interface and pprdrv and between pprdrv and pprd.
**
** This file defines the exit codes for an printer interface
** program as well as the codes it can expect as its fifth
** parameter.
**
** There is a shell script version of this file in the
** interfaces directory.
**
** There is no reason for the user to change anything
** in this file.
*/

/* Exit values for interfaces and pprdrv: */
#define EXIT_PRINTED 0							/* file was printed normally */
#define EXIT_PRNERR 1							/* printer error occured */
#define EXIT_PRNERR_NORETRY 2					/* printer error with no hope of retry */
#define EXIT_JOBERR 3							/* job is defective */
#define EXIT_SIGNAL 4							/* terminated after catching signal */
#define EXIT_ENGAGED 5							/* printer is otherwise engaged (connection refused) */
#define EXIT_STARVED 6							/* starved for system resources */
#define EXIT_PRNERR_NORETRY_ACCESS_DENIED 7		/* bad password? bad port permissions? */
#define EXIT_PRNERR_NOT_RESPONDING 8			/* just doesn't answer at all (turned off?) */
#define EXIT_PRNERR_NORETRY_BAD_SETTINGS 9		/* interface settings are invalid */
#define EXIT_PRNERR_NO_SUCH_ADDRESS 10			/* address lookup failed, may be transient */
#define EXIT_PRNERR_NORETRY_NO_SUCH_ADDRESS 11	/* address lookup failed, not transient */

/* Tell pprdrv what is the highest code an interface should return. */
#define EXIT_INTMAX EXIT_PRNERR_NORETRY_NO_SUCH_ADDRESS	

/* Exit values for pprdrv only: */
#define EXIT_INCAPABLE 50			/* printer wants (lacks) features or resources */

/* the possible jobbreak methods */
#define JOBBREAK_DEFAULT -1			/* <-- not a real setting, used only in ppad */
#define JOBBREAK_NONE 0				/* unusable */
#define JOBBREAK_SIGNAL 1			/* SIGUSR1 handshake (tricky) */
#define JOBBREAK_CONTROL_D 2			/* simple control-D protocol */
#define JOBBREAK_PJL 3				/* HP's Printer Job Language */
#define JOBBREAK_SIGNAL_PJL 4			/* SIGUSR1 handshake and PJL */
#define JOBBREAK_SAVE_RESTORE 5			/* silly too */
#define JOBBREAK_NEWINTERFACE 6			/* lame */

/* end of file */

