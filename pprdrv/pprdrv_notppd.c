/*
** mouse:~ppr/src/pprdrv/pprdrv_notppd.c
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
** Last modified 29 September 2002.
*/

/*
** These functions emmit PostScript code fragments for things the PPD files
** does not tell us how to do.	Some of the code in this module reads or
** creates NonPPDFeature comments as described in Adobe Technical Note #????.
*/

#include "before_system.h"
#include "gu.h"
#include "global_defines.h"
#include "pprdrv.h"
#include "interface.h"

/*
** This function sends to the printer the PostScript commands needed to set
** the jobname which shows up in the AppleTalk status and possibly other
** places on some printers.
*/
void set_jobname(const char jobname[])
	{
	if(Features.LanguageLevel < 2)
		{
		printer_puts("/statusdict where {pop statusdict /jobname (");
		printer_puts_escaped(jobname);
		printer_puts(") put } if % PPR\n");
		}
	else
		{
		printer_puts("<< /JobName (");
		printer_puts_escaped(jobname);
		printer_puts(") >> setuserparams % PPR\n");
		}

	} /* end of set_jobname() */

/*
** This function sends to the printer the bare code needed to set auto copies.
** This is called from two functions below.
*/
static void set_numcopies_bare(int copies)
	{
	if(Features.LanguageLevel < 2)
		{
		printer_printf("/#copies %d def %%PPR\n", copies);
		}
	else
		{
		char temp[25];
		begin_stopped();
		printer_printf("<< /NumCopies %d >> setpagedevice %%PPR\n", copies);
		snprintf(temp, sizeof(temp), "%d", copies);
		end_stopped("NumCopies", temp);
		}
	}

/*
** This is called from other modules to generate a properly commented
** code block to set auto copies.
*/
void set_numcopies(int copies)
	{
	printer_printf("%%%%BeginNonPPDFeature: NumCopies %d\n", copies);
	printer_putline("[0 0 0 0 0 0] currentmatrix %PPR");
	set_numcopies_bare(copies);
	printer_putline("setmatrix %PPR");
	printer_putline("%%EndNonPPDFeature\n");
	} /* end of set_numcopies() */

/*
** This is called whenever a "%%BeginNonPPDFeature:" comment is encountered.
** It copies the block (possibly modifying it as it goes) and leaves the
** "%%EndNonPPDFeature" line in line[].
*/
void begin_nonppd_feature(const char feature_name[], const char feature_value[], FILE *infile)
	{
	const char *function = "begin_nonppd_feature";
	int copies = -1;
	gu_boolean keep = TRUE;

	if(!feature_name || ! feature_value)
		return;

	if(strcmp(feature_name, "NumCopies") == 0 && copies_auto != -1)
		{
		printer_printf("%% %%%%BeginNonPPDFeature: %s %s\n", feature_name, feature_value);
		copies = copies_auto;
		printer_printf("%%%%BeginNonPPDFeature: %s %d\n", feature_name, copies);
		keep = FALSE;
		}
	else
		{
		printer_printf("%%%%BeginNonPPDFeature: %s %s\n", feature_name, feature_value);
		}

	/* Copy the code until we see "%%EndNonPPDFeature". */
	while(TRUE)
		{
		if(!dgetline(infile))
			{
			give_reason("defective NonPPDFeature invokation");
			fatal(EXIT_JOBERR, "%s(): unterminated NonPPDFeature code", function);
			}

		if(strcmp(line, "%%EndNonPPDFeature") == 0)
			break;

		if(keep)
			printer_putline(line);
		#ifdef KEEP_OLD_CODE
		else
			printer_printf("%% %s\n", line);
		#endif
		}

	if(copies != -1)
		{
		set_numcopies_bare(copies);
		}

	printer_puts("%%EndNonPPDFeature\n");
	} /* end of begin_nonppd_feature() */

/* end of file */
