/*
** mouse:~ppr/src/pprdrv/pprdrv_ppop_status.c
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
** Last modified 28 March 2005.
*/

#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "interface.h"
#include "pprdrv.h"
#include "respond.h"

/*
** This module contains functions for writing a printer's status file.  The
** printer status file is read by "ppop status".
**
** This module keeps track of the printer's status and dispatches
** commentary messages when necessary.
*/

/*===================================================================
** This is hrDeviceStatus and hrPrinterStatus
===================================================================*/

/* hrDeviceStatus values */
#define DST_unknown 1
#define DST_running 2
#define DST_warning 3
#define DST_testing 4
#define DST_down 5

/* hrPrinterStatus values */
#define PST_other 1
#define PST_unknown 2
#define PST_idle 3
#define PST_printing 4
#define PST_warmup 5

struct SNMP_STATUS {
	int hrDeviceStatus;					/* SNMP style */
	int hrPrinterStatus;				/* SNMP style */
	char details[64];					/* info beyond SNMP */
	time_t start;						/* time when condition first detected */
	time_t last_news;					/* time when last report received */
	time_t last_commentary;				/* time when commentary() last called about this */
	};

/*===================================================================
** We catagorize printer error conditions according the the numbers
** used to represent them in the Printer MIB's variable
** hrPrinterDetectedErrorState.  Here we define a structure to keep
** track of one bit.  We will use an array of these structures.
===================================================================*/

#define SNMP_BITS 32

struct SNMP_BIT {
	time_t start;				/* when we first heard of condition */
	time_t last_news;			/* when we last heard of the condition */
	time_t last_commentary;		/* last time we sent a commentary message about it */
	char details[64];
	gu_boolean shadowed;
	};

#define DES_lowPaper 0
#define DES_noPaper 1
#define DES_lowToner 2
#define DES_noToner 3
#define DES_doorOpen 4
#define DES_jammed 5
#define DES_offline 6
#define DES_serviceRequested 7
#define DES_inputTrayMissing 8
#define DES_outputTrayMissing 9
#define DES_markerSupplyMissing 10
#define DES_outputNearFull 11
#define DES_outputFull 12
#define DES_inputTrayEmpty 13
#define DES_overduePreventMaint 14

/*
** Experience shows that printers sometimes report overlapping conditions.
** We use this table when sending commentary messages to eliminate milder
** forms of more serious conditions which are present.  For example, it is
** silly to report "paper low" when we are also reporting "paper out".
**
** It will no doubt be necessary to expand this table.
*/

struct SHADOW_LIST {
	int bit;			/* which bit? */
	int first;			/* first bit it shadows */
	int second;			/* second bit it shadows or duplicate of first */
	};

static const struct SHADOW_LIST shadow_list[] = {
		{DES_noPaper, DES_lowPaper, DES_offline},
		{DES_noToner, DES_lowToner, DES_offline},
		{DES_doorOpen, DES_offline, DES_offline},
		{DES_jammed, DES_offline, DES_offline},
		{DES_inputTrayMissing, DES_offline, DES_offline},
		{DES_outputTrayMissing, DES_offline, DES_offline},
		{DES_markerSupplyMissing, DES_offline, DES_offline},
		{DES_outputFull, DES_outputNearFull, DES_offline},
		{-1, -1, -1}
	};

/*
** These are the bits that we clear when we have sucessfully printed a job
** or when we receive a message which indicates that the printer is
** printing again.
**
** We clear them because the fact that the job has finished shows that the
** conditions have been cleared.  So we assume that we simply haven't 
** gotten a new report during the time between when the condition was cleared
** and when we disconnected.
*/
static const int clear_on_printing_bits[] = {
	DES_noPaper,
	DES_noToner,
	DES_doorOpen,
	DES_jammed,
	DES_offline,
	DES_inputTrayMissing,
	DES_outputTrayMissing,
	DES_markerSupplyMissing,
	DES_outputFull,
	DES_inputTrayEmpty,
	-1
	};

/*============================================================================
** Here are the actual variables which we use to store the ongoing 
** status of the printer.
============================================================================*/

