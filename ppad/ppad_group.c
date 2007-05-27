/*
** mouse:~ppr/src/ppad/ppad_group.c
** Copyright 1995--2007, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 27 May 2007.
*/

/*==============================================================
** This module is part of the administrators utility.  It 
** contains the code for those sub-commands which manipulate 
** groups of printers.
<helptopic>
	<name>group</name>
	<desc>all settings for groups</desc>
</helptopic>
==============================================================*/

#include "config.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "util_exits.h"
#include "queueinfo.h"
#include "ppad.h"
#include "dispatch_table.h"

/*
<command helptopics="group">
	<name><word>group</word><word>show</word></name>
	<desc>show configuration of <arg>group</arg></desc>
	<args>
		<arg><name>group</name><desc>group to show</desc></arg>
	</args>
</command>
*/
int command_group_show(const char *argv[])
	{
	const char *function = "group_show";
	const char *group = argv[0];		/* The name of the group to show is the argument. */
	
	struct CONF_OBJ *obj;
	char *line, *ptr;
	int x;

	int rotate = TRUE;					/* Is rotate set for this group? */
	char *comment = (char*)NULL;		/* Group comment. */
	int member_count = 0;				/* Keep count of members. */
	char *members[MAX_GROUPSIZE];		/* The names of the members. */
	char *deffiltopts = (char*)NULL;	/* The default filter options string. */
	char *switchset = (char*)NULL;		/* The compressed switchset string. */
	char *passthru = (char*)NULL;
	char *acls = (char*)NULL;
	#define MAX_ADDONS 32
	char *addon[MAX_ADDONS];
	int addon_count = 0;

	if(!(obj = conf_open(QUEUE_TYPE_GROUP, group, CONF_ENOENT_PRINT)))
		return EXIT_BADDEST;

	while((line = conf_getline(obj)))			/* Read all the lines in the config file. */
		{
		if((ptr = lmatchp(line, "Rotate:")))
			{
			if(gu_torf_setBOOL(&rotate,ptr) == -1)
				gu_utf8_fprintf(stderr, _("WARNING: invalid \"%s\" setting: %s\n"), "Rotate", ptr);
			continue;
			}
		if(gu_sscanf(line, "Comment: %T", &ptr) == 1)
			{
			gu_free_if(comment);
			comment = ptr;
			continue;
			}
		if(gu_sscanf(line, "Printer: %S", &ptr) == 1)
			{
			if(member_count < MAX_GROUPSIZE)
				{
				members[member_count++] = ptr;
				}
			else
				{
				gu_utf8_fprintf(stderr, "%s(): too many members: %s\n", function, ptr);
				gu_free(ptr);
				}
			continue;
			}
		if(gu_sscanf(line, "Switchset: %T", &ptr) == 1)
			{
			gu_free_if(switchset);
			switchset = ptr;
			continue;
			}
		if(gu_sscanf(line, "DefFiltOpts: %T", &ptr) == 1)
			{
			gu_free_if(deffiltopts);
			deffiltopts = ptr;
			continue;
			}
		if(gu_sscanf(line, "PassThru: %T", &ptr) == 1)
			{
			gu_free_if(passthru);
			passthru = ptr;
			continue;
			}
		if(gu_sscanf(line, "ACLs: %T", &ptr) == 1)
			{
			gu_free_if(acls);
			acls = ptr;
			continue;
			}
		if(line[0] >= 'a' && line[0] <= 'z')	/* if in addon name space */
			{
			if(addon_count >= MAX_ADDONS)
				gu_utf8_fprintf(stderr, "%s(): addon[] overflow\n", function);
			else
				addon[addon_count++] = gu_strdup(line);
			continue;
			}


		} /* end of line reading while() loop */

	conf_close(obj);		/* We are done with the configuration file. */

	if(!machine_readable)
		{
		gu_utf8_printf(_("Group name: %s\n"), group);

		gu_utf8_printf(_("Comment: %s\n"), comment ? comment : "");

		gu_utf8_puts(_("Members:"));
		for(x=0;x<member_count;x++)		/* Show what printers are members. */
			{
			if(x==0)
				gu_utf8_printf(" %s", members[x]);
			else
				gu_utf8_printf(", %s", members[x]);
			}
		gu_putwc('\n');					/* End group members line. */

		gu_utf8_printf(_("Rotate: %s\n"), rotate ? _("True") : _("False"));

		{
		const char *s = _("Default Filter Options: ");
		gu_utf8_puts(s);
		if(deffiltopts)
			print_wrapped(deffiltopts, strlen(s));
		gu_putwc('\n');
		}

		gu_utf8_puts(_("Switchset: "));
		if(switchset)
			print_switchset(switchset);
		gu_putwc('\n');

		/* This rare parameter is shown only when it has a value. */
		if(passthru)
			gu_utf8_printf(_("PassThru types: %s\n"), passthru);

		/* Only displayed if set. */
		if(acls)
			gu_utf8_printf(_("ACLs: %s\n"), acls);

		/* Print the assembed addon settings. */
		if(addon_count > 0)
			{
			int x;
			gu_utf8_puts(_("Addon:"));
			for(x = 0; x < addon_count; x++)
				{
				gu_utf8_printf("\t%s\n", addon[x]);
				gu_free(addon[x]);
				}
			}

		}
	else
		{
		gu_utf8_printf("name\t%s\n", group);
		gu_utf8_printf("comment\t%s\n", comment ? comment : "");
		gu_utf8_puts("members\t");
		for(x=0; x < member_count; x++)
			gu_utf8_printf("%s%s", x > 0 ? " " : "", members[x]);
		gu_utf8_puts("\n");
		gu_utf8_printf("rotate\t%s\n", rotate ? "yes" : "no");
		gu_utf8_printf("deffiltopts\t%s\n", deffiltopts ? deffiltopts : "");
		gu_utf8_puts("switchset\t");
			if(switchset)
				print_switchset(switchset);
		   	gu_putwc('\n');
		gu_utf8_printf("passthru\t%s\n", passthru ? passthru : "");
		gu_utf8_printf("acls\t%s\n", acls ? acls : "");

		/* Addon lines */
		if(addon_count > 0)
			{
			int x;
			char *p;
			for(x = 0; x < addon_count; x++)
				{
				if((p = strchr(addon[x], ':')))
					{
					*p = '\0';
					p++;
					p += strspn(p, " \t");
					gu_utf8_printf("addon %s\t%s\n", addon[x], p);
					}
				else
					{
					gu_utf8_printf("addon\t%s\n", addon[x]);
					}
				gu_free(addon[x]);
				}
			}

		}

	gu_free_if(comment);
	for(x=0;x<member_count;x++)
		gu_free(members[x]);
	gu_free_if(deffiltopts);
	gu_free_if(switchset);
	gu_free_if(passthru);
	gu_free_if(acls);

	return EXIT_OK;
	} /* command_group_show() */

