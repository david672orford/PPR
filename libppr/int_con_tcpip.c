/*
** mouse:~ppr/src/libppr/int_con_tcpip.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 11 May 2001.
*/

#include "before_system.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#ifndef INADDR_NONE
#define INADDR_NONE -1
#endif
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#ifdef INTERNATIONAL
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

/* Set to TRUE by the SIGALRM handler to indicate a connect() timeout. */
static volatile int sigalrm_caught = FALSE;

/*
** This is the SIGALRM handler which detects if the alarm goes
** off before the remote system responds.  Notice that it does
** nothing but set a flag.
*/
static void sigalrm_handler(int sig)
    {
    DODEBUG(("SIGALRM"));
    sigalrm_caught = TRUE;
    } /* end of sigalrm_handler() */

/*
** Make the connection to the printer.
** Return the file descriptor.
*/
int int_connect_tcpip(int connect_timeout, int sndbuf_size, gu_boolean refused_engaged, int refused_retries, const char snmp_community[], unsigned int *address_ptr)
    {
    const char *address = int_cmdline.address;
    int sockfd;
    struct sockaddr_in printer_addr;
    char *ptr;
    struct hostent *hostinfo;
    int retval;
    int try_count;

    /* Install SIGALRM handler for connect() timeouts. */
    signal_interupting(SIGALRM, sigalrm_handler);

    /* Clear the printer address structure. */
    memset(&printer_addr, 0, sizeof(printer_addr));

    /* Parse the address into host and port. */
    if(strpbrk(address, " \t") != (char*)NULL)
    	{
    	alert(int_cmdline.printer, TRUE, _("Spaces and tabs not allowed in TCP/IP a printer address."
				"\"%s\" does not conform to this requirement."), address);
    	int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
    	}

    if((ptr = strchr(address, ':')) == (char*)NULL || ! isdigit(ptr[1]))
    	{
    	alert(int_cmdline.printer, TRUE, _("TCP/IP printer address must be in form \"host:portnum\","
					"\"%s\" does not conform to this requirement."), address);
	int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
	}

    /* Put the port number in the structure. */
    printer_addr.sin_port = htons(atoi(ptr+1));

    /* Terminate the host part so that the port number will be ignored for now. */
    *ptr = '\0';

    /*
    ** If convertion of a dotted address works, use it,
    ** otherwise, use gethostbyname().
    */
    if((printer_addr.sin_addr.s_addr=inet_addr(address)) != INADDR_NONE)
	{
	printer_addr.sin_family = AF_INET;
    	}
    else
    	{
	if((hostinfo = gethostbyname(address)) == (struct hostent *)NULL)
	    {
	    alert(int_cmdline.printer, TRUE, _("TCP/IP interface can't determine IP address for \"%s\"."), address);
	    int_exit(EXIT_PRNERR_NO_SUCH_ADDRESS);
	    }
	printer_addr.sin_family = hostinfo->h_addrtype;
	memcpy(&printer_addr.sin_addr, hostinfo->h_addr_list[0], hostinfo->h_length);
    	}

    /*
    ** Now that inet_addr() and gethostbyname() have had a chance to
    ** examine it, put the address back the way it was.  That way it
    ** will look ok in alert messages.
    */
    *ptr = ':';

    /*
    ** Connect the socket to the printer.
    ** Some systems, noteably Linux, fail to reliably implement
    ** connect() timeouts; that is why we use alarm() to
    ** create our own timeout.
    */
    for(try_count=0; TRUE; try_count++)
	{
        /* Create a socket.  We do this every time because Linux
           won't let us try again. */
        if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
            alert(int_cmdline.printer, TRUE, "%s interface: socket() failed, errno=%d (%s)", int_cmdline.int_basename, errno, gu_strerror(errno));
            int_exit(EXIT_PRNERR);
            }

	DODEBUG(("calling connect()"));
	alarm(connect_timeout);
	retval = connect(sockfd, (struct sockaddr*) &printer_addr, sizeof(printer_addr));
	alarm(0);
	DODEBUG(("connect() returned %d", retval));

	/* If a timeout occured, */
	if(sigalrm_caught)
	    {
	    alert(int_cmdline.printer, TRUE, _("Printer \"%s\" is not responding.\n"
					    "(Aborted after connect() blocked for %d seconds.)"), address, connect_timeout);
	    close(sockfd);
	    int_exit(EXIT_PRNERR_NOT_RESPONDING);
	    }

	/* If connect() failed, */
	if(retval < 0)
	    {
	    int saved_errno = errno;
	    close(sockfd);

	    DODEBUG(("connect() failed, errno=%d (%s)", saved_errno, gu_strerror(saved_errno)));
	    switch(saved_errno)
		{
		case ETIMEDOUT:
		    alert(int_cmdline.printer, TRUE, _("Timeout while trying to connect to printer."
					"(Connect() reported error ETIMEDOUT.)"));
		    int_exit(EXIT_PRNERR_NOT_RESPONDING);
		    break;
		case ECONNREFUSED:
		    if(try_count < refused_retries)
		    	{
		    	sleep(2);
		    	continue;
		    	}
		    if(refused_engaged)
			{
			struct gu_snmp *snmp_obj;
			int error_code;
		       	if(!(snmp_obj = gu_snmp_open(printer_addr.sin_addr.s_addr, snmp_community, &error_code)))
			    {
			    alert(int_cmdline.printer, TRUE, "gu_snmp_open() failed, error_code=%d", error_code);
			    return EXIT_PRNERR;
			    }
			int_snmp_status(snmp_obj);
			gu_snmp_close(snmp_obj);
			int_exit(EXIT_ENGAGED);
			}
		    else
		    	{
                        alert(int_cmdline.printer, TRUE, _("Printer at \"%s\" has refused connection."), address);
                        int_exit(EXIT_PRNERR);
		    	}
		    break;
		default:
		    alert(int_cmdline.printer, TRUE, "%s interface: connect() failed, errno=%d (%s)", int_cmdline.int_basename, saved_errno, gu_strerror(saved_errno));
		    int_exit(EXIT_PRNERR);
		    break;
		}
	    }

	/* If we get here it was a sucess */
	break;
	} /* end of connect retry loop */

    /*
    ** Turn on the socket option which detects dead connexions.
    */
    {
    int true_variable = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void*)&true_variable, sizeof(true_variable)) < 0)
    	{
    	alert(int_cmdline.printer, TRUE, "%s interface: setsockopt() failed for SO_KEEPALIVE, errno=%d (%s)", int_cmdline.int_basename, errno, gu_strerror(errno));
	int_exit(EXIT_PRNERR_NORETRY);
    	}
    }

    /*
    ** If the user has supplied an explicit output
    ** buffer size setting, use it.
    */
    if(sndbuf_size != 0)
	{
	DODEBUG(("setting SO_SNDBUF to %d", sndbuf_size));
	if(setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (void*)&sndbuf_size, sizeof(sndbuf_size) ) < 0)
	    {
	    alert(int_cmdline.printer, TRUE, "%s interface: setsockopt() failed SO_SNDBUF, errno=%d (%s)", int_cmdline.int_basename, errno, gu_strerror(errno));
	    int_exit(EXIT_PRNERR_NORETRY);
	    }
	}

    *address_ptr = printer_addr.sin_addr.s_addr;
    return sockfd;
    } /* end of int_connect_tcpip() */

/* end of int_con_tcpip.c */

