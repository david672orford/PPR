/*
** mouse:~ppr/src/pprdrv/pprdrv_snmp_messages.c
** Copyright 1995--2003, Trinity College Computing Center.
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
** Last modified 31 October 2003.
*/

#include "config.h"
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "pprdrv.h"

int translate_snmp_error(int bit, const char **description, const char **raw1, int *severity)
	{
	switch(bit)
		{
		case 0:							/* 0x00000001 */
			*description = N_("paper low");
			*raw1 = "lowPaper";
			*severity = 4;
			break;
		case 1:							/* 0x00000002 */
			*description = N_("out of paper");
			*raw1 = "noPaper";
			*severity = 6;
			break;
		case 2:							/* 0x00000004 */
			*description = N_("toner/ink is low");
			*raw1 = "lowToner";
			*severity = 4;
			break;
		case 3:							/* 0x00000008 */
			*description = N_("out of toner/ink");
			*raw1 = "noToner";
			*severity = 6;
			break;
		case 4:							/* 0x00000010 */
			*description = N_("cover open");
			*raw1 = "doorOpen";
			*severity = 6;
			break;
		case 5:							/* 0x00000020 */
			*description = N_("paper jam");
			*raw1 = "jammed";
			*severity = 6;
			break;
		case 6:							/* 0x00000040 */
			*description = N_("off line");
			*raw1 = "offline";
			*severity = 6;
			break;
		case 7:							/* 0x00000080 */
			*description = N_("service requested");
			*raw1 = "serviceRequested";
			*severity = 10;
			break;
		case 8:							/* 0x00000100 */
			*description = N_("no paper tray");
			*raw1 = "inputTrayMissing";
			*severity = 6;
			break;
		case 9:							/* 0x00000200 */
			*description = N_("no output tray");
			*raw1 = "outputTrayMissing";
			*severity = 6;
			break;
		case 10:						/* 0x00000400 */
			*description = N_("toner/ink cartridge missing");
			*raw1 = "markerSupplyMissing";
			*severity = 6;
			break;
		case 11:						/* 0x00000800 */
			*description = N_("output tray near full");
			*raw1 = "outputNearFull";
			*severity = 3;
			break;
		case 12:						/* 0x00001000 */
			*description = N_("output tray full");
			*raw1 = "outputFull";
			*severity = 6;
			break;
		case 13:						/* 0x00002000 */
			*description = N_("input tray empty");
			*raw1 = "inputTrayEmpty";
			*severity = 6;
			break;
		case 14:						/* 0x00004000 */
			*description = N_("overdue for preventative maintainance");
			*raw1 = "overduePreventMaint";
			*severity = 2;
			break;
		default:
			*description = "[undefined SNMP error code]";
			*raw1 = "";
			*severity = 1;
			break;
		}

	return 0;
	} /* end of translate_snmp_message() */

int translate_snmp_status(int device_status, int printer_status, const char **message, const char **raw1, int *severity)
	{
	const char *raw1a, *raw1b;
	static char scratch[32];

	switch(device_status)
		{
		case 1:
			raw1a = "unknown(1)";
			break;
		case 2:
			raw1a = "running(2)";
			break;
		case 3:
			raw1a = "warning(3)";
			break;
		case 4:
			raw1a = "testing(4)";
			break;
		case 5:
			raw1a = "down(5)";
			break;
		default:
			raw1a = "";
			break;
		}

	switch(printer_status)
		{
		case 1:
			*message = N_("other");
			raw1b = "other(1)";
			*severity = 3;
			break;
		case 2:
			*message = N_("unknown");
			raw1b = "unknown(2)";
			*severity = 3;
			break;
		case 3:
			*message = N_("idle");
			raw1b = "idle(3)";
			*severity = 1;
			break;
		case 4:
			*message = N_("busy");
			raw1b = "printing(4)";
			*severity = 2;
			break;
		case 5:
			*message = N_("warming up");
			raw1b = "warmup(5)";
			*severity = 2;
			break;
		default:
			*message = "[undefined]";
			raw1b = "";
			*severity = 1;
			break;
		}

	gu_snprintf(scratch, sizeof(scratch), "%s/%s", raw1a, raw1b);
	*raw1 = scratch;
	
	return 0;
	} /* end of translate_snmp_status() */

/* end of file */
