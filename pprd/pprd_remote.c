/*
** mouse:~ppr/src/pprd/pprd_remote.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 18 October 2000.
*/

/*
** This module contains functions for dispatching jobs to remote PPR spoolers.
*/

#include "before_system.h"

#include "gu.h"
#include "global_defines.h"

#include "global_structs.h"
#include "pprd.h"
#include "./pprd.auto_h"

void remote_new_job(struct QEntry *qentry)
    {
    FUNCTION4DEBUG("remote_new_job")
    DODEBUG_REMOTE(("%s()", function));

    }

gu_boolean remote_child_hook(pid_t pid, int wstat)
    {
    FUNCTION4DEBUG("remote_child_hook")
    DODEBUG_REMOTE(("%s()", function));

    return FALSE;
    }

void remote_tick(void)
    {
    FUNCTION4DEBUG("remote_tick")
    DODEBUG_TICK(("%s()", function));

    }

/* end of file */

