/*
** mouse:~ppr/src/libuprint/uprint_sysv.c
** Copyright 1997, 1998, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 16 September 1998.
*/

#include "before_system.h"
#include <string.h>
#include "gu.h"
#include "global_defines.h"

#include "uprint.h"
#include "uprint_private.h"

/*
** Here we parse the printer options. and fill in the
** cooresponding portions of the UPRINT structure.
** Notice that we ignore ones we don't recognize.
*/
int uprint_parse_lp_interface_options(void *p)
    {
    struct UPRINT *upr = (struct UPRINT *)p;
    DODEBUG(("uprint_parse_lp_interface_options(p=%d)", p));

    if(upr->lp_interface_options)
	{
	char *s = upr->lp_interface_options;
	char *s2;
	while((s2=strtok(s, " \t")) != (char*)NULL)
	    {
	    if(strcmp(s2, "nobanner") == 0)
		uprint_set_nobanner(upr, TRUE);
	    else if(strcmp(s2, "nofilebreak") == 0)
		uprint_set_filebreak(upr, FALSE);
	    else if(strncmp(s2, "length=", 7) == 0)
		uprint_set_length(upr, &s2[7]);
	    else if(strncmp(s2, "width=", 6) == 0)
		uprint_set_width(upr, &s2[6]);
	    else if(strncmp(s2, "lpi=", 4) == 0)
		uprint_set_lpi(upr, &s2[4]);
	    else if(strncmp(s2, "cpi=", 4) == 0)
		uprint_set_cpi(upr, &s2[4]);
	    else if(strncmp(s2, "stty=", 5) == 0)
		{    }

	    s = (char*)NULL;
	    }
	}

    return 0;
    } /* end of uprint_parse_lp_interface_options() */

/*
** And here we process a mode list if I can
** remember what a mode list is.
*/
int uprint_parse_lp_filter_modes(void *p)
    {
    struct UPRINT *upr = (struct UPRINT *)p;
    DODEBUG(("uprint_parse_lp_filter_modes(p=%d)", p));

    if(upr->lp_filter_modes)
	{
	char *s = upr->lp_filter_modes;
	char *s2;
	while((s2 = strtok(s, " \t")))
	    {


	    s = (char*)NULL;
	    }
	}

    return 0;
    } /* end of uprint_parse_lp_filter_modes() */

/* end of file */

