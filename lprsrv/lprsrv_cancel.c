/*
** mouse:~ppr/src/lprsrv/lprsrv_cancel.c
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

/*
** Handler for the ^E command.  It does some preliminary parsing
** and then passes the work on to a spooler specific function
** defined above.
*/
void do_request_lprm(char *command, const char fromhost[], const struct ACCESS_INFO *access_info)
	{
	const char *queue;			/* queue to delete the jobs from */
	const char *remote_user;	/* user requesting the deletion */
	uid_t uid_to_use;
	gid_t gid_to_use;
	const char *proxy_class = (const char *)NULL;
	#define MAX 100
	const char *list[MAX + 1];

	char *p = command;
	int i = 0;

	p++;
	queue = p;							/* first is queue to delete from */
	p += strcspn(queue, RFC1179_WHITESPACE);
	*p = '\0';

	p++;								/* second is remote_user making request */
	remote_user = p;
	p += strcspn(p, RFC1179_WHITESPACE);
	*p = '\0';

	p++;
	DODEBUG_LPRM(("remove_jobs(): queue=\"%s\", remote_user=\"%s\", list=\"%s\"", queue, remote_user, p));

	while(*p && i < MAX)
		{
		list[i++] = p;
		p += strcspn(p, RFC1179_WHITESPACE);
		*p = '\0';
		p++;
		}

	list[i] = (char*)NULL;

	get_proxy_identity(&uid_to_use, &gid_to_use, &proxy_class, fromhost, remote_user, printdest_claim_ppr(queue), access_info);

	/*
	** Use the UPRINT routine to run an appropriate command.
	** If uprint_lprm() returns -1 then the reason is in uprint_errno.
	** Unless the error is UPE_UNDEST, it will already have called
	** uprint_error_callback().  If uprint_lprm() runs a command that
	** fails, it will return the (positive) exit code of that
	** command.
	*/
	if(uprint_lprm(uid_to_use, gid_to_use, remote_user, proxy_class, queue, list, FALSE) == -1)
		{
		if(uprint_errno == UPE_UNDEST)
			{
			printf(_("The queue \"%s\" does not exist on the print server \"%s\".\n"), queue, this_node());
			}
		else if(uprint_errno == UPE_DENIED)
			{
			/* message already printed on stdout */
			}
		else
			{
			printf(_("Could not delete the job or jobs due to a problem with the print server\n"
				"called \"%s\".  Please ask the print server's\n"
				"system administrator to examine the log file \"%s\"\n"
				"to learn the details.\n"), this_node(), LPRSRV_LOGFILE);
			}
		fflush(stdout);
		}
	} /* end of do_request_lprm() */

/* end of file */