/*
<command acl="ppad" helptopics="group">
	<name><word>group</word><word>copy</word></name>
	<desc>copy group <arg>existing</arg> creating group <arg>new</arg></desc>
	<args>
		<arg><name>existing</name><desc>name of existing group</desc></arg>
		<arg><name>new</name><desc>name of new group</desc></arg>
	</args>
</command>
*/
int command_group_copy(const char *argv[])
	{
	return conf_copy(QUEUE_TYPE_GROUP, argv[0], argv[1]);
	} /* command_group_copy() */

/*
<command acl="ppad" helptopics="group">
	<name><word>group</word><word>comment</word></name>
	<desc>modify a group's comment field</desc>
	<args>
		<arg><name>group</name><desc>name of group to be modified</desc></arg>
		<arg flags="optional"><name>comment</name><desc>new comment (ommit to delete)</desc></arg>
	</args>
</command>
*/
int command_group_comment(const char *argv[])
	{
	return conf_set_name(QUEUE_TYPE_GROUP, argv[0], 0, "Comment", argv[1] ? "%s" : NULL, argv[1]);
	} /* group_comment() */

/*
<command acl="ppad" helptopics="group">
	<name><word>group</word><word>rotate</word></name>
	<desc>enable or disable rotational printer use</desc>
	<args>
		<arg><name>group</name><desc>name of group to be modified</desc></arg>
		<arg><name>rotate</name><desc>True for rotate mode</desc></arg>
	</args>
</command>
*/
int command_group_rotate(const char *argv[])
	{
	const char *group = argv[0];
	gu_boolean newstate;

	if(gu_torf_setBOOL(&newstate,argv[1]) == -1)
		{
		gu_utf8_fputs(_("Value must be \"true\" or \"false\".\n"), stderr);
		return EXIT_SYNTAX;
		}

	return conf_set_name(QUEUE_TYPE_GROUP, group, CONF_RELOAD, "Rotate", "%s", newstate ? "True" : "False");
	} /* group_rotate() */

