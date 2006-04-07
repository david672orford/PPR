/*
** mouse:~ppr/src/interfaces/parallel.c
** Copyright 1995--2006, Trinity College Computing Center.
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
** Last modified 7 April 2006.
*/

/*
** PPR interface program to drive a parallel printer.
*/

#include "config.h"
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
static int open_parallel(void)
	{
	struct stat statbuf;				/* buffer for stat on the port */
	int portfd;
	int open_flags;

	DODEBUG(("open_parallel()"));

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
			exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
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
				exit(EXIT_ENGAGED);
			case EACCES:
				alert(int_cmdline.printer, TRUE, _("Access to port \"%s\" is denied."), int_cmdline.address);
				exit(EXIT_PRNERR_NORETRY_ACCESS_DENIED);
			case ENOENT:		/* file not found */
			case ENOTDIR:		/* path not found */
				alert(int_cmdline.printer, TRUE, _("The port \"%s\" does not exist."), int_cmdline.address);
				exit(EXIT_PRNERR_NORETRY_NO_SUCH_ADDRESS);
			case ENXIO:
				alert(int_cmdline.printer, TRUE, _("The device file \"%s\" exists, but the device doesn't."), int_cmdline.address);
				exit(EXIT_PRNERR_NORETRY_NO_SUCH_ADDRESS);
			case ENFILE:
				alert(int_cmdline.printer, TRUE, _("System open file table is full."));
				exit(EXIT_STARVED);
			#ifdef ENOSR
			case ENOSR:
				alert(int_cmdline.printer, TRUE, _("System is out of STREAMS."));
				exit(EXIT_STARVED);
			#endif
			default:
				alert(int_cmdline.printer, TRUE, _("Can't open \"%s\", errno=%d (%s)."), int_cmdline.address, errno, gu_strerror(errno));
				exit(EXIT_PRNERR_NORETRY);
			}
		}

	return portfd;
	} /* end of open_parallel() */

/*
** This routine prints (on the pipe to pprdrv) a series of LaserWriter-style 
** descriptions of any Centronics error states indicated by the value 
** returned by	parallel_port_status().
*/
#define PARALLEL_DISABLED (PARALLEL_PORT_OFFLINE | PARALLEL_PORT_PAPEROUT | PARALLEL_PORT_FAULT)
static int describe_status(int s)
	{
	/*
	** If no fault bits set, the printer is now busy printing our job.  We 
	** should tell pprdrv about this because otherwise it will think
	** that any error conditions which we described earlier are still
	** present.
	*/
	if((s & PARALLEL_DISABLED) == 0)
		{
		gu_write_string(1, "%%[ status: busy ]%%\n");
		}
	/*
	** If one of the disabling conditions is indicated but the busy line is
	** active, it probably indicates that we are reading the status from
	** a turned-off printer or a dangling parallel cable.
	**
	** Note that this code may fail an absent printer since the parallel 
	** port line states depend on hard-to-predict electrical factors.
	*/
	else if(s & PARALLEL_PORT_BUSY && s & PARALLEL_DISABLED)
		{
		gu_write_string(1, "%%[ PrinterError: printer disconnected or powered down ]%%\n");
		}
	/*
	** If we get this far, print a Laserwriter-style message for each fault
	** that is present.  In the past we would have picked only the most
	** 'important' one, since later ones would replace earlier ones in 
	** the "ppop status" output, but pprdrv can deal with such things now.
	** it will integrate them all into an SNMP-printer-MIB-style status.
	*/
	else
		{
		if(s & PARALLEL_PORT_OFFLINE)
			gu_write_string(1, "%%[ PrinterError: off line ]%%\n");
		if(s & PARALLEL_PORT_PAPEROUT)
			gu_write_string(1, "%%[ PrinterError: out of paper ]%%\n");
		if(s & PARALLEL_PORT_FAULT)
			gu_write_string(1, "%%[ PrinterError: miscellaneous error ]%%\n");
		}

	/* Return the status with extranious bits masked out. */
	return s & PARALLEL_DISABLED;
	}

