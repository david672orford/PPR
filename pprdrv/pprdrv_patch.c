/*
** mouse:~ppr/src/pprdrv/pprdrv_patch.c
** Copyright 1995--2005, Trinity College Computing Center.
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
** Last modified 1 March 2005.
*/

#include "config.h"
#include <unistd.h>
#include <string.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "pprdrv.h"
#include "interface.h"

static int result_tripped = TRUE;
static int result_retval;
static gu_boolean result_boolean;

/*
** This is called by pprdrv_feedback.c whenever it gets a message.
** If we return TRUE, it will not go into the log file.
*/
gu_boolean patchfile_query_callback(const char message[])
	{
	if(result_tripped)		/* only swallow one line */
		return FALSE;
	DODEBUG_QUERY(("patchfile query response: %.*s", (int)strcspn(message, "\n"), message));
	result_retval = gu_torf_setBOOL(&result_boolean,message);
	result_tripped = TRUE;
	return TRUE;
	} /* end of patchfile_query_callback() */

/*
** Download any *PatchFile if it is not already downloaded.
** Return TRUE if we used ExitServer.
*/
void patchfile(void)
	{
	const char *code;
	const char *qcode;
	int patch_needed = TRUE;
	const char *excode;

	DODEBUG_QUERY(("patchfile()"));

	/* If there is a patch for this printer. */
	if((code = find_feature("*PatchFile", (char*)NULL)))
		{
		if(printer.Feedback)			/* if queries possible */
			{
			DODEBUG_QUERY(("Querying for *PatchFile"));

			if((qcode = find_feature("*?PatchFile", (char*)NULL)) == (char*)NULL)
				fatal(EXIT_PRNERR_NORETRY, "PPD file has *PatchFile but no *?PatchFile");

			job_start(JOBTYPE_QUERY);
			result_tripped = FALSE;

			printer_putline("%!PS-Adobe-3.0 Query");
			printer_putline("");
			printer_putline("%%?BeginQuery: PatchFile");
			printer_puts(qcode);
			printer_putline("\n%%EndQuery: True");
			printer_putline("%%EOF");
			printer_flush();

			/* Wait for the reply.  Allow up to 120 seconds because
			   we don't want to give up before the interface gives
			   up its connection attempt. */
			writemon_start("QUERY");
			while(!result_tripped)
				{
				if(feedback_wait(120, FALSE) == -1)
					fatal(EXIT_PRNERR, _("Timeout expired while waiting for printer to respond to *?PatchFile query."));
				}
			writemon_unstalled("QUERY");

			job_end();

			/* If no response or unintelligible response, */
			if(result_retval == -1)
				fatal(EXIT_PRNERR_NORETRY, _("Printer did not respond correctly to *?PatchFile query."));

			/*
			** If patch is in place, turn off the flag
			** which calls for it to be downloaded.
			*/
			else if(result_boolean)
				{
				DODEBUG_QUERY(("Patch is not needed"));
				patch_needed = FALSE;
				}

			#ifdef DEBUG_QUERY
			else
				debug("Patch is needed");
			#endif
			}

		#ifdef DEBUG_QUERY
		else
			debug("Queries are not possible, assuming patch is needed");
		#endif

		/* If we decided above that the patch should be
		   downloaded. */
		if(patch_needed)
			{
			DODEBUG_QUERY(("Downloading *PatchFile"));

			if((excode = find_feature("*ExitServer", (char*)NULL)) == (char*)NULL)
				fatal(EXIT_PRNERR_NORETRY, _("PPD file does not have *ExitServer code"));

			job_start(JOBTYPE_QUERY);

			printer_putline("%!PS-Adobe-3.0 ExitServer");
			printer_putline("%%Title: PatchFile");
			printer_putline("%%EndComments");
			printer_putc('\n');
			printer_putline("%%BeginExitServer: 000000");
			printer_putline("000000");							/* password */
			printer_puts(excode);								/* insert exitserver code */
			printer_putline("\n%%EndExitServer");
			printer_puts(code);
			printer_putline("\n%%EOF");

			job_end();

			DODEBUG_QUERY(("done"));
			}
		} /* end of if patch exists in PPD file */
	} /* end of patchfile() */

/*
** Download all the *JobPatchFile sections.
*/
void jobpatchfile(void)
	{
	int x;
	const char *code;
	char numstr[3];

	/* Download *JobPatchFile instances until one is missing. */
	for(x=1; x < 10; x++)
		{
		snprintf(numstr, sizeof(numstr), "%d", x);
		if((code = find_feature("*JobPatchFile", numstr)) == (char*)NULL)
			break;

		printer_printf("%% start of *JobPatchFile %d\n", x);
		printer_puts(code);
		printer_printf("\n%% end of *JobPatchFile %d\n", x);
		}

	} /* end of jobpatchfile() */

/* end of file */

