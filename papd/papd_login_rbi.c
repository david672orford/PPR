/*
** mouse:~ppr/src/papd/papd_login_rbi.c
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
** Last modified 9 November 2004.
*/

/*
** This module contains support for the "RBI" authentication implemented
** in LaserWriter 8.6.1 and later.  This protocol is described in
** "AppleShare IP Print Server Security Protocol", an Apple Computer 
** publication.
*/

#include "config.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include "gu.h"
#include "global_defines.h"
#include "userdb.h"
#include "papd.h"
#include "version.h"

static char user_username[16];
static char user_fullname[64];

/*
 * Handler function for:
 * %%?BeginQuery: RBISpoolerID
 * %%?BeginQuery: RBIUMAListQuery
 * %%?BeginQuery: RBILogin
 * %%?BeginQuery: RBILoginCont
*/
int rbi_query(int sesfd, void *qc)
	{
	DODEBUG_AUTHORIZE(("rbi_query(), line=\"%s\"", line));

	if(strcmp(tokens[1], "RBISpoolerID") == 0)
		{
		char temp[64];
		gu_snprintf(temp, sizeof(temp), "(PPR) %.*s (%s)\n", strspn(SHORT_VERSION, "0123456789."), SHORT_VERSION, SHORT_VERSION);
		REPLY(sesfd, temp);
		return 0;
		}

	else if(strcmp(tokens[1], "RBIUAMListQuery") == 0)
		{
		if(!queueinfo_chargeExists(qc))
			REPLY(sesfd, "NoAuthUAM\n");
		REPLY(sesfd, "ClearTxtUAM\n");
		/* REPLY(sesfd, "TwoWayRandnumUAM\n"); */
		REPLY(sesfd, "*\n");
		return 0;
		}

	else if(strcmp(tokens[1], "RBILogin") == 0 && tokens[2])
		{
		char *username;
		char *uamdata;
		if(strcmp(tokens[2], "NoAuthUAM") == 0 && (username = tokens[3]))
			{
			gu_strlcpy(user_username, username, sizeof(user_username));
			gu_strlcpy(user_fullname, username, sizeof(user_fullname));
			}
		else if(strcmp(tokens[2], "ClearTxtUAM") == 0 && (username = tokens[3]) && (uamdata = tokens[4]))
			{
			if(strcmp(username, uamdata) == 0)
				{
				gu_strlcpy(user_username, username, sizeof(user_username));
				gu_strlcpy(user_fullname, username, sizeof(user_fullname));
				REPLY(sesfd, "0\n");
				}
			else
				{
				REPLY(sesfd, "-1\n");
				}
			return 0;
			}
		else if(strcmp(tokens[2], "TwoWayRandnumUAM") == 0 && (username = tokens[3]) && (uamdata = tokens[4]))
			{
			}
		}

	else if(strcmp(tokens[1], "RBILoginCont") == 0)
		{
		}

	return -1;
	} /* end of rbi_query() */

/*
 * This is called when processing a printjob to retrieve the username
 * of the user who logged in using RBI.
 */
void login_rbi(struct USER *user)
	{
	if(strlen(user_username) > 0)
		{
		user->username = user_username;
		user->fullname = user_fullname;
		}
	}

gu_boolean login_rbi_active(void)
	{
	return strlen(user_username) > 0 ? TRUE : FALSE;
	}

/* end of file */