static int group_members_or_add_internal(gu_boolean do_add, const char *argv[])
	{
	const char *group = argv[0];
	struct CONF_OBJ *obj;
	char *line;
	int x;
	char *ptr;
	int count = 0;
	void *qobj = NULL;

	if(strpbrk(group, DEST_DISALLOWED))
		{
		gu_utf8_fputs(_("Group name contains a disallowed character.\n"), stderr);
		return EXIT_SYNTAX;
		}

	if(strchr(DEST_DISALLOWED_LEADING, (int)group[0]))
		{
		gu_utf8_fputs(_("Group name begins with a disallowed character.\n"), stderr);
		return EXIT_SYNTAX;
		}

	/*
	** Make sure the proposed new members really exist.
	** We do this by trying to open their configuration files
	** with prnopen().
	*/
	for(x=1; argv[x]; x++)
		{
		if(!(obj = conf_open(QUEUE_TYPE_PRINTER, argv[x], CONF_ENOENT_PRINT)))
			return EXIT_BADDEST;
		conf_close(obj);
		}

	/*
	** If the group to which we are adding a printer
	** does not exist, create it.
	*/
	if(!(obj = conf_open(QUEUE_TYPE_GROUP, group, CONF_MODIFY | CONF_CREATE | CONF_RELOAD)))
		return EXIT_INTERNAL;

	/* Copy up to but not including the 1st "Printer:" line. */
	while((line = conf_getline(obj)))
		{
		if(lmatch(line, "DefFiltOpts:"))		/* discard */
			continue;
		if(lmatch(line, "Printer:"))			/* stop */
			break;
		conf_printf(obj, "%s\n", line);			/* copy */
		}

	gu_Try
		{
		qobj = queueinfo_new(QUEUEINFO_GROUP, group);
		queueinfo_set_warnings_file(qobj, stderr);
		queueinfo_set_debug_level(qobj, debug_level);

		/* Copy all the remaining lines. */
		for( ; line; line = conf_getline(obj))
			{
			if((ptr = lmatchp(line, "Printer:")))
				{
				if(!do_add)				/* If we are adding, just delete it. */
					continue;

				for(x=1; argv[x]; x++)	/* Is this the same as one we are adding? */
					{
					if(strcmp(ptr, argv[x]) == 0)
						gu_CodeThrow(EXIT_ALREADY, _("Printer \"%s\" is already a member of \"%s\".\n"), argv[x], group);
					queueinfo_add_printer(qobj, ptr);
					count++;
					}
				}

			/* Delete old "DefFiltOpts:" lines as we go. */
			else if(lmatch(line, "DefFiltOpts:"))
				continue;

			/* Other lines we keep. */
			conf_printf(obj, "%s\n", line);
			}

		/* Add a "Printer:" line for each new member. */
		for(x=1; argv[x]; x++)
			{
			conf_printf(obj, "Printer: %s\n", argv[x]);
			queueinfo_add_printer(qobj, argv[x]);
			count++;
			}

		/* Emmit the new "DefFiltOpts:" line. */
		{
		const char *cp = queueinfo_computedDefaultFilterOptions(qobj);
		if(cp)
			conf_printf(obj, "DefFiltOpts: %s\n", cp);
		}

		/* See if adding our printer will make the group too big. */
		if(count > MAX_GROUPSIZE)
			gu_CodeThrow(EXIT_OVERFLOW, _("Group \"%s\" would have %d members, only %d are allowed.\n"), group, count, MAX_GROUPSIZE);

		/* Commit the changes. */
		conf_close(obj);
		}
	gu_Final
		{
		if(qobj)
			queueinfo_free(qobj);
		}
	gu_Catch
		{
		conf_abort(obj);		/* roll back the changes */
		gu_utf8_fprintf(stderr, "%s: %s\n", myname, gu_exception);
		return exception_to_exitcode(gu_exception_code);
		}

	return EXIT_OK;
	} /* end of group_members_or_add_internal() */

