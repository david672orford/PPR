/*
** mouse.trincoll.edu:~ppr/src/libppr/protected.c
** Copyright 1995, 1996, 1997, 1998, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 4 September 1998.
*/

/*
** This module determines if print destinations are "protected".
** A protected destination is one for which a per page charge
** is defined, even if that charge is 0.00 monetary units.
*/

#include "config.h"
#include <sys/stat.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"


/*
** Return true if we must verify that the user is in the users database.
*/
int destination_protected(const char *destnode, const char *destname)
	{
	char fname[MAX_PPR_PATH];
	struct stat statbuf;

	/*
	** For the time being we will asume that remote
	** printers are not protected.
	*/
	if(strcmp(destnode, ppr_get_nodename()) != 0)
		return FALSE;

	/* A group? */
	ppr_fnamef(fname, "%s/%s", GRCONF, destname);
	if(stat(fname, &statbuf) == 0)				/* if file found, */
		{
		if(statbuf.st_mode & S_IXOTH)			/* if ``other'' execute bit set, */
			return TRUE;						/* it is protected */
		}

	/* A printer? */
	else
		{
		ppr_fnamef(fname, "%s/%s", PRCONF, destname);
		if( stat(fname, &statbuf) == 0 )		/* if it exists, */
			{									/* and other execute set, */
			if(statbuf.st_mode & S_IXOTH)
				return TRUE;					/* it is protected */
			}
		}

	return FALSE;	/* if it doen't exist, sombody else will notice */
	} /* end of destination_protected() */

/* end of file */
