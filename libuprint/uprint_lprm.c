/*
** mouse:~ppr/src/libuprint/uprint_lprm.c
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
#include <string.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "uprint.h"
#include "uprint_private.h"

static int uprint_lprm_ppr(uid_t uid, gid_t gid, const char agent[], const char proxy_class[], const char *queue, const char **arglist)
	{
	int result_code = 0;
	char proxy_for[MAX_PRINCIPAL + 1];
	const char *item_ptr;
	int x;
	char job_name[MAX_DESTNAME + 4 + 1];
	const char *args[6];
	int i;

	DODEBUG(("uprint_lprm_ppr(agent = \"%s\", proxy_class = \"%s\", queue = \"%s\", arglist = %p", agent, proxy_class != (const char *)NULL ? proxy_class : "", queue, arglist));

	if(strlen(queue) > MAX_DESTNAME)
		{
		uprint_errno = UPE_BADARG;
		uprint_error_callback(_("The print queue name \"%s\" is too long for PPR."), queue);
		return -1;
		}

	if(strlen(agent) > LPR_MAX_P)
		{
		uprint_errno = UPE_BADARG;
		uprint_error_callback(_("The agent name \"%s\" is too long."), agent);
		return -1;
		}

	if(proxy_class && strlen(proxy_class) > LPR_MAX_H)
		{
		uprint_errno = UPE_BADARG;
		uprint_error_callback(_("The proxy class name \"%s\" is too long."), proxy_class);
		return -1;
		}

	/*
	** If this is a proxy job, build a string which represents ]
	** the user on whose behalf the proxy is requesting the deletions.
	**
	** This string is composed of 2 parts separated by a "@" sign.
	** the first part is the user name.  It will be "*" if the
	** user name is "root".  The second part is the proxy class
	** which is generally either the client's canonical host name
	** or the clients domain.
	*/
	if(proxy_class)
		snprintf(proxy_for, sizeof(proxy_for), "%s@%s", strcmp(agent, "root") == 0 ? "*": agent, proxy_class);

	/* Start to build a command line: */
	args[0] = "ppop";

	/* Process the list of jobs to be deleted, one job at a time. */
	for(x=0; (item_ptr = arglist[x]); x++)
		{
		i = 1;			/* reset on each iteration */

		/* If a job id (a string of digits), */
		if(strspn(item_ptr, "0123456789") == strlen(item_ptr))
			{
			snprintf(job_name, sizeof(job_name), "%s-%s", queue, item_ptr);

			/* If this is a proxy job, indicate whom the
			   proxy is acting for. */
			if(proxy_class)
				{
				DODEBUG(("uprint_lprm_ppr(): proxy for \"%s\", removing \"%s\"", proxy_for, job_name));
				args[i++] = "-X";
				args[i++] = proxy_for;
				}

			/* Notice will only be sent to the job's owner
			   if the job is canceled by root. */
			args[i++] = strcmp(agent, "root") ? "scancel" : "cancel";

			args[i++] = job_name;
			}

		/* Otherwise, it must be a user name: */
		else
			{
			char special_proxy_for[MAX_PRINCIPAL + 1];

			/* If not deleting own jobs and not root, then deny the request.
			   The error message is worded slightly differently depending
			   on whether or not this is a proxy job. */
			if(strcmp(agent, item_ptr) && strcmp(agent, "root"))
				{
				uprint_errno = UPE_DENIED;

				/* We write the stdout here because lpr would.  Thus, we do it for consistency. */
				if(proxy_class)
					{
					printf(_("You may not delete jobs belonging to \"%s@%s\" because\n"
								"they are not your's and you are not \"root@%s\".\n"), item_ptr, proxy_class, proxy_class);
					}
				else
					{
					printf(_("You may not delete jobs belonging to \"%s\" because\n"
								"they are not your's and you are not \"root\".\n"), item_ptr);
					}
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

			if(proxy_class)
				{
				snprintf(special_proxy_for, sizeof(special_proxy_for), "%s@%s", item_ptr, proxy_class == (const char *)NULL ? "*" : proxy_class);
				DODEBUG(("uprint_lprm_ppr(): removing all \"%s\" jobs from \"%s\"", special_proxy_for, queue));
				args[i++] = "-X";
				args[i++] = special_proxy_for;
				}
			else
				{
				DODEBUG(("uprint_lprm_ppr(): removing all \"%s\" jobs from \"%s\""));
				}

			/* User is informed of cancelation if job canceled by root. */
			args[i++] = strcmp(agent, "root") ? "scancel" : "cancel";

			args[i++] = queue;
			}

		args[i] = (const char *)NULL;

		if(uprint_run(uid, gid, PPOP_PATH, args) == -1)
			result_code = -1;
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
			if(proxy_class)
				{
				char all_mynode[2 + LPR_MAX_H + 1];
				snprintf(all_mynode, sizeof(all_mynode), "*@%s", proxy_class);
				args[i++] = "-X";
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
			/* If by proxy, delete only job with same
			   proxy class. */
			if(proxy_class)
				{
				DODEBUG(("Removing active job for \"%s\" on %s", proxy_for, queue));
				args[i++] = "-X";
				args[i++] = proxy_for;
				args[i++] = strcmp(agent, "root") ? "scancel-my-active" : "cancel-my-active";
				args[i++] = queue;
				}
			/* If not by proxy, delete any active job. */
			else
				{
				DODEBUG(("Removing active job on %s", queue));
				args[i++] = "cancel-active";
				args[i++] = queue;
				}
			}

		args[i] = (const char*)NULL;

		if(uprint_run(uid, gid, PPOP_PATH, args) == -1)
			result_code = 1;
		}

	return result_code;
	} /* end of uprint_lprm_ppr() */

/*
** Handle an lprm style cancel request.  The file names
** list is filled with a list of job numbers and
** user names.
**
** The 1st argument "agent" is the user requesting the deletion
** (The term "agent" is from RFC-1179).  The second (proxy_class) is
** the host from which the request comes.  This should be NULL
** if it is the local host whose users are allowed to submit jobs
** directly as the corresponding local users.
*/
int uprint_lprm(uid_t uid, gid_t gid, const char agent[], const char proxy_class[], const char queue[], const char **arglist, gu_boolean remote_too)
	{
	if(printdest_claim_ppr(queue))
		return uprint_lprm_ppr(uid, gid, agent, proxy_class, queue, arglist);

	{
	struct REMOTEDEST info;
	if(remote_too && printdest_claim_remote(queue, &info))
		return uprint_lprm_rfc1179(agent, (const char *)NULL, queue, arglist, &info);
	}

	/* Unknown queue: */
	uprint_errno = UPE_UNDEST;
	return -1;
	} /* end of uprint_lprm() */

/* end of file */

