/*
** mouse:~ppr/src/lprsrv/lprsrv_cancel.c
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
** Last modified 18 October 2005.
*/

#include "config.h"
#include <string.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "rfc1179.h"
#include "lprsrv.h"

/* This file needs a thorough review!!!  Due to changes in PPR, it probably
 * no longer does the right thing. */

static int uprint_lprm_ppr(const char agent[], const char user_domain[], const char queue[], const char **arglist)
	{
	int result_code = 0;
	char *user;
	const char *item_ptr;
	int x;
	const char *args[6];
	int i;

	DODEBUG_UPRINT(("uprint_lprm_ppr(agent = \"%s\", user_domain = \"%s\", queue = \"%s\", arglist = %p", agent, user_domain ? user_domain : "", queue, arglist));

	asprintf(&user, "%s@%s", strcmp(agent, "root") == 0 ? "*": agent, user_domain);

	/* Start to build a command line: */
	args[0] = "ppop";

	/* Process the list of jobs to be deleted, one job at a time. */
	for(x=0; (item_ptr = arglist[x]); x++)
		{
		char *job_name = NULL;
		i = 1;			/* reset on each iteration */

		/* If a job id (a string of digits), */
		if(strspn(item_ptr, "0123456789") == strlen(item_ptr))
			{
			args[i++] = "--user";
			args[i++] = user;

			/* Notice will only be sent to the job's owner
			   if the job is canceled by root. */
			args[i++] = strcmp(agent, "root") ? "scancel" : "cancel";

			asprintf(&job_name, "%s-%s", queue, item_ptr);
			args[i++] = job_name;
			}

		/* Otherwise, it must be a user name: */
		else
			{
			char special_proxy_for[MAX_PRINCIPAL + 1];

			/* If not deleting own jobs and not root, then deny the request. */
			if(strcmp(agent, item_ptr) && strcmp(agent, "root"))
				{
				uprint_errno = UPE_DENIED;

				printf(_("You may not delete jobs belonging to \"%s@%s\" because\n"
						"they are not your's and you are not \"root\".\n"), item_ptr, user_domain);
				fflush(stdout);

				result_code = -1;
				continue;
				}

			if(strlen(item_ptr) > LPR_MAX_P)
				{
				uprint_errno = UPE_BADARG;
				uprint_error_callback(_("Username \"%s\" is too long."), item_ptr);
				result_code = -1;
				continue;
				}

			if(strcmp(user_domain, "localhost") != 0)
				{
				snprintf(special_proxy_for, sizeof(special_proxy_for), "%s@%s", item_ptr, user_domain ? user_domain : "*");
				DODEBUG_UPRINT(("uprint_lprm_ppr(): removing all \"%s\" jobs from \"%s\"", special_proxy_for, queue));
				args[i++] = "--user";
				args[i++] = special_proxy_for;
				}
			else
				{
				DODEBUG_UPRINT(("uprint_lprm_ppr(): removing all \"%s\" jobs from \"%s\""));
				/* nothing to do */
				}

			/* User is informed of cancelation if job canceled by root. */
			args[i++] = strcmp(agent, "root") ? "scancel" : "cancel";

			args[i++] = queue;
			} /* is user name */

		args[i] = (const char *)NULL;

		if(uprint_run(PPOP_PATH, args) == -1)
			result_code = -1;

		gu_free_if(job_name);
		} /* end of jobs/users list loop */

	/* If no files or users were specified, */
	if(arglist[0] == (const char *)NULL)
		{
		i = 1;

		/* For some bizaar reason, the lpr/lpd protocol uses the special agent
		   name "-all" with an empty job/user list to indicate that all jobs
		   should be deleted. */
		if(strcmp(agent, "-all") == 0)
			{
			/* A proxy request, delete all with that proxy class, */
			if(strcmp(user_domain, "localhost") != 0)
				{
				char all_mynode[2 + LPR_MAX_H + 1];
				snprintf(all_mynode, sizeof(all_mynode), "*@%s", user_domain);
				args[i++] = "--user";
				args[i++] = all_mynode;
				args[i++] = "cancel";
				args[i++] = queue;
				}
			/* If this is not a proxy request, purge the queue, */
			else
				{
				args[i++] = "purge";
				args[i++] = queue;
				}
			}

		/* If no jobs/users list and agent not "-all",
		   delete active job, */
		else
			{
			/* If from a remote host, delete only jobs from that hosts user class. */
			if(strcmp(user_domain, "localhost") != 0)
				{
				DODEBUG_UPRINT(("Removing active job for \"%s\" on %s", proxy_for, queue));
				args[i++] = "--user";
				args[i++] = user;
				args[i++] = strcmp(agent, "root") ? "scancel-my-active" : "cancel-my-active";
				args[i++] = queue;
				}
			/* Otherwise, delete any active job. */
			else
				{
				DODEBUG_UPRINT(("Removing active job on %s", queue));
				args[i++] = "cancel-active";
				args[i++] = queue;
				}
			}

		args[i] = (const char*)NULL;

		if(uprint_run(PPOP_PATH, args) == -1)
			result_code = 1;
		}

	gu_free(user);
	return result_code;
	} /* end of uprint_lprm_ppr() */

/*
** Handle an lprm style cancel request.  The file names
** list is filled with a list of job numbers and
** user names.
**
** The 1st argument "agent" is the user requesting the deletion
** (The term "agent" is from RFC-1179).  The second (user_domain) is
** the host from which the request comes.  This should be NULL
** if it is the local host whose users are allowed to submit jobs
** directly as the corresponding local users.
*/
static int uprint_lprm(const char agent[], const char user_domain[], const char queue[], const char **arglist, gu_boolean remote_too)
	{
	if(uprint_claim_ppr(queue))
		return uprint_lprm_ppr(agent, user_domain, queue, arglist);

	/* Unknown queue: */
	uprint_errno = UPE_UNDEST;
	return -1;
	} /* end of uprint_lprm() */

/*
** Handler for the ^E command.  It does some preliminary parsing
** and then passes the work on to a spooler specific function
** defined above.
*/
void do_request_lprm(char *command, const char fromhost[], const struct ACCESS_INFO *access_info)
	{
	const char *queue;			/* queue to delete the jobs from */
	const char *remote_user;	/* user requesting the deletion */
	const char *remote_user_domain = (const char *)NULL;
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

	/*
	** Use the UPRINT routine to run an appropriate command.
	** If uprint_lprm() returns -1 then the reason is in uprint_errno.
	** Unless the error is UPE_UNDEST, it will already have called
	** uprint_error_callback().  If uprint_lprm() runs a command that
	** fails, it will return the (positive) exit code of that
	** command.
	*/
	if(uprint_lprm(remote_user, remote_user_domain, queue, list, FALSE) == -1)
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

