/*
** mouse:~ppr/src/interfaces/pros.c
** Copyright 1995--2005, Trinity College Computing Center.
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
** Last modified 13 January 2005.
*/

/*
** PPR printer interface program for the Axis PROS protocol.
*/

#include "config.h"
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

/* Change the 0 below to a 1 and recompile to turn on debugging. */
#if 0
#define DEBUG 1
#define DODEBUG(a) int_debug a
#else
#undef DEBUG
#define DODEBUG(a)
#endif

/*=========================================================================
** These are the constants of the PROS protocol.
=========================================================================*/

#define PROS_PORT 35
#define DEFAULT_PRINTER "LPT1"
#define DEFAULT_PASSWORD "netprinter"

/* These are the PROS error and printer (parallel port) status codes. */
#define PROSERR_HDR 0		/* bad header syntax */
#define PROSERR_MEM 1		/* print server out of memory */
#define PROSERR_NOA 2		/* access denied (wrong password maybe?) */
#define PROSERR_POC 3		/* printer "occupied" (talking to another client) */
#define PROSERR_BAN 4		/* bad printer name */
#define PROSERR_OLD 5		/* unsupported protocol version */
#define PROSERR_NOI 6		/* printer "not installed", whatever that means */
#define PROSERR_OFL 7		/* printer off-line */
#define PROSERR_EOP 8		/* printer out-of-paper */
#define PROSERR_BSY 9		/* printer busy */
#define PROSERR_PRO 10		/* protocol error */
#define PROSERR_UND 11		/* undefined error */

/* These are the PROS messages which the client (this program) 
 * can send to the print server.
 */
#define PROSMSG_EOF 32		/* no more data */
#define PROSMSG_UID 33		/* user name */
#define PROSMSG_HST 34		/* host name */
#define PROSMSG_PRN 35		/* queue select */
#define PROSMSG_PAS 36		/* password */
#define PROSMSG_DTP 37		/* data block */
#define PROSMSG_NOP 38		/* NOP - used as a 'tickle' to reset timeout */

/* These are the messages which the print server can send to the 
 * client (to this program).
 */
#define PROSMSG_JOK 48		/* PROSMSG_EOF acknowledge */
#define PROSMSG_JST 49		/* job acknowledgement */
#define PROSMSG_ACC 50		/* accounting data */
#define PROSMSG_DFP 51		/* data block */
#define PROSMSG_no_error 52	/* undocumented, indicates PROSERR_* cleared */

/* These values may be bitwise ored with any of the above. */
#define PROSBIT_FATAL 0x40	/* indicate that the condition described is fatal */
#define PROSBIT_DATA 0x80	/* indicates that a data section is included */

/*=========================================================================
** This structure is used to store the result of parsing the name=value 
** pairs in the interface options parameter on our command line.
=========================================================================*/
struct OPTIONS
	{
	struct TCP_CONNECT_OPTIONS connect;
	const char *password;
	};

/*=========================================================================
** These functions copy a job to the TCP connexion to the printer, using
** the PROS protocol.
**
** Note that though we are using select(), we are working with blocking
** file handles.
=========================================================================*/

static int pros_pack(unsigned char *buf, int buf_size, int buf_off, int code, const char string[]);
static int pros_reply_handler(unsigned char *buffer, int buffer_len, gu_boolean *recv_eoj, gu_boolean *job_start_acked);

