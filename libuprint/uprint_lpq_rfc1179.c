/*
** mouse:~ppr/src/libuprint/uprint_lpq_rfc1179.c
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

#include "before_system.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "uprint.h"
#include "uprint_private.h"

/*
** Handle an lpq style queue request.  The file names
** list is filled with a list of job numbers and
** user names.	The queue has previously been specified
** with uprint_set_dest().	This function can be
** used from uprint-lpq or from an lpd emulator in order
** to process requests from across the network.
*/
int uprint_lpq_rfc1179(const char *queue, int format, const char **arglist, struct REMOTEDEST *scratchpad)
	{
	const char function[] = "uprint_lpq_rfc1179";
	int x, y;
	char temp[1024];
	const char *args[5];

	DODEBUG(("%s(queue = \"%s\", format = %d, arglist = ?)", function, queue != (const char *)NULL ? queue : "", format));

	if(queue == (const char *)NULL)
		{
		uprint_error_callback("%s(): queue is NULL", function);
		uprint_errno = UPE_BADARG;
		return -1;
		}

	if(arglist == (const char **)NULL)
		{
		uprint_error_callback("%s(): arglist is NULL", function);
		uprint_errno = UPE_BADARG;
		return -1;
		}

	if(scratchpad == (struct REMOTEDEST *)NULL)
		{
		uprint_error_callback("%s(): scratchpad is NULL", function);
		uprint_errno = UPE_BADARG;
		return -1;
		}

	/* Check that the information from /etc/ppr/uprint-remote.conf
	   has been filled in. */
	if(scratchpad->node[0] == '\0' || scratchpad->printer[0] == '\0')
		{
		uprint_error_callback("%s(): printdest_claim_rfc1179() must suceed first", function);
		uprint_errno = UPE_BADORDER;
		return -1;
		}

	/* Build the list queue command: */
	snprintf(temp, sizeof(temp), "%c%s", format ? 4 : 3, scratchpad->printer);
	y = strlen(temp);

	for(x = 0; arglist[x] != (const char *)NULL; x++)
		{
		if((y + 2 + strlen(arglist[x])) > sizeof(temp))
			{
			uprint_errno = UPE_BADARG;
			uprint_error_callback("%s(): arglist too long", function);
			return -1;
			}

		temp[y++] = ' ';
		strcpy(temp + y, arglist[x]);
		y += strlen(arglist[x]);
		}

	temp[y++] = '\n';

	args[0] = UPRINT_RFC1179;
	args[1] = "command";
	args[2] = scratchpad->node;
	args[3] = temp;
	args[4] = NULL;

	return uprint_run_rfc1179(UPRINT_RFC1179, args);
	} /* end of uprint_lpq_rfc1179() */

/* end of file */
