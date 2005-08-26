/*
** mouse:~ppr/src/libppr/protected.c
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
** Last modified 24 August 2005.
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

/*
** Return true if we must verify that the user is in the users database.
*/
gu_boolean destination_protected(const char destname[])
	{
	char fname[MAX_PPR_PATH];
	struct stat statbuf;

	/* A group? */
	ppr_fnamef(fname, "%s/%s", GRCONF, destname);
	if(stat(fname, &statbuf) == 0)				/* if file found, */
		{
		if(statbuf.st_mode & S_IXOTH)			/* if ``other'' execute bit set, */
			return TRUE;						/* it is protected */
		}

	/* A printer? */
	else
		{
		ppr_fnamef(fname, "%s/%s", PRCONF, destname);
		if( stat(fname, &statbuf) == 0 )		/* if it exists, */
			{									/* and other execute set, */
			if(statbuf.st_mode & S_IXOTH)
				return TRUE;					/* it is protected */
			}
		}

	return FALSE;	/* if it doen't exist, sombody else will notice */
	} /* end of destination_protected() */

/* end of file */