static void pros_copy_job(int sockfd, struct OPTIONS *options, const char printer[], const char username[], const char hostname[])
    {
    unsigned char sendbuf[4096+3];			/* data being sent to the printer */
    int sendbuf_off;
    unsigned char recvbuf[4096+3];			/* data received from the printer */
	int last_stdin_read;					/* number of bytes received at last stdin read */
	gu_boolean job_start_acked = FALSE;
	gu_boolean sent_eoj = FALSE;			/* have we sent PROSMSG_EOF? */
	gu_boolean recv_eoj = FALSE;			/* have we received PROSMSG_JOK? */
	fd_set rfds, wfds;						/* select() sets for sockfd */
    int selret = -1;						/* return from last select() */
    time_t time_last_tickle = 0;
    struct timeval timeout;

    /* Build the PROS job header.  We will pack it into the print data buffer
     * whither we normally put the data which we receive from stdin.  It will
	 * be the first block we transmit to the print server.
     */
	sendbuf_off = 0;
	sendbuf_off = pros_pack(sendbuf, sizeof(sendbuf), sendbuf_off, PROSMSG_HST, hostname);
	sendbuf_off = pros_pack(sendbuf, sizeof(sendbuf), sendbuf_off, PROSMSG_UID, username);
	sendbuf_off = pros_pack(sendbuf, sizeof(sendbuf), sendbuf_off, PROSMSG_PRN, printer);
	sendbuf_off = pros_pack(sendbuf, sizeof(sendbuf), sendbuf_off, PROSMSG_PAS, options->password);
	last_stdin_read = sendbuf_off;

    /* Loop until there is nothing more to transmit and we have received an 
     * EOJ acknowledgement from the print server.
     */
	while(last_stdin_read || !recv_eoj)
		{
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_SET(sockfd, &rfds);
		timeout.tv_sec = 10;			/* lazy way of doing tickles */
		timeout.tv_usec = 0;

		if(sendbuf_off == 0)			/* if transmit buffer empty, */
			{
			if(!job_start_acked)
				{
				/* do nothing */
				}
			/* If we didn't get EOF on stdin, ask for more data. */
			else if(last_stdin_read != 0)
				{
				DODEBUG(("waiting for data on stdin"));
				FD_SET(0, &rfds);
				}
			/* If we get did EOF on stdin, but haven't sent EOJ to printer, prepare 
			 * the message.
			 */
			else if(!sent_eoj)
				{
				sendbuf[0] = PROSMSG_EOF;
				sendbuf_off = 1;
				sent_eoj = TRUE;
				time(&time_last_tickle);		/* start clock for first tickle */
				}
			/* Since we have sent EOJ already, see if it time for a 'tickle'.
			 * A tickle is a PROS no-operation message.  It serves no purpose
			 * other than to reasure the print server that we haven't crashed.
			 */
			else
				{
				time_t time_now;
				time(&time_now);
				if((time_now - time_last_tickle) >= 55)
					{
					DODEBUG(("time for a tickle"));
					time_last_tickle = time_now;
					sendbuf[0] = PROSMSG_NOP;
					sendbuf_off = 1;
					}
				}
			}

		/* If we have anything at all to send, tell select() we want to write
		 * to the socket.
		 */
		if(sendbuf_off > 0)
			{
			DODEBUG(("waiting to send a packet"));
			FD_SET(sockfd, &wfds);
			}

		/* Wait we can until:
			1) there is something to receive from stdin
			2) there is something to send to the print server
			3) there is something to receive from the print server
			*/
		if((selret = select(sockfd + 1, &rfds, &wfds, NULL, &timeout)) < 0)
			{
			alert(int_cmdline.printer, TRUE, "select() failed, errno=%d (%s)", errno, gu_strerror(errno));
			exit(EXIT_PRNERR);
			}

		/* If the printserver sent us something, */
		if(FD_ISSET(sockfd, &rfds))
			{
			int recvbuf_len = 0, ret;
			do	{
				if((ret = read(sockfd, recvbuf+recvbuf_len, sizeof(recvbuf)-recvbuf_len)) == -1)
					{
					alert(int_cmdline.printer, TRUE, "read() from socket failed, errno=%d (%s)", errno, gu_strerror(errno));
					exit(EXIT_PRNERR);
					}
				if(ret == 0)
					{
					alert(int_cmdline.printer, TRUE, "unexpected zero length read from socket");
					exit(EXIT_PRNERR);
					}
				DODEBUG(("received %d bytes from print server", ret));
				recvbuf_len += ret;
				} while((recvbuf_len = pros_reply_handler(recvbuf, recvbuf_len, &recv_eoj, &job_start_acked)) > 0);
			continue;
			}

		/* If the printserver is ready to receive data, */
		if(FD_ISSET(sockfd, &wfds))
			{
			int ret;
			DODEBUG(("dispatching %d byte packet to print server", sendbuf_off));
			if((ret = write(sockfd, sendbuf, sendbuf_off)) == -1)
				{
				alert(int_cmdline.printer, TRUE, "write() to socket failed, errno=%d (%s)", errno, gu_strerror(errno));
				exit(EXIT_PRNERR);
				}
			/* This is bad code. */
			if(ret != sendbuf_off)
				{
				alert(int_cmdline.printer, TRUE, "write() to socket wrote %d of %d bytes", ret, sendbuf_off);
				exit(EXIT_PRNERR);
				}
			sendbuf_off -= ret;
			}

		/* If stdin is ready to give us some data, */
		if(FD_ISSET(0, &rfds))
			{
			if((last_stdin_read = read(0, sendbuf+3, sizeof(sendbuf)-3)) == -1)
				{
				alert(int_cmdline.printer, TRUE, "read() from stdin failed, errno=%d (%s)", errno, gu_strerror(errno));
				exit(EXIT_PRNERR);
				}
			if(last_stdin_read == 0)
				{
				DODEBUG(("EOF on stdin"));
			    sendbuf_off = 0;
				}
			else
				{
				DODEBUG(("received %d bytes on stdin", last_stdin_read));
				sendbuf[0] = PROSMSG_DTP | PROSBIT_DATA;
				sendbuf[1] = last_stdin_read >> 8;
				sendbuf[2] = last_stdin_read & 0xFF;
				sendbuf_off = last_stdin_read + 3;
				}
			}
		}
    } /* end of pros_copy_job() */

