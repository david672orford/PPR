/*
** mouse:~ppr/src/ppr/ppr_conffile.c
** Copyright 1995--2004, Trinity College Computing Center.
** Written by David Chappell
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
** Last modified 4 June 2004.
*/

/*
** Find and parse the group or printer configuration file.  The first
** time this is called we will cache the information so that we
** do not have to perform expensive disk accesses to get other
** lines from the file.
**
** This module has much in common with ../libppr/queueinfo.c.
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
static char *switchset;
static char *deffiltopts;
static char *passthru;
static char *forwhat;
static char *acls;

#define set_field(name,value) if(name) gu_free(name); name = gu_strdup(value)

/*
** First, we have a routine which finds the file and extracts
** the information.
**
** To understand this code you must know that aliases may
** not have "Deffiltopts:" lines, for them we read it from the
** underlying queue.
**
** This this function has support for aliases.  An alias doesn't inherit the
** passthru and switchset of the thing it points to it does inherit the default
** filter options and the ACLs.
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
		char *tempptr;
		const char *look_for_name;

		/* If  this is an alias, resolve it first. */
		ppr_fnamef(fname, "%s/%s", ALIASCONF, qentry.destname);
		if((f = fopen(fname, "r")))
			{
			while((line = gu_getline(line, &len, f)))
				{
				if(gu_sscanf(line, "Switchset: %Z", &tempptr) == 1)
					{
					set_field(switchset, tempptr);
					continue;
					}
				if(gu_sscanf(line, "PassThru: %Z", &tempptr) == 1)
					{
					set_field(passthru, tempptr);
					continue;
					}
				if(gu_sscanf(line, "ForWhat: %S", &tempptr) == 1)
					{
					set_field(forwhat, tempptr);
					continue;
					}
				}
			fclose(f);

			if(!forwhat)
				fatal(PPREXIT_OTHERERR, "The alias \"%s\" has no \"ForWhat:\" line", qentry.destname);
			}

		/* If we found an alias, look for the group or printer it points to.  Otherwise
		   look for a group printer with the name which the user specified. */
		look_for_name = forwhat ? forwhat : qentry.destname;

		/* try a group */
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
				if(!forwhat)	/* If we didn't get here thru an alias, */
					{
					if((tempptr = lmatchp(line, "Switchset:")))
						{
						if(switchset)		/* Switchsets are cumulative. */
							{
							gu_asprintf(&tempptr, "%s %s", switchset, tempptr);
							gu_free(switchset);
							switchset = tempptr;
							}
						else
							{
							switchset = gu_strdup(tempptr);
							}
						continue;
						}
					if(gu_sscanf(line, "PassThru: %Z", &tempptr) == 1)
						{
						set_field(passthru, tempptr);
						continue;
						}
					}
				if(gu_sscanf(line, "DefFiltOpts: %Z", &tempptr) == 1)
					{
					set_field(deffiltopts, tempptr);
					continue;
					}
				if(gu_sscanf(line, "ACLs: %Z", &tempptr) == 1)
					{
					set_field(acls, tempptr);
					continue;
					}
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
