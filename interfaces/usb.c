/*
** mouse:~ppr/src/interfaces/usb.c
** Copyright 1995--2004, Trinity College Computing Center.
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
** Last modified 29 May 2004.
*/

/*
** PPR interface program to drive a USB printer.
**
** Though this program should compile and run on any Unix,
** it currently contains explicit support only for Linux.
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
** the file "/var/spool/ppr/logs/interface_usb".
*/
#if 0
#define DEBUG 1
#define DODEBUG(a) int_debug a
#else
#undef DEBUG
#define DODEBUG(a)
#endif

static const char *port_patterns[] =
	{
	"/dev/usb/lp%d",			/* Linux 2.4.x and 2.6.x with devfs */
	"/dev/usb/usblp%d",
	"/dev/usblp%d",
	NULL
	};

/* The name=value style options from the command line are stored here. */
struct OPTIONS {
	int idle_status_interval;
	int status_interval;
	const char *init;
	} ;

/*
** Open the printer port.
*/
static int connect_usb(const char port[])
	{
	struct stat statbuf;		/* buffer for stat() on the port */
	int portfd;
	int open_flags;

	DODEBUG(("connect_usb(port=\"%s\")", port));

	/*
	** Make sure the address we were given is a tty.
	** If stat() fails, we will ignore the error
	** for now, we will let open() catch it.
	*/
	if(stat(port, &statbuf) == 0)
		{
		if( ! (statbuf.st_mode & S_IFCHR) )
			{
			alert(int_cmdline.printer, TRUE, _("The file \"%s\" is not a tty."), port);
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
	for(x=0; (portfd = open(port, open_flags)) < 0 && errno == EBUSY && x < 30; x++)
		sleep(2);
	}

	if(portfd == -1)	/* If error, */
		{
		switch(errno)
			{
			case EBUSY:
				exit(EXIT_ENGAGED);
			case EACCES:
				alert(int_cmdline.printer, TRUE, _("Access to port \"%s\" is denied."), port);
				exit(EXIT_PRNERR_NORETRY_ACCESS_DENIED);
			case ENOENT:		/* file not found */
			case ENOTDIR:		/* path not found */
				alert(int_cmdline.printer, TRUE, _("The port \"%s\" does not exist."), port);
				exit(EXIT_PRNERR_NOT_RESPONDING);
			case ENXIO:
				alert(int_cmdline.printer, TRUE, _("The device file \"%s\" exists, but the device doesn't."), port);
				exit(EXIT_PRNERR_NORETRY_NO_SUCH_ADDRESS);
			case ENFILE:
				alert(int_cmdline.printer, TRUE, _("System open file table is full."));
				exit(EXIT_STARVED);
			default:
				alert(int_cmdline.printer, TRUE, _("Can't open \"%s\", errno=%d (%s)."), port, errno, gu_strerror(errno));
				exit(EXIT_PRNERR_NORETRY);
			}
		}

	return portfd;
	} /* end of connect_usb() */

/*
** Explain why reading from or writing to the printer port failed.
*/
static void printer_error(int error_number)
	{
	switch(error_number)
		{
		case ENODEV:
			alert(int_cmdline.printer, TRUE,
				_("Printer turned off or disconnected while printing."));
			exit(EXIT_PRNERR);

		default:	 /* If all else fails, we end up here. */
			alert(int_cmdline.printer, TRUE,
				_("USB port communication failed, errno=%d (%s)."),
			   	error_number,
			   	gu_strerror(error_number)
				);
			exit(EXIT_PRNERR);
		}
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
	options->init = NULL;

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
				o.error = N_("value must be a positive integer or zero");
				retval = -1;
				break;
				}
			}
		else if(strcmp(name, "init") == 0)
			{
			options->init = "\35\000\000\000\033\001@EJL 1284.4\n@EJL     \n\033@";
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
		alert(int_cmdline.printer, TRUE, _("Option parsing error: %s"), gettext(o.error));
		alert(int_cmdline.printer, FALSE, "%s", o.options);
		alert(int_cmdline.printer, FALSE, "%*s^ %s", o.index, "", _("right here"));
		exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
		}

	/* We can't use control-T status updates if the job isn't PostScript. */
	if(int_cmdline.barbarlang[0])
		options->idle_status_interval = 0;

	} /* end of parse_options() */

/*
 * Hunt up a printer matching the description specified in the address.
 */
