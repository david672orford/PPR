/*
** mouse:~ppr/src/libppr/acl.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 9 February 2000.
*/

#include "before_system.h"
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
    FILE *f;
    char fname[MAX_PPR_PATH];
    char line[256];
    ppr_fnamef(fname, "%s/%s.allow", ACLDIR, acl);
    if((f = fopen(fname, "r")))
	{
	while(fgets(line, sizeof(line), f))
	    {
	    /* Skip comments. */
	    if(line[0] == '#' || line[0] == ';')
	    	continue;

	    /* Trim trailing whitespace. */
	    line[strcspn(line, " \t\r\n")] = '\0';

	    /* Does it match? */
	    if(strcmp(line, user) == 0)
	    	{
	    	fclose(f);
	    	return TRUE;
	    	}
	    }
	fclose(f);
	}
    }

    /* For compability with versions of PPR before 1.32,
       see if the user is a member of a group named
       for the ACL. */
    {
    struct group *gr;
    int x;
    char *ptr;

    if((gr = getgrnam(acl)) != (struct group *)NULL)
	{
	x = 0;
	while((ptr = gr->gr_mem[x++]) != (char*)NULL)
	    {
    	    if(strcmp(ptr, user) == 0)
		{
		fputs(X_("Warning: your privledges are granted by membership in a Unix\n"
			"group, which is deprecated.  Please use the new ACLs.\n"), stderr);
    	    	return TRUE;
    	    	}
	    }
	}
    }

    return FALSE;
    } /* end of user_acl_allows() */

/* end of file */
