/*
** mouse:~ppr/src/libppr/jobid.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 29 December 2000.
*/

#include "before_system.h"
#include <string.h>
#include "gu.h"
#include "global_defines.h"


/*
   This one is for printing the names of jobs destined
   for the local system.

   * The destination node name is never included.
   * The job id number is always included.
   * The subid is included only if it is not zero.
   * If the originating system is this system or a
     wildcard, the home node name is ommited.
*/
char *local_jobid(const char *destname, int qid, int subid, const char *homenode)
    {
    static char return_str[MAX_DESTNAME + 17 + MAX_NODENAME];

    if(subid > 0)
    	snprintf(return_str, sizeof(return_str), "%s-%d.%d", destname, qid, subid);
    else
    	snprintf(return_str, sizeof(return_str), "%s-%d", destname, qid);

    /* If the homenode isn't a wildcard and it isn't this node, add it. */
    if(strcmp(homenode, "*") && strcmp(homenode, ppr_get_nodename()))
	{
	int len = strlen(return_str);
    	snprintf(return_str + len, sizeof(return_str) - len, "(%s)", ppr_get_nodename());
    	}

    return return_str;
    } /* end of local_jobid() */

/*
   This one is for printing full jobid names.

   * If the destination node is this system, it is ommited.
   * The job id number is always included.
   * The subid is included only if it is not zero.
   * If the origionating system is this system or a wildcard,
     the home node name is ommited.
*/
char *remote_jobid(const char *destnode, const char *destname, int id, int subid, const char *homenode)
    {
    static char return_str[MAX_NODENAME + MAX_DESTNAME + 17 + MAX_NODENAME];
    int len = 0;

    /* If the destination node is not this node, say it. */
    if( strcmp(destnode, ppr_get_nodename()) )
	{
	snprintf(return_str, sizeof(return_str), "%s:", destnode);
	len += strlen(return_str);
	}	/* It is not safe to combine these! */

    /* Say the destination name and the id number. */
    snprintf(return_str + len, sizeof(return_str) - len, "%s-%d", destname, id);
    len += strlen(return_str + len);

    /* If the subid is not zero, specify it. */
    if(subid > 0)
	{
	snprintf(return_str + len, sizeof(return_str) - len, ".%d", subid);
	len += strlen(return_str + len);
	}

    /* If the home node name is not the local node, specify it. */
    if(strcmp(homenode, "*") && strcmp(homenode, ppr_get_nodename()))
    	snprintf(return_str + len, sizeof(return_str) - len, "(%s)", homenode);

    return return_str;
    } /* end of remote_jobid() */

/* end of file */
