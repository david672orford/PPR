/*
** mouse:~ppr/src/interfaces/parallel.c
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
** Last modified 20 June 2001.
*/

/*
** PPR interface to drive a parallel printer.
*/

#include "before_system.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <ctype.h>
#include <termios.h>
#include <string.h>
#include <sys/ioctl.h>		/* for Linux */
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "interface.h"
#include "libppr_int.h"
#include "parallel.h"

/*
** Here we possible enable debugging.  The debuging output goes to
** the file "/var/spool/ppr/logs/interface_parallel".
*/
#if 0
#define DEBUG 1
#define DODEBUG(a) int_debug a
#else
#undef DEBUG
#define DODEBUG(a)
#endif

/*
** Open the printer port.
*/
static int int_connect_parallel(void)
    {
    struct stat statbuf;		/* buffer for stat on the port */
    int portfd;
    int open_flags;

    DODEBUG(("int_connect_parallel()"));

    /*
    ** Make sure the address we were given is a tty.
    ** If stat() fails, we will ignore the error
    ** for now, we will let open() catch it.
    */
    if(stat(int_cmdline.address, &statbuf) == 0)
    	{
	if( ! (statbuf.st_mode & S_IFCHR) )
	    {
	    alert(int_cmdline.printer, TRUE, _("The file \"%s\" is not a tty."), int_cmdline.address);
	    int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
	    }
	}

    /*
    ** Set the open flags according to whether we are doing
    ** feedback or not.
    */
    open_flags = (int_cmdline.feedback ? O_RDWR : O_WRONLY) | O_NONBLOCK | O_NOCTTY | O_EXCL;

    /*
    ** Open the port.  If the error EBUSY occurs, we will retry up to
    ** 30 times at 2 second intervals.
    */
    {
    int x;
    for(x=0; (portfd = open(int_cmdline.address, open_flags)) < 0 && errno == EBUSY && x < 30; x++)
    	sleep(2);
    }

    if(portfd == -1)	/* If error, */
    	{
	switch(errno)
	    {
	    case EBUSY:
	    	int_exit(EXIT_ENGAGED);
	    case EACCES:
	    	alert(int_cmdline.printer, TRUE, "Access to port \"%s\" is denied.", int_cmdline.address);
		int_exit(EXIT_PRNERR_NORETRY_ACCESS_DENIED);
	    case EIO:
	    	alert(int_cmdline.printer, TRUE, "Hangup or other error while opening \"%s\".", int_cmdline.address);
		int_exit(EXIT_PRNERR);
	    case ENFILE:
	    	alert(int_cmdline.printer, TRUE, "System open file table is full.");
	    	int_exit(EXIT_STARVED);
	    case ENOENT:	/* file not found */
	    case ENOTDIR:	/* path not found */
	    	alert(int_cmdline.printer, TRUE, "The port \"%s\" does not exist.", int_cmdline.address);
	    	int_exit(EXIT_PRNERR_NORETRY_NO_SUCH_ADDRESS);
	    case ENXIO:
	    	alert(int_cmdline.printer, TRUE, "The device file \"%s\" exists, but the device doesn't.", int_cmdline.address);
		int_exit(EXIT_PRNERR_NORETRY_NO_SUCH_ADDRESS);
	    #ifdef ENOSR
	    case ENOSR:
	    	alert(int_cmdline.printer, TRUE, "System is out of STREAMS.");
	    	int_exit(EXIT_STARVED);
	    #endif
	    default:
	    	alert(int_cmdline.printer, TRUE, "Can't open \"%s\", errno=%d (%s).", int_cmdline.address, errno, gu_strerror(errno));
		int_exit(EXIT_PRNERR_NORETRY);
	    }
    	}

    return portfd;
    } /* end of int_connect_parallel() */

/*
** Use the options to set the baud rate and such.
** This is a parser and interpreter.
**
** The options string is in the form:
** speed=9600 parity=none bits=8 xonxoff=yes
*/
static void parse_options(int portfd, struct OPTIONS *options)
    {
    struct OPTIONS_STATE o;
    char name[16];
    char value[16];
    int retval;

    /* Set default values. */
    options->idle_status_interval = 0;
    options->reset_before = TRUE;
    options->reset_on_cancel = FALSE;

    /* If feedback is on and control-d handshaking is on, turn on the ^T stuff. */
    if(int_cmdline.feedback && int_cmdline.jobbreak == JOBBREAK_CONTROL_D)
	options->idle_status_interval = 15;

    options_start(int_cmdline.options, &o);
    while((retval = options_get_one(&o, name, sizeof(name), value, sizeof(value))) > 0)
    	{
	DODEBUG(("name=\"%s\", value=\"%s\"", name, value));

	/* Intepret the keyword. */
	if(strcmp(name, "idle_status_interval") == 0)
	    {
	    if((options->idle_status_interval = atoi(value)) < 0)
	    	{
		o.error = N_("Negative value not allowed");
		retval = -1;
		break;
	    	}
	    }
	else if(strcmp(name, "reset_before") == 0)
	    {
	    int answer;
	    if((answer = gu_torf(value)) == ANSWER_UNKNOWN)
	    	{
		o.error = N_("Value must be boolean");
		retval = -1;
		break;
	    	}
	    options->reset_before = answer ? TRUE : FALSE;
            }
	else if(strcmp(name, "reset_on_cancel") == 0)
	    {
	    int answer;
	    if((answer = gu_torf(value)) == ANSWER_UNKNOWN)
	    	{
		o.error = N_("Value must be boolean");
		retval = -1;
		break;
	    	}
	    options->reset_on_cancel = answer ? TRUE : FALSE;
            }
	else
	    {
	    o.error = N_("unrecognized keyword");
	    o.index = o.index_of_name;
	    retval = -1;
	    break;
	    }
	} /* end of while() */

    /* See if final call to options_get_one() detected an error: */
    if(retval == -1)
    	{
    	alert(int_cmdline.printer, TRUE, "Option parsing error:  %s", gettext(o.error));
    	alert(int_cmdline.printer, FALSE, "%s", o.options);
    	alert(int_cmdline.printer, FALSE, "%*s^ %s", o.index, "", _("right here"));
    	int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
    	}

    /* We can't use control-T status updates if the job isn't PostScript. */
    if(int_cmdline.barbarlang[0])
    	options->idle_status_interval = 0;

    } /* end of parse_options() */

