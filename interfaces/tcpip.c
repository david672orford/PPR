/*
** mouse:~ppr/src/interfaces/tcpip.c
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
** Last modified 10 June 2004.
*/

/*
** This interface program uses raw TCP/IP to communicating with devices such
** as HP Jetdirect cards or Extended Systems Pocket Print Servers.
**
** There are at least two variants of this protocol, known as SocketAPI and
** AppSocket.  These are described at:
**		<http://www.lprng.com/LPRng-HOWTO-Multipart/socketapi.htm>
**		<http://www.lprng.com/LPRng-HOWTO-Multipart/appsocket.htm>
**
** JetDirect cards appear to conform to SocketAPI.  The author of this code
** (David Chappell) doesn't have a printer that implements AppSocket, but
** from a reading of the above documents he believes that this the following
** options will set a queue to speak AppSocket instead of SocketAPI:
**
** $ ppad myprn jobbreak newinterface
** $ ppad myprn options appsocket_status_interval=15
**
** If one uses this interface under its alias "appsocket" these options
** will be selected automatically.
**
** If one uses it under the alias "jetdirect", the option
** snmp_status_interval=15 will be selected.
*/

#include "before_system.h"
#include <sys/types.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#ifndef INADDR_NONE
#define INADDR_NONE -1
#endif
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

/* Default port (9100 is the port used by HP JetDirect) */
#define DEFAULT_PORT 9100

/*
** This structure is used to store the result of parsing the name=value pairs
** in the interface options parameter on our command line.
*/
struct OPTIONS
	{
	struct TCP_CONNECT_OPTIONS connect;
	char *snmp_community;
	int idle_status_interval;
	int snmp_status_interval;
	int appsocket_status_interval;
	int sleep;
	gu_boolean use_shutdown;
	};

/*
** Explain why reading from or writing to the TCP connnection to the printer
** failed.
*/
static void explain_error_in_context(int error_number)
	{
	switch(error_number)
		{
		case EIO:
			alert(int_cmdline.printer, TRUE, _("Connection to printer lost."));
			break;
		default:
			alert(int_cmdline.printer, TRUE, _("TCP/IP communication failed, errno=%d (%s)."), error_number, gu_strerror(error_number));			break;
		}
	int_exit(EXIT_PRNERR);
	}

/*
** Use the shutdown() function to tell the printer we are done sending data.
*/
static void do_shutdown(int fd)
	{
	if(shutdown(fd, SHUT_WR) < 0)
		{
		alert(int_cmdline.printer, TRUE, _("%s() failed, errno=%d (%s)"), "shutdown", errno, gu_strerror(errno));
		int_exit(EXIT_PRNERR);
		}
	}

/*
** This fetches the printer status using SNMP and prints it in a special
** format which is picked up by the feedback reader in pprdrv.
*/
static void snmp_status(void *p)
	{
	struct gu_snmp *snmp_obj = p;
	int n1, n2;
	unsigned int n3;
	static gu_boolean failed = FALSE;

	if(!failed)
		{
		gu_Try
			{
			gu_snmp_get(snmp_obj,
				"1.3.6.1.2.1.25.3.2.1.5.1", GU_SNMP_INT, &n1,
				"1.3.6.1.2.1.25.3.5.1.1.1", GU_SNMP_INT, &n2,
				"1.3.6.1.2.1.25.3.5.1.2.1", GU_SNMP_BIT, &n3,
				NULL);

			/* This will be picked up by pprdrv. */
			printf("%%%%[ PPR SNMP: %d %d %08x ]%%%%\n", n1, n2, n3);
			fflush(stdout);
			}
		gu_Catch
			{
			if(strstr(gu_exception, "(noSuchName)"))
				{
				if(strcmp(int_cmdline.printer, "-") != 0)
					{
					alert(int_cmdline.printer, TRUE,
						_("This printer doesn't support the Host and Printer MIBs.  You should upgrade the\n"
						"firmware, if possible.  If not, you can turn off SNMP queries by adding the\n"
						"interface option \"snmp_status_interval=0\".")
						);
					}
				}
			else
				{
				alert(int_cmdline.printer, TRUE, "gu_snmp_get() failed: %s", gu_exception);
				}
			failed = TRUE;
			return;
			}
		}
		
	} /* end of snmp_status() */

