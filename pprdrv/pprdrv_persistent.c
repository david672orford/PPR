/*
** ~ppr/src/pprdrv/pprdrv_persistent.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 4 June 1999.
*/

#include "config.h"
#include "gu.h"
#include "global_defines.h"

#include "pprdrv.h"

/*
** This is called by pprdrv_feedback.c whenever it gets a message.
** If we return TRUE, it will not go into the log file.
*/
int persistent_query_callback(char *message)
	{
	return FALSE;
	} /* end of persistent_query_callback() */

/*
** This is called after the connexion to the printer has been
** opened and before any initialization strings have been sent.
** It should review drvres[] and download anything it thinks
** should be made persistent.  It should send a properly formed
** PostScript job with proper initialization, and cleanup.
*/
void persistent_download_now(void)
	{

	} /* end of persistent_download_now() */

/*
** This is called after the print job is done to give
** the persistent download machinery a chance to write
** an updated statistics file.  Presumably this file
** will keep track of what is currently downloaded
** and how frequently various resources have been used
** (in order to help decide whether they should be made
** persistent).
*/
void persistent_finish(void)
	{

	} /* end of persistent_finish() */

/* end of file */
