/*
** mouse:~ppr/src/interfaces/usblp.c
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
** Last modified 10 January 2003.
*/

/*
** PPR interface to drive a USB printer.
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
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "interface.h"
#include "libppr_int.h"

/*
** Here we possible enable debugging.  The debuging output goes to
** the file "/var/spool/ppr/logs/interface_usblp".
*/
#if 0
#define DEBUG 1
#define DODEBUG(a) int_debug a
#else
#undef DEBUG
#define DODEBUG(a)
#endif

struct OPTIONS {
	int idle_status_interval;
	int status_interval;
	} ;

/*
** Open the printer port.
*/
static int connect_usblp(void)
    {
    struct stat statbuf;		/* buffer for stat on the port */
    int portfd;
    int open_flags;

    DODEBUG(("connect_parallel()"));

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
	    	alert(int_cmdline.printer, TRUE, _("Access to port \"%s\" is denied."), int_cmdline.address);
		int_exit(EXIT_PRNERR_NORETRY_ACCESS_DENIED);
	    case ENOENT:	/* file not found */
	    case ENOTDIR:	/* path not found */
	    	alert(int_cmdline.printer, TRUE, _("The port \"%s\" does not exist."), int_cmdline.address);
	    	int_exit(EXIT_PRNERR_NORETRY_NO_SUCH_ADDRESS);
	    case ENXIO:
	    	alert(int_cmdline.printer, TRUE, _("The device file \"%s\" exists, but the device doesn't."), int_cmdline.address);
		int_exit(EXIT_PRNERR_NORETRY_NO_SUCH_ADDRESS);
	    case ENFILE:
	    	alert(int_cmdline.printer, TRUE, _("System open file table is full."));
	    	int_exit(EXIT_STARVED);
	    default:
	    	alert(int_cmdline.printer, TRUE, _("Can't open \"%s\", errno=%d (%s)."), int_cmdline.address, errno, gu_strerror(errno));
		int_exit(EXIT_PRNERR_NORETRY);
	    }
    	}

    return portfd;
    } /* end of connect_parallel() */

/*
** Explain why reading from or writing to the printer port failed.
*/
static void printer_error(int error_number)
    {
    alert(int_cmdline.printer, TRUE, _("Parallel port communication failed, errno=%d (%s)."), error_number, gu_strerror(error_number));
    int_exit(EXIT_PRNERR);
    }

/*
** If status_interval= is greater than zero, then this will be called
** every status_interval seconds.
*/
static void status_function(void *p)
    {
    }

/*
** Parse the name=value pairs in the options parameter.
*/
static void parse_options(int portfd, struct OPTIONS *options)
    {
    struct OPTIONS_STATE o;
    char name[16];
    char value[16];
    int retval;

    /* Set default values. */
    options->idle_status_interval = 0;
    options->status_interval = 0;

    /* If feedback is on and control-d handshaking is on, turn on the ^T stuff. */
    if(int_cmdline.feedback && int_cmdline.jobbreak == JOBBREAK_CONTROL_D)
	options->idle_status_interval = 15;

    options_start(int_cmdline.options, &o);
    while((retval = options_get_one(&o, name, sizeof(name), value, sizeof(value))) > 0)
    	{
	DODEBUG(("name=\"%s\", value=\"%s\"", name, value));

	/* Interpret the keyword. */
	if(strcmp(name, "idle_status_interval") == 0)
	    {
	    if((options->idle_status_interval = atoi(value)) < 0)
	    	{
		o.error = N_("Negative value not allowed");
		retval = -1;
		break;
	    	}
	    }
	else if(strcmp(name, "status_interval") == 0)
	    {
	    if((options->status_interval = atoi(value)) < 0)
	    	{
		o.error = N_("value must be 0 or a positive integer");
		retval = -1;
		break;
	    	}
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
    /* !!! Is the true for USB ??? */
    if(int_cmdline.codes == CODES_Binary)
    	{
	alert(int_cmdline.printer, TRUE,
		_("The codes setting \"Binary\" is not compatible with the PPR interface\n"
		"program \"%s\"."), int_cmdline.int_basename);
	int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
    	}

    /* Open the printer port and esablish default settings: */
    portfd = connect_usblp();

    /* Parse printer_options and set struct OPTIONS and
       printer port apropriately: */
    parse_options(portfd, &options);

    /* Read the job data from stdin and send it to portfd. */
    int_copy_job(portfd,
	options.idle_status_interval,
	printer_error,
	NULL,
	status_function,
	(void*)&portfd,
	options.status_interval);

    close(portfd);

    DODEBUG(("sucessful completion"));
    int_exit(EXIT_PRINTED);
    } /* end of main() */

/* end of file */
