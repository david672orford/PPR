/*
** mouse:~ppr/src/libppr/ppr_gcmd.c
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
** Last modified 14 November 2003.
*/

#include "config.h"
#include <string.h>
#include <ctype.h>
#ifndef HAVE_TERMIOS_H
#ifdef SET_BACKSPACE
#include <termios.h>
#endif
#endif
#include "gu.h"
#include "global_defines.h"

/*! \file
*/


/** read interactive mode command line

This function is used in the interactive utilities to print a prompt and
get a command from the user.  Any trailing spaces are removed from the line.

*/
char *ppr_get_command(const char *prompt, int machine_input)
	{
	static char line[1024];		/* buffer for input line */
	char *result;

	#ifndef HAVE_TERMIOS_H
	#ifdef SET_BACKSPACE
	struct termios term;		/* used to set erase character */
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
		tcgetattr(0, &term);					/* Get terminal settings. */
		saved_erase = term.c_cc[VERASE];		/* Save erase character. */
		term.c_cc[VERASE] = '\b';				/* Set to backspace. */
		tcsetattr(0, TCSANOW, &term);			/* Put terminal settings. */
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
		term.c_cc[VERASE] = saved_erase;		/* Restore old erase character. */
		tcsetattr(0,TCSANOW,&term);				/* Put terminal settings. */
		}
	#endif
	#endif

	if(result)
		{
		int len;
		for(len = strlen(result); len > 0; )
			{
			len--;
			if(isspace(result[len]))
				result[len] = '\0';
			else
				break;
			}
		}

	return result;
	} /* end of ppr_get_command() */

/* end of file */
