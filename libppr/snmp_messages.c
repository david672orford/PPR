/*
** mouse:~ppr/src/pprdrv/pprdrv_snmp_messages.c
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
** Last modified 31 March 2005.
*/

/*! \file */

#include "config.h"
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

/** Translate an SNMP hrPrinterErrorState bit to English
 *
 * Produce a translatable English-language description and the description 
 * from RFC 3805.  Also, assess the severity of this condition on a scale 
 * from 1 to 10.  A severity of 7 or above indicates that the printer is 
 * non-operational.  Particularly:
 *
 * 1 idle
 * 2 waking up from standby and rewarming
 * 3 standby or power-save mode
 * 4 busy
 * 5 running low on paper or initial warmup
 * 6 running low on toner or maintainance required soon
 * 7 non-operational, easy to fix such as by adding paper or clearing a jam
 * 8 non-operational, harder to fix, such as by adding toner
 * 9
 * 10 requires a repairman
 *
 * If you don't need a particular result value, pass a NULL pointer.
 */
int translate_snmp_error(int bit, const char **set_description, const char **set_raw1, int *set_severity)
	{
	char *description = NULL;
	char *raw1 = NULL;
	int severity = 1;

	switch(bit)
		{
		case 0:							/* 0x00000001 */
			description = N_("paper low");
			raw1 = "lowPaper";
			severity = 5;
			break;
		case 1:							/* 0x00000002 */
			description = N_("out of paper");
			raw1 = "noPaper";
			severity = 7;
			break;
		case 2:							/* 0x00000004 */
			description = N_("toner/ink is low");
			raw1 = "lowToner";
			severity = 6;
			break;
		case 3:							/* 0x00000008 */
			description = N_("out of toner/ink");
			raw1 = "noToner";
			severity = 8;
			break;
		case 4:							/* 0x00000010 */
			description = N_("cover open");
			raw1 = "doorOpen";
			severity = 7;
			break;
		case 5:							/* 0x00000020 */
			description = N_("paper jam");
			raw1 = "jammed";
			severity = 7;
			break;
		case 6:							/* 0x00000040 */
			description = N_("off line");
			raw1 = "offline";
			severity = 7;
			break;
		case 7:							/* 0x00000080 */
			description = N_("service requested");
			raw1 = "serviceRequested";
			severity = 10;
			break;
		case 8:							/* 0x00000100 */
			description = N_("no paper tray");
			raw1 = "inputTrayMissing";
			severity = 7;
			break;
		case 9:							/* 0x00000200 */
			description = N_("no output tray");
			raw1 = "outputTrayMissing";
			severity = 7;
			break;
		case 10:						/* 0x00000400 */
			description = N_("toner/ink cartridge missing");
			raw1 = "markerSupplyMissing";
			severity = 8;
			break;
		case 11:						/* 0x00000800 */
			description = N_("output tray near full");
			raw1 = "outputNearFull";
			severity = 5;
			break;
		case 12:						/* 0x00001000 */
			description = N_("output tray full");
			raw1 = "outputFull";
			severity = 7;
			break;
		case 13:						/* 0x00002000 */
			description = N_("input tray empty");
			raw1 = "inputTrayEmpty";
			severity = 7;
			break;
		case 14:						/* 0x00004000 */
			description = N_("overdue for preventative maintainance");
			raw1 = "overduePreventMaint";
			severity = 6;
			break;
		default:
			description = "[undefined SNMP error code]";
			raw1 = "";
			severity = 1;
			break;
		}

	if(set_description)
		*set_description = description;
	if(set_raw1)
		*set_raw1 = raw1;
	if(set_severity)
		*set_severity = severity;

	return 0;
	} /* end of translate_snmp_message() */

/** Convert hrDeviceStatus and hrPrinterStatus to English
 *
 * Given a printer status expressed as hrDeviceStatus and hrPrinterStatus,
 * return a message based on RFC 3805 Printer MIB v2 Appendix E as well
 * as a string representation of the two codes and a severity assessment
 * as described above for translate_snmp_message().
 *
 * If you don't need a particular result value, pass a NULL pointer.
 */
int translate_snmp_status(int device_status, int printer_status, const char **set_description, const char **set_raw1, int *set_severity)
	{
	const char *raw1a, *raw1b;
	const char *raw1 = NULL;
	const char *description = NULL;
	int severity = 1;
	static char scratch[32];

	/* hrDeviceStatus from */
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

	/* hrPrinterStatus (from based on RFC 3805 Printer MIB v2) */
	switch(printer_status)
		{
		case 1:
			raw1b = "other(1)";
			break;
		case 2:
			raw1b = "unknown(2)";
			break;
		case 3:
			raw1b = "idle(3)";
			break;
		case 4:
			raw1b = "printing(4)";
			break;
		case 5:
			raw1b = "warmup(5)";
			break;
		default:
			raw1b = "";
			break;
		}

	/* Combine the device status and the printer status to form
	 * the raw description and the default cooked description.
	 */
	gu_snprintf(scratch, sizeof(scratch), "%s/%s", raw1a, raw1b);
	raw1 = scratch;
	
	/* Soon to be based on RFC 3805 Printer MIB v2 appendix E
	 * which is available from:
	 * <ftp://ftp.pwg.org/pub/pwg/pmp/contributions/Top25Errors.pdf>
	 */
	if(device_status == 2)				/* running */
		{
		if(printer_status == 3)			/* running/idle */
			{
			description = N_("idle");
			severity = 1;
			}
		else if(printer_status == 1)	/* running/other */
			{
			description = N_("standby");
			severity = 3;
			}
		else if(printer_status == 5)	/* running/warmup */
			{
			description = N_("warmup");
			severity = 2;
			}
		else if(printer_status == 4)	/* running/printing */
			{
			description = N_("busy");
			severity = 4;
			}
		else
			{
			severity = 4;
			}
		}
	else if(device_status == 5)			/* down */
		{
		if(printer_status == 1)			/* down/other */
			{
			description = N_("off line");
			severity = 7;
			}
		else if(printer_status == 5)	/* down/warmup */
			{
			description = N_("initial warmup");
			severity = 6;				/* <-- will clear soon */
			}
		else
			{
			severity = 7;
			}
		}
	else if(device_status == 3)			/* warning */
		{
		severity = 6;					/* <-- barely operational */
		if(printer_status == 3)			/* warning/idle */
			{
			description = N_("idle (with warning)");
			}
		else if(printer_status == 4)	/* warning/printing */
			{
			description = N_("busy (with warning)");
			}
		else if(printer_status == 5)	/* warning/warmup */
			{
			description = N_("warmup (with warning)");
			}
		}
	else								/* unknown or testing */
		{
		severity = 7;					/* <-- non-operational */
		}

	if(!description)
		description = raw1;

	if(set_description)
		*set_description = description;
	if(set_raw1)
		*set_raw1 = raw1;
	if(set_severity)
		*set_severity = severity;

	return 0;
	} /* end of translate_snmp_status() */

/* end of file */