/*
** Explain why reading from or writing to the printer port failed.
*/
static void printer_error(int error_number)
    {
    /* Maybe we tried to read data back from a one-way port. */
    if(error_number == EINVAL && int_cmdline.feedback)
	{
	alert(int_cmdline.printer, TRUE, _("Port \"%s\" does not support 2-way communication."), int_cmdline.address);
	int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
	}

    alert(int_cmdline.printer, TRUE, _("Parallel port communication failed, errno=%d (%s)."), error_number, gu_strerror(error_number));
    int_exit(EXIT_PRNERR);
    }

/*
** This routine prints a LaserWriter-style description of the
** error state returned by parallel_port_status().
**
** We print only one because some of them tend to be contributing
** causes of the others.
*/
static void describe_status(int s)
    {
    if(s & PARALLEL_PORT_OFFLINE)
        fputs("%%[ PrinterError: off line ]%%\n", stdout);
    else if(s & PARALLEL_PORT_PAPEROUT)
        fputs("%%[ PrinterError: out of paper ]%%\n", stdout);
    else if(s & PARALLEL_PORT_FAULT)
        fputs("%%[ PrinterError: miscellaneous error ]%%\n", stdout);
    fflush(stdout);
    }

/*
** Tie it all together.
*/
int main(int argc, char *argv[])
    {
    int portfd;				/* file handle of the printer port */
    struct OPTIONS options;

    /* Initialize international messages library. */
    #ifdef INTERNATIONAL
    setlocale(LC_MESSAGES, "");
    bindtextdomain(PACKAGE_INTERFACES, LOCALEDIR);
    textdomain(PACKAGE_INTERFACES);
    #endif

    /* Copy command line and substitute env variables
       into the structure called "int_cmdline". */
    int_cmdline_set(argc, argv);

    DODEBUG(("============================================================"));
    DODEBUG(("\"%s\", \"%s\", \"%s\", %d, %d, %d",
    	int_cmdline.printer,
    	int_cmdline.address,
    	int_cmdline.options,
    	int_cmdline.jobbreak,
    	int_cmdline.feedback,
    	int_cmdline.codes));

    /* Check for unusable job break methods. */
    if(int_cmdline.jobbreak == JOBBREAK_SIGNAL || int_cmdline.jobbreak == JOBBREAK_SIGNAL_PJL)
    	{
    	alert(int_cmdline.printer, TRUE,
    		_("The jobbreak methods \"signal\" and \"signal/pjl\" are not compatible with\n"
    		"the PPR interface program \"%s\"."), int_cmdline.int_basename);
    	int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
    	}

    /* Check for unusable codes settings. */
    if(int_cmdline.codes == CODES_Binary)
    	{
	alert(int_cmdline.printer, TRUE,
		_("The codes setting \"Binary\" is not compatible with the PPR interface\n"
		"program \"%s\"."), int_cmdline.int_basename);
	int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
    	}

    /* Open the printer port and esablish default settings: */
    portfd = int_connect_parallel();

    /* Parse printer_options and set struct OPTIONS and
       printer port apropriately: */
    parse_options(portfd, &options);

    /* Setup the options using OS-specific code. */
    parallel_port_setup(portfd, &options);

    /* Make sure the printer is ready. */
    {
    int s;
    if((s = parallel_port_status(portfd)) != 0)
	{
	describe_status(s);
    	int_exit(EXIT_ENGAGED);
    	}
    }

    /* Possibly do a reset. */
    if(options.reset_before)
    	parallel_port_reset(portfd);

    /* Read the job data from stdin and send it to portfd. */
    int_copy_job(portfd, options.idle_status_interval, printer_error, NULL, NULL, 0);

    DODEBUG(("closing port"));
    parallel_port_cleanup(portfd);
    close(portfd);

    DODEBUG(("sucessful completion"));
    int_exit(EXIT_PRINTED);
    } /* end of main() */

/* end of file */

