/*
** mouse:~ppr/src/papd/papd_login_aufs.c
** Copyright 1995--2006, Trinity College Computing Center.
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
** Last modified 11 April 2006.
*/

/*
** This module contains support for CAP AUFS authorization.
*/

#include "config.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include "gu.h"
#include "global_defines.h"
#include "queueinfo.h"
#include "userdb.h"
#include "papd.h"

/*
** If the user has not loggined in over PAP and login is required,
** then this function is called just before launching ppr so that
** the proper charge information can be passed to ppr.
*/
void login_aufs(int net, int node, struct USER *user)
	{
	FUNCTION4DEBUG("login_aufs")
	const char *aufs_security_dir;
	char aufs_username[64];
	struct passwd *pw;
	char aufs_security_file[MAX_PPR_PATH];
	FILE *f;

	DODEBUG_AUTHORIZE(("%s(*user, net=%d, node=%d)", function, net, node));

	if(!(aufs_security_dir = gu_ini_query(PPR_CONF, "papd", "aufssecuritydir", 0, NULL)))
		return;

	/*
	** Try to open the AUFS security file.  If it can't be found,
	** say the user doesn't have a volume mounted.
	*/
	ppr_fnamef(aufs_security_file, "%s/net%d.%dnode%d", aufs_security_dir, net / 256, net % 256, node);
	if(!(f = fopen(aufs_security_file, "r")))
		{
		DODEBUG_AUTHORIZE(("AUFS security file \"%s\" not found", aufs_security_file));
		return;
		}

	/*
	** Try to read a line from the AUFS security file.  If we can't
	** we will assume the file is empty which indicates that the user
	** has no volumes mounted.
	*/
	if(!fgets(aufs_username, sizeof(aufs_username), f))
		{
		DODEBUG_AUTHORIZE(("AUFS security file \"%s\" is empty", aufs_security_file));
		return;
		}

	fclose(f);

	aufs_username[strcspn(aufs_username,"\n")] = '\0';
	DODEBUG_AUTHORIZE(("aufs_username[] = \"%s\"", aufs_username));

	if(!(pw = getpwnam(aufs_username)))
		{
		DODEBUG_AUTHORIZE(("User \"%s\" (from the AUFS security file) doesn't exist in /etc/passwd", aufs_username));
		return;
		}

	user->username = gu_strdup(pw->pw_name);
	user->fullname = gu_strdup(pw->pw_gecos);
	} /* end of login_aufs() */

/* end of file */
