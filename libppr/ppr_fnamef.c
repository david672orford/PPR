/*
** mouse:~ppr/src/libppr/ppr_fnamef.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 12 December 2000.
*/

#include "before_system.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"

void ppr_fnamef(char target[], const char pattern[], ...)
    {
    const char *function = "ppr_fnamef";
    va_list va;
    const char *si = pattern;
    char *di = target;
    size_t space_left = (MAX_PPR_PATH - 1);
    int c;
    char tempint[32];		/* overly large */
    const char *str;
    size_t len;
    int item_number = 0;

    va_start(va, pattern);

    while((c = *(si++)))
	{
	switch(c)
	    {
	    case '%':
		item_number++;
		switch((c = *(si++)))
		    {
		    case 'l':
			if((c = *(si++)) != 'd')
			    libppr_throw(EXCEPTION_BADUSAGE, function, "unrecognized format: \"%s\" item %d", pattern, item_number);
			snprintf(tempint, sizeof(tempint), "%ld", va_arg(va, long int));
			str = tempint;
		        goto skip;

		    case 'd':
			snprintf(tempint, sizeof(tempint), "%d", va_arg(va, int));
			str = tempint;
			goto skip;

		    case 's':
			str = va_arg(va, char*);
			goto skip;

		    skip:
			len = strlen(str);
			if(len > space_left)
			    {
			    *di = '\0';
			    libppr_throw(EXCEPTION_BADUSAGE, function, "overflow: \"%s\" -> \"%s\" item %d: str=\"%s\", len=%d, space_left=%d", pattern, target, item_number, str, len, space_left);
			    }

			/* debug("y1 pattern=\"%s\", target=%p, di=%p, str=%p, str=\"%s\", space_left=%d", pattern, target, di, str, str, space_left); */
			/* strcpy(di, str); */
			memcpy(di, str, len + 1);
			/* debug("y2"); */

			di += len;
			space_left -= len;
			break;

		    default:
			libppr_throw(EXCEPTION_BADUSAGE, function, "unrecognized format: \"%s\" item %d", pattern, item_number);
			break;
		    }
	    	break;
	    default:
		if(space_left < 1)
		    {
		    *di = '\0';
		    libppr_throw(EXCEPTION_BADUSAGE, function, "overflow in pattern: \"%s\" -> \"%s\"", pattern, target);
		    }
	    	*di++ = c;
	    	space_left--;
	    	break;
	    }
	}

    *di = '\0';
    va_end(va);

    /* This is a hack for Cygnus Win32.  It changes the colons
       to exclaimation points because Win32 treats colons as
       the separator after the drive letter.  Since this hack makes
       the change when a queue file is created and when it is opened
       later, the system still works.
       */
    #ifdef COLON_FILENAME_BUG
    for(di=target; *di; di++)
    	if(*di == ':') *di = '!';
    #endif
    }

/* end of file */

