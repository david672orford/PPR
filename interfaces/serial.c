/*
** mouse:~ppr/src/interfaces/serial.c
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
** PPR interface to drive a serial printer.
*/

#include "before_system.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>		/* for time() */
#include <sys/time.h>		/* for select() */
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <ctype.h>
#include <termios.h>
#include <string.h>
#include <sys/ioctl.h>		/* for Linux */
#ifdef HAVE_SYS_MODEM_H
#include <sys/modem.h>		/* for HP-UX */
#endif
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "interface.h"
#include "libppr_int.h"

#if 0
#define DEBUG 1
#define DODEBUG(a) int_debug a
#else
#undef DEBUG
#define DODEBUG(a)
#endif

/* Those interface options which are not serial line settings: */
struct OPTIONS {
	#ifdef TIOCMGET
	int online;
	#endif
	gu_boolean detect_hangups;
	int idle_status_interval;
	} ;

/*
** POSIX speeds we support.
** Rare and unlikely speeds have been ommited.
** If the system defines them, we also include
** some speeds not defined in the POSIX standard.
*/
struct SPEED
    {
    int bps;
    speed_t setting;
    } ;

struct SPEED posix_speeds[] =
    {
    {300, B300},
    {600, B600},
    {1200, B1200},
    {1800, B1800},
    {2400, B2400},
    {4800, B4800},
    {9600, B9600},
    {19200, B19200},
    {38400, B38400}
    #ifdef B57600
    , {57600, B57600}
    #endif
    #ifdef B76800
    , {76800, B76800}
    #endif
    #ifdef B115200
    , {115200, B115200}
    #endif
    #ifdef B153600
    , {153600, B153600}
    #endif
    #ifdef B230400
    , {230400, B230400}
    #endif
    #ifdef B307200
    , {307200, B307200}
    #endif
    #ifdef B460800
    , {460800, B460800}
    #endif
    } ;

/*
** Open the printer port.
*/
static int open_port(const char *printer_name, const char *printer_address, struct termios *settings)
    {
    struct stat statbuf;		/* buffer for stat on the port */
    int portfd;

    DODEBUG(("open_port(printer_name=\"%s\", printer_address=\"%s\", settings=%p", printer_name, printer_address, settings));

    /*
    ** Make sure the address we were given is a tty.
    ** If stat() fails, we will ignore the error
    ** for now, we will let open() catch it.
    */
    if(stat(printer_address, &statbuf) == 0)
    	{
	if( ! (statbuf.st_mode & S_IFCHR) )
	    {
	    alert(int_cmdline.printer, TRUE, "The file \"%s\" is not a tty.");
	    int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
	    }
	}

    /*
    ** Open the port.  We use alarm() to give this operation a
    ** timeout since printers which are off line have been known
    ** to cause open() to block indefinitely.
    */
    portfd = open(printer_address, O_RDWR | O_NONBLOCK | O_NOCTTY | O_EXCL);

    if(portfd == -1)	/* If error, */
    	{
	switch(errno)
	    {
	    case EACCES:
	    	alert(int_cmdline.printer, TRUE, "Access to port \"%s\" is denied.", printer_address);
		int_exit(EXIT_PRNERR_NORETRY_ACCESS_DENIED);
	    case EIO:
	    	alert(int_cmdline.printer, TRUE, "Hangup or other error while opening \"%s\".", printer_address);
		int_exit(EXIT_PRNERR);
	    case ENFILE:
	    	alert(int_cmdline.printer, TRUE, "System open file table is full.");
	    	int_exit(EXIT_STARVED);
	    case ENOENT:	/* file not found */
	    case ENOTDIR:	/* path not found */
	    	alert(int_cmdline.printer, TRUE, "The port \"%s\" does not exist.", printer_address);
	    	int_exit(EXIT_PRNERR_NORETRY_NO_SUCH_ADDRESS);
	    case ENXIO:
	    	alert(int_cmdline.printer, TRUE, "The device file \"%s\" exists, but the device doesn't.", printer_address);
		int_exit(EXIT_PRNERR_NORETRY_NO_SUCH_ADDRESS);
	    #ifdef ENOSR
	    case ENOSR:
	    	alert(int_cmdline.printer, TRUE, "System is out of STREAMS.");
	    	int_exit(EXIT_STARVED);
	    #endif
	    default:
	    	alert(int_cmdline.printer, TRUE, "Can't open \"%s\", %s.", printer_address, gu_strerror(errno));
		int_exit(EXIT_PRNERR_NORETRY);
	    }
    	}

    /*
    ** Wait for output to drain and then get the current port settings.
    ** Getting the current settings is said to be important because the
    ** termios structure might have some implementations specific bits
    ** that we should not mess with.
    **
    ** (See POSIX Programmers Guide, Donald Lewine, page 161.)
    */
    tcdrain(portfd);
    tcgetattr(portfd, settings);

    /*
    ** Establish some default port settings.
    ** I wonder if we are messing with some of those
    ** implementation specific bits.
    */
    settings->c_iflag = IXON | IXOFF;
    settings->c_cflag = CS8 | CREAD | HUPCL | CLOCAL;
    settings->c_oflag = 0;
    settings->c_lflag = 0;
    settings->c_cc[VMIN] = 1;	/* Don't return with zero characters */
    settings->c_cc[VTIME] = 2;	/* Wait 0.2 second for next character */
    cfsetispeed(settings, B9600);
    cfsetospeed(settings, B9600);

    return portfd;
    } /* end of open_port() */

