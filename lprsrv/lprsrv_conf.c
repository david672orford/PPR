/*
** mouse:~ppr/src/lprsrv/lprsrv_conf.c
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
** Last modified 19 August 2005.
*/

#include "config.h"
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#ifdef HAVE_INNETGROUP
#include <netdb.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "rfc1179.h"
#include "lprsrv.h"

static gu_boolean authorized_file_check(const char name[], const char file[]);

/*
** This function return TRUE if if an entry matching the
** triple (node,NULL,NULL) is found in the netgroup.
*/
static gu_boolean netgroup_matched(const char node[], const char netgroup[])
	{
	#ifdef HAVE_INNETGROUP
	if(innetgr(netgroup, node, NULL, NULL))
		return TRUE;
	else
	#endif
		return FALSE;
	}

/*
** This is used when reading lprsrv.conf, hosts.lpd, and hosts.equiv.  It
** tests whether a node name matches one of the patterns from one of those
** files.
*/
static gu_boolean node_pattern_match(const char node[], const char pattern[])
	{
	/* if it specifies a domain, */
	if(pattern[0] == '.')
		{
		int lendiff = strlen(node) - strlen(pattern);
		if(lendiff >= 1)
			{
			if(gu_strcasecmp(pattern, &node[lendiff]) == 0)
				{
				return TRUE;
				}
			}
		}

	/* If it specifies a netgroup, */
	else if(pattern[0] == '@' && netgroup_matched(node, pattern + 1))
		{
		return TRUE;
		}

	/* If it specifies a file, */
	else if(pattern[0] == '/' && authorized_file_check(node, pattern))
		{
		return TRUE;
		}
		
	/* It must specify a host. */
	else
		{
		if(gu_strcasecmp(pattern, node) == 0)
			{
			return TRUE;
			}
		}

	return FALSE;
	} /* end of node_pattern_match() */

/*
** This function used used by authorized().  This function checks to see
** if the indicated node or a domain which encompasses it is named
** in the indicated file.  This is used for files such as hosts.lpd
** and hosts.equiv.
*/
static gu_boolean authorized_file_check(const char name[], const char file[])
	{
	FILE *f;
	char *line = NULL;
	int line_space = 80;
	gu_boolean answer = FALSE;

	if((f = fopen(file, "r")))
		{
		while((line = gu_getline(line, &line_space, f)))
			{
			if(node_pattern_match(name, line))
				{
				answer = TRUE;
				break;
				}
			}

		if(line)
			gu_free(line);

		fclose(f);
		}

	return answer;
	} /* end of authorized_file_check() */

/*
** Return TRUE if the client is authorized to connect.	(That is,
** according to traditional LPD access control rules, not
** necessarily the rules lprsrv will follow.  get_access_settings()
** uses this for the [traditional] section of lprsrv.conf.)
**
** Note: /etc/hosts.lpd has been extended to accept domain names.
** ie: an entry such as ".eecs.umich.edu" in the hosts.lpd file would
**	   allow all machines in the eecs.umich.edu domain to connect.
** There is now also a /etc/hosts.lpd_deny file; any machine that fits there
** will be rejected (machines listed here take precendence over those
** listed in hosts.lpd)
*/
static gu_boolean authorized(const char name[])
	{
	int answer = FALSE;

	if(authorized_file_check(name, "/etc/hosts.lpd"))
		answer = TRUE;
	else if(authorized_file_check(name, "/etc/hosts.equiv"))
		answer = TRUE;

	/* If the host was accepted above, reject reject it if it is also listed 
	   in hosts.lpd_deny. */
	if(answer && authorized_file_check(name, "/etc/hosts.lpd_deny"))
		answer = FALSE;

	return answer;
	} /* end of authorized() */

/*
** This function is called by get_access_settings() to read a
** lprsrv.conf section and copy the new values into the
** supplied structure.
*/
static void get_access_settings_read_section(struct ACCESS_INFO *access, FILE *conffile, long startat, int linenum)
	{
	const char function[] = "get_access_settings_read_section";
	char line[LPRSRV_CONF_MAXLINE+1];
	char *si, *di, *name, *value;

	DODEBUG_CONF(("%s(access=%p, conffile=%p, startat=%ld, linenum=%d)", function, access, conffile, startat, linenum));

	if(fseek(conffile, startat, SEEK_SET))
		fatal(1, "%s(): fseek() failed, errno=%d (%s)", function, errno, gu_strerror(errno));

	while(fgets(line, sizeof(line), conffile) && line[0] != '[')
		{
		linenum++;
		si = di = line;

		while(*si)
			{
			if(*si == ';' || *si == '#') break;
			if(isspace(*si)) { si++; continue; }
			*(di++) = *(si++);
			}
		*di = '\0';

		/* Skip blank lines */
		if(! line[0]) continue;

		DODEBUG_CONF(("line %d: %s", linenum, line));

		/*
		** Set name and value to point to the keyword
		** and value strings.
		*/
		name = line;
		if((value = strchr(line, '=')) == (char*)NULL)
			{
			warning("Line %d of \"%s\" is not name=value", linenum, LPRSRV_CONF);
			continue;
			}
		*value = '\0';
		value++;
		if(! *value)
			{
			warning("Value missing from line %d of \"%s\"", linenum, LPRSRV_CONF);
			continue;
			}

		if(strcmp(name, "allow") == 0)
			{
			if(gu_torf_setBOOL(&access->allow, value) == -1)
				warning("Invalid value for \"%s =\" at \"%s\" line %d", "allow", LPRSRV_CONF, linenum);
			}
		else if(strcmp(name, "insecureports") == 0)
			{
			if(gu_torf_setBOOL(&access->insecure_ports, value) == -1)
				warning("Invalid value for \"%s =\" at \"%s\" line %d", "insecure ports", LPRSRV_CONF, linenum);
			}
		else if(strcmp(name, "userdomain") == 0)
			{
			if(strlen(value) > MAX_USER_DOMAIN)
				warning("Value in \"%s\" line %d is too long", LPRSRV_CONF, linenum);
			gu_strlcpy(access->user_domain, value, MAX_USER_DOMAIN);
			}
		else if(strcmp(name, "forcemail") == 0)
			{
			if(gu_torf_setBOOL(&access->force_mail, value) == -1)
				warning("Invalid value for \"%s =\" at \"%s\" line %d", "force mail", LPRSRV_CONF, linenum);
			}
		else
			{
			warning("Unrecognized keyword in \"%s\" line %d", LPRSRV_CONF, linenum);
			}
		}
	} /* end of get_access_settings_read_section() */