static struct {

	struct {
		gu_boolean important;
		char message[80];
		} lw_status;

	struct {
		int online;

		gu_boolean important;
		int code;
		char message[80];

		gu_boolean important2;
		int code2;
		char message2[80];
		} pjl_status;

	} ppop_status;

static int message_exit = EXIT_PRINTED;
static char message_snmp_status[80] = {'\0'};
static char message_job[80] = {'\0'};
static char message_writemon_operation[20] = {'\0'};
static int message_writemon_minutes = 0;
static const char *message_writemon_connecting = NULL;
static char message_pagemon[20];

struct SNMP_STATUS status;		/* current device status */
struct SNMP_STATUS ostatus;		/* for detecting changes */

static struct SNMP_BIT snmp_bits[SNMP_BITS];

/*============================================================================
** These functions manage the printer status file which "ppop status" reads.
============================================================================*/

/*
** This is called at program startup.  It initializes the data structures
** which keep track of the printer's status and reloads values left
** by the last copy of pprdrv to run for this printer.
*/
void ppop_status_init(void)
	{
	const char function[] = "ppop_status_init";

	/* Initialize the LaserWriter-style status to unknown. */
	ppop_status.lw_status.important = FALSE;
	ppop_status.lw_status.message[0] = '\0';

	/* Initialize the PJL status to unknown. */
	ppop_status.pjl_status.online = PJL_ONLINE_UNKNOWN;

	ppop_status.pjl_status.code = -1;
	ppop_status.pjl_status.message[0] = '\0';
	ppop_status.pjl_status.important = FALSE;

	ppop_status.pjl_status.code2 = -1;
	ppop_status.pjl_status.message2[0] = '\0';
	ppop_status.pjl_status.important2 = FALSE;

	/* Initialize the device and printer status to unknown */
	status.hrDeviceStatus = -1;
	status.hrPrinterStatus = -1;
	status.details[0] = '\0';
	status.start = 0;
	status.last_news = 0;
	status.last_commentary = 0;
	memcpy(&ostatus, &status, sizeof(struct SNMP_STATUS));

	/* Clear all bits in the representation of hrPrinterDetectedErrorState. */
	{
	int x;
	for(x=0; x < SNMP_BITS; x++)
		{
		snmp_bits[x].start = 0;
		snmp_bits[x].last_news = 0;
		snmp_bits[x].last_commentary = 0;
		snmp_bits[x].details[0] = '\0';
		snmp_bits[x].shadowed = FALSE;
		}
	}

	/*
	** Read the previous values from the file that "ppop status" reads.
	** Note that only bits with "start" set to non-zero will have been
	** saved.
	*/
	{
	char fname[MAX_PPR_PATH];
	FILE *f;
	ppr_fnamef(fname, "%s/%s", STATUSDIR, printer.Name);
	if((f = fopen(fname, "r")))
		{
		char *line = NULL;
		int line_space = 80;
		int count, bit; long int start, last_news, last_commentary; int details_start;
		while((line = gu_getline(line, &line_space, f)))
			{
			if((count = gu_sscanf(line, "status: %d %d %t %t %t", &status.hrDeviceStatus, &status.hrPrinterStatus, &status.start, &status.last_news, &status.last_commentary)) > 0)
				{
				if(count < 3)
					error("%s(): can't parse \"%s\"", function, line);
				continue;
				}
			if((count = gu_sscanf(line, "errorstate: %d %t %t %t %n", &bit, &start, &last_news, &last_commentary, &details_start)) > 0)
				{
				if(count < 4)
					{
					error("%s(): can't parse \"%s\"", function, line);
					continue;
					}
				if(bit < 0 || bit >= SNMP_BITS)
					{
					error("%s(): bit %d is out of range 0--%d inclusive", function, bit, SNMP_BITS-1);
					continue;
					}
				snmp_bits[bit].start = start;
				snmp_bits[bit].last_news = last_news;
				snmp_bits[bit].last_commentary = last_commentary;
				if(count > 4)
					gu_strlcpy(snmp_bits[bit].details, &line[details_start], sizeof(snmp_bits[0].details));
				continue;
				}
			}
		fclose(f);
		}
	}

	} /* end of ppop_status_init() */

