/*
** mouse:~ppr/src/ppad/query_wrapper.c
** Copyright 1995--2004, Trinity College Computing Center.
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
** Last modified 24 May 2004.
*/

/*
 * This program is invoked by the code in query.c.  It is setuid root.
 * It is a wrapper around the interface programs.  It provides a way
 * for utilities such as ppad to invoke the interfaces with all of 
 * the suplementatal group membership that the user "ppr" has and which
 * may be required for access to printer ports.
 */

#include "config.h"
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "gu.h"
#include "global_defines.h"

const char myname[] = "query_wrapper";

int main(int argc, char *argv[])
	{
	struct passwd *pw;
	char fname[MAX_PPR_PATH];

	if(argc < 1 || strchr(argv[0], '/'))
		{
		fprintf(stderr, "%s: incorrect invokation.\n", myname);
		return 1;
		}

	if((pw = getpwnam(USER_PPR)) == NULL)
		{
		fprintf(stderr, "%s: the user \"%s\" doesn't exist.\n", myname, USER_PPR);
		return 1;
		}

	/* fprintf(stderr, "%ld %ld %ld\n", getuid(), geteuid(), pw->pw_uid); */

	if(getuid() != pw->pw_uid)		/* don't let anyone use us */
		{
		fprintf(stderr, "%s: permission denied.\n", myname);
		return 1;
		}

	#ifdef HAVE_INITGROUPS
	if(initgroups(USER_PPR, pw->pw_gid) == -1)
		{
		fprintf(stderr, "%s: setgroups(\"%s\", %ld) failed, errno=%d (%s)\n", myname, USER_PPR, (long)pw->pw_gid, errno, gu_strerror(errno));
		return 1;
		}
	#else
	if(setgid(pw->pw_gid) == -1)
		{
		fprintf(stderr, "%s: setgid(%ld) failed, errno=%d (%s)\n", myname, (long)pw->pw_gid, errno, gu_strerror(errno));
		return 1;
		}
	#endif

	if(setuid(pw->pw_uid) == -1)	/* renounce superuser privledge */
		{
		fprintf(stderr, "%s: setuid(%ld) failed, errno=%d (%s)\n", myname, (long)pw->pw_uid, errno, gu_strerror(errno));
		return 1;
		}
	
	if(setuid(0) != -1)				/* paranoid */
		{
		fprintf(stderr, "%s: setuid(0) failed to fail!\n", myname);
		return 1;
		}

	ppr_fnamef(fname, "%s/%s", INTDIR, argv[0]);
	execv(fname, argv);
	return 1;
	} /* end of main() */

/* end of file */
