/*
** mouse:~ppr/src/libppr_int/int_prpapst.c
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
** Last modified 24 June 1999.
*/

#include "config.h"
#include <string.h>
#include "gu.h"
#include "global_defines.h"

#include "libppr_int.h"

/*
** This is called from interfaces/atalk_*.c.
**
** Print the pascal format string as a LaserWriter
** style message on stdout.
**
** This is a subroutine because the HP deskjet produces
** defective status messages which must be fixed.  It
** is called from interfaces/atalk_*.c.
*/
void print_pap_status(const unsigned char *status)
	{
	#ifndef DESKJET_STATUS_FIX
	printf("%%%%[ %.*s ]%%%%\n", (int)status[0], &status[1]);

	#else
	const unsigned char *p = &status[1];
	int len = (int)status[0];
	unsigned char lastc = '\0';

	while(len > 0 && p[len - 1] == '\n')
		len--;

	fputs("%%[ ", stdout);

	while(len--)
		{
		if((lastc == ':' || lastc == ';') && *p != ' ')
			fputc(' ', stdout);

		fputc((lastc = *p++), stdout);
		}

	fputs(" ]%%\n", stdout);
	#endif
	} /* end of print_lw_message() */

/*
** This is called from interfaces/atalk_*.c.  It returns true
** if the status message is one which should be printed on
** the interface's stdout even if the PAPOpen() suceeds.
*/
gu_boolean is_pap_PrinterError(const unsigned char *status)
	{
	if(status[0] > 21 && strncmp("status: PrinterError:", (char*)&status[1], 21) == 0)
		return TRUE;

	#ifdef DESKJET_STATUS_FIX
	if(status[0] > 20 && strncmp("status:PrinterError:", (char*)&status[1], 20) == 0)
		return TRUE;
	#endif

	return FALSE;
	} /* is_pap_PrinterError() */

/* end of file */
