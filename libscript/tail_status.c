/*
** mouse:~ppr/src/misc/tail_status.c
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
** Last modified 17 November 2002.
*/

/*
** This program is used by scripts that want to monitor what PPR is doing.
** It works like the tail(1) command.  That is, it watches special PPR
** log files.  Actually, these files don't grow continually, but the
** PPR library function tail_status() takes care of all of those messy
** details.  C programs that want to do the same thing naturaly call
** tail_status() themselves and don't use this wrapper.
*/

#include "config.h"
#include <stdio.h>
#include "gu.h"
#include "global_defines.h"
#include "version.h"

static gu_boolean print_function(char *p, void *extra)
	{
	if(!p)						/* If this is a keepalive tick, */
		{
		printf("\n");
		}
	else						/* otherwise it must be real data. */
		{
		printf("%s\n", p);
		}
	fflush(stdout);
	return TRUE;
	}

int main(int argc, char *argv[])
	{
	/* We print the PPR version since the script may not be distributed 
	   with PPR and may want to verify that it is listening to a version
	   of PPR with which it is compatible.
	   */
	printf("VERSION %s\n", SHORT_VERSION);

	tail_status(TRUE, TRUE, print_function, 60, (void*)NULL);

	/* I don't think we ever actually reach this line.  Violence (such
	   as SIGTERM or SIGPIPE) is the only incentive to leave off that 
	   we understand.
	   */
	return 0;
	} /* end of main */

/* end of file */