/*
** This function packs a PROS operation code, two byte big endian length, and
** a string into a buffer.  It takes an offset into the buffer at which it 
** should store the bytes and returns the offset which should be used next 
** time it is called.
*/
static int pros_pack(unsigned char *buf, int buf_size, int buf_off, int code, const char string[])
	{
	int string_len = strlen(string);

	if((buf_size - buf_off) < (3 + string_len))
		return -1;

	buf[buf_off++] = code | PROSBIT_DATA;
	buf[buf_off++] = string_len >> 8;
	buf[buf_off++] = string_len & 0xFF;

	memcpy(&buf[buf_off], string, string_len);
	buf_off += string_len;

	return buf_off;
	} /* end of pros_pack() */

/*
** This function handles PROS message from the printer.  If it can't process
** all of the buffer (because there is a partial message), it will remove
** any messages which it is able to processes which proceeded the one it can't
** and return the number of bytes in the partial message.  Of course, if all
** messages are processed, it returns 0.
*/
static int pros_reply_handler(unsigned char *buffer, int buffer_len, gu_boolean *recv_eoj, gu_boolean *job_start_acked)
	{
    int code, masked_code, len, consumed;
    unsigned char *data;

	while(buffer_len > 0)
		{
	    code = buffer[0];
	    consumed = 1;
	    len = 0;
	    data = NULL;

		/* If this reply contains data, */
	    if(code & PROSBIT_DATA)
	    	{
			consumed += 2;		/* 16 bit length */

			/* If not even the header has been received yet, go back and wait. */
			if(buffer_len < consumed)
				return buffer_len;

			len = (buffer[1] << 8) | buffer[2];
			data = &buffer[3];
			consumed += len;

			/* If we don't have all of it yet, go back and wait. */
			if(consumed > buffer_len)
				return buffer_len;
			}

		masked_code = code & ~(PROSBIT_FATAL | PROSBIT_DATA);

		/* Process what we got. */
		DODEBUG(("Printer message: %d (0x%.2x) \"%.*s\"", code, code, len, data ? (char*)data : ""));

		if(code & PROSBIT_FATAL)				/* fatal error condition */
			{
			alert(int_cmdline.printer, TRUE, "Fatal PROS protocol error: %d (%.*s)", masked_code, len, data ? (char*)data : "");
			exit(EXIT_PRNERR);
			}
		else if(masked_code == PROSERR_POC)		/* printer occupied */
			{
			exit(EXIT_ENGAGED);
			}
		else if(masked_code == PROSMSG_JST)		/* printer willing to accept job */
			{
			*job_start_acked = TRUE;
			}
		else if(masked_code == PROSMSG_JOK)		/* EOJ acknowledgement */
			{
			*recv_eoj = TRUE;
			}
		else if(masked_code == PROSMSG_DFP)		/* data from printer (thru but not print print server) */
			{
			/*printf("%.*s", len, data ? (char*)data : "");*/
			write(1, data ? (char*)data : "", len);
			}
		else									/* other non-fatal condition */
			{
			/* debugging code */
			#ifdef DEBUG
			printf("%%%%[ PROS: %d %.*s ]%%%%\n", masked_code, len, data ? (char*)data : "");
			#endif

			/* Print a message that pprdrv will understand, just like
			   the parallel and serial interfaces do.
			   */
			switch(masked_code)
				{
				case PROSERR_OFL:
					gu_write_string(1, "%%[ PrinterError: off line ]%%\n");
					break;
				case PROSERR_EOP:
					gu_write_string(1, "%%[ PrinterError: out of paper ]%%\n");
					break;
				case PROSMSG_no_error:
					gu_write_string(1, "%%[ status: busy ]%%\n");	/* again busy printing our job */
					break;
				}
			}

        /* Move any remaining content down the buffer. */
		buffer_len -= consumed;
		if(buffer_len > 0)
			memmove(buffer, buffer+len+3, buffer_len-len-3);
		}

	return 0;
	} /* pros_reply_handler() */

