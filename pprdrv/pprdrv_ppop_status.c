/*
** mouse:~ppr/src/pprdrv/pprdrv_ppop_status.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 23 May 2001.
*/

#include "before_system.h"
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
#include "commentary.h"

/*
** This module contains functions for writing a printer's status file.  The
** printer status file is read by "ppop status".
**
** This module keeps track of the printer's status and dispatches
** commentary messages when necessary.
*/

/*============================================================================
** These function take care of maintaining the status file which is used
** to make the printer's status known to "ppop status".
============================================================================*/

static int message_exit = EXIT_PRINTED;
static char message_lw_status[80] = {'\0'};
static char message_pjl_status[80] = {'\0'};
static char message_snmp_status[80] = {'\0'};
static char message_job[80] = {'\0'};
static char message_writemon_operation[20] = {'\0'};
static int message_writemon_minutes = 0;
static gu_boolean message_writemon_connecting = FALSE;
static char message_pagemon[20];

/*===================================================================
** We catagorize printer error conditions according the the numbers
** used to represent them in the Printer MIB's variable
** PrinterDetectedErrorState.  This is the array of structures in
** which we keep track of which of the possible conditions are set.
===================================================================*/

#define SNMP_BITS 32

struct SNMP_BIT {
    time_t start;   		/* when we first heard of condition */
    time_t last_news;		/* when we last heard of the condition */
    time_t last_commentary;	/* last time we sent a commentary message about it */
    char details[64];
    gu_boolean shadowed;
    };

static struct SNMP_BIT snmp_bits[SNMP_BITS];

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
** We use this table when sending commentary messages to eliminate mild
** conditions which logically follow from more severe conditions which
** are also present.
**
** It will no doubt be necessary to expand this table.
*/

struct SHADOW_LIST {
    int bit;		/* which bit? */
    int first;		/* first bit it shadows */
    int second;		/* second bit it shadows or duplicate of first */
    };

static struct SHADOW_LIST shadow_list[] = {
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
** These are the bits that we clear when we have sucessfully printed a job.
*/
static int clear_on_exit_bits[] = {
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
    int hrDeviceStatus;
    int hrPrinterStatus;
    char details[64];
    time_t start;
    time_t last_news;
    time_t last_commentary;
    };

struct SNMP_STATUS status;	/* current device status */
struct SNMP_STATUS ostatus;	/* for detecting changes */

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
    for(x=0; x<SNMP_BITS; x++)
	{
	snmp_bits[x].start = 0;
	snmp_bits[x].last_news = 0;
	snmp_bits[x].last_commentary = 0;
	snmp_bits[x].details[0] = '\0';
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
	    if((count = gu_sscanf(line, "snmp: %d %t %t %t %n", &bit, &start, &last_news, &last_commentary, &details_start)) > 0)
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
		    gu_StrCopyMax(snmp_bits[bit].details, sizeof(snmp_bits[0].details), &line[details_start]);
		continue;
		}
	    }
	fclose(f);
	}
    }

    } /* end of ppop_status_init() */

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

    if(message_lw_status[0] != '\0')
	gu_snprintfcat(buffer, sizeof(buffer), "lw-status: %s\n", message_lw_status);

    if(message_pjl_status[0] != '\0')
	gu_snprintfcat(buffer, sizeof(buffer), "pjl-status: %s\n", message_pjl_status);

    if(message_snmp_status[0] != '\0')
	gu_snprintfcat(buffer, sizeof(buffer), "snmp-status: %s\n", message_snmp_status);

    /* The combined SNMP-style status. */
    if(status.start > 0)
    	gu_snprintfcat(buffer, sizeof(buffer), "status: %d %d %ld %ld %ld\n", status.hrDeviceStatus, status.hrPrinterStatus, (long)status.start, (long)status.last_news, (long)status.last_commentary);

    /* The combined SNMP-style error state. */
    {
    int x;
    for(x=0; x<SNMP_BITS; x++)
	{
	if(snmp_bits[x].start > 0)
	    gu_snprintfcat(buffer, sizeof(buffer), "error: %02d %ld %ld %ld %s\n", x, (long)snmp_bits[x].start, (long)snmp_bits[x].last_news, (long)snmp_bits[x].last_commentary, snmp_bits[x].details);
	}
    }

    if(message_writemon_operation[0] != '\0')
	gu_snprintfcat(buffer, sizeof(buffer), "operation: %s %d\n", message_writemon_connecting ? "CONNECT" : message_writemon_operation, message_writemon_minutes);

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
============================================================================*/
static void dispatch_commentary(void)
    {
    int x;
    time_t time_now = time(NULL);

    /*
    ** Dispatch a commentator message if the printer status is new or if it
    ** it is known and not "running" and has been for at least 5 minutes.
    */
    {
    gu_boolean is_new = FALSE;
    if(status.hrDeviceStatus != ostatus.hrDeviceStatus
    		|| status.hrPrinterStatus != ostatus.hrPrinterStatus
    		|| strcmp(status.details, ostatus.details)
    		)
	{
	is_new = TRUE;
	status.start = status.last_news;			/* !!! is done late !!! */
	memcpy(&ostatus, &status, sizeof(struct SNMP_STATUS));	/* save to detect future changes */
	}

    if(is_new || (status.hrDeviceStatus != DST_running && status.hrDeviceStatus != -1 && (status.last_commentary - time(NULL)) >= 300))
	{
	const char *message, *raw1; int severity;
	translate_snmp_status(status.hrDeviceStatus, status.hrPrinterStatus, &message, &raw1, &severity);
	commentary(COM_PRINTER_STATUS, message, status.details, NULL, severity);
	status.last_commentary = time(NULL);
	}
    }

    /*
    ** Set the shadowed flag on any error condition which can be taken for
    ** granted because a condition that implies it is also present.
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

    /*
    ** Dispatch commentator messages for those errors which are new or haven't
    ** been announced in 5 minutes or more and aren't shadowed.
    */
    for(x=0; x<SNMP_BITS; x++)
	{
	if(snmp_bits[x].start && (snmp_bits[x].last_commentary == 0 || (time_now - snmp_bits[x].last_commentary) >= 300))
	    {
	    const char *description; int severity;
	    char temp[10];
	    translate_snmp_error(x, &description, &severity);
	    if(!snmp_bits[x].shadowed)
		{
		snprintf(temp, sizeof(temp), "%d", (int)((time_now - snmp_bits[x].start) / 60));
		commentary(COM_PRINTER_ERROR, description, snmp_bits[x].details[0] != '\0' ? snmp_bits[x].details : NULL, temp, severity);
		}
	    snmp_bits[x].last_commentary = time_now;
	    }
	}

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
    gu_StrCopyMax(message_writemon_operation, sizeof(message_writemon_operation), operation);
    message_writemon_minutes = minutes;
    ppop_status_write();
    }