/*
 * This should be called whenever the SNMP style status is changed, before
 * calling ppop_status_write() or dispatch_commentary().
 */
static void snmp_status_fixup(void)
	{
	int x;

	/*
	** This is where we handle status.start.  The functions that record the status in
	** this structure don't bother to set status.start, they just set status.last_news
	** to the current time.  Here we see if the status has changed and if so, set 
	** status.start to the time in status.last_news.
	*/
	if(status.hrDeviceStatus != ostatus.hrDeviceStatus					/* if device status changed */
				|| status.hrPrinterStatus != ostatus.hrPrinterStatus	/* if printer status changed */
				|| strcmp(status.details, ostatus.details)				/* if printer status details changed */
				)
		{
		status.start = status.last_news;
		}

	/*
	** Set the shadowed flag on any error condition which can be taken for
	** granted because a more serious condition that implies it is also true.
	*/
	for(x=0; x<SNMP_BITS; x++)
		{
		snmp_bits[x].shadowed = FALSE;
		}
	for(x=0; shadow_list[x].bit != -1; x++)
		{
		int bit = shadow_list[x].bit;
		if(snmp_bits[bit].start > 0)
			{
			snmp_bits[shadow_list[bit].first].shadowed = TRUE;
			snmp_bits[shadow_list[bit].second].shadowed = TRUE;
			}
		}
	} /* end of snmp_status_fixup() */

/*
** This function writes the status file.  This status file is
** read by the "ppop status" command to get auxiliary information
** such as why the printer is off-line.
**
** Other functions in this module call this after updating the
** in-memory data structures which describe the printer's state.
*/
static void ppop_status_write(void)
	{
	const char function[] = "ppop_status_write";
	char buffer[1024];
	static int statfile = -1;

	DODEBUG_PPOP_STATUS(("%s()", function));

	buffer[0] = '\0';

	if(message_exit != EXIT_PRINTED)
		gu_snprintfcat(buffer, sizeof(buffer), "exit: %d\n", message_exit);

	/* The combined SNMP-style status.  This includes status in other formats 
	   converted to SNMP format.  The member status.start is the time in
	   Unix format when we received this information.  It will be zero if we
	   haven't.
	   */
	if(status.start)
		gu_snprintfcat(buffer, sizeof(buffer), "status: %d %d %ld %ld %ld\n", status.hrDeviceStatus, status.hrPrinterStatus, (long)status.start, (long)status.last_news, (long)status.last_commentary);

	/* This is the combined SNMP-style printer error state flags.  Note that 
	   this is not the raw SNMP error state since this includes status 
	   messages received by other means that have been converted to 
	   SNMP format.
	*/
	{
	int x;
	for(x=0; x<SNMP_BITS; x++)
		{
		if(snmp_bits[x].start > 0)
			gu_snprintfcat(buffer, sizeof(buffer), "errorstate: %02d %ld %ld %ld %s\n", x, (long)snmp_bits[x].start, (long)snmp_bits[x].last_news, (long)snmp_bits[x].last_commentary, snmp_bits[x].details);
		}
	}

	if(ppop_status.lw_status.message[0] != '\0')
		{
		gu_snprintfcat(buffer, sizeof(buffer), "lw-status: %d %s\n",
				ppop_status.lw_status.important ? 1 : 0,
				ppop_status.lw_status.message);
		}

	if(ppop_status.pjl_status.online != PJL_ONLINE_UNKNOWN)
		{
		gu_snprintfcat(buffer, sizeof(buffer), "pjl-status: 0 %s\n",
				ppop_status.pjl_status.online == PJL_ONLINE_TRUE ? "ONLINE" : ppop_status.pjl_status.online == PJL_ONLINE_FALSE ? "OFFLINE" : "?");
		}
		
	if(ppop_status.pjl_status.code > 0)
		{
		gu_snprintfcat(buffer, sizeof(buffer), "pjl-status: %d %d %s\n",
				ppop_status.pjl_status.important ? 1 : 0,
				ppop_status.pjl_status.code,
				ppop_status.pjl_status.message);
		}

	if(ppop_status.pjl_status.code > 0)
		{
		gu_snprintfcat(buffer, sizeof(buffer), "pjl-status: %d %d %s\n",
				ppop_status.pjl_status.important2 ? 1 : 0,
				ppop_status.pjl_status.code2,
				ppop_status.pjl_status.message2);
		}

	if(message_snmp_status[0] != '\0')
		gu_snprintfcat(buffer, sizeof(buffer), "snmp-status: 0 %s\n", message_snmp_status);

	if(message_writemon_operation[0] != '\0')
		{
		gu_snprintfcat(buffer, sizeof(buffer), "operation: %s %d\n",
				message_writemon_connecting ? message_writemon_connecting : message_writemon_operation,
				message_writemon_minutes
				);
		}

	if(message_pagemon[0] != '\0')
		gu_snprintfcat(buffer, sizeof(buffer), "page: %s\n", message_pagemon);

	if(message_job[0] != '\0')
		gu_snprintfcat(buffer, sizeof(buffer), "job: %s\n", message_job);

	DODEBUG_PPOP_STATUS(("%s(): buffer=\"%s\"", function, buffer));

	if(test_mode)
		{
		write(2, buffer, strlen(buffer));
		}
	else
		{
		/* If the status file isn't open yet, open it now.  We don't close it,
		   it gets closed when we exit. */
		if(statfile == -1)
			{
			char fname[MAX_PPR_PATH];
			ppr_fnamef(fname, "%s/%s", STATUSDIR, printer.Name);
			if((statfile = open(fname, O_WRONLY | O_CREAT | O_TRUNC, UNIX_644)) == -1)
				fatal(EXIT_PRNERR, "%s(): failed to open \"%s\" for write, errno=%d (%s)", function, fname, errno, gu_strerror(errno));
			gu_set_cloexec(statfile);
			}

		/* Move back to the begining of the file and write the new status. */
		ftruncate(statfile, (off_t)0);
		lseek(statfile, (off_t)0, SEEK_SET);
		write(statfile, buffer, strlen(buffer));
		}
	} /* end of ppop_status_write() */

