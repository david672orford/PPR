/*
** mouse:~ppr/src/libppr/destinfo.c
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
** Last modified 17 March 2003.
*/

#include "before_system.h"
#include <stdio.h>
#include <string.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

const char *dest_ppdfile(const char destnode[], const char destname[])
    {
    char fname[MAX_PPR_PATH];
    FILE *f = NULL;
    char *line = NULL;
    int line_space = 128;
    char *ppdfile = NULL;
    char *p;
    char *freeme = NULL;

    /* If the specified destination is an alias, replace destname with the thing
       which the alias points to. */
    ppr_fnamef(fname, "%s/%s", ALIASCONF, destname);
    if((f = fopen(fname, "r")))
	{
	while((line = gu_getline(line, &line_space, f)))
	    {
	    if((p = lmatchp(line, "ForWhat:")))
	    	{
	    	destname = freeme = gu_strdup(p);
	    	break;
	    	}
	    }
	}

    /* If the specified destination (as possibly dealiased above) is a group, take its
       first member.
       */
    ppr_fnamef(fname, "%s/%s", GRCONF, destname);
    if((f = fopen(fname, "r")))
	{
	while((line = gu_getline(line, &line_space, f)))
	    {
	    if((p = lmatchp(line, "Printer:")))
		{
		if(freeme)
		    gu_free(freeme);
	    	destname = freeme = gu_strdup(p);
		break;
	    	}
	    }
	}
	
    /* At this point we should have a printer name.  We will open the 
       configuration file and extract the PPD file name.
       */
    ppr_fnamef(fname, "%s/%s", PRCONF, destname);
    if((f = fopen(fname, "r")))
	{
	while((line = gu_getline(line, &line_space, f)))
	    {
	    if(gu_sscanf(line, "PPDFile: %Z", &ppdfile) == 1)
	    	break;
	    }	
	}

    if(line)
	gu_free(line);

    if(f)
	fclose(f);

    if(freeme)
	gu_free(freeme);

    return ppdfile;
    }

/* end of file */
