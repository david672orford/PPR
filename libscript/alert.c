/*
** mouse:~ppr/src/interfaces/alert.c
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
** Last modified 13 January 2005.
*/

/*
** This small program can be used by shell script printer
** interfaces which can't call alert() directly.
*/

#include "config.h"
#include <unistd.h>
#include "gu.h"
#include "global_defines.h"

#include "util_exits.h"

int main(int argc, char *argv[])
	{
	int stamp;

	chdir(HOMEDIR);

	if(argc != 4)
		{
		fprintf(stderr,"%s: wrong number of parameters (3 required, %d received)\n", argv[0], argc - 1);
		return EXIT_SYNTAX;
		}

	if((stamp = gu_torf(argv[2])) == ANSWER_UNKNOWN)
		{
		fprintf(stderr, "%s: second parameter must be TRUE or FALSE\n", argv[0]);
		return EXIT_SYNTAX;
		}

	/* Call the library alert function to post the alert. */
	alert(argv[1], stamp, argv[3]);

	/* We assume it worked, exit. */
	return EXIT_OK;
	} /* end of main() */

/* end of file */
