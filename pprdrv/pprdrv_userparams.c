/*
** mouse:~ppr/src/pprdrv/pprdrv_userparams.c
** Copyright 1995--2002, Trinity College Computing Center.
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
** Last modified 27 September 2002.
*/

#include "before_system.h"
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "pprdrv.h"

static void pre(const char setting[], int value)
    {
    printer_printf("%%%%BeginNonPPDFeature: %s %d\n", setting, value);
    printer_putline("[{");
    }

static void pre_str(const char setting[], const char value[])
    {
    printer_printf("%%%%BeginNonPPDFeature: %s %s\n", setting, value);
    printer_putline("[{");
    }

static void post(const char setting[])
    {
    printer_printf("}stopped{(Failed to set %s\\n)print}if cleartomark\n", setting);
    printer_putline("%%EndNonPPDFeature");
    }

/*
** This is called at the end of document setup.
*/
void insert_userparams(void)
    {
    if(printer.userparams.WaitTimeout >= 0)
	{
	pre("WaitTimeout", printer.userparams.WaitTimeout);
	if(Features.LanguageLevel >= 2)
	    printer_printf("<</WaitTimeout %d>>setuserparams %%PPR\n", printer.userparams.WaitTimeout);
	else
            printer_printf("%d statusdict /setwaittimeout get exec\n", printer.userparams.WaitTimeout);
	post("WaitTimeout");
	}

    if(printer.userparams.ManualfeedTimeout >= 0)
	{
	pre("ManualfeedTimeout", printer.userparams.ManualfeedTimeout);
        printer_printf("%d statusdict /setmanualfeedtimeout get exec %%PPR\n", printer.userparams.ManualfeedTimeout);
	post("ManualfeedTimeout");
	}

    if(printer.userparams.DoPrintErrors != -1)
	{
	if(Features.LanguageLevel >= 2)
	    {
	    pre_str("DoPrintErrors", printer.userparams.DoPrintErrors ? "True" : "False");
	    printer_printf("<</DoPrintErrors %s>>setuserparams %%PPR\n", printer.userparams.DoPrintErrors ? "true" : "false");
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
	pre("JobTimeout", printer.userparams.JobTimeout);
	printer_printf("%d statusdict /setjobtimeout get exec %%PPR\n", printer.userparams.JobTimeout);
	post("JobTimeout");
	}
    } /* end of insert_userparams_jobtimeout() */

/* end of file */
