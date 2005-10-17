/*
** mouse:~ppr/src/templates/module.c
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
** Last modified 17 October 2005.
*/

/*! \file
	\brief drop root privileges
*/

#include "config.h"
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
/** drop root privileges
*/
int renounce_root_privs(const char progname[], const char username[], const char groupname[])
	{
	uid_t uid, euid, target_uid;
	gid_t gid, egid, target_gid;

	/*
	** Look up username[] and possible groupname[] in order to determine
	** the user and group IDs under which we should be running.
	*/
	{
	struct passwd *pw;
	struct group *gp;

	if(!(pw = getpwnam(username)))
		{
		fprintf(stderr, _("%s: The user \"%s\" does not exist.\n"), progname, username);
		return 1;
		}
	target_uid = pw->pw_uid;

	if(groupname)
		{
		if(!(gp = getgrnam(groupname)))
			{
			fprintf(stderr, _("%s: The group \"%s\" does not exist.\n"), progname, groupname);
			return 1;
			}
	
		if(pw->pw_gid != gp->gr_gid)
			fprintf(stderr, _("%s: Warning: primary group for user \"%s\" is not \"%s\".\n"), progname, username, groupname);

		target_gid = gp->gr_gid;
		}
	else
		{
		target_gid = pw->pw_gid;
		}
	}

	uid = getuid();
	euid = geteuid();
	gid = getgid();
	egid = getegid();

	/*
	 * Some of the PPR deamons were once setuid and setgid, but they are no
	 * longer.
	 */
	if(euid != uid || egid != gid)
		{
		fprintf(stderr, _("%s: should not be setuid or setgid\n"), progname);
		return 1;
		}

	/*
	 * If the uid and euid are root (we have already verified that they
	 * are equal), then whatever may be wrong, we can make it right.
	 */
	if(euid == 0)
		{
		/*
		** Relinquish root priviledge.  This is difficult because
		** not all systems have precisely the same semantics with regard to when
		** the saved IDs are set.
		*/
	
		#ifdef HAVE_INITGROUPS
		if(initgroups(username, target_gid) == -1)
			{
			fprintf(stderr, _("%s: setgroups(\"%s\", %ld) failed, errno=%d (%s)\n"), progname, username, (long)target_gid, errno, gu_strerror(errno));
			return 1;
			}
		#endif
	
		/* MacOS 10.2 manpage (which is probably the BSD manpage)suggests that
		 * this will set the saved IDs. */
		if(setgid(target_gid) == -1)
			{
			fprintf(stderr, _("%s: setgid(%ld) failed, errno=%d (%s)\n"), progname, (long)target_gid, errno, gu_strerror(errno));
			return 1;
			}
		if(setuid(target_uid) == -1)
			{
			fprintf(stderr, _("%s: setuid(%ld) failed, errno=%d (%s)\n"), progname, (long)target_uid, errno, gu_strerror(errno));
			return 1;
			}
	
		/* Linux manpage suggests that this will set the saved IDs. */
		if(setreuid(target_uid, target_uid) == -1)
			{
			fprintf(stderr, _("%s: setreuid(%ld, %ld) failed, errno=%d (%s)\n"), progname, (long)target_uid, (long)target_uid, errno, gu_strerror(errno));
			return 1;
			}
		if(setregid(target_gid, target_gid) == -1)
			{
			fprintf(stderr, _("%s: setregid(%ld, %ld) failed, errno=%d (%s)\n"), progname, (long)target_gid, (long)target_gid, errno, gu_strerror(errno));
			return 1;
			}
	
		/*
		 * Make sure the system semantics haven't bitten us.  If the saved UID
		 * hasn't been set, then setuid() will suceed.
		 */
		if(setuid(0) != -1)
			{
			fprintf(stderr, X_("%s: setuid(0) did not fail!\n"), progname);
			return 1;
			}
		} /* if root */

	else if(uid != target_uid)
		{
		fprintf(stderr, _("%s: Only \"%s\" or \"root\" may start %s.\n"), progname, username, progname);
		return 1;
		}

	else if(gid != target_gid)
		{
		fprintf(stderr, X_("%s: gid is %ld when it should be %ld\n"), progname, (long)gid, (long)target_gid);	
		return 1;
		}

	return 0;
	} /* renounce_root_privs() */