/*
** This function loads the access settings for the indicated host into
** the supplied structure.  It does this by reading the entire file and
** noting the position of the [global], [traditional], and [other] sections
** and the section whose name is the longest match for the hostname.  It then
** calls get_access_settings_read_section() to read the values from the
** [global] section and then calls it again to read the longest match,
** [traditional], or [other] section.
*/
void get_access_settings(struct ACCESS_INFO *access, const char hostname[])
	{
	DODEBUG_CONF(("get_access_settings()"));

	access->allow = FALSE;
	access->insecure_ports = FALSE;
	access->user_domain[0] = '\0';
	access->force_mail = FALSE;

	/*
	** Find the [global] section and the section and note their locations.
	*/
	{
	FILE *f;
	char *line = NULL;
	int line_space = 80;
	int linenum = 0;							/* line we are processing right now */
	char *p;
	int len;

	long offset_global = -1;					/* offset of [global] section */
	long offset_traditional = -1;				/* offset of [traditional] section */
	long offset_best = -1;						/* offset of longest match so far */
	long offset_other = -1;						/* offset of [other] section */
	int linenum_global = -1000;					/* -1000 is an obviously */
	int linenum_traditional = -1000;			/*	 wrong value that should */
	int linenum_best = -1000;					/*	 stick out like a sore */
	int linenum_other = -1000;					/*	 thumb. */
	int len_best = 0;							/* length of longest match so far */

	if((f = fopen(LPRSRV_CONF, "r")) == (FILE*)NULL)
		fatal(1, "Can't open \"%s\", errno=%d (%s)", LPRSRV_CONF, errno, gu_strerror(errno));

	while((line = gu_getline(line, &line_space, f)))
		{
		linenum++;

		/* Skip lines that don't follow pattern [*]. */
		if(line[0] != '[')
			continue;
		if(!(p = strchr(line, ']')))
			continue;

		*p = '\0';
		len = (p - line - 1);

		if(strcmp(line+1, "global") == 0)
			{
			offset_global = ftell(f);
			linenum_global = linenum;
			continue;
			}
		if(strcmp(line+1, "other") == 0)
			{
			offset_other = ftell(f);
			linenum_other = linenum;
			continue;
			}
		if(strcmp(line+1, "traditional") == 0)
			{
			offset_traditional = ftell(f);
			linenum_traditional = linenum;
			continue;
			}

		/* Pretend netgroup and file patterns very short so anything will override 
		   them.  This may not be the best way to do it, but it is easy to code.
		   Other ideas welcome!
		   */
		if(line[1] == '@' || line[1] == '/')
			len = 0;

		/* Long matches are better than short ones. */
		if(len < len_best)
			continue;

		/* Run it through the host pattern matcher. */
		if(node_pattern_match(hostname, line + 1))
			{
			len_best = len;
			offset_best = ftell(f);
			linenum_best = linenum;
			}
		} /* end of line reading loop */

	if(line)
		gu_free(line);

	DODEBUG_CONF(("get_access_settings(): offset_global=%ld, offset_traditional=%ld, offset_best=%ld, offset_other=%ld", offset_global, offset_traditional, offset_best, offset_other));

	/* The [global] and [other] sections are mandatory. */
	if(offset_global == -1)
		fatal(1, "No [global] section in \"%s\"", LPRSRV_CONF);
	if(offset_other == -1)
		fatal(1, "No [other] section in \"%s\"", LPRSRV_CONF);

	/*
	** OK, we know where they are, now we must read them in.
	*/

	/* Read the [global] section. */
	get_access_settings_read_section(access, f, offset_global, linenum_global);

	/* Make sure the [global] section has set everything. */
	if(access->user_domain[0] == '\0')
		fatal(1, "No \"%s =\" in \"%s\" [global]", "user domain", LPRSRV_CONF);

	/* If no section matched, and the client is listed in hosts.lpd or hosts.equiv,
	   choose the [traditional] section, otherwise choose the [other] section. */
	if(offset_best == -1)
		{
		if(offset_traditional != -1 && authorized(hostname))
			{
			offset_best = offset_traditional;
			linenum_best = linenum_traditional;
			}
		else
			{
			offset_best = offset_other;
			linenum_best = linenum_other;
			}
		}

	/* Whatever section was finally chosen, read it now. */
	get_access_settings_read_section(access, f, offset_best, linenum_best);

	fclose(f);
	}

	DODEBUG_CONF(("allow = %s", access->allow ? "yes" : "no"));
	DODEBUG_CONF(("insecureports = %s", access->insecure_ports ? "yes" : "no"));
	DODEBUG_CONF(("user format = \"%s\"", access->user_format));
	} /* end of get_access_settings() */

/* end of file */

