/*
** mouse:~ppr/src/libppr/destspec.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 8 July 1999.
*/

#include "config.h"
#include <string.h>
#include "gu.h"
#include "global_defines.h"


/*
** If the destination system is not this system, return "$destnode:$destname",
** if it is this system, return just "$destname".
**
** This is probably used when printing messages.
*/
const char *network_destspec(const char *destnode, const char *destname)
	{
	static char return_str[MAX_NODENAME+MAX_DESTNAME+1];

	if(!destname)
		{
		return "???";
		}
	else if( destnode && strcmp(destnode, ppr_get_nodename()) )
		{
		snprintf(return_str, sizeof(return_str), "%s:%s", destnode, destname);
		return return_str;
		}
	else
		{
		return destname;
		}
	} /* end of network_destspec() */

/* end of file */
