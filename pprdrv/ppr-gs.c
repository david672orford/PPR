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
** Last modified 15 November 2002.
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
    const char *gs_exe = NULL;
    char *outputfile = NULL;
    gu_boolean saw_DEVICE = FALSE;
    char *cupsfilter = NULL;
    char *ijsserver = NULL;
    const char **gs_args;
    int si, di;
    char *p;

    /*
    ** First step is to find a copy of Ghostscript which we can use.  Using
    ** the list supplied above, we will look for them in what we consider
    ** order of decreasing deliberateness of their installation.  In other
    ** words, PPR-GS is first, Ghostscript at the default location when
    ** built from source (/usr/local/bin) is next, and last is Ghostscript
    ** at the system location (/usr/bin/gs).
    */
    {
    const char *gs_exe_list[] = {HOMEDIR"/../ppr-gs/bin/gs", "/usr/local/bin/gs", "/usr/bin/gs", NULL};
    int i;
    for(i=0 ; gs_exe_list[i]; i++)
	{
	if(access(gs_exe_list[i], X_OK) == 0)
	    {
	    gs_exe = gs_exe_list[i];
	    break;
	    }
	}
    if(!gs_exe)
	{
	fprintf(stderr, "Can't find Ghostscript!\n");
	return 1;
	}
    }

    /*
    ** Here we start to build the Ghostscript command line with the options
    ** supplied in the printer configuration or PPD file (and which were
    ** passed to us on our command line.)  The extra space allocated
    ** in the array is for "-q -dSAFER -sOutputFile='...' -" and the NULL
    ** which terminates the array.  These will be added later.
    **
    ** Note that there are some option that we intercept and don't pass
    ** thru to Ghostscript.  These are in name=value form.  At the moment
    ** these all specify output post-processing of some kind.
    */
    gs_args = gu_alloc(argc + 5, sizeof(const char *));

    di = 0;
    gs_args[di++] = "gs";

    for(si=1; si<argc; si++)
	{
	if((p=lmatchp(argv[si], "cupsfilter=")))
	    {
	    cupsfilter = gu_strdup(p);
	    gu_asprintf(&outputfile, "-sOutputFile=| %s x x x 1 '' >&3", cupsfilter);
	    }

	else if((p=lmatchp(argv[si], "ijsserver=")))
	    {
	    ijsserver = gu_strdup(p);
	    /* something missing here */
	    }

	/* Unrecognized options must be for Ghostscript. */
	else
	    {
	    if(lmatch(argv[si], "-sDEVICE="))
	    	saw_DEVICE=TRUE;
	    gs_args[di++] = argv[si];
	    }
	}

    /*
    ** Make sure -sDEVICE= was specified somewhere in that command line.  If
    ** it wasn't, we print a message that looks like the message that
    ** Ghostscript prints if one asks for a device that doesn't exist.
    */
    if(!saw_DEVICE)
	{
	fprintf(stderr, "Unknown device: unspecified\n");
	return 1;
	}

    /*
    ** OK, here come the boilerplate options.  Notice that we allow
    ** an -sOutputFile= option from above the override the boilerplate
    ** one here.
    */
    gs_args[di++] = "-q";
    gs_args[di++] = "-dSAFER";

    if(outputfile)
	gs_args[di++] = outputfile;
    else
	gs_args[di++] = "-sOutputFile=| cat - >&3";

    gs_args[di++] = "-";

    gs_args[di++] = NULL;

    /*
    ** If we will be piping the output through a CUPS filter, add the CUPS
    ** filter directory at the head of the PATH.  The search list starts
    ** with the PPR-GS directory where we may find a private copy of the
    ** CUPS filters, procedes to /usr/local, and ends in /usr.
    */
    if(cupsfilter)
	{
	const char *cups_bin_list[] = {HOMEDIR"/../ppr-gs/bin", "/usr/local/lib/cups/filter", "/usr/lib/cups/filter", NULL};
	int i;
	char temp[MAX_PPR_PATH];
	gu_boolean found = FALSE;
	for(i=0 ; cups_bin_list[i]; i++)
	    {
	    ppr_fnamef(temp, "%s/%s", cups_bin_list[i], cupsfilter);
	    if(access(temp, X_OK) == 0)
		{
		char *path_equals;		/* <-- don't free this!  putenv() keeps a reference. */
		gu_asprintf(&path_equals, "PATH=%s:%s", cups_bin_list[i], getenv("PATH"));
		putenv(path_equals);
		found = TRUE;
		break;
		}
	    }
	if(!found)
	    {
	    fprintf(stderr, "Unknown device: cups (can't find %s)\n", cupsfilter);
	    }
	}

    /*
    ** If we will be using a driver that requires an IJS server, find the 
    ** external server and add its directory at the head of PATH.
    */
    if(ijsserver)
	{
	const char *ijs_bin_list[] = {HOMEDIR"/../ppr-gs/bin", "/usr/local/bin", "/usr/bin", NULL};
	int i;
	char temp[MAX_PPR_PATH];
	gu_boolean found = FALSE;
	for(i=0 ; ijs_bin_list[i]; i++)
	    {
	    ppr_fnamef(temp, "%s/%s", ijs_bin_list[i], ijsserver);
	    if(access(temp, R_OK) == 0)
		{
		char *path_equals;		/* <-- don't free this!  putenv() keeps a reference. */
		gu_asprintf(&path_equals, "PATH=%s:%s", ijs_bin_list[i], getenv("PATH"));
		putenv(path_equals);
		found = TRUE;
		break;
		}
	    }
	if(!found)
	    {
	    fprintf(stderr, "Unknown device: ijs (can't find %s)\n", ijsserver);
	    }
	}

    /* Replace ourself with Ghostscript. */
    execv(gs_exe, (char**)gs_args);

    /* We reach here only if execv() failed. */
    fprintf(stderr, "%s: can't exec Ghostscript, errno=%d (%s)\n", myname, errno, gu_strerror(errno));
    return 1;
    } /* end of main() */

/* end of file */
