/*
** mouse:~ppr/src/libppr/prune_env.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 25 August 1999.
*/

#include "before_system.h"
#include <stdlib.h>
#include "gu.h"
#include "global_defines.h"

#include "version.h"

/*
** Set various environment variables to appropriate values
** for PPR programs, especially daemons.
*/
void set_ppr_env(void)
    {
    #ifdef HAVE_PUTENV
    putenv("PATH=" SAFE_PATH);
    putenv("IFS= \t");
    putenv("SHELL=/bin/sh");
    putenv("HOME=" HOMEDIR);
    putenv("PPR_VERSION=" SHORT_VERSION);
    putenv("XAUTHORITY=" RUNDIR "/Xauthority");
    #endif
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
    #elif defined(HAVE_PUTENV)
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