/*
<command acl="ppad" helptopics="group">
	<name><word>group</word><word>add</word></name>
	<desc>add <arg>members</arg> to a <arg>group</arg> (possibly creating <arg>group</arg>)</desc>
	<args>
		<arg><name>group</name><desc>name of group to create or modify</desc></arg>
		<arg flags="repeat"><name>members</name><desc>members to add to <arg>group</arg></desc></arg>
	</args>
</command>
*/
int command_group_add(const char *argv[])
	{
	return group_members_or_add_internal(TRUE, argv);
	} /* command_group_add() */

/*
<command acl="ppad" helptopics="group">
	<name><word>group</word><word>members</word></name>
	<desc>set members list of <arg>group</arg> to <arg>members</arg> (possibly creating <arg>group</arg></desc>
	<args>
		<arg><name>group</name><desc>name of group to create or modify</desc></arg>
		<arg flags="repeat"><name>member</name><desc>list of members for <arg>group</arg></desc></arg>
	</args>
</command>
*/
int command_group_members(const char *argv[])
	{
	return group_members_or_add_internal(FALSE, argv);
	} /* command_group_members() */

/*
** Remove a member from a group.  This is a separate function because
** it is called from ppad_printer.c when a printer is deleted and
** it is the member of a group.
**
** sucess:		EXIT_OK
** bad group:	EXIT_BADDEST
** bad member:	EXIT_NOTFOUND
*/
int group_remove_internal(const char *group, const char *member)
	{
	struct CONF_OBJ *obj;
	char *line, *p;
	int found = FALSE;
	void *qobj = NULL;

	if(!(obj = conf_open(QUEUE_TYPE_GROUP, group, CONF_MODIFY | CONF_RELOAD)))
		return EXIT_BADDEST;

	gu_Try
		{
		qobj = queueinfo_new(QUEUEINFO_GROUP, group);
		queueinfo_set_warnings_file(qobj, stderr);
		queueinfo_set_debug_level(qobj, debug_level);

		/*
		** Copy the configuration file.
		*/
		while((line = conf_getline(obj)))
			{
			if(lmatch(line, "DefFiltOpts:"))		/* delete old lines */
				continue;
	
			if((p = lmatchp(line, "Printer:")))			/* if member name, */
				{
				if(strcmp(p, member) == 0)				/* If it is the one we */
					{									/* are deleting, */
					found = TRUE;						/* set a flag */
					continue;							/* and don't copy. */
					}
				queueinfo_add_printer(qobj, p);			/* Otherwise, add its PPD file, */
				}
	
			conf_printf(obj, "%s\n", line);
			}

		/* Emmit the new "DefFiltOpts:" line. */
		{
		const char *cp = queueinfo_computedDefaultFilterOptions(qobj);
		if(cp)
			conf_printf(obj, "DefFiltOpts: %s\n", cp);
		}

		/* Commit the changes only if the printer was found. */
		if(found)
			conf_close(obj);
		else
			conf_abort(obj);
		}
	gu_Final
		{
		if(qobj)
			queueinfo_free(qobj);
		}
	gu_Catch
		{
		conf_abort(obj);
		gu_utf8_fprintf(stderr, "%s: %s\n", myname, gu_exception);
		return exception_to_exitcode(gu_exception_code);
		}

	if(found)
		return EXIT_OK;
	else
		return EXIT_NOTFOUND;
	} /* end of group_remove_internal() */