static const char *find_usb_printer(const char address[])
	{
	const char *port_pattern;
	char port_temp[64];
	unsigned char device_id[1024];
	int i;
	char *temp, *p, *item, *name, *value;
	char *search_mfg, *search_mdl, *search_sern;
	char *mfg, *mdl, *sern;
	char *ret = NULL;

	for(i=0; (port_pattern = port_patterns[i]); i++)
		{
		gu_snprintf(port_temp, sizeof(port_temp), port_pattern, 0);
		if(access(port_temp, F_OK) == 0)
			break;
		}
	
	if(!port_pattern)	/* If no USB printer ports, */
		return NULL;

	search_mfg = search_mdl = search_sern = NULL;
	for(p = temp = gu_strdup(address); (item = gu_strsep(&p, ";")); )
		{
		/*printf("item=%s\n", item);*/
		if((name = gu_strsep(&item, ":")) && (value = gu_strsep(&item, "")))
			{
			/*printf("name=%s value=%s\n", name, value);*/
			if(strcmp(name, "MFG") == 0 || strcmp(name, "MANUFACTURER") == 0)
				{
				search_mfg = value;
				continue;
				}
			if(strcmp(name, "MDL") == 0 || strcmp(name, "MODEL") == 0)
				{
				search_mdl = value;
				continue;
				}
			if(strcmp(name, "SERN") == 0)
				{
				search_sern = value;
				continue;
				}
			}
		alert(int_cmdline.printer, TRUE, _("Unrecognized search key in address: %s"), name ? name : item);
		exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
		}

	for(i=0; TRUE; i++)
		{
		gu_snprintf(port_temp, sizeof(port_temp), port_pattern, i);
		if(access(port_temp, F_OK) != 0)
			break;

		if(get_device_id(port_temp, device_id, sizeof(device_id)) == -1)
			{
			printf("; Can't get device ID for port %s, errno=%d (%s)\n", port_temp, errno, gu_strerror(errno));
			continue;
			}

		mfg = mdl = sern = NULL;
		for(p = device_id; (item = gu_strsep(&p, ";")); )
			{
			if((name = gu_strsep(&item, ":")) && (value = gu_strsep(&item, "")))
				{
				/*printf("name=%s value=%s\n", name, value);*/
				if(strcmp(name, "MFG") == 0 || strcmp(name, "MANUFACTURER") == 0)
					mfg = value;
				else if(strcmp(name, "MDL") == 0 || strcmp(name, "MODEL") == 0)
					mdl = value;
				else if(strcmp(name, "SERN") == 0)
					sern = value;
				}
			}

		if((!search_mfg || strcmp(mfg, search_mfg) == 0)
			&& (!search_mdl || strcmp(mdl, search_mdl) == 0)
			&& (!search_sern || strcmp(sern, search_sern) == 0)
			)
			{
			ret = port_temp;
			break;
			}
		}

	gu_free(temp);

	if(ret)
		return gu_strdup(ret);
	else
		return NULL;
	}

/*
** Implementation of the --probe option.
*/
static int usb_port_probe(const char address[])
	{
	#ifdef PPR_LINUX
	unsigned char device_id[1024];		/* IEEE 1284 DeviceID string */
	char *p, *item, *name, *value;

	if(get_device_id(address, device_id, sizeof(device_id)) == -1)
		{
		if(errno == EACCES)
			return EXIT_PRNERR_NORETRY_ACCESS_DENIED;
		if(errno == EEXIST)
			return EXIT_PRNERR_NO_SUCH_ADDRESS;
		fprintf(stderr, "device_id() failed, errno=%d (%s)\n", errno, gu_strerror(errno));
		return EXIT_PRNERR;
		}

	for(p = device_id; (item = gu_strsep(&p, ";")); )
		{
		if((name = gu_strsep(&item, ":")) && (value = gu_strsep(&item, "")))
			{
			printf("PROBE: 1284DeviceID %s=%s\n", name, value);
			}
		}

	return EXIT_PRINTED;
	#else
	return EXIT_PRNERR_NORETRY;
	#endif
	}

/*
** Tie it all together.
*/
int int_main(int argc, char *argv[])
	{
	const char *port;
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
		int retval = usb_port_probe(int_cmdline.address);
		switch(retval)
			{
			case EXIT_PRINTED:
				break;
			case EXIT_PRNERR_NORETRY:
				fprintf(stderr, _("USB port probing not implemented on this OS.\n"));
				break;
			case EXIT_PRNERR_NORETRY_NO_SUCH_ADDRESS:
				fprintf(stderr, _("Probing not implemented for port \"%s\".\n"), int_cmdline.address);
				break;
			case EXIT_PRNERR_NO_SUCH_ADDRESS:
				fprintf(stderr, _("Port \"%s\" not found.\n"), int_cmdline.address);
				break;
			case EXIT_PRNERR_NORETRY_ACCESS_DENIED:
				fprintf(stderr, _("Access to port \"%s\" denied.\n"), int_cmdline.address);
				break;
			case EXIT_PRNERR:
			default:
				fprintf(stderr, _("Probe failed.\n"));
				break;
			}
		return retval;
		}

	/* Check for unsuitable job break methods. */
	if(int_cmdline.jobbreak == JOBBREAK_SIGNAL || int_cmdline.jobbreak == JOBBREAK_SIGNAL_PJL)
		{
		alert(int_cmdline.printer, TRUE,
				_("The jobbreak methods \"signal\" and \"signal/pjl\" are not compatible with\n"
				"the PPR interface program \"%s\"."), int_cmdline.int_basename);
		return EXIT_PRNERR_NORETRY_BAD_SETTINGS;
		}

	/*
	 * Check for unusable codes settings.
	 * !!! Is this true for USB ??? 
	 */
	if(int_cmdline.codes == CODES_Binary)
		{
		alert(int_cmdline.printer, TRUE,
				_("The codes setting \"Binary\" is not compatible with the PPR interface\n"
				"program \"%s\"."), int_cmdline.int_basename);
		exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
		}

	if(int_cmdline.address[0] == '/')
		port = int_cmdline.address;
	else if(!(port = find_usb_printer(int_cmdline.address)))
		{
		alert(int_cmdline.printer, TRUE,
			_("No printer matching address \"%s\" is presently connected."), int_cmdline.address);
		exit(EXIT_PRNERR);
		}
	
	gu_write_string(1, "%%[ PPR connecting ]%%\n");

	/* Open the printer port and esablish default settings: */
	portfd = connect_usb(port);

	/* Parse printer_options and set struct OPTIONS and
	   printer port apropriately: */
	parse_options(portfd, &options);

	gu_write_string(1, "%%[ PPR connected ]%%\n");

	/* Read the job data from stdin and send it to portfd. */
	/*kill(getpid(), SIGSTOP);*/
	int_copy_job(portfd,
		options.idle_status_interval,
		printer_error,
		NULL,
		status_function,
		(void*)&portfd,
		options.status_interval,
		options.init);

	close(portfd);

	DODEBUG(("sucessful completion"));
	exit(EXIT_PRINTED);
	} /* end of main() */

/* end of file */

