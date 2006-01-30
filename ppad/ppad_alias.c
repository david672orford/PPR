/*
** mouse:~ppr/src/ppad/ppad_alias.c
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
** Last modified 27 January 2006.
*/

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

int alias_show(const char *argv[])
	{
	const char *alias = argv[0];
	struct CONF_OBJ *obj;
	char *line, *p;
	char *comment = NULL, *forwhat = NULL, *switchset = NULL, *passthru = NULL;

	if(!alias)
		{
		fputs(_("You must supply the name of an alias to show.\n"), errors);
		return EXIT_SYNTAX;
		}

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
		printf(_("Alias name: %s\n"), alias);
		printf(_("Comment: %s\n"), comment ? comment : "");
		printf(_("For what: %s\n"), forwhat ? forwhat : "<missing>");
		gu_puts(_("Switchset: ")); if(switchset) print_switchset(switchset); putchar('\n');
		printf(_("PassThru types: %s\n"), passthru ? passthru : "");
		}
	else
		{
		printf("name\t%s\n", alias);
		printf("comment\t%s\n", comment ? comment : "");
		printf("forwhat\t%s\n", forwhat ? forwhat : "");
		gu_puts("switchset\t"); if(switchset) print_switchset(switchset); putchar('\n');
		printf("passthru\t%s\n", passthru ? passthru : "");
		}

	gu_free_if(comment);
	gu_free_if(forwhat);
	gu_free_if(switchset);
	gu_free_if(passthru);

	return EXIT_OK;
	}

int alias_copy(const char *argv[])
	{
	if( ! argv[0] || ! argv[1] )
		{
		fputs(_("You must supply the name of an existing printer and\n"
				"a name for the new printer.\n"), errors);
		return EXIT_SYNTAX;
		}
	return conf_copy(QUEUE_TYPE_PRINTER, argv[0], argv[1]);
	}

int alias_forwhat(const char *argv[])
	{
	const char *alias = argv[0];
	const char *forwhat = argv[1];
	struct CONF_OBJ *obj;
	char *line;

	if( ! am_administrator() )
		return EXIT_DENIED;

	if(!alias || !forwhat)
		{
		fputs(_("You must supply the name of a new or existing alias and\n"
				"the name of the queue it is to be an alias for.\n"), errors);
		return EXIT_SYNTAX;
		}

	if(strpbrk(alias, DEST_DISALLOWED))
		{
		fputs(_("Alias name contains a disallowed character.\n"), errors);
		return EXIT_SYNTAX;
		}

	if(strchr(DEST_DISALLOWED_LEADING, (int)alias[0]) != (char*)NULL)
		{
		fputs(_("Alias name begins with a disallowed character.\n"), errors);
		return EXIT_SYNTAX;
		}

	/* Make sure the preposed forwhat exists. */
	if(!(obj = conf_open(QUEUE_TYPE_GROUP, forwhat, 0)) && !(obj = conf_open(QUEUE_TYPE_PRINTER, forwhat, 0)))
		{
		fprintf(errors, _("The name \"%s\" is not that of an existing group or printer.\n"), forwhat);
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
	} /* alias_forwhat() */

int alias_delete(const char *argv[])
	{
	const char *alias = argv[0];
	char fname[MAX_PPR_PATH];

	if( ! am_administrator() )
		return EXIT_DENIED;

	if(!alias)
		{
		fputs("error!\n", errors);
		return EXIT_SYNTAX;
		}

	ppr_fnamef(fname, "%s/%s", ALIASCONF, alias);
	if(unlink(fname))
		{
		if(errno==ENOENT)
			{
			fprintf(errors, _("The alias \"%s\" does not exist.\n"), alias);
			return EXIT_BADDEST;
			}
		else
			{
			fprintf(errors, "unlink(\"%s\") failed, errno=%d (%s)\n", fname, errno, gu_strerror(errno));
			return EXIT_INTERNAL;
			}
		}

	return EXIT_OK;
	} /* alias_delete() */

int alias_comment(const char *argv[])
	{
	const char *alias = argv[0];
	const char *comment = argv[1];

	if( ! alias || ! comment )
		{
		fputs(_("You must supply the name of an existing alias and\n"
				"a comment to attach to it.\n"), errors);
		return EXIT_SYNTAX;
		}

	return conf_set_name(QUEUE_TYPE_ALIAS, alias, 0, "Comment", "%s", comment);
	} /* alias_comment() */

int alias_switchset(const char *argv[])
	{
	const char *alias = argv[0];
	char newset[256];

	if(!alias)
		{
		fputs(_("You must supply the name of an alias and a set of switches.\n"), errors);
		return EXIT_SYNTAX;
		}

	/* convert the switch set to a line */
	if(make_switchset_line(newset, &argv[1]))
		{
		fputs(_("Bad set of switches.\n"), errors);
		return EXIT_SYNTAX;
		}

	return conf_set_name(QUEUE_TYPE_ALIAS, alias, 0, "Switchset", newset[0] ? "%s" : NULL, newset);
	} /* alias_switchset() */

int alias_passthru(const char *argv[])
	{
	const char *alias = argv[0];
	char *passthru;
	int retval;

	if(!alias)
		{
		fputs(_("You must specify an alias and a (possibly empty) list\n"
				"of file types.  These file types should be the same as\n"
				"those used with the \"ppr -T\" option.\n"), errors);
		return EXIT_SYNTAX;
		}

	passthru = list_to_string(&argv[1]);
	retval = conf_set_name(QUEUE_TYPE_ALIAS, alias, 0, "PassThru", passthru ? "%s" : NULL, passthru);
	gu_free_if(passthru);
	return retval;
	} /* alias_passthru() */

/*
** Set an alias addon option.
*/
int alias_addon(const char *argv[])
	{
	const char *alias = argv[0];
	const char *name = argv[1];
	const char *value = argv[2];

	if(!alias || !name || (value && argv[3]))
		{
		fputs(_("You must supply the name of an existing alias, the name of an addon\n"
				"parameter.  A value for the parameter is optional.  If you do not\n"
				"supply a value, the parameter will be unset.\n"), errors);
		return EXIT_SYNTAX;
		}

	if(!(name[0] >= 'a' && name[0] <= 'z'))
		{
		fputs(_("Addon parameter names must begin with a lower-case ASCII letter.\n"), errors);
		return EXIT_SYNTAX;
		}

	return conf_set_name(QUEUE_TYPE_ALIAS, alias, 0, name, value ? "%s" : NULL, value);
	} /* alias_addon() */

/* end of file */

