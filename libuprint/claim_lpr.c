/*
** mouse:~ppr/src/libuprint/claim_lpr.c
** Copyright 1995--2002, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 22 April 2002.
*/

#include "before_system.h"
#include <string.h>
#include <ctype.h>
#include "gu.h"
#include "global_defines.h"
#include "uprint.h"

/*
** Return TRUE if the destname is the name
** of a valid LPR destination.
**
** In order to acomplish this, we search
** /etc/printcap.
*/
int printdest_claim_lpr(const char *destname)
    {
    if(uprint_lpr_installed())
    {
    FILE *f;

    if((f = fopen(LPR_PRINTCAP, "r")) != (FILE*)NULL)
    	{
	char line[256];
	char *p;

	while(fgets(line, sizeof(line), f))
	    {
	    if(isspace(line[0]))
	    	continue;

	    if((p = strchr(line, ':')) == (char*)NULL)
	    	continue;

	    *p = '\0';

	    for(p = line; (p = strtok(p, "|")) != (char*)NULL; p = (char*)NULL)
	    	{
		if(strcmp(p, destname) == 0)
		    {
		    fclose(f);
		    return TRUE;
		    }
	    	}
	    }

    	fclose(f);
    	}
    }

    return FALSE;
    } /* end of printdest_claim_lpr() */

/* end of file */
