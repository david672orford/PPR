/*
** mouse:~ppr/src/libppr/gnodename.c
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
** Last modified 6 March 2003.
*/

/*! \file
	\brief determine name of this network node
*/

#include "before_system.h"
#include <string.h>
#include <sys/utsname.h>
#include <errno.h>
#include "gu.h"
#include "global_defines.h"

/** determine name of this node

This function returns the nodename which identifies this PPR node.
All jobs coming from this node will be stamped with this name.  Other
nodes will use this name to send jobs to this node.  In the current
implementation this is the system name truncated before the first period (if
present) and furthur truncated to 16 characters.

Notice that this routine is call frequently.  It should determine
the node name only the first time it is called.  It should save
that name and returned the saved name on subsequent calls.

*/
const char *ppr_get_nodename(void)
	{
	static char *nodename = (char*)NULL;

	if(!nodename)
		{
		/* First we try the configuration file. */
		nodename = gu_ini_query(PPR_CONF, "network", "nodename", 0, NULL);

		/* If that didn't work, we use uname(). */
		if(!nodename)
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
			len = strcspn(s.nodename, ".");

			/*
			** If the node name is too long, truncate it.
			** That nodenames be unique in the first 16 characters
			** does not seem unreasonable.
			*/
			if(len > MAX_NODENAME)
				len = MAX_NODENAME;

			/*
			** Allocate memory for and make a copy of the
			** nodename we have determined uppon.
			*/
			nodename = gu_strndup(s.nodename, len);
			}
		}

	return nodename;
	} /* end of ppr_get_nodename() */

/* end of file */
