/*
** mouse:~ppr/src/interfaces/tcpip.c
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
** Last revised 11 May 2001.
*/

/*
** Raw TCP/IP interface for communicating with HP Jetdirect cards or
** Extended Systems Pocket Print Servers.  This interface is either
** used directly to send PostScript, or is called by the gstcpip interface
** to send Ghostscript's output to the printer.
*/

#include "before_system.h"
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "libppr_int.h"
#include "interface.h"

#if 0
#define DEBUG 1
#define DODEBUG(a) int_debug a
#else
#undef DEBUG
#define DODEBUG(a)
#endif

/*
** Tie it all together.
*/
int main(int argc, char *argv[])
    {
    int sndbuf_size = 0;		/* size for SO_SNDBUF, 0 means don't set it */
    int option_sleep = 0;		/* time to sleep() after printing */
    int connect_timeout = 20;		/* connexion timeout in seconds */
    int idle_status_interval = 0;	/* frequency of ^T transmission */
    int snmp_status_interval = 0;
    const char *snmp_community = NULL;
    gu_boolean refused_engaged = TRUE;
    int refused_retries = 5;
    int sockfd;
    unsigned int address;		/* IP address of printer */
    struct gu_snmp *snmp_obj = NULL;	/* Object for SNMP queries */

    /* Initialize internation messages library. */
    #ifdef INTERNATIONAL
    setlocale(LC_MESSAGES, "");
    bindtextdomain(PACKAGE_INTERFACES, LOCALEDIR);
    textdomain(PACKAGE_INTERFACES);
    #endif

    /* Process the command line and leave the results in
       a global structure called "int_cmdline". */
    int_cmdline_set(argc, argv);

    DODEBUG(("============================================================"));
    DODEBUG(("tcpip printer=\"%s\", address=\"%s\", options=\"%s\", jobbreak=%d, feedback=%d, codes=%d, jobname=\"%s\", routing=\"%s\", forline=\"%s\", barbarlang=\"%s\"",
    	int_cmdline.printer,
    	int_cmdline.address,
    	int_cmdline.options,
    	int_cmdline.jobbreak,
    	int_cmdline.feedback,
    	int_cmdline.codes,
    	int_cmdline.jobname,
    	int_cmdline.routing,
    	int_cmdline.forline,
    	int_cmdline.barbarlang));

    /* Check for unusable job break methods. */
    if(int_cmdline.jobbreak == JOBBREAK_SIGNAL || int_cmdline.jobbreak == JOBBREAK_SIGNAL_PJL)
    	{
    	alert(int_cmdline.printer, TRUE,
    		_("The jobbreak methods \"signal\" and \"signal/pjl\" are not compatible with\n"
    		"the PPR interface program \"%s\"."), int_cmdline.int_basename);
    	int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
    	}

    /* If feedback is on, and control-d handshaking is on, turn on the ^T stuff. */
    if(int_cmdline.feedback && int_cmdline.jobbreak == JOBBREAK_CONTROL_D)
	idle_status_interval = 15;

    /* Parse the options string, searching for name=value pairs. */
    {
    struct OPTIONS_STATE o;
    char name[32];
    char value[16];
    int retval;

    options_start(int_cmdline.options, &o);
    while((retval = options_get_one(&o, name, sizeof(name), value, sizeof(value))) > 0)
    	{
	/*
	** The delay after closing connection, before exiting.
	*/
	if(strcmp(name, "sleep") == 0)
	    {
	    if((option_sleep = atoi(value)) < 0)
	    	{
	    	o.error = N_("value must be 0 or a positive integer");
	    	retval = -1;
	    	break;
	    	}
	    }
	/*
	** Size for TCP/IP send buffer
	*/
	else if(strcmp(name, "sndbuf_size") == 0)
	    {
	    if((sndbuf_size = atoi(value)) < 1)
	    	{
		o.error = N_("value must be a positive integer");
		retval = -1;
		break;
	    	}
	    }
	/*
	** Connect timeout
	*/
	else if(strcmp(name, "connect_timeout") == 0)
	    {
	    if((connect_timeout = atoi(value)) < 1)
	    	{
		o.error = N_("value must be a positive integer");
		retval = -1;
		break;
	    	}
	    }
	/*
	** Frequency of ^T transmission
	*/
	else if(strcmp(name, "idle_status_interval") == 0)
	    {
	    if((idle_status_interval = atoi(value)) < 0)
	    	{
	    	o.error = N_("value must be 0 or a positive integer");
	    	retval = -1;
	    	break;
	    	}
	    }
	/*
	** Frequency of SNMP status queries
	*/
	else if(strcmp(name, "snmp_status_interval") == 0)
	    {
	    if((snmp_status_interval = atoi(value)) < 0)
	    	{
		o.error = N_("value must be 0 or a positive integer");
		retval = -1;
		break;
	    	}
	    }
	/*
	** SNMP community name.
	*/
	else if(strcmp(name, "snmp_community") == 0)
	    {
	    snmp_community = gu_strdup(value);
	    }
	/*
	** refused=engaged
	** refused=error
	*/
	else if(strcmp(name, "refused") == 0)
	    {
	    if(strcmp(value, "engaged") == 0)
	    	refused_engaged = TRUE;
	    else if(strcmp(value, "error") == 0)
	    	refused_engaged = FALSE;
	    else
	    	{
		o.error = N_("value must be \"engaged\" or \"error\"");
		retval = -1;
		break;
	    	}
	    }
	/*
	** refused_retries
	*/
	else if(strcmp(name, "refused_retries") == 0)
	    {
	    if((refused_retries = atoi(value)) < 0)
	    	{
		o.error = N_("value must be 0 or a positive integer");
		retval = -1;
		break;
	    	}
	    }
	/*
	** Catch anything else.
	*/
	else
	    {
	    o.error = N_("unrecognized keyword");
	    o.index = o.index_of_name;
	    retval = -1;
	    break;
	    }

	} /* end of while() loop */

    if(retval == -1)
    	{
    	alert(int_cmdline.printer, TRUE, _("Option parsing error:  %s"), gettext(o.error));
    	alert(int_cmdline.printer, FALSE, "%s", o.options);
    	alert(int_cmdline.printer, FALSE, "%*s^ %s", o.index, "", _("right here"));
    	int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
    	}
    }

    /* We can't use control-T status updates if the job isn't PostScript. */
    if(int_cmdline.barbarlang[0])
	{
    	idle_status_interval = 0;
	DODEBUG(("barbarlang=\"%s\", setting idle_status_interval to 0", int_cmdline.barbarlang));
    	}

    /* Describe the options in the debuging output. */
    DODEBUG(("option_sleep=%d, connect_timeout=%d, sndbuf_size=%d, idle_status_interval=%d",
	option_sleep,
    	connect_timeout,
    	sndbuf_size,
    	idle_status_interval));

    /* Connect to the printer */
    gu_write_string(1, "%%[ PPR connecting ]%%\n");
    sockfd = int_connect_tcpip(connect_timeout, sndbuf_size, refused_engaged, refused_retries, snmp_community, &address);
    gu_write_string(1, "%%[ PPR connected ]%%\n");

    /* Disable SIGPIPE.  We will catch the error on write(). */
    signal_interupting(SIGPIPE, SIG_IGN);

    /* If we will be doing SNMP queries, create the SNMP object. */
    if(snmp_status_interval > 0)
    	{
	int error_code;
       	if(!(snmp_obj = gu_snmp_open(address, snmp_community, &error_code)))
	    {
	    alert(int_cmdline.printer, TRUE, "gu_snmp_open() failed, error_code=%d", error_code);
	    return EXIT_PRNERR;
	    }
	}

    /* Copy the file from STDIN to the printer. */
    int_copy_job(sockfd, idle_status_interval, int_printer_error_tcpip, int_snmp_status, snmp_obj, snmp_status_interval);

    /* Destroy the SNMP object. */
    if(snmp_status_interval > 0)
	gu_snmp_close(snmp_obj);

    /* Close the connection */
    close(sockfd);

    /*
    ** Pocket print servers have been known to reject a new
    ** connection for a few seconds after closing the previous one.
    ** If more than one job is in the queue at one time, this can result
    ** in every other print attempt producing a fault.  This
    ** problem is minor and can go unnoticed, but we will have
    ** an option to sleep for a specified number of seconds
    ** after closing the connection.
    **
    ** option_sleep is set with the option "sleep=SECONDS".
    */
    if(option_sleep > 0)
	{
	DODEBUG(("Sleeping for %d seconds for printer recovery", option_sleep));
	sleep(option_sleep);
	}

    DODEBUG(("sucessful completion"));

    /* We can assume that it was printed. */
    return EXIT_PRINTED;
    } /* end of main() */

/* end of file */

