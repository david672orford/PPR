/*
** mouse:~ppr/src/libuprint/uprint_loop.c
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
** Last modified 14 February 2000.
*/

#include "config.h"
#include <stdlib.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

#include "uprint.h"

#define VARIABLE "UPRINT_LOOP_CHECK"

/*
** Break loops.
*/
int uprint_loop_check(const char *myname)
	{
	#ifdef HAVE_PUTENV

	static char temp[64];
	char *p;

	if((p = getenv(VARIABLE)))
		{
		uprint_error_callback(_("Loop detected, %s called %s!\n"
				"(Probably uprint.conf has been changed since uprint-newconf was run.)"), p, myname);
		return -1;
		}

	snprintf(temp, sizeof(temp), "%s=%.45s", VARIABLE, myname);
	putenv(temp);

	#endif

	return 0;
	} /* end of uprint_loop_check() */

/* end of file */
