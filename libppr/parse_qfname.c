/*
** mouse:~ppr/src/libppr/parse_qfname.c
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
** Last modified 14 January 2005.
*/

#include "config.h"
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"

/*
** This function parses a PPR queue file name and extracts the components of
** the jobid.  This function would be much simpler with regular expressions.
** because it is so complicated and touchy, both pprd and pprdrv use it.
** Note that ppop doesn't use this function since it must support a destination
** node, a missing subid, and partial (wildcard) jobid specifications.
*/
int parse_qfname(char *buffer, const char **destname, short int *id, short int *subid)
	{
	char *ptr;

	*destname = buffer;
	if(!(ptr = strrchr(buffer, '-')))
		return -1;
	*ptr = '\0';
	ptr++;

	/* Scan for id number, and subid number. */
	if(gu_sscanf(ptr, "%d.%d", id, subid) != 2
			|| *id < 0 || *subid < 0)
		return -1;

	return 0;
	}

/* end of file */
