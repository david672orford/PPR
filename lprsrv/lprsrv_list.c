/*
** mouse:~ppr/src/lprsrv/lprsrv_list.c
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
** Last modified 19 February 2003.
*/

#include "config.h"
#include <string.h>
#include <unistd.h>		/* for getuid() */
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "lprsrv.h"
#include "uprint.h"

/*=================================================================
** List the files in the queue
** This function is passed the command character so it can know
** if it should produce a long or short listing.
=================================================================*/
void do_request_lpq(char *command)
	{
	int format;							/* long or short listing */
	char *queue = command + 1;			/* name queue to list */
	#define MAX 100
	const char *list[MAX + 1];

	char *p;
	int i = 0;

	/* Long or short queue format: */
	if(command[0] == 3)
		format = 0;
	else
		format = 1;

	/* Find and terminate queue name: */
	p = queue + strcspn(queue, RFC1179_WHITESPACE);
	*p = '\0';

	/* Find the other parameters and build them
	   into an array of string pointers. */
	p++;
	p += strspn(p, RFC1179_WHITESPACE);
	while(*p && i < MAX)
		{
		list[i++] = p;
		p += strcspn(p, RFC1179_WHITESPACE);
		*p = '\0';
		p++;
		}
	list[i] = (char*)NULL;

	/*
	** Use the UPRINT routine to run an appropriate command.
	** If uprint_lpq() returns -1 then the reason is in uprint_errno.
	** Unless the error is UPE_UNDEST, it will already have called
	** uprint_error_callback().  If uprint_lpq() runs a command that
	** fails, it will return the (positive) exit code of that
	** command.
	*/
	if(uprint_lpq(geteuid(), getegid(), "???", queue, format, list, FALSE) == -1)
		{
		if(uprint_errno == UPE_UNDEST)
			{
			printf(_("The queue \"%s\" does not exist on the print server \"%s\".\n"), queue, this_node());
			}
		else
			{
			printf(_("Could not get a queue listing due to a problem with the print server\n"
				"called \"%s\".  Please ask the print server's\n"
				"system administrator to examine the log file \"%s\"\n"
				"to learn the details.\n"), this_node(), LPRSRV_LOGFILE);
			}
		fflush(stdout);
		}
	} /* end of do_request_lpq() */

