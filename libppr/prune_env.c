/*
** mouse:~ppr/src/libppr/prune_env.c
** Copyright 1995--2005, Trinity College Computing Center.
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
** Last modified 19 October 2005.
*/

#include "config.h"
#include <stdlib.h>
#include "gu.h"
#include "global_defines.h"
#include "version.h"

/*
** Set various environment variables to appropriate values for PPR programs, 
** especially daemons.
*/
void set_ppr_env()
	{
	putenv("PPR_VERSION=" SHORT_VERSION);
	putenv("PATH=" SAFE_PATH);
	putenv("IFS= \t\n");
	putenv("SHELL=/bin/sh");
	putenv("HOME=" LIBDIR);
	putenv("XAUTHORITY=" RUNDIR "/Xauthority");
	} /* end of set_ppr_env() */

/*
** Remove unnecessary of misleading variables from the environment.
*/
void prune_env(void)
	{
	#ifdef HAVE_UNSETENV
	unsetenv("TERM");
	unsetenv("TERMINFO");
	unsetenv("USER");
	unsetenv("LOGNAME");
	unsetenv("MAIL");
	unsetenv("MANPATH");
	unsetenv("DISPLAY");
	unsetenv("WINDOWID");
	#else
	putenv("TERM=");
	putenv("TERMINFO=");
	putenv("USER=");
	putenv("LOGNAME=");
	putenv("MAIL=");
	putenv("MANPATH=");
	putenv("DISPLAY=");
	putenv("WINDOWID=");
	#endif
	} /* end of prune_env() */

/* end of file */
