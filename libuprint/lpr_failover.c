/*
** mouse:~ppr/src/libuprint/lpr_failover.c
** Copyright 1995--2003, Trinity College Computing Center.
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
** Last modified 12 February 2003.
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