/*=========================================================================
** Here we implement the AppSocket status mechanism.
=========================================================================*/

struct appsocket
	{
	int socket;
	char result[128];
	gu_boolean refused;
	};

static void *appsocket_status_open(unsigned int ip_address, int port)
	{
	struct sockaddr_in server_ip;
	struct sockaddr_in my_ip;
	int fd;
	struct appsocket *p;

	memset(&server_ip, 0, sizeof(server_ip));
	server_ip.sin_family = AF_INET;
	memcpy(&server_ip.sin_addr, &ip_address, sizeof(ip_address));
	server_ip.sin_port = htons(port);

	memset(&my_ip, 0, sizeof(my_ip));
	my_ip.sin_family = AF_INET;
	my_ip.sin_addr.s_addr = htonl(INADDR_ANY);
	my_ip.sin_port = htons(0);

	if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		gu_Throw("socket() failed, errno=%d (%s)", errno, gu_strerror(errno));

	gu_Try {
		if(bind(fd, (struct sockaddr *)&my_ip, sizeof(my_ip)) < 0)
			gu_Throw("bind() failed, errno=%d (%s)", errno, gu_strerror(errno));

		if(connect(fd, (struct sockaddr *)&server_ip, sizeof(server_ip)) < 0)
			gu_Throw("connect() failed, errno=%d (%s)", errno, gu_strerror(errno));
		}
	gu_Catch
		{
		int saved_errno = errno;
		close(fd);
		errno = saved_errno;
		gu_ReThrow();
		}

	/* We have suceeded!  Go ahead and allocate the structure and fill it in. */
	p = (struct appsocket *)gu_alloc(1, sizeof(struct gu_snmp));
	p->socket = fd;
	p->refused = FALSE;
	return (void*)p;
	}

static void appsocket_status(void *p)
	{
	struct appsocket *obj = (struct appsocket *)p;

	/* If we get "connection refused" on recv(), don't try again. */
	if(obj->refused)
		return;

	/* Send and resent the request until we get a response or the retries
	   are exhausted. */
	gu_Try {
		int attempt;
		fd_set rfds;
		struct timeval timeout;
		int len;

		for(attempt=0; attempt < 5; attempt++)
			{
			/* Send the request (an empty UDP packet). */
			DODEBUG(("sending..."));
			if(send(obj->socket, "", 0, 0) < 0)
				gu_Throw("send() failed, errno=%d (%s)", errno, gu_strerror(errno));

			/* Wait up to 1 second for a response. */
			FD_ZERO(&rfds);
			FD_SET(obj->socket, &rfds);
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;
			if(select(obj->socket + 1, &rfds, NULL, NULL, &timeout) < 0)
				gu_Throw("select() failed, errno=%d (%s)", errno, gu_strerror(errno));

			/* If there was nothing to read, start next iteration
			   (which will result in a resend. */
			if(!FD_ISSET(obj->socket, &rfds))
				continue;

			/* Receive the packet. */
			if((len = recv(obj->socket, obj->result, sizeof(obj->result), 0)) < 0)
				{
				if(errno == ECONNREFUSED)
					obj->refused = TRUE;
				gu_Throw("recv() failed, errno=%d (%s)", errno, gu_strerror(errno));
				}
			DODEBUG(("Got %d bytes", len));
			printf("%%%%[ %.*s ]%%%%\n", len, obj->result);
			return;
			}

		/* If we reach here, there was no valid response. */
		gu_Throw("timeout");
		}
	gu_Catch
		{
		printf("appsocket_status(): %s\n", gu_exception);
		}
	} /* end of appsocket_status() */

static void appsocket_status_close(void *p)
	{
	struct appsocket *obj = (struct appsocket *)p;
	close(obj->socket);
	gu_free(obj);
	}

