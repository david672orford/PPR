/*
** mouse:~ppr/src/pprdrv/pprdrv_userparams.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 8 December 2000.
*/

#include "before_system.h"
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "pprdrv.h"

static void pre(const char setting[])
    {
    printer_printf("%% PPR sets %s:\n", setting);
    printer_putline("[{");
    }

static void post(const char setting[])
    {
    printer_printf("}stopped{(Failed to set %s\\n)print}if cleartomark\n", setting);
    printer_putline("%% PPR done.");
    }

/*
** This is called at the end of document setup.
*/
void insert_userparams(void)
    {
    if(printer.userparams.WaitTimeout >= 0)
	{
	pre("WaitTimeout");
	if(Features.LanguageLevel >= 2)
	    printer_printf("<</WaitTimeout %d>>setuserparams\n", printer.userparams.WaitTimeout);
	else
            printer_printf("%d statusdict /setwaittimeout get exec\n", printer.userparams.WaitTimeout);
	post("WaitTimeout");
	}

    if(printer.userparams.ManualfeedTimeout >= 0)
	{
	pre("ManualfeedTimeout");
        printer_printf("%d statusdict /setmanualfeedtimeout get exec\n", printer.userparams.ManualfeedTimeout);
	post("ManualfeedTimeout");
	}

    if(printer.userparams.DoPrintErrors != -1)
	{
	if(Features.LanguageLevel >= 2)
	    {
	    pre("DoPrintErrors");
	    printer_printf("<</DoPrintErrors %s>>setuserparams\n", printer.userparams.DoPrintErrors ? "true" : "false");
	    post("WaitTimeout");
	    }
	}

    } /* end of insert_userparams() */

/*
** This is called at the start of each page.  It resets the
** job timeout.
*/
void insert_userparams_jobtimeout(void)
    {
    if(printer.userparams.JobTimeout >= 0)
	{
	pre("JobTimeout");
	printer_printf("%d statusdict /setjobtimeout get exec\n", printer.userparams.JobTimeout);
	post("JobTimeout");
	}
    } /* end of insert_userparams_jobtimeout() */

/* end of file */
