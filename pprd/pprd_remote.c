/*
** mouse:~ppr/src/pprd/pprd_remote.c
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
** Last modified 6 May 2003.
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

/*
** Spawn a process to transmit the job.
*/
static void remote_spawn(struct QEntry *qentry)
	{
    
	}

/*
** This is called whenever a remote job enters the queue, either as a new
** job or as one from a previous life of pprd.
*/
void remote_job(struct QEntry *qentry)
	{
	FUNCTION4DEBUG("remote_new_job")
	DODEBUG_REMOTE(("%s()", function));

    remote_spawn(struct QEntry *qentry);
	}

/*
** This is called from reapchild().  It should return TRUE if the pid is that
** of a pprd_xmit process.
*/
gu_boolean remote_child_hook(pid_t pid, int wstat)
	{
	FUNCTION4DEBUG("remote_child_hook")
	DODEBUG_REMOTE(("%s()", function));

	return FALSE;
	}

/*
** This is our chance to retry transmission to destination nodes.
*/
void remote_tick(void)
	{
	FUNCTION4DEBUG("remote_tick")
	DODEBUG_TICK(("%s()", function));

	}

/* end of file */