/*
<command acl="ppad" helptopics="group">
	<name><word>group</word><word>remove</word></name>
	<desc>remove <arg>member</arg> from <arg>group</arg></desc>
	<args>
		<arg><name>group</name><desc>name of existing group from which <arg>member</arg> is to be removed</desc></arg>
		<arg><name>member</name><desc>name of member to remove from <arg>group</arg></desc></arg>
	</args>
</command>
*/
int command_group_remove(const char *argv[])
	{
	const char function[] = "group_remove";
	const char *group = argv[0];
	int x;

	for(x=1; argv[x]; x++)
		{
		switch(group_remove_internal(group, argv[x]))
			{
			case EXIT_OK:				/* continue if no error yet */
				break;
			case EXIT_BADDEST:
				gu_utf8_fprintf(stderr, _("The group \"%s\" does not exist.\n"), group);
				return EXIT_BADDEST;
			case EXIT_NOTFOUND:
				gu_utf8_fprintf(stderr, _("The group \"%s\" does not have a member called \"%s\".\n"), group, argv[x]);
				return EXIT_NOTFOUND;
			default:
				gu_utf8_fprintf(stderr, "%s(): assertion failed\n", function);
				return EXIT_INTERNAL;
			}
		}

	return EXIT_OK;
	} /* end of group_remove() */

/*
<command acl="ppad" helptopics="group">
	<name><word>group</word><word>delete</word></name>
	<desc>delete <arg>group</arg></desc>
	<args>
		<arg><name>group</name><desc>group to delete</desc></arg>
	</args>
</command>
*/
int command_group_delete(const char *argv[])
	{
	const char *group = argv[0];
	char fname[MAX_PPR_PATH];

	/*
	** Use ppop commands to accept no more jobs
	** and cancel existing jobs.
	*/
	ppop2("reject", group);
	ppop2("purge", group);

	ppr_fnamef(fname, "%s/%s", GRCONF, group);
	if(unlink(fname))
		{
		if(errno==ENOENT)
			{
			gu_utf8_fprintf(stderr, _("The group \"%s\" does not exist.\n"), group);
			return EXIT_BADDEST;
			}
		else
			{
			gu_utf8_fprintf(stderr, "unlink(\"%s\") failed, errno=%d\n",fname,errno);
			return EXIT_INTERNAL;
			}
		}
	else
		{
		write_fifo("NG %s\n", group);
		return EXIT_OK;
		}
	} /* end of group_remove() */

/*
<command acl="ppad" helptopics="group">
	<name><word>group</word><word>touch</word></name>
	<desc>instruct pprd to reload <arg>group</arg></desc>
	<args>
		<arg><name>group</name><desc>group to reload</desc></arg>
	</args>
</command>
*/
int command_group_touch(const char *argv[])
	{
	const char *group = argv[0];
	struct CONF_OBJ *obj;

	if(!(obj = conf_open(QUEUE_TYPE_GROUP, group, CONF_ENOENT_PRINT | CONF_RELOAD)))
		return EXIT_BADDEST;
	conf_close(obj);

	return EXIT_OK;
	} /* end of group_touch() */

/*
<command acl="ppad" helptopics="group">
	<name><word>group</word><word>switchset</word></name>
	<desc>attach a set of switches to a group</desc>
	<args>
		<arg><name>group</name><desc>name of group to be modified</desc></arg>
		<arg flags="optional,repeat"><name>switchset</name><desc>switches to attach (ommit to delete list)</desc></arg>
	</args>
</command>
*/
int command_group_switchset(const char *argv[])
	{
	const char *group = argv[0];		/* name of group */
	char newset[256];					/* new set of switches */

	/* convert the switch set to a line */
	if(make_switchset_line(newset, &argv[1]))
		{
		gu_utf8_fputs(_("Bad set of switches.\n"), stderr);
		return EXIT_SYNTAX;
		}

	return conf_set_name(QUEUE_TYPE_GROUP, group, 0, "Switchset", newset[0] ? "%s" : NULL, newset);
	} /* end of group_switchset() */