/*
** This informs us of how much time has been spent on the current page.
** These calls come from pprdrv_writemon.c.
*/
void ppop_status_pagemon(const char string[])
    {
    gu_StrCopyMax(message_pagemon, sizeof(message_pagemon), string);
    ppop_status_write();
    }

/*
** The pprdrv_feedback.c code calls this with TRUE after receiving
** "%%[ PPR connecting ]%%" and with FALSE after receiving
** "%%[ PPR connected ]%%".  (Some of the more advanced interface programs
** print these messages for our benefit.)
*/
void ppop_status_connecting(gu_boolean connecting)
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
	message_lw_status[0] = '\0';
	message_pjl_status[0] = '\0';
	message_job[0] = '\0';

	/* Now children, since we have printed a job we can assume that any
	   condition that ought to have prevented us from printing a job is
	   no longer present. */
	{
	int x, bit;
	for(x=0; (bit = clear_on_exit_bits[x]) != -1; x++)
	    {
	    snmp_bits[bit].start = 0;
	    snmp_bits[bit].last_news = 0;
	    }
	}
	}

    /* The next operation will be "waiting for commentators to exit". */
    gu_StrCopyMax(message_writemon_operation, sizeof(message_writemon_operation), "COM_WAIT");

    /* Erase the page clock. */
    message_pagemon[0] = '\0';

    /* Flush the status file so that "ppop status" can read it. */
    ppop_status_write();

    /* Does this call for PrinterDetectedErrorState commentary? */
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
**	%%[ PrinterError: xxx ]%%
**	%%[ status: xxx ]%%
**	%%[ job: xxx ]%%
**	%%[ job: xxx; status: yyy ]%%
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
	    gu_StrCopyMax(message_job, sizeof(message_job), job);
    	}

    if(pstatus)
	{
	int value1, value2, value3;
	const char *details;

	/* This is for "ppop --verbose status". */
	gu_StrCopyMax(message_lw_status, sizeof(message_lw_status), pstatus);

	if(translate_lw_message(pstatus, &value1, &value2, &value3, &details) == 0)
	    {
	    if(value1 != -1 || value2 != -1)		/* printer status */
		{
		status.hrDeviceStatus = value1;
		status.hrPrinterStatus = value2;
		gu_StrCopyMax(status.details, sizeof(status.details), details);
		status.last_news = time(NULL);
		}
	    if(value3 != -1)				/* printer error */
		{
		time_t time_now = time(NULL);
		if(value3 >= 0 && value3 <= SNMP_BITS)
		    {
		    if(snmp_bits[value3].start == 0)
			snmp_bits[value3].start = time_now;
		    snmp_bits[value3].last_news = time_now;
		    gu_StrCopyMax(snmp_bits[value3].details, sizeof(snmp_bits[0].details), details);
		    }
		else
		    {
		    error("%s(): bit %d is out of range 0--%d inclusive", function, value3, SNMP_BITS-1);
		    }
		}
	    }

        progress_new_status(pstatus);
	}

    /* Flush to status file for "ppop status". */
    ppop_status_write();

    /* Does this call for PrinterDetectedErrorState commentary? */
    dispatch_commentary();
    } /* end of handle_lw_status() */

