/*
** mouse:~ppr/src/pprdrv/pprdrv_patch.c
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

#include "before_system.h"
#include <unistd.h>
#include <string.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "pprdrv.h"
#include "interface.h"

static int tripped = TRUE;
static int result;

/*
** This is called by pprdrv_feedback.c whenever it gets a message.
** If we return TRUE, it will not go into the log file.
*/
gu_boolean patchfile_query_callback(const char message[])
	{
	if(tripped) return FALSE;
	DODEBUG_QUERY(("patchfile query response: %.*s", (int)strcspn(message, "\n"), message));
	result = gu_torf(message);
	tripped = TRUE;
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

			result = ANSWER_UNKNOWN;

			job_start(JOBTYPE_QUERY);
			tripped = FALSE;

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
			while(!tripped)
				{
				if(feedback_wait(120, FALSE) == -1)
					fatal(EXIT_PRNERR, _("Timeout expired while waiting for printer to respond to *?PatchFile query."));
				}
			writemon_unstalled("QUERY");

			job_end();

			/* If no response or unintelligible response, */
			if(result == ANSWER_UNKNOWN)
				fatal(EXIT_PRNERR_NORETRY, _("Printer did not respond correctly to *?PatchFile query."));

			/*
			** If patch is in place, turn off the flag
			** which calls for it to be downloaded.
			*/
			else if(result == ANSWER_TRUE)
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