/*
** If status_interval= is greater than zero, then this will be called
** every status_interval seconds.
*/
static void status_function(void *p)
	{
	describe_status(parallel_port_status(*(int*)p));
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
	options->status_interval = 15;
	options->reset_before = TRUE;
	options->reset_on_cancel = FALSE;

	/* If feedback is on and control-d handshaking is on, turn on the ^T stuff. */
	if(int_cmdline.feedback 
			&& (int_cmdline.jobbreak == JOBBREAK_CONTROL_D 
				|| int_cmdline.jobbreak == JOBBREAK_PJL)
		)
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
				o.error = N_("value must a positive integer or zero");
				retval = -1;
				break;
				}
			}
		else if(strcmp(name, "reset_before") == 0)
			{
			if(gu_torf_setBOOL(&options->reset_before,value) == -1)
				{
				o.error = N_("Value must be boolean");
				retval = -1;
				break;
				}
			}
		else if(strcmp(name, "reset_on_cancel") == 0)
			{
			if(gu_torf_setBOOL(&(options->reset_on_cancel),value) == -1)
				{
				o.error = N_("Value must be boolean");
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
		alert(int_cmdline.printer, TRUE, "Option parsing error:	 %s", gettext(o.error));
		alert(int_cmdline.printer, FALSE, "%s", o.options);
		alert(int_cmdline.printer, FALSE, "%*s^ %s", o.index, "", _("right here"));
		exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
		}

	/* We can't use control-T status updates if the job isn't PostScript. */
	if(strcmp(int_cmdline.PDL, "postscript") != 0)
		options->idle_status_interval = 0;

	} /* end of parse_options() */

/*
** Tie it all together.
*/
int int_main(int argc, char *argv[])
	{
	int portfd;							/* file handle of the printer port */
	struct OPTIONS options;

	/* Initialize international messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
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

	/* If the --probe option was used, */
	if(int_cmdline.probe)
		{
		int retval = parallel_port_probe(int_cmdline.address);
		switch(retval)
			{
			case EXIT_PRINTED:
				break;
			case EXIT_PRNERR_NORETRY:
				fprintf(stderr, _("Parallel port probing not implemented on this OS.\n"));
				break;
			case EXIT_PRNERR_NORETRY_NO_SUCH_ADDRESS:
				fprintf(stderr, _("Probing not implemented for port \"%s\".\n"), int_cmdline.address);
				break;
			case EXIT_PRNERR_NO_SUCH_ADDRESS:
				fprintf(stderr, _("Port \"%s\" not found.\n"), int_cmdline.address);
				break;
			case EXIT_PRNERR:
			default:
				fprintf(stderr, _("Probe failed.\n"));
				break;
			}
		return retval;
		}

	/* Check for unusable job break methods. */
	if(int_cmdline.jobbreak == JOBBREAK_SIGNAL || int_cmdline.jobbreak == JOBBREAK_SIGNAL_PJL)
		{
		alert(int_cmdline.printer, TRUE,
				_("The jobbreak methods \"signal\" and \"signal/pjl\" are not compatible with\n"
				"the PPR interface program \"%s\"."), int_cmdline.int_basename);
		exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
		}

	/* Check for unusable codes settings. */
	if(int_cmdline.codes == CODES_Binary)
		{
		alert(int_cmdline.printer, TRUE,
				_("The codes setting \"Binary\" is not compatible with the PPR interface\n"
				"program \"%s\"."), int_cmdline.int_basename);
		exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
		}

	gu_write_string(1, "%%[ PPR connecting ]%%\n");

	/* Open the printer port and esablish default settings: */
	portfd = open_parallel();

	/* Parse printer_options and set struct OPTIONS and
	   printer port apropriately: */
	parse_options(portfd, &options);

	/* Setup the options using OS-specific code. */
	parallel_port_setup(portfd, &options);

	/* Make sure the printer is ready. */
	if(describe_status(parallel_port_status(portfd)))
		exit(EXIT_ENGAGED);

	/* Possibly do a reset. */
	if(options.reset_before)
		parallel_port_reset(portfd);

	gu_write_string(1, "%%[ PPR connected ]%%\n");

	/* Read the job data from stdin and send it to portfd. */
	/*kill(getpid(), SIGSTOP);*/
	int_copy_job(portfd,
		options.idle_status_interval,
		parallel_port_error,
		NULL,
		status_function,
		(void*)&portfd,
		options.status_interval,
		NULL
		);

	DODEBUG(("closing port"));
	parallel_port_cleanup(portfd);
	close(portfd);

	DODEBUG(("sucessful completion"));
	exit(EXIT_PRINTED);
	} /* end of main() */

/* end of file */

