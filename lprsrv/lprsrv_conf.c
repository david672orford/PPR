/*
** mouse:~ppr/src/lprsrv/lprsrv_conf.c
** Copyright 1995--2002, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 13 March 2002.
*/

#include "before_system.h"
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#ifdef HAVE_NETGROUP
#include <netdb.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "lprsrv.h"
#include "uprint.h"

/*
** This utility function is used by many modules in lprsrv.
** It is here because both lprsrv and lprsrv-test are linked
** with this module and this module requires clipcopy().
**
** Do a truncating string copy.  The parameter maxlen
** specifies the maximum length to copy exclusive of the
** NULL which terminates the string.
*/
void clipcopy(char *dest, const char *source, int maxlen)
    {
    while(maxlen-- && *source)
	*(dest++) = *(source++);
    *dest = '\0';
    } /* end of clipcopy() */

/*
** This function return TRUE if if an entry matching the
** triple (node,NULL,NULL) is found in the netgroup.
*/
static gu_boolean netgroup_matched(const char node[], const char netgroup[])
    {
    #ifdef HAVE_NETGROUP
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

    /* It must specify a host. */
    else
    	{
    	if(gu_strcasecmp(pattern, node) == 0)
    	    {
	    return TRUE;
    	    }
	}

    return FALSE;
    }

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
** Return TRUE if the client is authorized to connect.  (That is,
** according to traditional LPD access control rules, not
** necessarily the rules lprsrv will follow.  get_access_settings()
** uses this for the [traditional] section of lprsrv.conf.)
**
** Note: /etc/hosts.lpd has been extended to accept domain names.
** ie: an entry such as ".eecs.umich.edu" in the hosts.lpd file would
**     allow all machines in the eecs.umich.edu domain to connect.
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

    /* reject any hosts listed in hosts.lpd_deny */
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
	    int answer = gu_torf(value);
	    if(answer == ANSWER_UNKNOWN)
	    	warning("Invalid value for \"%s =\" at \"%s\" line %d", "allow", LPRSRV_CONF, linenum);
	    else
	    	access->allow = answer ? TRUE : FALSE;
	    }
	else if(strcmp(name, "insecureports") == 0)
	    {
	    int answer = gu_torf(value);
	    if(answer == ANSWER_UNKNOWN)
	    	warning("Invalid value for \"%s =\" at \"%s\" line %d", "insecure ports", LPRSRV_CONF, linenum);
	    else
	    	access->insecure_ports = answer ? TRUE : FALSE;
	    }
	else if(strcmp(name, "pprbecomeuser") == 0)
	    {
	    int answer = gu_torf(value);
	    if(answer == ANSWER_UNKNOWN)
	    	warning("Invalid value for \"%s =\" at \"%s\" line %d", "ppr become user", LPRSRV_CONF, linenum);
	    else
	    	access->ppr_become_user = answer ? TRUE : FALSE;
	    }
	else if(strcmp(name, "otherbecomeuser") == 0)
	    {
	    int answer = gu_torf(value);
	    if(answer == ANSWER_UNKNOWN)
	    	warning("Invalid value for \"%s =\" at \"%s\" line %d", "other become user", LPRSRV_CONF, linenum);
	    else
	    	access->other_become_user = answer ? TRUE : FALSE;
	    }
	else if(strcmp(name, "pprrootas") == 0)
	    {
	    if(strlen(value) > MAX_USERNAME)
	    	warning("Value in \"%s\" line %d is too long", LPRSRV_CONF, linenum);
	    clipcopy(access->ppr_root_as, value, MAX_USERNAME);
	    }
	else if(strcmp(name, "otherrootas") == 0)
	    {
	    if(strlen(value) > MAX_USERNAME)
	    	warning("Value in \"%s\" line %d is too long", LPRSRV_CONF, linenum);
	    clipcopy(access->other_root_as, value, MAX_USERNAME);
	    }
	else if(strcmp(name, "pprproxyuser") == 0)
	    {
	    if(strlen(value) > MAX_USERNAME)
	    	warning("Value in \"%s\" line %d is too long", LPRSRV_CONF, linenum);
	    clipcopy(access->ppr_proxy_user, value, MAX_USERNAME);
	    }
	else if(strcmp(name, "otherproxyuser") == 0)
	    {
	    if(strlen(value) > MAX_USERNAME)
	    	warning("Value in \"%s\" line %d is too long", LPRSRV_CONF, linenum);
	    clipcopy(access->other_proxy_user, value, MAX_USERNAME);
	    }
	else if(strcmp(name, "pprproxyclass") == 0)
	    {
	    if(strlen(value) > MAX_PROXY_CLASS)
	    	warning("Value in \"%s\" line %d is too long", LPRSRV_CONF, linenum);
	    clipcopy(access->ppr_proxy_class, value, MAX_PROXY_CLASS);
	    }
	else if(strcmp(name, "ppruserformat") == 0)
	    {
	    if(strlen(value) > MAX_FROM_FORMAT)
	    	warning("Value in \"%s\" line %d is too long", LPRSRV_CONF, linenum);
	    clipcopy(access->ppr_from_format, value, MAX_FROM_FORMAT);
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
    access->ppr_become_user = FALSE;
    access->other_become_user = FALSE;
    access->ppr_root_as[0] = '\0';
    access->other_root_as[0] = '\0';
    access->ppr_proxy_user[0] = '\0';
    access->other_proxy_user[0] = '\0';
    access->ppr_proxy_class[0] = '\0';
    access->ppr_from_format[0] = '\0';

    /*
    ** Find the [global] section and the section and note their locations.
    */
    {
    FILE *f;
    char *line = NULL;
    int line_space = 80;
    int linenum = 0;				/* line we are processing right now */
    char *p;
    int len;

    long offset_global = -1;			/* offset of [global] section */
    long offset_traditional = -1;		/* offset of [traditional] section */
    long offset_best = -1;			/* offset of longest match so far */
    long offset_other = -1;			/* offset of [other] section */
    int linenum_global = -1000;			/* -1000 is an obviously */
    int linenum_traditional = -1000;		/*   wrong value that should */
    int linenum_best = -1000;			/*   stick out like a sore */
    int linenum_other = -1000;			/*   thumb. */
    int len_best = 0;				/* length of longest match so far */

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

	/* Pretend netgroup patterns very short so anything will override them.
	   This may not be the best way to do it, but it is easy to code.
	   Other ideas welcome!  */
	if(line[1] == '@')
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
    if(access->ppr_root_as[0] == '\0')
	fatal(1, "No \"%s =\" in \"%s\" [global]", "ppr root as", LPRSRV_CONF);
    if(access->other_root_as[0] == '\0')
	fatal(1, "No \"%s =\" in \"%s\" [global]", "other root as", LPRSRV_CONF);
    if(access->ppr_proxy_user[0] == '\0')
	fatal(1, "No \"%s =\" in \"%s\" [global]", "ppr proxy user", LPRSRV_CONF);
    if(access->other_proxy_user[0] == '\0')
	fatal(1, "No \"%s =\" in \"%s\" [global]", "other proxy user", LPRSRV_CONF);
    if(access->ppr_proxy_class[0] == '\0')
	fatal(1, "No \"%s =\" in \"%s\" [global]", "ppr proxy class", LPRSRV_CONF);
    if(access->ppr_from_format[0] == '\0')
	fatal(1, "No \"%s =\" in \"%s\" [global]", "ppr user format", LPRSRV_CONF);

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
    DODEBUG_CONF(("ppr become user = %s", access->ppr_become_user ? "yes" : "no"));
    DODEBUG_CONF(("other become user = %s", access->other_become_user ? "yes" : "no"));
    DODEBUG_CONF(("ppr root as = \"%s\"", access->ppr_root_as));
    DODEBUG_CONF(("other root as = \"%s\"", access->other_root_as));
    DODEBUG_CONF(("ppr proxy user = \"%s\"", access->ppr_proxy_user));
    DODEBUG_CONF(("other proxy user = \"%s\"", access->other_proxy_user));
    DODEBUG_CONF(("ppr proxy class = \"%s\"", access->ppr_proxy_class));
    DODEBUG_CONF(("ppr user format = \"%s\"", access->ppr_from_format));
    } /* end of get_access_settings() */

/*
** Convert a user name to a user id number.
*/
static int username_to_uid(const char username[], uid_t *uid)
    {
    struct passwd *pw;

    if((pw = getpwnam(username)) == (struct passwd *)NULL)
	return -1;

    *uid = pw->pw_uid;
    return 0;
    }

/*
** Based on the ACCESS_INFO structure, hostname, and queuename
** supplied, set uid_to_use and proxy_class to the correct values.
**
** Note that the values we set depend not only on the access
** information but on whether the queue is a PPR queue or belongs
** to some other spooling system.
*/
void get_proxy_identity(uid_t *uid_to_use, const char **proxy_class, const char fromhost[], const char *requested_user, gu_boolean is_ppr_queue, const struct ACCESS_INFO *access_info)
    {
    const char *proxy_user = is_ppr_queue ? access_info->ppr_proxy_user : access_info->other_proxy_user;

    if(is_ppr_queue ? access_info->ppr_become_user : access_info->other_become_user)
    	{
	const char *user = requested_user;

	if(strcmp(user, "root") == 0)
	    {
	    if(is_ppr_queue)
	    	user = access_info->ppr_root_as;
	    else
	    	user = access_info->other_root_as;
	    }

	if(username_to_uid(user, uid_to_use) == -1)
	    {
	    if(user != requested_user)
	    	fatal(1, "Root substitute user \"%s\" does not exist", user);
	    user = proxy_user;
	    }

	/* If we used root substitute or non-proxy user,
	   we are done. */
	if(user != proxy_user) return;
	}

    if(username_to_uid(proxy_user, uid_to_use) == -1)
	fatal(1, "Proxy user \"%s\" does not exist", proxy_user);

    if(strcmp(access_info->ppr_proxy_class, "$cname") == 0)
    	*proxy_class = fromhost;
    else
    	*proxy_class = access_info->ppr_proxy_class;
    } /* get_proxy_identity() */

/* end of file */

