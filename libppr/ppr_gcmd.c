/*
** mouse:~ppr/src/libppr/ppr_gcmd.c
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
** Last modified 16 February 2001.
*/

#include "before_system.h"
#ifndef HAVE_TERMIOS_H
#ifdef SET_BACKSPACE
#include <termios.h>
#endif
#endif
#include "gu.h"
#include "global_defines.h"


/*
** This function is used in the interactive utilities to print a prompt and
** get a command from the user.
*/
char *ppr_get_command(const char *prompt, int machine_input)
    {
    static char line[1024];	/* buffer for input line */
    char *result;

    #ifndef HAVE_TERMIOS_H
    #ifdef SET_BACKSPACE
    struct termios term;	/* used to set erase character */
    #ifdef GNUC_HAPPY
    cc_t saved_erase = 0;
    #else
    cc_t saved_erase;
    #endif
    #endif
    #endif

    #ifndef HAVE_TERMIOS_H
    #ifdef SET_BACKSPACE
    if( ! machine_input )
	{
	tcgetattr(0, &term);			/* Get terminal settings. */
	saved_erase = term.c_cc[VERASE];	/* Save erase character. */
	term.c_cc[VERASE] = '\b';		/* Set to backspace. */
	tcsetattr(0, TCSANOW, &term);		/* Put terminal settings. */
	}
    #endif
    #endif

    if(! machine_input )
    	fputs(prompt, stdout);

    result = fgets(line, sizeof(line), stdin);

    #ifndef HAVE_TERMIOS_H
    #ifdef SET_BACKSPACE
    if( ! machine_input )
	{
	term.c_cc[VERASE] = saved_erase;	/* Restore old erase character. */
	tcsetattr(0,TCSANOW,&term);		/* Put terminal settings. */
	}
    #endif
    #endif

    return result;
    } /* end of ppr_get_command() */

/* end of file */
