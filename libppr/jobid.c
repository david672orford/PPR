/*
** mouse:~ppr/src/libppr/jobid.c
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
** Last modified 23 September 2005.
*/

/*! \file */

#include "config.h"
#include <string.h>
#include "gu.h"
#include "global_defines.h"

/** Format the job id in standard format
 *
 * The job format is destname-id.  If the subid is not zero, then
 * the format is destname-id.subid.  A pointer is returned to a buffer
 * belonging to this function.
*/
const char *jobid(const char *destname, int id, int subid)
	{
	static char *storage = NULL;
	static int storage_size = 0;
	int possibly_needed;

	possibly_needed = strlen(destname);
	possibly_needed += 10;		/* "-XXXX.XXX\0" */

	if(possibly_needed > storage_size)
		{
		gu_pool_suspend(TRUE);
		storage = gu_realloc(storage, possibly_needed, sizeof(char));
		storage_size = possibly_needed;
		gu_pool_suspend(FALSE);
		}

	/* Say the destination name and the id number. */
	snprintf(storage, storage_size, "%s-%d", destname, id);

	/* If the subid is not zero, specify it. */
	if(subid > 0)
		gu_snprintfcat(storage, storage_size, ".%d", subid);

	return storage;
	} /* end of jobid() */

/* end of file */