/*
** This is called whenever a "@PJL USTATUS DEVICE" message
** is received from the printer.
**
** This function needs a translation file to convert it to
** something more readable.
*/
void handle_ustatus_device(enum PJL_ONLINE online, int code, const char message[], int code2, const char message2[])
    {
    FUNCTION4DEBUG("handle_ustatus_device")
    time_t time_now = time(NULL);

    DODEBUG_FEEDBACK(("%s(online=%d, code=%d, message[]=\"%s\", code2=%d, message2[]=\"%s\")", function, (int)online, code, message, code2, message2));

    /* This is for "ppop --verbose status". */
    snprintf(message_pjl_status, sizeof(message_pjl_status), "%s %d (%s)",
	online == PJL_ONLINE_TRUE ? "ONLINE" : online == PJL_ONLINE_FALSE ? "OFFLINE" : "?",
	code,
	message);
    if(code2 != 0)
	{
	gu_snprintfcat(message_pjl_status, sizeof(message_pjl_status), " %d (%s)",
		code2,
		message2);
	}

    /*
    ** A PJL USTATUS message can have up to two codes, each of which has a
    ** message.  Since the messages are localized (according to the printer's
    ** control-panel language setting), the messages are used only for
    ** debugging.
    **
    ** The codes themselves are looked up in pjl-messages.conf.  We use the
    ** translate_pjl_messages() function to do this.  It returns a category
    ** (0 printer status, 1 printer error, 2 PJL error) and a numberic code.
    ** For printer status the code is the SNMP status number.  For printer
    ** errors it is the SNMP DetectedErrorState bit value to which the message
    ** cooresponds.  For PJL errors the returned code has no meaning.
    */
    for( ; code; code=code2,message=message2,code2=0)
	{
	int value1, value2, value3;
	const char *details;

        if(translate_pjl_message(code, message, &value1, &value2, &value3, &details) != -1)
	    {
	    if(value1 != -1 || value2 != -1)		/* printer status */
		{
		status.hrDeviceStatus = value1;
		status.hrPrinterStatus = value2;
		gu_StrCopyMax(status.details, sizeof(status.details), details);
		status.last_news = time(NULL);
		}
	    if(value3 != -1)				/* printer error */
		{
		if(value3 >= 0 && value3 <= SNMP_BITS)
		    {
		    if(snmp_bits[value3].start == 0)
			snmp_bits[value3].start = time_now;
		    snmp_bits[value3].last_news = time_now;
		    gu_StrCopyMax(snmp_bits[value3].details, sizeof(snmp_bits[0].details), details);
		    }
		else
		    {
		    error("out of range bit %d", value3);
		    }
		}
	    if(value1 == -1 && value2 == -1 && value3 == -1)	/* PJL error */
		{
		error("PJL error: %d %s \"%s\"", code, details, message);
		}
            }
	}

    /*
    ** Use the "ONLINE=" from the PJL message to set the PrinterDetectedErrorState
    ** bit for "offline" (DES_offline, bit 6).
    */
    if(online == PJL_ONLINE_TRUE)			/* now online */
	{
	if(snmp_bits[DES_offline].start > 0)		/* if was off before, */
	    {
	    snmp_bits[DES_offline].start = 0;
	    snmp_bits[DES_offline].last_news = 0;
	    }
	}
    else						/* now offline */
	{
	if(snmp_bits[DES_offline].start == 0)		/* if was already offline, */
	    snmp_bits[DES_offline].start = time_now;
	snmp_bits[DES_offline].last_news = time_now;
	}

    /* Flush the changes to ppop. */
    ppop_status_write();

    /* Does this call for PrinterDetectedErrorState commentary? */
    dispatch_commentary();

    /* Let writemon know about online state so it can start or stop clocks. */
    writemon_online(online == PJL_ONLINE_TRUE);
    } /* end of handle_ustatus_device() */

/*
** This is called every time a "%%[ PPR SNMP: XX XX XXXXXXXX ]%%" message
** is received from the interface program.
*/
void handle_snmp_status(int device_status, int printer_status, unsigned int errorstate)
    {
    FUNCTION4DEBUG("handle_snmp_status")
    int x;
    time_t time_now;
    DODEBUG_FEEDBACK(("%s(device_status=%d, printer_status=%d, errorstate=0x%08X)", function, device_status, printer_status, errorstate));

    time(&time_now);

    /* for "ppop --verbose status". */
    snprintf(message_snmp_status, sizeof(message_snmp_status), "%d %d", device_status, printer_status);

    status.hrDeviceStatus = device_status;
    status.hrPrinterStatus = printer_status;
    status.last_news = time(NULL);

    /* step thru the bits */
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

    /* Flush to "ppop status". */
    ppop_status_write();

    /* Is it time for PrinterDetectedErrorState commentary? */
    dispatch_commentary();

    /* We mustn't do this because it will expect us to tell it when
       pages drop too! */
    #if 0
    /* Let writemon know about online state so it can start or stop clocks. */
    writemon_online(snmp_bits[DES_offline].start == 0);
    #endif
    } /* end of handle_snmp_errorstate() */

/* end of file */
