/*
** mouse:~ppr/src/pprdrv/pprdrv_pagecount.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 11 November 1999.
*/

#include "before_system.h"
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
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

gu_boolean pagecount_query_callback(const char message[])
	{
	if(tripped) return FALSE;
	DODEBUG_QUERY(("pagecount query response: %.*s", (int)strcspn(message, "\n"), message));
	if(isdigit(message[0]))
		result = atoi(message);
	tripped = TRUE;
	return TRUE;
	}

int pagecount(void)
	{
	result = -1;

	if(printer.Feedback && printer.PageCountQuery > 0)
		{
		DODEBUG_QUERY(("sending pagecount query"));

		job_start(JOBTYPE_QUERY);
		tripped = FALSE;

		printer_putline("%!PS-Adobe-3.0 Query");
		printer_putline("");
		printer_putline("%%?BeginQuery: pagecount");
		printer_putline("statusdict /pagecount get exec == flush");
		printer_putline("%%EndQuery: -1");
		printer_putline("%%EOF");
		printer_flush();

		/* Wait for the reply.  Allow up to 120 seconds because
		   we don't want to give up before the interface gives
		   up its connection attempt. */
		writemon_start("QUERY");
		while(!tripped)
			{
			if(feedback_wait(120, FALSE) == -1)
				fatal(EXIT_PRNERR, _("Timeout expired while waiting for printer to respond to pagecount query."));
			}
		writemon_unstalled("QUERY");

		job_end();
		}

	return result;
	}

/* end of file */

