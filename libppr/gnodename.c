/*
** mouse:~ppr/src/libppr/gnodename.c
** Copyright 1996, 1998, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 22 November 2000.
*/

#include "before_system.h"
#include <string.h>
#include <sys/utsname.h>
#include <errno.h>
#include "gu.h"
#include "global_defines.h"

/*
** This function returns the nodename which identifies this PPR node.
** All jobs coming from this node will be stamped with this name.  Other
** nodes will use this name to send jobs to this node.
**
** Notice that this routine is call frequently.  It should determine
** the node name only the first time it is called.  It should save
** that name and returned the saved name on subsequent calls.
*/
const char *ppr_get_nodename(void)
    {
    static char *nodename = (char*)NULL;

    if(nodename == (char*)NULL)
    	{
	struct utsname s;
	int len;

	/*
	** Ask the system for system name information.
	** What we really care about is the nodename.
	*/
	if(uname(&s) == -1)
	     libppr_throw(EXCEPTION_OTHER, "ppr_get_nodename", "uname() failed, errno=%d (%s)", errno, gu_strerror(errno));

	/*
	** If the domain name is included in the node name,
	** only use the part before the first dot.
	*/
	len = strcspn(s.nodename,".");

	/*
	** If the node name is too long, truncate it.
	** That nodenames be unique in the first 16 characters
	** does not seem unreasonable.
	*/
	if(len > MAX_NODENAME) len = MAX_NODENAME;

	/*
	** Allocate memory for and make a copy of the
	** nodename we have determined uppon.
	*/
	nodename = (char*)gu_alloc(len+1, sizeof(char));
	strncpy(nodename, s.nodename, len);
	nodename[len] = '\0';
    	}

    return nodename;
    } /* end of ppr_get_nodename() */

/* end of file */