/*============================================================================
** This function dispatches commentary for printer errors.
** It sends a commentary message when the error first occurs and every
** 5 minutes thereafter.
**
** This code is pretty sad, but it won't be better until we revamp the 
** commentator system.
============================================================================*/
static void dispatch_commentary(void)
	{
	int x;
	int greatest_severity = 0;
	time_t time_now = time(NULL);

	/*
	** Dispatch commentator messages for those SNMP hrPrinterDetectedErrorState-style
	** errors which are new or haven't been announced in 5 minutes or more and aren't 
	** shadowed.
	*/
	for(x=0; x<SNMP_BITS; x++)
		{
		if(snmp_bits[x].start && (snmp_bits[x].last_commentary == 0 || (time_now - snmp_bits[x].last_commentary) >= 300))
			{
			const char *description, *raw1;
			int severity;
			translate_snmp_error(x, &description, &raw1, &severity);
			if(!snmp_bits[x].shadowed)
				{
				commentary(COM_PRINTER_ERROR,
						description,
						raw1,
						snmp_bits[x].details[0] != '\0' ? snmp_bits[x].details : NULL,
						(int)(time_now - snmp_bits[x].start),
						severity);
				if(severity > greatest_severity)
					greatest_severity = severity;
				}
			snmp_bits[x].last_commentary = time_now;
			}
		}

	/*
	** Does hrDeviceStatus indicate something is wrong?  If so and we didn't
	** anounced anything important above, then send a COM_PRINTER_STATUS
	** message based on hrDeviceStatus and hrPrinterStatus.
	*/
	if((status.hrDeviceStatus != -1 && status.hrDeviceStatus != DST_running) 
				&& (status.start != ostatus.start || (status.last_commentary - time(NULL)) >= 300))
		{
		if(greatest_severity <= 5)
			{
			const char *message, *raw1; int severity;
			translate_snmp_status(status.hrDeviceStatus, status.hrPrinterStatus, &message, &raw1, &severity);
			commentary(COM_PRINTER_STATUS, message, raw1, status.details, 0, severity);
			}
		status.last_commentary = time(NULL);
		}

	memcpy(&ostatus, &status, sizeof(struct SNMP_STATUS));		/* save to detect future changes */
	} /* end of dispatch_commentary() */

