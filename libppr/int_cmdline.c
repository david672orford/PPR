/*
** mouse:~ppr/src/libppr/int_cmdline.c
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
** Last modified 16 October 2003.
*/

#include "before_system.h"
#include <stdlib.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "libppr_int.h"
#include "interface.h"

struct INT_CMDLINE int_cmdline;

/*
** Notice that the opt_address and opt_options fields
** may be overridden by an environment variable
** whose name begins with "PPR_GS_INTERFACE_HACK_".
*/
void int_cmdline_set(int argc, char *argv[])
	{
	int_cmdline.probe = FALSE;
	if(argc >= 2 && strcmp(argv[1], "--probe") == 0)
		{
		int_cmdline.probe = TRUE;
		argv++;
		argc--;
		}

	if(argc < 3)
		{
		fprintf(stderr, "%s interface: insufficient parameters\n", (argc > 0) ? argv[0] : "???");
		exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
		}

	/* Path to interface */
	int_cmdline.int_name = argv[0];

	/* Name portion of path */
	{
	char *p = strrchr(argv[0], '/');
	if(p)
		int_cmdline.int_basename = p + 1;
	else
		int_cmdline.int_basename = argv[0];
	}

	/* Printer to print to */
	int_cmdline.printer = argv[1];

	/* Address of that printer in correct format for interface */
	if((int_cmdline.address = getenv("PPR_GS_INTERFACE_HACK_ADDRESS")) == (char*)NULL)
		int_cmdline.address = argv[2];

	/* Options in correct format for interface */
	if((int_cmdline.options = getenv("PPR_GS_INTERFACE_HACK_OPTIONS")) == (char*)NULL)
		int_cmdline.options = argc >= 4 ? argv[3] : "";

	/* Jobbreak method being used */
	if(argc >= 5)
		int_cmdline.jobbreak = atoi(argv[4]);
	else
		int_cmdline.jobbreak = JOBBREAK_NONE;

	/* Are we expected to do feedback? */
	if(argc >= 6)
		int_cmdline.feedback = atoi(argv[5]) ? TRUE : FALSE;
	else
		int_cmdline.feedback = TRUE;

	/* What set of character codes has it been claimed we can pass? */
	if(argc >= 7)
		int_cmdline.codes = (enum CODES)atoi(argv[6]);	/* !!! */
	else
		int_cmdline.codes = CODES_UNKNOWN;

	/* jobname */
	if(argc >= 8)
		int_cmdline.jobname = argv[7];
	else
		int_cmdline.jobname = "???";

	/* routing */
	if(argc >= 9)
		int_cmdline.routing = argv[8];
	else
		int_cmdline.routing = "";

	/* forline */
	if(argc >= 10)
		int_cmdline.forline = argv[9];
	else
		int_cmdline.forline = "???";

	/* barbarlang */
	if(argc >= 11)
		int_cmdline.barbarlang = argv[10];
	else
		int_cmdline.barbarlang = "";

	/* title */
	if(argc >= 12)
		int_cmdline.title = argv[11];
	else
		int_cmdline.title = "";

	} /* end of int_cmdline_set() */

/* end of file */

