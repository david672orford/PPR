/*
** mouse:~ppr/src/ppad/ppad_alias.c
** Copyright 1995--2002, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 19 April 2002.
*/

#include "before_system.h"
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
    char *ptr;
    char *comment = NULL, *forwhat = NULL, *switchset = NULL, *passthru = NULL;

    if(!alias)
    	{
    	fputs(_("You must supply the name of an alias to show.\n"), errors);
    	return EXIT_SYNTAX;
    	}

    if(confopen(QUEUE_TYPE_ALIAS, alias, FALSE) == -1)
	{
	fprintf(errors, _("The alias \"%s\" does not exist.\n"), alias);
	return EXIT_BADDEST;
	}

    while(confread())
	{
	if(gu_sscanf(confline, "ForWhat: %S", &ptr) == 1)
	    {
	    if(forwhat) gu_free(forwhat);
	    forwhat = ptr;
	    }
	else if(gu_sscanf(confline, "Comment: %Z", &ptr) == 1)
	    {
	    if(comment) gu_free(comment);
	    comment = ptr;
	    }
	else if(gu_sscanf(confline, "Switchset: %Z", &ptr) == 1)
	    {
	    if(switchset) gu_free(switchset);
	    switchset = ptr;
	    }
	else if(gu_sscanf(confline, "PassThru: %Z", &ptr) == 1)
	    {
	    if(passthru) gu_free(passthru);
	    passthru = ptr;
	    }
	}

    confclose();

    if(! machine_readable)
	{
	printf(_("Alias name: %s\n"), alias);
	printf(_("Comment: %s\n"), comment ? comment : "");
	printf(_("For what: %s\n"), forwhat ? forwhat : "<missing>");
	PUTS(_("Switchset: ")); if(switchset) print_switchset(switchset); putchar('\n');
	printf(_("PassThru types: %s\n"), passthru ? passthru : "");
	}
    else
	{
	printf("name\t%s\n", alias);
	printf("comment\t%s\n", comment ? comment : "");
	printf("forwhat\t%s\n", forwhat ? forwhat : "");
	PUTS("switchset\t"); if(switchset) print_switchset(switchset); putchar('\n');
	printf("passthru\t%s\n", passthru ? passthru : "");
	}

    if(comment) gu_free(comment);
    if(forwhat) gu_free(forwhat);
    if(switchset) gu_free(switchset);
    if(passthru) gu_free(passthru);

    return EXIT_OK;
    }

int alias_forwhat(const char *argv[])
    {
    const char *alias = argv[0];
    const char *forwhat = argv[1];

    if( ! am_administrator() )
    	return EXIT_DENIED;

    if(!alias || !forwhat)
	{
	fputs(_("You must supply the name of a new or existing alias and\n"
		"the name of the queue it is to be an alias for.\n"), errors);
        return EXIT_SYNTAX;
	}

    if(strlen(alias) > MAX_DESTNAME)
    	{
    	fputs(_("Alias name is too long.\n"), errors);
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
    if(confopen(QUEUE_TYPE_GROUP, forwhat, FALSE) == -1 && confopen(QUEUE_TYPE_PRINTER, forwhat, FALSE) == -1)
    	{
	fprintf(errors, _("The name \"%s\" is not that of an existing group or printer.\n"), forwhat);
	return EXIT_BADDEST;
    	}
    confclose();

    /* If we can't open an old one, create a new one. */
    if(confopen(QUEUE_TYPE_ALIAS, alias, TRUE) == -1)
	{
	FILE *newconf;
	char fname[MAX_PPR_PATH];

	ppr_fnamef(fname, "%s/%s", ALIASCONF, alias);
	if((newconf = fopen(fname, "w")) == (FILE*)NULL)
	    {
	    fprintf(errors, _("Failed to create alias config file \"%s\", errno=%d (%s).\n"), fname, errno, gu_strerror(errno));
	    return EXIT_INTERNAL;
	    }

	fprintf(newconf, "ForWhat: %s\n", forwhat);

	fclose(newconf);
	}

    /* Modify an old alias. */
    else
	{
        while(confread() && !lmatch(confline, "ForWhat:"))
            {
            conf_printf("%s\n", confline);
            }

        conf_printf("ForWhat: %s\n", forwhat);

        while(confread())
            {
            if(!lmatch(confline, "ForWhat:"))
                conf_printf("%s\n", confline);
            }

        confclose();
        }

    return EXIT_OK;
    }

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
    }

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

    return conf_set_name(QUEUE_TYPE_ALIAS, alias, "Comment", "%s", comment);
    }

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

    return conf_set_name(QUEUE_TYPE_ALIAS, alias, "Switchset", newset[0] ? "%s" : NULL, newset);
    }

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
    retval = conf_set_name(QUEUE_TYPE_ALIAS, alias, "PassThru", passthru ? "%s" : NULL, passthru);
    if(passthru) gu_free(passthru);

    return retval;
    }

/*
** Set an alias addon option.
*/
int alias_addon(const char *argv[])
    {
    const char *alias = argv[0];
    const char *name = argv[1];
    const char *value = argv[2];

    if(!alias || !name)
    	{
	fputs(_("You must supply the name of an existing alias, the name of an addon\n"
		"parameter.  A value for the paremeter is optional.  If you do not\n"
		"supply a value, the parameter will be unset.\n"), errors);
	return EXIT_SYNTAX;
    	}

    if(!(name[0] >= 'a' && name[0] <= 'z'))
    	{
	fputs(_("Addon parameter names must begin with a lower-case ASCII\n"
		"letter.\n"), errors);
	return EXIT_SYNTAX;
    	}

    return conf_set_name(QUEUE_TYPE_ALIAS, alias, name, value ? "%s" : NULL, value);
    }

/* end of file */