/*============================================================================
** These functions change fields in the status file and then write
** it out to disk.
============================================================================*/

/*
** This is called every time the writemon system sends a commentator message.
** This allows "ppop status" to display information about stalled jobs.
** (A job is considered stalled when communication with the printer hasn't
** made progress for several minutes.)
*/
void ppop_status_writemon(const char operation[], int minutes)
	{
	DODEBUG_PPOP_STATUS(("ppop_status_writemon(operation=\"%s\", minutes=%d)", operation, minutes));
	gu_strlcpy(message_writemon_operation, operation, sizeof(message_writemon_operation));
	message_writemon_minutes = minutes;
	ppop_status_write();
	}

/*
** This informs us of how much time has been spent on the current page.
** These calls come from pprdrv_writemon.c.
*/
void ppop_status_pagemon(const char string[])
	{
	gu_strlcpy(message_pagemon, string, sizeof(message_pagemon));
	ppop_status_write();
	}

/*
** The pprdrv_feedback.c code calls this with TRUE after receiving
** "%%[ PPR connecting ]%%" and with FALSE after receiving
** "%%[ PPR connected ]%%".	 (Some of the more advanced interface programs
** print these messages for our benefit.)
*/
void ppop_status_connecting(const char connecting[])
	{
	message_writemon_connecting = connecting;
	ppop_status_write();
	}

/*
** This is called when pprdrv is almost ready to exit.  The only thing it
** does afterward is call comentator_wait() and ppop_status_shutdown().
**
** The parameter is the value which will be passed to exit().  We use it to
** determine if the job was printed sucessfully.  If the job was printed
** sucessfully, we clear certain error conditions.
*/
void ppop_status_exit_hook(int retval)
	{
	DODEBUG_PPOP_STATUS(("ppop_status_exit_hook(retval=%d)", retval));

	/* This will help "ppop status" display "Fault:". */
	message_exit = retval;

	if(retval == EXIT_PRINTED || retval == EXIT_JOBERR)
		{
		/* Since we sucessfully printed a job, these have surely either ceased
		   to be true or ceased to be interesting. */
		ppop_status.lw_status.message[0] = '\0';
		ppop_status.pjl_status.code = -1;
		ppop_status.pjl_status.code2 = -1;
		message_job[0] = '\0';

		/* If the last report was that the printer is "printing",
		   then set status.start to zero as this will be untrue as soon as
		   we disconnect.
		   */
		if(status.hrPrinterStatus == PST_printing)
			{
			status.hrPrinterStatus = PST_idle;
			status.start = status.last_news = time(NULL);
			}

		/* Now children, since we have printed a job we can assume that any
		   condition that ought to have prevented us from printing a job is
		   no longer present. */
		{
		int x, bit;
		for(x=0; (bit = clear_on_printing_bits[x]) != -1; x++)
			{
			snmp_bits[bit].start = 0;
			snmp_bits[bit].last_news = 0;
			}
		}
		}

	/* The next operation will be "waiting for commentators to exit". */
	gu_strlcpy(message_writemon_operation, "COM_WAIT", sizeof(message_writemon_operation));

	/* Erase the page clock. */
	message_pagemon[0] = '\0';

	snmp_status_fixup();
	
	/* Flush the status file so that "ppop status" can read it. */
	ppop_status_write();

	/* Does this call for hrPrinterDetectedErrorState commentary? */
	dispatch_commentary();
	} /* end of ppop_status_exit_hook() */

