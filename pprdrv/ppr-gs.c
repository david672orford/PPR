/*
** mouse:~ppr/src/pprdrv/ppr-gs.c
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
** Last modified 22 October 2003.
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

/*
** Where do we seek Ghostscript?
**
** We will look for them in what we consider order of decreasing
** deliberateness of their installation.  In other words, PPR-GS is first,
** Ghostscript at the default location when built from source
** (/usr/local/bin) is next, and last is Ghostscript at the system location
** (/usr/bin/gs).
*/
const char *gs_exe_list[] = {
		HOMEDIR"/../ppr-gs/bin/gs",				/* PPR Ghostscript distribution */
		"/usr/local/bin/gs",					/* Installation from Source Tarball */
		"/sw/bin/gs",							/* Fink on MacOS X */
		"/usr/bin/gs",							/* System package per FHS */
		NULL
		};

/* 
** Where do we seek rastertohp, rastertoepson, and other external CUPS drivers?
*/
const char *cups_bin_list[] = {
		HOMEDIR"/../ppr-gs/bin",				/* PPR Ghostscript distribution */
		"/usr/local/lib/cups/filter",			/* Installation from CUPS Source Tarball */
		"/usr/lib/cups/filter",					/* System package per Linux FHS */
		"/usr/libexec/cups/filter",				/* System package BSD */
		NULL
		};

/*
** Where do we seek IJS servers such as hpijs?
*/
const char *ijs_bin_list[] = {
		HOMEDIR"/../ppr-gs/bin",				/* PPR Ghostscript distribution */
		"/usr/local/bin",						/* Installation from Source Tarball */
		"/usr/bin",								/* System package */
		NULL
		};
		
/*===========================================================================
** Here we go.  We must either find the necessary components and exec 
** Ghostscript or print an error message on stderr in a format that pprdrv 
** will recognize and exit with a value of 1.  Error messages begining with 
** either of these two phrases will be recognized as indicative of a RIP 
** invokation error when the exit code is 1:
**
**		Unknown device:
**
**		RIP:
**
===========================================================================*/
int main(int argc, char *argv[])
	{
	const char *gs_exe = NULL;
	char *outputfile = NULL;
	gu_boolean saw_DEVICE = FALSE;
	const char **gs_args;
	int si, di;
	char *p;

	/*
	** First step is to find a copy of Ghostscript which we can use.  We use
	** list specified above.
	*/
	{
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
		fprintf(stderr, "RIP: Can't find Ghostscript!\n");
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
	gs_args = gu_alloc(argc + 10, sizeof(const char *));

	di = 0;
	gs_args[di++] = "gs";

	/* Loop over the command line arguments. */
	for(si=1; si<argc; si++)
		{
		if((p=lmatchp(argv[si], "cups=")))
			{
			int i;
			char exepath[MAX_PPR_PATH];
			gu_boolean found = FALSE;
			for(i=0 ; cups_bin_list[i]; i++)
				{
				ppr_fnamef(exepath, "%s/%s", cups_bin_list[i], p);
				if(access(exepath, X_OK) == 0)
					{
					found = TRUE;
					break;
					}
				}
			if(!found)
				{
				fprintf(stderr, "RIP: can't find %s\n", p);
				return 1;
				}
			gu_asprintf(&outputfile, "-sOutputFile=| %s x x x 1 '' >&3", exepath);
			gs_args[di++] = "-sDEVICE=cups";
			saw_DEVICE = TRUE;
			}

		else if((p=lmatchp(argv[si], "ijs=")))
			{
			char *server, *manufacturer, *model;
			char *temp_ptr;
			char *copy;
			int i;
			char exepath[MAX_PPR_PATH];
			gu_boolean found = FALSE;

			p = copy = gu_strdup(p);

			if(!(server = gu_strsep(&p, ",")) || !(manufacturer = gu_strsep(&p, ",")) || !(model = gu_strsep(&p, ",")))
				{
				fprintf(stderr, "RIP: can't parse ijs=%s\n", p);
				return 1;
				}

			for(i=0 ; ijs_bin_list[i]; i++)
				{
				ppr_fnamef(exepath, "%s/%s", ijs_bin_list[i], server);
				if(access(exepath, R_OK) == 0)
					{
					found = TRUE;
					break;
					}
				}
			if(!found)
				{
				fprintf(stderr, "RIP: can't find %s\n", server);
				return 1;
				}

			gs_args[di++] = "-sDEVICE=ijs";

			gu_asprintf(&temp_ptr, "-sIjsServer=%s", exepath);
			gs_args[di++] = temp_ptr;

			gu_asprintf(&temp_ptr, "-sDeviceManufacturer=%s", manufacturer);
			gs_args[di++] = temp_ptr;
			
			gu_asprintf(&temp_ptr, "-sDeviceModel=%s", model);
			gs_args[di++] = temp_ptr;

			gs_args[di++] = "-dIjsUseOutputFD";

			gu_free(copy);
			
			saw_DEVICE = TRUE;
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
		fprintf(stderr, "RIP: no device specified in RIP options\n");
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
		{
		gs_args[di++] = outputfile;
		}
	else if(access("/dev/fd/3", W_OK) == 0)
		{
		gs_args[di++] = "-sOutputFile=/dev/fd/3";
		}
	else
		{
		gs_args[di++] = "-sOutputFile=| cat - >&3";
		}

	/* PostScript text will be read from stdin. */
	gs_args[di++] = "-";

	gs_args[di++] = NULL;

	/* Replace ourself with Ghostscript. */
	execv(gs_exe, (char**)gs_args);

	/* We reach here only if execv() failed. */
	fprintf(stderr, "%s: can't exec Ghostscript, errno=%d (%s)\n", myname, errno, gu_strerror(errno));
	return 1;
	} /* end of main() */

/* end of file */
