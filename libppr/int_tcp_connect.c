/*
** mouse:~ppr/src/interfaces/libppr/int_tcp_connect.c
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
** Last modified 6 April 2003.
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
#include <sys/socket.h>
#include <netinet/in.h>
#ifndef INADDR_NONE
#define INADDR_NONE -1
#endif
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "libppr_int.h"
#include "interface.h"

/* Change the zero below to a one and recompile to turn on debugging. */
#if 0
#define DEBUG 1
#define DODEBUG(a) int_debug a
#else
#undef DEBUG
#define DODEBUG(a)
#endif

int int_tcp_connect_option(const char name[], const char value[], struct OPTIONS_STATE *o, struct TCP_CONNECT_OPTIONS *options)
	{
	/*
	** refused_retries
	*/
	if(strcmp(name, "refused_retries") == 0)
		{
		if((options->refused_retries = atoi(value)) < 0)
			{
			o->error = N_("value must be 0 or a positive integer");
			return -1;
			}
		return 1;
		}
	/*
	** refused=engaged
	** refused=error
	*/
	if(strcmp(name, "refused") == 0)
		{
		if(strcmp(value, "engaged") == 0)
			options->refused_engaged = TRUE;
		else if(strcmp(value, "error") == 0)
			options->refused_engaged = FALSE;
		else
			{
			o->error = N_("value must be \"engaged\" or \"error\"");
			return -1;
			}
		return 1;
		}
	/*
	** Connect timeout
	*/
	if(strcmp(name, "connect_timeout") == 0)
		{
		if((options->timeout = atoi(value)) < 1)
			{
			o->error = N_("value must be a positive integer");
			return -1;
			}
		return 1;
		}
	/*
	** Size for TCP/IP send buffer
	*/
	if(strcmp(name, "sndbuf_size") == 0)
		{
		if((options->sndbuf_size = atoi(value)) < 1)
			{
			o->error = N_("value must be a positive integer");
			return -1;
			}
		return 1;
		}

    return 0;
	} /* end of int_tcp_connect_option() */

/*
** This is the SIGALRM handler which detects if the alarm goes off before the
** remote system responds to connect().  Notice that it does nothing but set
** a flag.
*/
static volatile int sigalrm_caught = FALSE;
static void sigalrm_handler(int sig)
	{
	DODEBUG(("SIGALRM"));
	sigalrm_caught = TRUE;
	} /* end of sigalrm_handler() */

/*
** Parse a printer address and return an IP address and port.  The program is
** aborted if the parse fails or a DNS lookup fails.  The address syntax
** is host:port where host can be a name or an IP address and port can be
** a name or a decimal number.
*/
void int_tcp_parse_address(const char address[], int default_port, struct sockaddr_in *printer_addr)
	{
	char *ptr;
	struct hostent *hostinfo;

	/* Clear the printer address structure. */
	memset(printer_addr, 0, sizeof(printer_addr));

	/* Parse the address into host and port. */
	if(strpbrk(address, " \t"))
		{
		alert(int_cmdline.printer, TRUE, _("Spaces and tabs not allowed in a TCP/IP printer address."
								"\"%s\" does not conform to this requirement."), address);
		int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
		}

	if((ptr = strchr(address, ':')))
		{
		if(! isdigit(ptr[1]))
			{
			alert(int_cmdline.printer, TRUE,
				_("TCP ports must be specified numberically.  The port \"%s\" is not a number."), ptr+1);
			int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
			}
		/* Put the port number in the structure. */
		printer_addr->sin_port = htons(atoi(ptr+1));

		/* Terminate the host part so that the port number will be ignored for now. */
		*ptr = '\0';
		}
	else
		{
		printer_addr->sin_port = htons(default_port);
		}

	/*
	** If convertion of a dotted address works, use it,
	** otherwise, use gethostbyname().
	*/
	if((printer_addr->sin_addr.s_addr = inet_addr(address)) != INADDR_NONE)
		{
		printer_addr->sin_family = AF_INET;
		}
	else
		{
		if((hostinfo = gethostbyname(address)) == (struct hostent *)NULL)
			{
			alert(int_cmdline.printer, TRUE, _("TCP/IP interface can't determine IP address for \"%s\"."), address);
			int_exit(EXIT_PRNERR_NO_SUCH_ADDRESS);
			}
		printer_addr->sin_family = hostinfo->h_addrtype;
		memcpy(&printer_addr->sin_addr, hostinfo->h_addr_list[0], hostinfo->h_length);
		}

	/*
	** Now that inet_addr() and gethostbyname() have had a chance to
	** examine it, put the address back the way it was.  That way it
	** will look ok in alert messages.
	*/
	if(ptr)
		*ptr = ':';
	} /* end of int_tcp_parse_address() */

/*
** Make the connection to the printer.
** Return the file descriptor.
*/
int int_tcp_open_connexion(const char address[], struct sockaddr_in *printer_addr, struct TCP_CONNECT_OPTIONS *options, void (*status_function)(void *), void *status_obj)
	{
	int sockfd;
	int retval;
	int try_count;

	/* Install SIGALRM handler for connect() timeouts. */
	signal_interupting(SIGALRM, sigalrm_handler);

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
		alarm(options->timeout);
		retval = connect(sockfd, (struct sockaddr*)printer_addr, sizeof(struct sockaddr_in));
		alarm(0);
		DODEBUG(("connect() returned %d", retval));

		/* If a timeout occured, */
		if(sigalrm_caught)
			{
			alert(int_cmdline.printer, TRUE, _("Printer \"%s\" is not responding.\n"
											"(Aborted after connect() blocked for %d seconds.)"), address, options->timeout);
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
					if(try_count < options->refused_retries)
						{
						sleep(2);
						continue;
						}
					if(options->refused_engaged && status_function)
						{
						(*status_function)(status_obj);
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
	if(options->sndbuf_size != 0)
		{
		DODEBUG(("setting SO_SNDBUF to %d", sndbuf_size));
		if(setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (void*)&options->sndbuf_size, sizeof(options->sndbuf_size) ) < 0)
			{
			alert(int_cmdline.printer, TRUE, "%s interface: setsockopt() failed SO_SNDBUF, errno=%d (%s)", int_cmdline.int_basename, errno, gu_strerror(errno));
			int_exit(EXIT_PRNERR_NORETRY);
			}
		}

	return sockfd;
	} /* end of int_tcp_open_connexion() */

/* end of file */