/*
** This is called immediately before pprdrv exits.
*/
void ppop_status_shutdown(void)
	{
	/* All operations have ceased. */
	message_writemon_operation[0] = '\0';

	/* Flush to status file for "ppop status". */
	ppop_status_write();
	} /* end of ppop_status_shutdown() */

/*========================================================================
** These functions are the hooks that pprdrv_feedback.c calls whenever
** it receives new information about the printer's status.
========================================================================*/

/*
** This is called whenever a message in any of these forms is received
** from the printer:
**
**		%%[ PrinterError: xxx ]%%
**		%%[ status: xxx ]%%
**		%%[ job: xxx ]%%
**		%%[ job: xxx; status: yyy ]%%
**
** The parameter pstatus[] is the text after "status:" while job[] is the text
** after "job:".
*/
void handle_lw_status(const char pstatus[], const char job[])
	{
	const char function[] = "handle_lw_status";
	DODEBUG_FEEDBACK(("%s(pstatus=\"%s\", job=\"%s\")", function, pstatus ? pstatus : "", job ? job : ""));

	/* If there is a job name, clear the job string if it is our job,
	   otherwise record the job name. */
	if(job)
		{
		if(strcmp(job, QueueFile) == 0)
			message_job[0] = '\0';
		else
			gu_strlcpy(message_job, job, sizeof(message_job));
		}

	/* If we got the printer status, */
	if(pstatus)
		{
		int lookup_retcode;
		int value1, value2, value3;
		const char *details;

		/* Now we look it up in a dictionary of LW status messages.  The goal is 
		   to integrate it into the SNMP way of describing printer condition.
		   */
		if((lookup_retcode = translate_lw_message(pstatus, &value1, &value2, &value3, &details)) != -1)
			{
			DODEBUG_FEEDBACK(("%s(): value1=%d, value2=%d, value3=%d", function, value1, value2, value3));
			if(value1 != -1 || value2 != -1)			/* printer status */
				{
				status.hrDeviceStatus = value1;
				status.hrPrinterStatus = value2;
				gu_strlcpy(status.details, details, sizeof(status.details));
				status.last_news = time(NULL);
				}
			if(value3 != -1)							/* printer errors */
				{
				time_t time_now = time(NULL);
				DODEBUG_FEEDBACK(("%s(): error bit %d is set at %ld", function, value3, (long)time_now));
				if(value3 >= 0 && value3 < SNMP_BITS)
					{
					if(snmp_bits[value3].start == 0)
						snmp_bits[value3].start = time_now;
					snmp_bits[value3].last_news = time_now;
					gu_strlcpy(snmp_bits[value3].details, details, sizeof(snmp_bits[0].details));
					}
				else
					{
					error("%s(): bit %d is out of range 0--%d inclusive", function, value3, SNMP_BITS-1);
					}
				}

			/* If there is a new hrPrinterStatus of printing(4), then clear bits which
			 * indicate problems which would prevent printing.
			 */
			if(value2 == 4)
				{
				int x, bit;
				for(x=0; (bit = clear_on_printing_bits[x]) != -1; x++)
					{
					snmp_bits[bit].start = 0;
					snmp_bits[bit].last_news = 0;
					}
				}
			}

		/* This is for "ppop --verbose status".  It will be described as
		   "Raw LW Status".  The number in front of it will be 1 if we didn't
		   understand its meaning.  That will prompt ppop to display it
		   even if --verbose wasn't used.
		   */
		ppop_status.lw_status.important = (lookup_retcode == -1) ? 1 : 0;
		gu_strlcpy(ppop_status.lw_status.message, pstatus, sizeof(ppop_status.lw_status.message));

		/* Send this to GUI interfaces and other things that want up-to-the minute updates. */
		progress_new_status(pstatus);
		}

	snmp_status_fixup();
	
	/* Flush to status file for "ppop status". */
	ppop_status_write();

	/* Does this call for hrPrinterDetectedErrorState commentary? */
	dispatch_commentary();
	} /* end of handle_lw_status() */