/*=========================================================================
** Main
=========================================================================*/
int int_main(int argc, char *argv[])
	{
	struct OPTIONS options;
	const char *printer, *address;
	int sockfd;
	struct sockaddr_in printer_address;

	options.connect.refused_retries = 5;
	options.connect.refused_engaged = TRUE;
	options.connect.sndbuf_size = 0;			/* size for SO_SNDBUF, 0 means don't set it */
	options.connect.timeout = 20;				/* connexion timeout in seconds */
	options.password = DEFAULT_PASSWORD;

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

	/* Check for --probe. */
	if(int_cmdline.probe)
		{
		fprintf(stderr, _("The interface program \"%s\" does not support probing.\n"), int_cmdline.int_basename);
	    exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
		}

	/* Check for unusable job break methods. */
	if(int_cmdline.jobbreak == JOBBREAK_SIGNAL || int_cmdline.jobbreak == JOBBREAK_SIGNAL_PJL)
		{
		alert(int_cmdline.printer, TRUE,
				_("The jobbreak methods \"signal\" and \"signal/pjl\" are not compatible with\n"
				"the PPR interface program \"%s\"."), int_cmdline.int_basename);
		exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
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
		/*
		** First options shared with other TCP interface programs.
		*/
		if((retval = int_tcp_connect_option(name, value, &o, &options.connect)))
			{
			if(retval == -1)
				{
				/* o.error will be already set */
				break;
				}
			}
		/*
		** Now any additional options for this specific program.
		*/
		if(strcmp(name, "password") == 0)
			{
			options.password = gu_strdup(value);
			}
		/*
		** Here we catch anything else.
		*/
		else
			{
			o.error = N_("unrecognized keyword");
			o.index = o.index_of_name;
			retval = -1;
			break;
			}
		} /* end of while() loop */

	/* If parsing failed, use the information the OPTION_STATE structure
	 * to explain what happened.
	 */
	if(retval == -1)
		{
		alert(int_cmdline.printer, TRUE, _("Option parsing error:  %s"), gettext(o.error));
		alert(int_cmdline.printer, FALSE, "%s", o.options);
		alert(int_cmdline.printer, FALSE, "%*s^ %s", o.index, "", _("right here"));
		exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
		}
	}

    /* Separate the printer name from the rest of the address. */
	{
	char *p;
    if((p = strchr(int_cmdline.address, '@')))
    	{
		printer = int_cmdline.address;
		*p = '\0';
		address = p + 1;
    	}
	else
		{
		printer = DEFAULT_PRINTER;
		address = int_cmdline.address;
		}
	}

	/* Parse the host[:port] part of the printer address doing a DNS
	 * lookup if necessary.
	 */
	int_tcp_parse_address(address, PROS_PORT, &printer_address);

	/* Connect to the printer */
	gu_write_string(1, "%%[ PPR connecting ]%%\n");
	sockfd = int_tcp_open_connexion(int_cmdline.address, &printer_address, &options.connect, NULL, NULL);
	gu_write_string(1, "%%[ PPR connected ]%%\n");

	/* Disable SIGPIPE.  We will catch the error on write(). */
	signal_interupting(SIGPIPE, SIG_IGN);

    /* Do the PROS thing. */
    pros_copy_job(sockfd, &options, printer, int_cmdline.forline, "myhostname");

	/* Close the connection */
	close(sockfd);

	DODEBUG(("sucessful completion"));

	/* We can assume that it was printed. */
	return EXIT_PRINTED;
	} /* end of main() */

/* end of file */
