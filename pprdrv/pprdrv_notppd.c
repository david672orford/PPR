/*
** mouse:~ppr/src/pprdrv/pprdrv_notppd.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 23 May 2001.
*/

/*
** These functions emmit PostScript code fragments
** for things the PPD files does not tell us how to do.
*/

#include "before_system.h"
#include "gu.h"
#include "global_defines.h"
#include "pprdrv.h"

void set_jobname(void)
    {
    if(Features.LanguageLevel < 2)
	{
	printer_puts("/statusdict where {pop statusdict /jobname (");
	printer_puts_escaped(QueueFile);
	printer_puts(") put } if % PPR\n");
	}
    else
	{
	printer_puts("<< /JobName (");
	printer_puts_escaped(QueueFile);
	printer_puts(") >> setuserparams % PPR\n");
	}

    } /* end of set_jobname() */

void set_copies(int copies)
    {
    if(Features.LanguageLevel < 2)
	{
	printer_printf("/#copies %d def %%PPR\n", copies);
	}
    else
	{
	char temp[25];
	printer_putline("[0 0 0 0 0 0] currentmatrix %PPR");
	begin_stopped();
	printer_printf("<< /NumCopies %d >> setpagedevice %%PPR\n", copies);
	snprintf(temp, sizeof(temp), "%d", copies);
	end_stopped("NumCopies", temp);
	printer_putline("setmatrix %PPR");
	}
    } /* end of set_copies() */

/* end of file */