/*
** This converts a numberic serial port speed number
** in bits per second to the coresponding POSIX constant.
**
** If there is an error, it returns the constant B0.
**
** This function uses an overkill binary search.
*/
static speed_t speed_convert(const char *speed_str)
    {
    int target;
    int lower = 0;
    int upper = sizeof(posix_speeds) / sizeof(struct SPEED);
    int hunt, x;

    if((target = atoi(speed_str)) <= 0)
    	return B0;

    while(TRUE)
    	{
	hunt = (upper + lower) / 2;

	if((x = posix_speeds[hunt].bps) == target)
	    return posix_speeds[hunt].setting;

	if(upper == lower)
	    return B0;

	if(x > target)
	    upper = hunt - 1;
	else
	    lower = hunt + 1;
	}
    } /* end of speed_convert() */

/*
** Use the options to set the baud rate and such.
** This is a parser and interpreter.
**
** The options string is in the form:
** speed=9600 parity=none bits=8 xonxoff=yes
*/
static void set_options(const char *printer_name, const char *printer_options, int portfd, struct termios *settings, struct OPTIONS *options)
    {
    struct OPTIONS_STATE o;
    char name[16];
    char value[16];
    int retval;
    struct termios readback;

    /* Set defaults.  If changeable, default for online is DSR and CTS
       must be asserted.  We don't normaly opt to detect hangups (which
       I believe means loss of CD).  We don't normally send ^T's. */
    #ifdef TIOCMGET
    options->online = TIOCM_DSR | TIOCM_CTS;
    #endif
    options->detect_hangups = FALSE;
    options->idle_status_interval = 0;

    /* If feedback is on and control-d handshaking is on, turn on the ^T stuff. */
    if(int_cmdline.feedback && int_cmdline.jobbreak == JOBBREAK_CONTROL_D)
	options->idle_status_interval = 15;

    /* Parse the interface options. */
    options_start(printer_options, &o);
    while((retval = options_get_one(&o, name, sizeof(name), value, sizeof(value))) > 0)
    	{
	/* Intepret the keyword. */
	if(strcmp(name, "speed") == 0)
	    {
	    speed_t speed;
	    if((speed = speed_convert(value)) == B0
	    		|| cfsetispeed(settings, speed) == -1
	    		|| cfsetospeed(settings, speed) == -1)
		{
		o.error = N_("Illegal \"speed=\" value");
		retval = -1;
		break;
	    	}
	    }
	else if(strcmp(name, "xonxoff") == 0)
	    {
	    int answer;
	    if((answer = gu_torf(value)) == ANSWER_UNKNOWN)
	    	{
		o.error = N_("Invalid boolean value");
		retval = -1;
		break;
		}
	    if(answer)		/* on */
	    	settings->c_iflag |= IXON | IXOFF;
	    else			/* off */
	    	settings->c_iflag &= ~(IXON | IXOFF);
	    }
	else if(strcmp(name, "rtscts") == 0)
	    {
	    int answer;
	    if((answer = gu_torf(value)) == ANSWER_UNKNOWN)
	    	{
		o.error = N_("Invalid boolean value");
		retval = -1;
		break;
		}

	    #ifdef CRTSCTS

	    #ifdef CRTSXOFF	/* SunOS 5.x */
	    if(answer)
	    	settings->c_cflag |= (CRTSCTS | CRTSXOFF);
	    else
	    	settings->c_cflag &= ~(CRTSCTS | CRTSXOFF);

	    #else		/* Linux */
	    if(answer)
	    	settings->c_cflag |= CRTSCTS;
	    else
	    	settings->c_cflag &= ~CRTSCTS;
	    #endif

	    #else
	    o.error = N_("No OS support for RTS/CTS handshaking");
	    retval = -1;
	    break;
	    #endif
	    }
	else if(strcmp(name, "parity") == 0)
	    {
	    if(strcmp(value, "none") == 0)
	    	{
	    	settings->c_cflag &= ~PARENB;		/* clear parity enable */
	    	}
	    else if(strcmp(value, "even") == 0)
	    	{
		settings->c_cflag &= ~PARODD;		/* clear odd parity */
	    	settings->c_cflag |= PARENB;		/* enable parity */
	    	}
	    else if(strcmp(value, "odd") == 0)
	    	{
		settings->c_cflag |= PARODD;		/* set odd parity */
	    	settings->c_cflag |= PARENB;		/* enable parity */
	    	}
	    else
	    	{
		o.error = N_("Only valid values are \"odd\", \"even\", and \"none\"");
		retval = -1;
		break;
	    	}
	    }
	else if(strcmp(name, "bits") == 0)
	    {
	    int bits;
	    if((bits = atoi(value)) != 7 && bits != 8)
	    	{
		o.error = N_("Only valid values are 7 and 8");
		retval = -1;
		break;
	    	}
	    settings->c_cflag &= ~CSIZE;	/* clear old setting */
	    if(bits == 7)
		settings->c_cflag |= CS7;
	    else
		settings->c_cflag |= CS8;
	    }
	else if(strcmp(name, "online") == 0)
	    {
	    #ifdef TIOCMGET
	    if(gu_strcasecmp(value, "dsr/cts") == 0)
		options->online = TIOCM_DSR | TIOCM_CTS;
	    else if(gu_strcasecmp(value, "dsr") == 0)
		options->online = TIOCM_DSR;
	    else if(gu_strcasecmp(value, "cts") == 0)
		options->online = TIOCM_CTS;
	    else if(gu_strcasecmp(value, "none") == 0)
	    	options->online = 0;
	    else
		{
		o.error = N_("Only valid values are \"DSR/CTS\", \"DSR\", \"CTS\", and \"none\"");
		retval = -1;
		break;
		}
	    #else
	    o.error = N_("No OS support for DSR or CTS state reporting");
	    retval = -1;
	    break;
	    #endif
	    }
	else if(strcmp(name, "detect_hangups") == 0)
	    {
	    int answer;
	    if((answer = gu_torf(value)) == ANSWER_UNKNOWN)
	    	{
		o.error = N_("Invalid boolean value");
		retval = -1;
		break;
		}
	    options->detect_hangups = answer ? TRUE : FALSE;
	    }
	else if(strcmp(name, "hangup_on_close") == 0)
	    {
	    int answer;
	    if((answer = gu_torf(value)) == ANSWER_UNKNOWN)
	    	{
		o.error = N_("Invalid boolean value");
		retval = -1;
		break;
		}
	    if(answer)
	    	settings->c_cflag |= HUPCL;
	    else
	    	settings->c_cflag &= ~HUPCL;
	    }
	else if(strcmp(name, "idle_status_interval") == 0)
	    {
	    if((options->idle_status_interval = atoi(value)) < 0)
	    	{
		o.error = N_("Negative value not allowed");
		retval = -1;
		break;
	    	}
	    }
	else
	    {
	    o.error = N_("unrecognized keyword");
	    retval = -1;
	    break;
	    }
	} /* end of while() */

    /* See if final call to options_get_one() detected an error: */
    if(retval == -1)
    	{
    	alert(int_cmdline.printer, TRUE, _("Option parsing error:  %s"), gettext(o.error));
    	alert(int_cmdline.printer, FALSE, "%s", o.options);
    	alert(int_cmdline.printer, FALSE, "%*s^ %s", o.index, "", _("right here"));
    	int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
    	}

    /* We can't use control-T status updates if the job
       isn't PostScript, so override it in that case. */
    if(int_cmdline.barbarlang[0])
    	options->idle_status_interval = 0;

    /* Make sure the codes setting is ok. */
    if((settings->c_cflag & CSIZE) == CS7
    		&& int_cmdline.codes != CODES_Clean7Bit
    		&& int_cmdline.codes != CODES_UNKNOWN)
    	{
    	alert(int_cmdline.printer, TRUE, _("%s interface: \"codes\" setting must be \"Clean7Bit\" if the option \"bits=7\" is set."), int_cmdline.int_basename);
    	int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
    	}

    /* If detect_hangups was set to true, clear CLOCAL: */
    if(options->detect_hangups)
    	settings->c_cflag &= ~CLOCAL;

    /* Write the new port settings. */
    if(tcsetattr(portfd, TCSANOW, settings) == -1)
    	{
    	alert(int_cmdline.printer, TRUE, "%s interface: tcsetattr() failed, errno=%d (%s)", int_cmdline.int_basename, errno, gu_strerror(errno));
    	int_exit(EXIT_PRNERR);
    	}

    /* Make sure they were written correctly: */
    if(tcgetattr(portfd, &readback) == -1)
    	{
    	alert(int_cmdline.printer, TRUE, "%s interface: tcgetattr() failed, errno=%d (%s)", int_cmdline.int_basename, errno, gu_strerror(errno));
    	int_exit(EXIT_PRNERR);
    	}

    /* This code doesn't work. */
#if 0
    if(memcmp(settings, &readback, sizeof(struct termios)))
    	{
	alert(int_cmdline.printer, TRUE, _("%s interface: serial port driver does not support selected options"), int_cmdline.int_basename);
	int_exit(EXIT_PRNERR_NORETRY);
    	}
#endif
    } /* end of set_options() */