/*
** Set the DefFiltOpts: line for a group.
*/
int group_deffiltopts_internal(const char *group)
	{
	struct CONF_OBJ *obj;
	char *line, *p;

	if(!(obj = conf_open(QUEUE_TYPE_GROUP, group, CONF_MODIFY | CONF_ENOENT_PRINT)))
		return EXIT_BADDEST;

	{
	void *qobj = NULL;
	gu_Try {
		qobj = queueinfo_new(QUEUEINFO_GROUP, group);
		queueinfo_set_warnings_file(qobj, stderr);
		queueinfo_set_debug_level(qobj, debug_level);
			
		/* Modify the group's configuration file. */
		while((line = conf_getline(obj)))
			{
			if(lmatch(line, "DefFiltOpts:"))
				continue;

			if((p = lmatchp(line, "Printer:")))
				queueinfo_add_printer(qobj, p);
	
			conf_printf(obj, "%s\n", line);
			}
	
		{
		const char *opts;
		if((opts = queueinfo_computedDefaultFilterOptions(qobj)))
			conf_printf(obj, "DefFiltOpts: %s\n", opts);
		}

		conf_close(obj);
		}
	gu_Final {
		if(qobj)
			queueinfo_free(qobj);
		}
	gu_Catch {
		conf_abort(obj);
		gu_utf8_fprintf(stderr, "%s: %s\n", myname, gu_exception);
		return exception_to_exitcode(gu_exception_code);
		}
	}

	return EXIT_OK;
	} /* end of group_deffiltopts_internal() */

/*
<command acl="ppad" helptopics="group">
	<name><word>group</word><word>deffiltopts</word></name>
	<desc>update a group's default filter options</desc>
	<args>
		<arg><name>group</name><desc>group to update</desc></arg>
	</args>
</command>
*/
int command_group_deffiltopts(const char *argv[])
	{
	const char *group = argv[0];		/* name of group */

	return group_deffiltopts_internal(group);
	} /* end of group_deffiltopts() */

/*
<command acl="ppad" helptopics="group">
	<name><word>group</word><word>passthru</word></name>
	<desc>set a group's passthru language list</desc>
	<args>
		<arg><name>group</name><desc>name of group to be modified</desc></arg>
		<arg flags="optional,repeat"><name>languages</name><desc>languages to pass thru (ommit to delete list)</desc></arg>
	</args>
</command>
*/
int command_group_passthru(const char *argv[])
	{
	const char *group = argv[0];
	char *passthru;
	int retval;
	passthru = list_to_string(&argv[1]);
	retval = conf_set_name(QUEUE_TYPE_GROUP, group, 0, "PassThru", passthru ? "%s" : NULL, passthru);
	gu_free_if(passthru);
	return retval;
	} /* end of group_passthru() */

/*
<command acl="ppad" helptopics="group">
	<name><word>group</word><word>acls</word></name>
	<desc>set list of ACLs listing those who can submit to <arg>group</arg></desc>
	<args>
		<arg><name>group</name><desc>name of group to be modified</desc></arg>
		<arg flags="optional,repeat"><name>acls</name><desc>list of ACLs (ommit to remove restrictions)</desc></arg>
	</args>
</command>
*/
int command_group_acls(const char *argv[])
	{
	const char *group = argv[0];
	char *acls;
	int retval;

	acls = list_to_string(&argv[1]);
	retval = conf_set_name(QUEUE_TYPE_GROUP, group, 0, "ACLs", acls ? "%s" : NULL, acls);
	gu_free_if(acls);
	return retval;
	} /* end of group_acls() */

/*
<command acl="ppad" helptopics="group">
	<name><word>group</word><word>addon</word></name>
	<desc>set group parameters for use by a PPR extension</desc>
	<args>
		<arg><name>group</name><desc>name of group to be modified</desc></arg>
		<arg><name>param</name><desc>addon parameter to modify</desc></arg>
		<arg flags="optional"><name>value</name><desc>new value for <arg>param</arg> (ommit to delete)</desc></arg>
	</args>
</command>
*/
int command_group_addon(const char *argv[])
	{
	const char *group = argv[0];
	const char *name = argv[1];
	const char *value = argv[2];

	if(!(name[0] >= 'a' && name[0] <= 'z'))
		{
		gu_utf8_fputs(_("Addon parameter names must begin with a lower-case ASCII letter.\n"), stderr);
		return EXIT_SYNTAX;
		}

	return conf_set_name(QUEUE_TYPE_GROUP, group, 0, name, (value && value[0]) ? "%s" : NULL, value);
	}

/* end of file */
