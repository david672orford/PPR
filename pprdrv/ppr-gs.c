/*
** mouse:~ppr/src/pprdrv/ppr-gs.c
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
** Last modified 14 August 2002.
*/

/*
** This program is a wrapper around Ghostscript.  The version you see here
** is designed to work with the PPR-GS Ghostscript distribution.  Its purpose
** is to hide details such as the location of Ghostscript, CUPS filters,
** IJS filters, etc.  This allows PPD file authors to write execution
** information which ignores these issues.
*/

#include "before_system.h"
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "gu.h"
#include "global_defines.h"

const char myname[] = "ppr-gs";

int main(int argc, char *argv[])
    {
    const char rip_exe[] = "../ppr-gs/bin/gs";
    char *outputfile = NULL;
    gu_boolean saw_DEVICE = FALSE;
    const char **rip_args;
    int si, di;
    char *p;

    /* Here we build the Ghostscript command line.  The extra space is 
       for "-q -dSAFER -sOutputFile='...' -" and the NULL.
       */
    rip_args = gu_alloc(argc + 5, sizeof(const char *));

    di = 0;
    rip_args[di++] = rip_exe;

    for(si=1; si<argc; si++)
	{
	if((p=lmatchp(argv[si], "cupsfilter=")))
	    {
	    gu_asprintf(&outputfile, "-sOutputFile=| %s x x x 1 '' >&3", p);
	    }
	/* Unrecognized options must be for Ghostscript. */
	else
	    {
	    if(lmatch(argv[si], "-sDEVICE="))
	    	saw_DEVICE=TRUE;
	    rip_args[di++] = argv[si];
	    }
	}

    /* Make sure -sDEVICE= was specified. */
    if(!saw_DEVICE)
	{
	fprintf(stderr, "Unknown device: unspecified\n");
	return 1;
	}

    rip_args[di++] = "-q";
    rip_args[di++] = "-dSAFER";

    if(outputfile)
	rip_args[di++] = outputfile;
    else
	rip_args[di++] = "-sOutputFile=| cat - >&3";

    rip_args[di++] = "-";

    rip_args[di++] = NULL;

    /* We add a directory to the PATH so that Ghostscript -sOutputfile= commands can
       find CUPS and IJS filters.
       */
    {
    char *path_equals;		/* <-- don't free this! */
    gu_asprintf(&path_equals, "PATH=%s/../ppr-gs/bin:%s", HOMEDIR, getenv("PATH"));
    putenv(path_equals);
    }

    /* Replace ourself with Ghostscript. */
    execv(rip_exe, (char**)rip_args);

    /* We reach here only if execv() failed. */
    fprintf(stderr, "%s: can't exec Ghostscript, errno=%d (%s)\n", myname, errno, gu_strerror(errno));
    return 1;
    } /* end of main() */

/* end of file */
