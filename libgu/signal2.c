/*
** mouse:~ppr/src/libppr/signal1.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 18 February 1999.
*/

#include "config.h"
#include <signal.h>
#include "gu.h"
#include "global_defines.h"


/*
** Install a reliable signal handler and ask that interupted
** system calls be restarted automatically.
*/
void (*signal_restarting(int signum, void (*handler)(int)))(int)
	{
	struct sigaction new_handler, old_handler;
	sigemptyset(&new_handler.sa_mask);
	#ifdef SA_RESTART
	new_handler.sa_flags = SA_RESTART;
	#else
	new_handler.sa_flags = 0;
	#endif
	new_handler.sa_handler = handler;
	sigaction(signum, &new_handler, &old_handler);
	return old_handler.sa_handler;
	}

/* end of file */