/*
** Explain why reading from or writing to the printer port failed.
*/
static void printer_error(int error_number)
    {
    switch(error_number)
    	{
	case EIO:
	    alert(int_cmdline.printer, TRUE, _("Connection to printer lost."));
	    break;
    	default:
	    alert(int_cmdline.printer, TRUE, _("Serial communication failed, errno=%d (%s)."), error_number, gu_strerror(error_number));
	    break;
	}
    int_exit(EXIT_PRNERR);
    }

/*
** Tie it all together.
*/
int main(int argc, char *argv[])
    {
    int portfd;				/* file handle of the printer port */
    struct termios settings;		/* printer port settings */
    struct OPTIONS options;		/* a bundle of other options */

    /* Initialize internation messages library. */
    #ifdef INTERNATIONAL
    setlocale(LC_MESSAGES, "");
    bindtextdomain(PACKAGE_INTERFACES, LOCALEDIR);
    textdomain(PACKAGE_INTERFACES);
    #endif

    int_cmdline_set(argc, argv);

    DODEBUG(("============================================================"));
    DODEBUG(("\"%s\", \"%s\", \"%s\", %d, %s %d",
    	int_cmdline.printer,
    	int_cmdline.address,
    	int_cmdline.options,
    	int_cmdline.jobbreak,
    	int_cmdline.feedback ? "TRUE" : "FALSE",
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
    portfd = open_port(int_cmdline.printer, int_cmdline.address, &settings);

    /* Parse printer_options and set struct OPTIONS and
       printer port apropriately: */
    set_options(int_cmdline.printer, int_cmdline.options, portfd, &settings, &options);

    /*
    ** Make sure the necessary modem control lines are on to
    ** indicate that the printer is on line.
    **
    ** I think I got the information to write this test from
    ** the SunOS 5.x ioctl() man page.
    */
    #ifdef TIOCMGET
    if(options.online != 0)
	{
	int x;
	int modem_status;
	struct timeval tv;

	/* We must retry this operation because the printer may not
	   have had time to respond to our raising of DTR and RTS. */
	for(x=0; x < 20; x++)
	    {
	    if(ioctl(portfd, TIOCMGET, &modem_status) < 0)
		printer_error(errno);

	    DODEBUG(("modem status: %s %s %s",
		    modem_status & TIOCM_CD ? "CD" : "",
		    modem_status & TIOCM_DSR ? "DSR" : "",
		    modem_status & TIOCM_CTS ? "CTS" : ""
		    ));

	    if( (modem_status & options.online) == options.online )
		break;

	    /* Delay for 1 tenth of one second. */
	    tv.tv_sec = 0;
	    tv.tv_usec = 100000;
	    select(0, NULL, NULL, NULL, &tv);
	    }

	if(x == 20)
	    {
	    DODEBUG(("offline"));
	    fputs("%%[ PrinterError: off line ]%%\n", stdout);
	    int_exit(EXIT_ENGAGED);
	    }
	}
    #endif

    int_copy_job(portfd, options.idle_status_interval, printer_error, NULL, NULL, 0);

    DODEBUG(("closing port"));
    close(portfd);

    DODEBUG(("sucessful completion"));
    return EXIT_PRINTED;	/* needn't call int_exit() */
    } /* end of main() */

/* end of file */

