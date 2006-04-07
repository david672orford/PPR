/*
** mouse:~ppr/src/libppr/protected.c
** Copyright 1995--2006, Trinity College Computing Center.
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
** Last modified 7 April 2006.
*/

/*
** This module determines if print destinations are "protected".
** A protected destination is one for which a per page charge
** is defined, even if that charge is 0.00 monetary units.
*/

#include "config.h"
#include <sys/stat.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"

/*
** Return true if we must verify that the user is in the users' database.
*/
gu_boolean destination_protected(const char destname[])
	{
	char fname[MAX_PPR_PATH];
	struct stat statbuf;

	/* A group? */
	ppr_fnamef(fname, "%s/%s", GRCONF, destname);
	if(stat(fname, &statbuf) == 0)				/* if file found, */
		{
		struct GROUP_SPOOL_STATE gstate;
		group_spool_state_load(&gstate, destname);
		return gstate.protected;
		}

	/* A printer? */
	ppr_fnamef(fname, "%s/%s", PRCONF, destname);
	if( stat(fname, &statbuf) == 0 )		/* if it exists, */
		{
		struct PRINTER_SPOOL_STATE pstate;
		printer_spool_state_load(&pstate, destname);
		return pstate.protected;
		}

	return FALSE;	/* if it doen't exist, sombody else will notice */
	} /* end of destination_protected() */

/* end of file */
