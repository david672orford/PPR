/*
** mouse:~ppr/src/ppr/ppr_conffile.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 4 September 2001.
*/

/*
** Find and parse the group or printer configuration file.  The first
** time this is called we will cache the information so that we
** do not have to perform expensive disk accesses to get other
** lines from the file.
**
** This module now has support for aliases.  An alias can override the
** passthru and switchset of the thing it points to but not the default
** filter options or ACL list.
*/

#include "before_system.h"
#include <stdio.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "ppr.h"
#include "ppr_exits.h"
#include "ppr_conffile.h"

static int information_cached = FALSE;
static const char *switchset;
static const char *deffiltopts;
static const char *passthru;
static const char *forwhat;
static const char *acls;

/*
** First, we have a routine which finds the file and extracts
** the information.
**
** To understand this code you must know that aliases may
** not have "Deffiltopts:" lines, for them we read it from the
** underlying queue.
*/
static void cache_info(void)
    {
    switchset = deffiltopts = passthru = forwhat = acls = NULL;

    /* If it is being submitted to a local queue, */
    if(!qentry.destnode || strcmp(qentry.destnode, qentry.homenode) == 0)
	{
        char fname[MAX_PPR_PATH];
        FILE *f;
        int len = 128;
        char *line = NULL;
	const char *look_for_name;

	/* Look for an alias first */
	ppr_fnamef(fname, "%s/%s", ALIASCONF, qentry.destname);
	if((f = fopen(fname, "r")))
            {
            while((line = gu_getline(line, &len, f)))
                {
                if(!switchset && gu_sscanf(line, "Switchset: %Z", &switchset) == 1)
                    continue;
                if(!passthru && gu_sscanf(line, "PassThru: %Z", &passthru) == 1)
                    continue;
                if(!forwhat && gu_sscanf(line, "ForWhat: %S", &forwhat) == 1)
                    continue;
                }
	    fclose(f);

	    if(!forwhat)
		fatal(PPREXIT_OTHERERR, "The alias \"%s\" has no \"ForWhat:\" line", qentry.destname);
            }

	/* If we found an alias, look for the group or printer it points to. */
        look_for_name = forwhat ? forwhat : qentry.destname;

        /* if not, then try a group */
        ppr_fnamef(fname, "%s/%s", GRCONF, look_for_name);
        if(!(f = fopen(fname, "r")))
            {
            /* if not, then try a printer */
            ppr_fnamef(fname, "%s/%s", PRCONF, look_for_name);
            f = fopen(fname, "r");
            }

        /*
        ** If a file was found, use it.  Here is not the place
        ** to worry about groups and printers which do not exist.
        */
        if(f)
            {
            while((line = gu_getline(line, &len, f)))
                {
		if(!forwhat)
		    {
		    if(!switchset && gu_sscanf(line, "Switchset: %Z", &switchset) == 1)
                    	continue;
		    if(!passthru && gu_sscanf(line, "PassThru: %Z", &passthru) == 1)
			continue;
		    if(!acls && gu_sscanf(line, "ACLs: %Z", &acls) == 1)
			continue;
		    }
                if(!deffiltopts && gu_sscanf(line, "DefFiltOpts: %Z", &deffiltopts) == 1)
                    continue;
                }
            fclose(f);
            }
	}

    /* If a remote queue, */
    else
	{
	fprintf(stderr, "Note: default filter options, switchsets, and passthru not implemented\n"
			"for remote queues.\n");
	}

    information_cached = TRUE;
    } /* end of cache_info() */

/*
** Extract the value from the "Switchset:" line.  If there is none, return
** a NULL pointer.
*/
const char *extract_switchset(void)
    {
    if( ! information_cached )
    	cache_info();
    return switchset;
    } /* end of extract_switchset() */

/*
** Extract the "DefFileOpts:" line.  If we find none, we will return
** a NULL pointer.
*/
const char *extract_deffiltopts(void)
    {
    if( ! information_cached )
	cache_info();
    return deffiltopts;
    } /* end of extract_deffiltopts() */

/*
** Extract the "PassThru:" line.  If there is none,
** we will return a NULL pointer.
*/
const char *extract_passthru(void)
    {
    if( ! information_cached )
	cache_info();
    return passthru;
    } /* end of extract_passthru() */

/*
** If it is an alias, return the name of the queue it is an alias for.
** Notice that in this case we return NULL if there is no ForWhat.
*/
const char *extract_forwhat(void)
    {
    if( ! information_cached )
	cache_info();
    return forwhat;
    }

/*
** Extract the "ACLs: line.  If there is none, we will return a NULL pointer.
*/
const char *extract_acls(void)
    {
    if( ! information_cached )
	cache_info();
    return acls;
    }

/* end of file */