/*
** This is called whenever a "@PJL USTATUS DEVICE" message
** is received from the printer.
**
** This function consults a translation file in order to convert it to
** something more readable.
*/
void handle_ustatus_device(enum PJL_ONLINE online, int code, const char message[], int code2, const char message2[])
	{
	FUNCTION4DEBUG("handle_ustatus_device")
	time_t time_now = time(NULL);
	int i;
	struct USTATUS {
		int code;
		const char *message;
		gu_boolean understood;
		};
	struct USTATUS ustatus[] = {
		{code, message, FALSE},
		{code2, message2, FALSE}
		};
	
	DODEBUG_FEEDBACK(("%s(online=%d, code=%d, message[]=\"%s\", code2=%d, message2[]=\"%s\")", function, (int)online, code, message, code2, message2));

	/*
	** A PJL USTATUS message can have up to two codes, each of which has a
	** message.  Since the messages are localized (according to the printer's
	** control-panel language setting), the messages are used only for
	** debugging.
	**
	** The codes themselves are looked up in pjl-messages.conf.  We use the
	** translate_pjl_messages() function to do this.
	*/
	for(i=0 ; i<2; i++)
		{
		int hrDeviceStatus, hrPrinterStatus, hrPrinterDetectedErrorState;
		const char *details;

		/* The code will be zero if the printer didn't send one, so do nothing. */
		if(ustatus[i].code == 0)
			{
			ustatus[i].understood = TRUE;
			}

		else if(translate_pjl_message(ustatus[i].code, ustatus[i].message, &hrDeviceStatus, &hrPrinterStatus, &hrPrinterDetectedErrorState, &details) != -1)
			{
			/* If we derived an SNMP device or printer status from that code, */
			if(hrDeviceStatus != -1 || hrPrinterStatus != -1)
				{
				status.hrDeviceStatus = hrDeviceStatus;
				status.hrPrinterStatus = hrPrinterStatus;
				gu_strlcpy(status.details, details, sizeof(status.details));
				status.last_news = time_now;
				ustatus[i].understood = TRUE;
				}

			/* If we derived an SNMP hrPrinterDetectedErrorState from that code, */
			if(hrPrinterDetectedErrorState != -1)
				{
				if(hrPrinterDetectedErrorState >= 0 && hrPrinterDetectedErrorState < SNMP_BITS)
					{
					if(snmp_bits[hrPrinterDetectedErrorState].start == 0)
						snmp_bits[hrPrinterDetectedErrorState].start = time_now;
					snmp_bits[hrPrinterDetectedErrorState].last_news = time_now;
					gu_strlcpy(snmp_bits[hrPrinterDetectedErrorState].details, details, sizeof(snmp_bits[0].details));
					ustatus[i].understood = TRUE;
					}
				else
					{
					error("out of range bit %d", hrPrinterDetectedErrorState);
					}
				}

			/* If we can make nothing of it, it probly was a PJL error having nothing to do
			   with the state of the hardware.
			   */
			if(hrDeviceStatus == -1 && hrPrinterStatus == -1 && hrPrinterDetectedErrorState == -1)
				{
				error("PJL error: %d \"%s\" \"%s\"", ustatus[i].code, details, ustatus[i].message);
				}
			}
		}

	/*
	** Use the "ONLINE=" from the PJL message to set the hrPrinterDetectedErrorState
	** bit for "offline" (DES_offline, bit 6).
	*/
	if(online == PJL_ONLINE_TRUE)						/* now online */
		{
		if(snmp_bits[DES_offline].start > 0)			/* if was offline before, */
			{
			snmp_bits[DES_offline].start = 0;
			snmp_bits[DES_offline].last_news = 0;
			}
		}
	else												/* now offline */
		{
		if(snmp_bits[DES_offline].start == 0)			/* if was online, */
			snmp_bits[DES_offline].start = time_now;	/* record time went offline */
		snmp_bits[DES_offline].last_news = time_now;	/* update record of when last known to be offline */
		}

    /*
    ** Clear any hrPrinterDetectedErrorState bits that weren't re-asserted in this message.
    ** Things such as "paper low" have been observed to appear as code2 with code2
    ** disappearing when the condition is gone.
    */
    for(i=0; i < SNMP_BITS; i++)
    	{
		if(snmp_bits[i].start > 0 && snmp_bits[i].last_news < time_now)
			{
			snmp_bits[i].start = 0;
			snmp_bits[i].last_news = 0;
			}
    	}

	/*
	** Copy this into the record as raw PJL status information.  Note that this
	** block of code must come _after_ the loop that translates PJL codes to
	** SNMP codes since it sets the .important member for codes that weren't
	** recognized in the loop.
	*/
	ppop_status.pjl_status.online = online;

	ppop_status.pjl_status.important = !ustatus[0].understood;
	ppop_status.pjl_status.code = ustatus[0].code;
	gu_strlcpy(ppop_status.pjl_status.message, ustatus[0].message, sizeof(ppop_status.pjl_status.message));

	ppop_status.pjl_status.important2 = !ustatus[1].understood;
	ppop_status.pjl_status.code2 = ustatus[1].code;
	gu_strlcpy(ppop_status.pjl_status.message2, ustatus[1].message, sizeof(ppop_status.pjl_status.message2));

	snmp_status_fixup();
	
	/*
	** Flush the changes to a place where "ppop status" can find them.
	*/
	ppop_status_write();

	/*
	** Does the current state call for hrPrinterDetectedErrorState commentary?
	** If so, this does it.
	*/
	dispatch_commentary();

	/*
	** Let writemon know about online state so it can start or stop clocks such
	** as the "page clock" and the clock that detects unexplained stalls in
	** data transmission.
	*/
	writemon_online(online == PJL_ONLINE_TRUE);
	} /* end of handle_ustatus_device() */

