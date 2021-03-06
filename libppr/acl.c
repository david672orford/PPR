/*
** mouse:~ppr/src/libppr/acl.c
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
** Last modified 14 October 2005.
*/

/*! \file
	\brief access control lists
*/

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <grp.h>
#include "gu.h"
#include "global_defines.h"

static const char *internal_list[] = {
		"root",
		USER_PPR,
		USER_PPRWWW,
		NULL
		};

/** check if a given user is listed in a given ACL
*/
gu_boolean user_acl_allows(const char user[], const char acl[])
	{
	/* Look in one of the internal lists.  These lists are compiled in
	   because the system will cease to function correctly if these usernames
	   are removed. */
	{
	int i;
	for(i = 0; internal_list[i]; i++)
		{
		if(strcmp(user, internal_list[i]) == 0)
			return TRUE;
		}
	}

	/* Look for a line with the user name in the .allow
	   file for the ACL list. */
	{
	gu_boolean answer = FALSE;
	FILE *f;
	char fname[MAX_PPR_PATH];
	char *line = NULL;
	int line_space = 80;
	ppr_fnamef(fname, "%s/%s.allow", ACLDIR, acl);
	if((f = fopen(fname, "r")))
		{
		while((line = gu_getline(line, &line_space, f)))
			{
			/* Skip comments. */
			if(line[0] == '#' || line[0] == ';')
				continue;

			/* Use the line as a username pattern */
			if(username_match(user, line))
				{
				answer = TRUE;
				break;
				}
			}
		gu_free_if(line);
		fclose(f);
		}
	return answer;
	}
	} /* end of user_acl_allows() */

/* end of file */
