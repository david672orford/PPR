/*
** mouse:~ppr/src/include/interface.h
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
** Last modified 11 May 2001.
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
#define EXIT_PRINTED 0				/* file was printed normally */
#define EXIT_PRNERR 1				/* printer error occured */
#define EXIT_PRNERR_NORETRY 2			/* printer error with no hope of retry */
#define EXIT_JOBERR 3				/* job is defective */
#define EXIT_SIGNAL 4				/* terminated after catching signal */
#define EXIT_ENGAGED 5				/* printer is otherwise engaged (connection refused) */
#define EXIT_STARVED 6				/* starved for system resources */
#define EXIT_PRNERR_NORETRY_ACCESS_DENIED 7	/* bad password? bad port permissions? */
#define EXIT_PRNERR_NOT_RESPONDING 8		/* just doesn't answer at all (turned off?) */
#define EXIT_PRNERR_NORETRY_BAD_SETTINGS 9	/* interface settings are invalid */
#define EXIT_PRNERR_NO_SUCH_ADDRESS 10		/* address lookup failed, may be transient */
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

