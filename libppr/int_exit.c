/*
** mouse:~ppr/src/libppr/int_exit.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 10 May 2001.
*/

#include "before_system.h"
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "gu.h"
#include "global_defines.h"
#include "libppr_int.h"
#include "interface.h"

/*
** We exit through this function so that if the environment variable
** PPR_GS_INTERFACE_PID was set, we can send SIGUSR1, SIGUSR2
** or SIGINT to the Ghostscript interface script.  This is
** necessary because we cannot rely on Gostscript to return
** sensible exit codes when its output program fails.
**
** This will be removed soon since the gs* interfaces are obsolete.
*/
void int_exit(int exitvalue)
	{
	char *p;

	if((p = getenv("PPR_GS_INTERFACE_PID")))
		{
		long int patron = atol(p);

		switch(exitvalue)
			{
			case EXIT_PRINTED:
				break;
			case EXIT_PRNERR:
			case EXIT_PRNERR_NOT_RESPONDING:
			case EXIT_PRNERR_NO_SUCH_ADDRESS:
			default:
				kill((pid_t)patron, SIGUSR1);
				break;
			case EXIT_PRNERR_NORETRY:
			case EXIT_PRNERR_NORETRY_ACCESS_DENIED:
			case EXIT_PRNERR_NORETRY_BAD_SETTINGS:
				kill((pid_t)patron, SIGUSR2);
				break;
			case EXIT_ENGAGED:
				kill((pid_t)patron, SIGTTIN);
				break;
			}

		/* Allow time for the signal to be caught before
		   Ghostscript and the interface script are
		   distracted by our exit. */
		sleep(1);
		}

	exit(exitvalue);
	} /* end of do_exit() */

/* end of int_exit.c */