/*
** This is called every time a "%%[ PPR SNMP: XX XX XXXXXXXX ]%%" message
** is received from the interface program.
**
** device_status (hrDeviceStatus)
**
** printer_status (hrPrinterStatus)
**
** errorstate (hrPrinterDetectedErrorState)
**		This is a bitmap of current printer error conditions.
*/
void handle_snmp_status(int device_status, int printer_status, unsigned int errorstate)
	{
	FUNCTION4DEBUG("handle_snmp_status")
	int x;
	time_t time_now;
	DODEBUG_FEEDBACK(("%s(device_status=%d, printer_status=%d, errorstate=0x%08X)", function, device_status, printer_status, errorstate));

	time(&time_now);

	/* Note when this message came in. */
	status.last_news = time(NULL);

	/* for "ppop --verbose status". */
	snprintf(message_snmp_status, sizeof(message_snmp_status), "%d %d", device_status, printer_status);

	/* These are easy.  We just save them. */
	status.hrDeviceStatus = device_status;
	status.hrPrinterStatus = printer_status;

	/* For hrPrinterDetectedErrorState we must step thru the bits. */
	for(x=0; x<SNMP_BITS; x++)
		{
		/* If the bit is set right now, */
		if(errorstate & (1 << x))
			{
			DODEBUG_FEEDBACK(("%s(): bit %d is set", function, x));

			gu_snprintfcat(message_snmp_status, sizeof(message_snmp_status), " %d", x);

			/* if it wasn't set before, */
			if(snmp_bits[x].start == 0)
				snmp_bits[x].start = time_now;
			snmp_bits[x].last_news = time_now;
			}
		/* If the bit is not set right now, */
		else
			{
			DODEBUG_FEEDBACK(("%s(): bit %d is clear", function, x));

			/* if it was set before, */
			if(snmp_bits[x].start > 0)
				{
				snmp_bits[x].start = 0;
				snmp_bits[x].last_news = 0;
				}
			}
		}

	snmp_status_fixup();
	
	/* Flush to "ppop status". */
	ppop_status_write();

	/* Is it time for hrPrinterDetectedErrorState commentary? */
	dispatch_commentary();

	/* We mustn't do this because it would expect us to tell it when
	   pages drop too! */
	#if 0
	/* Let writemon know about online state so it can start or stop clocks. */
	writemon_online(snmp_bits[DES_offline].start == 0);
	#endif
	} /* end of handle_snmp_status() */

/* end of file */
