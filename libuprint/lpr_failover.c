/*
** mouse:~ppr/src/libuprint/lpr_failover.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 12 August 1999.
*/

#include "before_system.h"
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

#include "uprint.h"

/*
** This is like uprint_lpr_make_connection(), but it takes a comma
** and/or space separated list of hostnames.
*/
int uprint_lpr_make_connection_with_failover(const char *address)
    {
    int sockfd = -1;
    char *copy, *p, *p2;
    int x = 0;

    p = copy = gu_strdup(address);

    while((p2 = gu_strsep(&p, ", \t")))
    	{
	if(p2[0] == '\0')	/* skip "empty fields" */
	    continue;

	if(x > 0)
	    uprint_error_callback(_("Resorting to backup server \"%s\"."), p2);

	if((sockfd = uprint_lpr_make_connection(p2)) >= 0)
	    break;

	x++;
    	}

    gu_free(copy);

    return sockfd;
    }

/* end of file */

