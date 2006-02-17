/*
** mouse:~ppr/src/templates/module.c
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
** Last modified 17 February 2006.
*/

#include "config.h"
#include <string.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

/*! \file
	\brief username matching

*/

/* Examine the provided hostname and return it only if it does not
 * represent the local host.  If it does represent the local host
 * return NULL. */
static char *localhost_remove(char *host)
	{
	if(host == NULL)
		return NULL;
	if(strcmp(host, "localhost") == 0)
		return NULL;
	if(strcmp(host, "127.0.0.1") == 0)
		return NULL;
	return host;
	}

/** compare username to pattern
**
** This function compares a job's username to a username pattern
** which can include basic wildcards.  If they match, it returns TRUE.
*/
gu_boolean username_match(const char username[], const char pattern[])
	{
	gu_boolean answer;
	char *username_at_host, *pattern_at_host;
	char *username_host, *pattern_host;

	/* If they match exactly, our job is easy. */
	if(strcmp(username, pattern) == 0)
		return TRUE;

	if((username_at_host = strchr(username, '@')))
		{
		*username_at_host = '\0';
		username_host = localhost_remove(username_at_host + 1);
		}
	else
		{
		username_host = NULL;
		}

	if((pattern_at_host = strchr(pattern, '@')))
		{
		*pattern_at_host = '\0';
		pattern_host = localhost_remove(pattern_at_host + 1);
		}
	else
		{
		pattern_host = NULL;
		}
		
	do	{
		answer = TRUE;
		/* If the usernames don't match, we fail here. */
		if(!gu_wildmat(username, pattern))
			{
			answer = FALSE;
			break;
			}
		/* If the username is on a remote host and the pattern doesn't
		 * specify a host other than localhost or the hosts don't match,
		 * we fail here.
		 */
		if(username_host && (!pattern_host 
				|| !gu_wildmat(username_host, pattern_host)
				))
			{
			answer = FALSE;
			break;
			}
		/* If the username is local and the pattern specifies a 
		 * host other than localhost or full wildcard, fail here.
		 */
		if(!username_host && pattern_host && strcmp(pattern_host, "*") != 0)
			{
			answer = FALSE;
			break;
			}
		} while(FALSE);

	/* Where there where at signs, put them back. */
	if(username_at_host)
		*username_at_host = '@';
	if(pattern_at_host)
		*pattern_at_host = '@';
	
	return answer;
	} /* username_match() */

/* end of file */
