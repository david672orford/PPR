/*
** mouse:~ppr/src/libuprint/claim_lpr.c
** Copyright 1995--2002, Trinity College Computing Center.
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
** Last modified 1 November 2002.
*/

#include "before_system.h"
#include <string.h>
#include <ctype.h>
#include "gu.h"
#include "global_defines.h"
#include "uprint.h"
#include "uprint_private.h"

/*
** Return TRUE if the destname is the name
** of a valid LPR destination.
**
** In order to acomplish this, we search
** /etc/printcap.
*/
int printdest_claim_lpr(const char *destname)
    {
    if(uprint_lpr_installed())
	{
	FILE *f;

	if((f = fopen(uprint_lpr_printcap(), "r")) != (FILE*)NULL)
	    {
	    char *line;
	    int line_len = 80;
	    char *p;

	    while((line = gu_getline(line, &line_len, f)))
		{
		if(!line[0])
		    continue;

		/* Find the first colon.  If there is none, skip this line. */
		if((p = strchr(line, ':')) == (char*)NULL)
		    continue;

		*p = '\0';	/* truncate line at first colon */

		/* Iterate through a |-separated listed of queue names and aliases. */
		for(p = line; (p = strtok(p, "|")); p = (char*)NULL)
		    {
		    if(strcmp(p, destname) == 0)
			{
			gu_free(line);
			fclose(f);
			return TRUE;
			}
		    }
		}

	    fclose(f);
	    }
	}

    return FALSE;
    } /* end of printdest_claim_lpr() */

/* end of file */