/*=========================================================================
** Tie it all together.
=========================================================================*/
int int_main(int argc, char *argv[])
	{
	struct OPTIONS options;
	int sockfd;
	struct sockaddr_in printer_address;

	options.connect.refused_retries = 5;
	options.connect.refused_engaged = TRUE;
	options.connect.sndbuf_size = 0;			/* size for SO_SNDBUF, 0 means don't set it */
	options.connect.timeout = 20;				/* connexion timeout in seconds */
	options.idle_status_interval = 0;			/* frequency of ^T transmission */
	options.snmp_status_interval = 0;
	options.appsocket_status_interval = 0;
	options.snmp_community = NULL;
	options.use_shutdown = FALSE;
	options.sleep = 0;							/* time to sleep() after printing */

	/* Initialize internation messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_INTERFACES, LOCALEDIR);
	textdomain(PACKAGE_INTERFACES);
	#endif

	/* Process the command line and leave the results in
	   a global structure called "int_cmdline". */
	int_cmdline_set(argc, argv);

	DODEBUG(("============================================================"));
	DODEBUG(("%s printer=\"%s\", address=\"%s\", options=\"%s\", jobbreak=%d, feedback=%d, codes=%d, jobname=\"%s\", routing=\"%s\", forline=\"%s\", barbarlang=\"%s\"",
		int_cmdline.int_basename,
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
			  "the PPR interface program \"%s\"."),
			int_cmdline.int_basename
			);
		int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
		}

	/* If feedback is on, and control-d handshaking is on, turn on the ^T stuff. */
	if(int_cmdline.feedback && int_cmdline.jobbreak == JOBBREAK_CONTROL_D)
		options.idle_status_interval = 15;

	if(strcmp(int_cmdline.int_basename, "jetdirect") == 0)
		{
		options.snmp_status_interval = 15;
		}
	else if(strcmp(int_cmdline.int_basename, "appsocket") == 0)
		{
		options.appsocket_status_interval = 15;
		}

	/* Parse the options string, searching for name=value pairs. */
	{
	struct OPTIONS_STATE o;
	char name[32];
	char value[16];
	int retval;

	options_start(int_cmdline.options, &o);
	while((retval = options_get_one(&o, name, sizeof(name), value, sizeof(value))) > 0)
		{
		if((retval = int_tcp_connect_option(name, value, &o, &options.connect)))
			{
			if(retval == -1)
				{
				/* o.error is already set */
				break;
				}
			}
		/*
		** Frequency of ^T transmission
		*/
		else if(strcmp(name, "idle_status_interval") == 0)
			{
			if((options.idle_status_interval = atoi(value)) < 0)
				{
				o.error = N_("value must be a positive integer or zero");
				retval = -1;
				break;
				}
			}
		/*
		** Frequency of SNMP status queries
		*/
		else if(strcmp(name, "snmp_status_interval") == 0)
			{
			if((options.snmp_status_interval = atoi(value)) < 0)
				{
				o.error = N_("value must be a positive integer or zero");
				retval = -1;
				break;
				}
			}
		/*
		** SNMP community name.
		*/
		else if(strcmp(name, "snmp_community") == 0)
			{
			if(options.snmp_community)
				gu_free(options.snmp_community);
			options.snmp_community = gu_strdup(value);
			}
		/*
		** appsocket_status_interval
		*/
		else if(strcmp(name, "appsocket_status_interval") == 0)
			{
			if((options.appsocket_status_interval = atoi(value)) < 0)
				{
				o.error = N_("value must be a positive integer or zero");
				retval = -1;
				break;
				}
			}
		/*
		** use_shutdown
		*/
		else if(strcmp(name, "use_shutdown") == 0)
			{
			int answer;
			if((answer = gu_torf(value)) == ANSWER_UNKNOWN)
				{
				o.error = N_("Invalid boolean value");
				retval = -1;
				break;
				}
			options.use_shutdown = answer ? TRUE : FALSE;
			}
		/*
		** The delay after closing connection, before exiting.
		*/
		else if(strcmp(name, "sleep") == 0)
			{
			if((options.sleep = atoi(value)) < 0)
				{
				o.error = N_("value must be a positive integer or zero");
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
		options.idle_status_interval = 0;
		DODEBUG(("barbarlang=\"%s\", setting idle_status_interval to 0", int_cmdline.barbarlang));		}

	/* Describe the options in the debuging output. */
	DODEBUG(("sleep=%d, connect.timeout=%d, connect.sndbuf_size=%d, idle_status_interval=%d",
		options.sleep,
		options.connect.timeout,
		options.connect.sndbuf_size,
		options.idle_status_interval
		));

	/* We send this message so that commands such as "ppad ppdq" won't give up on us. */
	gu_write_string(1, "%%[ PPR address lookup ]%%\n");

	/* Parse the printer address and do a DNS lookup if necessary. */
	int_tcp_parse_address(int_cmdline.address, DEFAULT_PORT, &printer_address);

	/* Was --probe on the command line?  If so, probe the printer at the indicated
	   TCP/IP address rather than connecting to it for printing.
	   */
	if(int_cmdline.probe)
		{
		return int_tcp_probe(&printer_address, options.snmp_community);
		}

	/* Within this block we connect, transfer the data, and close the connexion. */
	{
	void (*status_function)(void *) = NULL;
	void *status_obj = NULL;
	int status_interval = 0;

	if(options.snmp_status_interval > 0)
		{
		gu_Try
			{
			status_obj = gu_snmp_open(printer_address.sin_addr.s_addr, options.snmp_community);
			}
		gu_Catch
			{
			alert(int_cmdline.printer, TRUE, "gu_snmp_open() failed: %s", gu_exception);
			return EXIT_PRNERR;
			}
		status_function = snmp_status;
		status_interval = options.snmp_status_interval;
		}
	else if(options.appsocket_status_interval > 0)
		{
		gu_Try
			{
			status_obj = appsocket_status_open(printer_address.sin_addr.s_addr, ntohs(printer_address.sin_port) + 1);
			}
		gu_Catch
			{
			alert(int_cmdline.printer, TRUE, "appsocket_status_open() failed: %s", gu_exception);
			return EXIT_PRNERR;
			}
		status_function = appsocket_status;
		status_interval = options.appsocket_status_interval;
		}

	/* Get initial status now in case the connection goes through slowly. */
	if(status_function)
		(*status_function)(status_obj);

	/* Connect to the printer */
	gu_write_string(1, "%%[ PPR connecting ]%%\n");
	sockfd = int_tcp_open_connexion(int_cmdline.address, &printer_address, &options.connect, status_function, status_obj);
	gu_write_string(1, "%%[ PPR connected ]%%\n");

	/* Disable SIGPIPE.  We will catch the error on write(). */
	signal_interupting(SIGPIPE, SIG_IGN);

	/* Copy stdin to the printer. */
	int_copy_job(sockfd,								/* connection to printer */
		options.idle_status_interval,					/* how often to send control-T */
		explain_error_in_context,						/* error printing function */
		options.use_shutdown ? do_shutdown : NULL,		/* EOJ function */
		status_function, status_obj, status_interval,	/* SNMP or AppSocket UDP status function */
		NULL											/* no init string */
		);

	/* Close the connection */
	close(sockfd);

	if(options.snmp_status_interval > 0)
		gu_snmp_close(status_obj);
	else if(options.appsocket_status_interval > 0)
		appsocket_status_close(status_obj);
	}

	/*
	** Extended Systems Pocket print servers have been known to reject a new
	** connection for a few seconds after closing the previous one.  If more
	** than one job is in the queue at one time, this can result in every
	** other print attempt producing a fault.  This problem is minor and can
	** go unnoticed, but we have the an option to sleep for a specified number
	** of seconds after closing the connection.
	**
    ** Note that this option has been made obsolete by the connection retry
    ** code.
    **
	** options.sleep is set with the option "sleep=SECONDS".
	*/
	if(options.sleep > 0)
		{
		DODEBUG(("Sleeping for %d seconds for printer recovery", options.sleep));
		sleep(options.sleep);
		}

	DODEBUG(("sucessful completion"));

	/* We can assume that it was printed. */
	return EXIT_PRINTED;
	} /* end of main() */

/* end of file */
