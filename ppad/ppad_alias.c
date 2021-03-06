/*
** mouse:~ppr/src/ppad/ppad_alias.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 25 April 2006.
*/

/*==============================================================
** This module is part of the administrators utility.  It 
** contains the code for those sub-commands which manipulate 
** printer aliases.
<helptopic>
	<name>alias</name>
	<desc>all settings for aliases</desc>
</helptopic>
==============================================================*/
#include "config.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "ppad.h"
#include "util_exits.h"
#include "dispatch_table.h"

/*
<command helptopics="alias">
	<name><word>alias</word><word>show</word></name>
	<desc>show configuration of <arg>alias</arg></desc>
	<args>
		<arg><name>alias</name><desc>alias to show</desc></arg>
	</args>
</command>
*/
int command_alias_show(const char *argv[])
	{
	const char *alias = argv[0];
	struct CONF_OBJ *obj;
	char *line, *p;
	char *comment = NULL, *forwhat = NULL, *switchset = NULL, *passthru = NULL;

	if(!(obj = conf_open(QUEUE_TYPE_ALIAS, alias, CONF_ENOENT_PRINT)))
		return EXIT_BADDEST;

	while((line = conf_getline(obj)))
		{
		if(gu_sscanf(line, "ForWhat: %S", &p) == 1)
			{
			gu_free_if(forwhat);
			forwhat = p;
			}
		else if(gu_sscanf(line, "Comment: %T", &p) == 1)
			{
			gu_free_if(comment);
			comment = p;
			}
		else if(gu_sscanf(line, "Switchset: %T", &p) == 1)
			{
			gu_free_if(switchset);
			switchset = p;
			}
		else if(gu_sscanf(line, "PassThru: %T", &p) == 1)
			{
			gu_free_if(passthru);
			passthru = p;
			}
		}

	conf_close(obj);

	if(! machine_readable)
		{
		gu_utf8_printf(_("Alias name: %s\n"), alias);
		gu_utf8_printf(_("Comment: %s\n"), comment ? comment : "");
		gu_utf8_printf(_("For what: %s\n"), forwhat ? forwhat : "<missing>");
		gu_utf8_puts(_("Switchset: "));
			if(switchset)
				print_switchset(switchset);
			gu_putwc('\n');
		gu_utf8_printf(_("PassThru types: %s\n"), passthru ? passthru : "");
		}
	else
		{
		gu_utf8_printf("name\t%s\n", alias);
		gu_utf8_printf("comment\t%s\n", comment ? comment : "");
		gu_utf8_printf("forwhat\t%s\n", forwhat ? forwhat : "");
		gu_utf8_puts("switchset\t");
			if(switchset)
				print_switchset(switchset);
			gu_putwc('\n');
		gu_utf8_printf("passthru\t%s\n", passthru ? passthru : "");
		}

	gu_free_if(comment);
	gu_free_if(forwhat);
	gu_free_if(switchset);
	gu_free_if(passthru);

	return EXIT_OK;
	} /* command_alias_show() */

/*
<command acl="ppad" helptopics="alias">
	<name><word>alias</word><word>copy</word></name>
	<desc>copy alias <arg>existing</arg> creating alias <arg>new</arg></desc>
	<args>
		<arg><name>existing</name><desc>name of existing alias</desc></arg>
		<arg><name>new</name><desc>name of new alias</desc></arg>
	</args>
</command>
*/
int command_alias_copy(const char *argv[])
	{
	return conf_copy(QUEUE_TYPE_PRINTER, argv[0], argv[1]);
	} /* command_alias_copy() */

/*
<command acl="ppad" helptopics="alias">
	<name><word>alias</word><word>forwhat</word></name>
	<desc>modify the target of an alias or create a new alias</desc>
	<args>
		<arg><name>alias</name><desc>name of new or existing alias</desc></arg>
		<arg><name>for</name><desc>new target of alias</desc></arg>
	</args>
</command>
*/
int command_alias_forwhat(const char *argv[])
	{
	const char *alias = argv[0];
	const char *forwhat = argv[1];
	struct CONF_OBJ *obj;
	char *line;

	if(strpbrk(alias, DEST_DISALLOWED))
		{
		gu_utf8_fputs(_("Alias name contains a disallowed character.\n"), stderr);
		return EXIT_SYNTAX;
		}

	if(strchr(DEST_DISALLOWED_LEADING, (int)alias[0]))
		{
		gu_utf8_fputs(_("Alias name begins with a disallowed character.\n"), stderr);
		return EXIT_SYNTAX;
		}

	/* Make sure the preposed forwhat exists. */
	if(!(obj = conf_open(QUEUE_TYPE_GROUP, forwhat, 0)) && !(obj = conf_open(QUEUE_TYPE_PRINTER, forwhat, 0)))
		{
		gu_utf8_fprintf(stderr, _("The name \"%s\" is not that of an existing group or printer.\n"), forwhat);
		return EXIT_BADDEST;
		}
	conf_close(obj);

	if(!(obj = conf_open(QUEUE_TYPE_ALIAS, alias, CONF_MODIFY | CONF_CREATE)))
		return EXIT_INTERNAL;

	while((line = conf_getline(obj)) && !lmatch(line, "ForWhat:"))
		conf_printf(obj, "%s\n", line);

	conf_printf(obj, "ForWhat: %s\n", forwhat);

	while((line = conf_getline(obj)))
		{
		if(!lmatch(line, "ForWhat:"))
			conf_printf(obj, "%s\n", line);
		}

	conf_close(obj);

	return EXIT_OK;
	} /* command_alias_forwhat() */

