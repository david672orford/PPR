/*
** mouse:~ppr/src/libuprint/uprint_lpq.c
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
** Last modified 14 January 2005.
*/

#include "config.h"
#include <stdio.h>
#include "gu.h"
#include "global_defines.h"
#include "uprint.h"
#include "uprint_private.h"

#define ARGS_SIZE 100

/*
** Handle an lpq style queue request.  The file names
** list is filled with a list of job numbers and
** user names.  This function is used by uprint-lpq
** to process local requests and from the new lprsrv in order
** to process requests from across the network.
**
** The term "agent" is from RFC-1179 and should probably
** be "remote_user".  Notice that it is not used.
*/
int uprint_lpq(uid_t uid, gid_t gid, const char agent[], const char queue[], int format, const char *arglist[], gu_boolean remote_too)
	{
	DODEBUG(("uprint_lpq(agent = \"%s\", queue = \"%s\", format = %d, arglist = ?)", agent, queue ? queue : "", format));

	if(queue == (char*)NULL)
		{
		uprint_error_callback("uprint_lpq(): queue is NULL");
		uprint_errno = UPE_NODEST;
		return -1;
		}

	/*
	** PPR spooler:
	** Use ppop.
	*/
	if(printdest_claim_ppr(queue))
		{
		const char *args[ARGS_SIZE + 1];
		int i, x;

		i = 0;
		args[i++] = "ppop";

		if(uprint_arrest_interest_interval)
			{
			args[i++] = "--arrest-interest-interval";
			args[i++] = uprint_arrest_interest_interval;
			}

		if(format == 0)
			args[i++] = "lpq";
		else
			args[i++] = "nhlist";

		args[i++] = queue;

		if(arglist != (const char **)NULL)
			{
			for(x = 0; arglist[x] != (const char *)NULL && x < ARGS_SIZE; x++, i++)
				{
				args[i] = arglist[x];
				}
			}

		args[i] = (const char *)NULL;

		return uprint_run(uid, gid, PPOP_PATH, args);
		}

	/*
	** Thru lpr/lpd protocol over the network:
	*/
	{
	struct REMOTEDEST info;
	if(remote_too && printdest_claim_remote(queue, &info))
		return uprint_lpq_rfc1179(queue, format, arglist, &info);
	}

	/*
	** We will only reach here if none of
	** the spoolers claimed the queue.
	*/
	uprint_errno = UPE_UNDEST;
	return -1;
	} /* end of uprint_lpq() */

/* end of file */