/*
<command acl="ppad" helptopics="alias">
	<name><word>alias</word><word>delete</word></name>
	<desc>delete an alias</desc>
	<args>
		<arg><name>alias</name><desc>name of alias to be deleted</desc></arg>
	</args>
</command>
*/
int command_alias_delete(const char *argv[])
	{
	const char *alias = argv[0];
	char fname[MAX_PPR_PATH];

	ppr_fnamef(fname, "%s/%s", ALIASCONF, alias);
	if(unlink(fname))
		{
		if(errno==ENOENT)
			{
			gu_utf8_fprintf(stderr, _("The alias \"%s\" does not exist.\n"), alias);
			return EXIT_BADDEST;
			}
		else
			{
			gu_utf8_fprintf(stderr, "unlink(\"%s\") failed, errno=%d (%s)\n", fname, errno, gu_strerror(errno));
			return EXIT_INTERNAL;
			}
		}

	return EXIT_OK;
	} /* command_alias_delete() */

/*
<command acl="ppad" helptopics="alias">
	<name><word>alias</word><word>comment</word></name>
	<desc>modify an alias's comment field</desc>
	<args>
		<arg><name>alias</name><desc>name of alias to be modified</desc></arg>
		<arg flags="optional"><name>comment</name><desc>comment to attach (ommit to delete)</desc></arg>
	</args>
</command>
*/
int command_alias_comment(const char *argv[])
	{
	return conf_set_name(QUEUE_TYPE_ALIAS, argv[0], 0, "Comment", argv[1] ? "%s" : NULL, argv[1]);
	} /* command_alias_comment() */

/*
<command acl="ppad" helptopics="alias">
	<name><word>alias</word><word>switchset</word></name>
	<desc>attach a set of switches to an alias</desc>
	<args>
		<arg><name>alias</name><desc>name of alias to be modified</desc></arg>
		<arg flags="optional,repeat"><name>switchset</name><desc>switches to attach (ommit to delete list)</desc></arg>
	</args>
</command>
*/
int command_alias_switchset(const char *argv[])
	{
	const char *alias = argv[0];
	char newset[256];

	/* convert the switch set to a line */
	if(make_switchset_line(newset, &argv[1]))
		{
		gu_utf8_fputs(_("Bad set of switches.\n"), stderr);
		return EXIT_SYNTAX;
		}

	return conf_set_name(QUEUE_TYPE_ALIAS, alias, 0, "Switchset", newset[0] ? "%s" : NULL, newset);
	} /* command_alias_switchset() */

/*
<command acl="ppad" helptopics="alias">
	<name><word>alias</word><word>passthru</word></name>
	<desc>set an alias's passthru language list</desc>
	<args>
		<arg><name>alias</name><desc>name of alias to be modified</desc></arg>
		<arg flags="optional,repeat"><name>languages</name><desc>languages to pass thru (ommit to delete list)</desc></arg>
	</args>
</command>
*/
int command_alias_passthru(const char *argv[])
	{
	const char *alias = argv[0];
	char *passthru;
	int retval;

	passthru = list_to_string(&argv[1]);
	retval = conf_set_name(QUEUE_TYPE_ALIAS, alias, 0, "PassThru", passthru ? "%s" : NULL, passthru);
	gu_free_if(passthru);
	return retval;
	} /* command_alias_passthru() */

/*
<command acl="ppad" helptopics="alias">
	<name><word>alias</word><word>addon</word></name>
	<desc>set alias parameters for use by a PPR extension</desc>
	<args>
		<arg><name>alias</name><desc>name of alias to be modified</desc></arg>
		<arg><name>param</name><desc>addon parameter to modify</desc></arg>
		<arg flags="optional"><name>value</name><desc>new value for <arg>param</arg> (ommit to delete)</desc></arg>
	</args>
</command>
*/
int command_alias_addon(const char *argv[])
	{
	const char *alias = argv[0];
	const char *name = argv[1];
	const char *value = argv[2];

	if(!(name[0] >= 'a' && name[0] <= 'z'))
		{
		gu_utf8_fputs(_("Addon parameter names must begin with a lower-case ASCII letter.\n"), stderr);
		return EXIT_SYNTAX;
		}

	return conf_set_name(QUEUE_TYPE_ALIAS, alias, 0, name, value ? "%s" : NULL, value);
	} /* command_alias_addon() */

/* end of file */

