/*
** mouse:~ppr/src/interfaces/lpr.c
** Copyright 1995--2005, Trinity College Computing Center.
** Written by David Chappell and Damian Ivereigh.
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
** Last modified 14 September 2005.
*/

/*
** This interface sends the print job to another computer
** using the Berkeley LPR protocol.
*/

#include "config.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <sys/utsname.h>
#ifndef INADDR_NONE
#define INADDR_NONE -1
#endif
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "interface.h"
#include "libppr_int.h"
#include "rfc1179.h"

/* Possible enable debugging.  The debuging output goes to
   a file in /var/spool/ppr/logs. */
#if 0
#define DODEBUG(a) int_debug a
#else
#define DODEBUG(a)
#endif

/* Time to wait for connect() to finish: */
#define LPR_CONNECT_TIMEOUT 20

/* The file we use to get the next lpr jobid: */
#define LPR_PREVID_FILE STATEDIR"/lastid_uprint_lpr"

/* These are the names we give to the data and control files. */
#define DF_TEMPLATE "dfA%03d%s"
#define CF_TEMPLATE "cfA%03d%s"

/*
** TIMEOUT_HANDSHAKE is the time to wait on operations that
** should be completed immediately, such as stating the desired
** queue name.  TIMEOUT_PRINT is the amount of time to wait for
** a response to the zero byte sent at the end of a data file.
** TIMEOUT_PRINT can be set separately because some printer
** Ethernet boards don't respond right away.
*/
#define TIMEOUT_HANDSHAKE 30
#define TIMEOUT_PRINT 0

/* Our parse options list */
struct OPTIONS
	{
	gu_boolean banner;
	char lpr_typecode;
	int chunk_size;
	int exaggerated_size;
	gu_boolean temp_first;
	char *snmp_community;
	} ;

/*
** This function is called by libuprint when it wants to print
** an error message.
*/
static void lpr_error_callback(const char *format, ...)
	{
	va_list va;
	alert(int_cmdline.printer, TRUE, "The PPR interface program \"%s\" failed for the following reason:", int_cmdline.int_basename);
	va_start(va, format);
	valert(int_cmdline.printer, FALSE, format, va);
	va_end(va);
	} /* end of lpr_error_callback() */

static void sigalrm_handler(int sig)
	{ }

/*
** Make the connection to the printer.
** Return the file descriptor.
*/
static int lpr_make_connection(const char address[])
	{
	int sockfd;							/* handle of socket we will connect to LPR server */
	struct sockaddr_in printer_addr;	/* internet address of LPR server */
	struct hostent *hostinfo;			/* return value of gethostbyname() */
	int retval;

	/* Check for any obvious syntax error in the address */
	if(strpbrk(address, " \t"))
		{
		lpr_error_callback(_("Spaces and tabs are not allowed in LPR hostnames."));
		return -EXIT_PRNERR_NORETRY_BAD_SETTINGS;
		}

	/* Clear the printer internet address structure. */
	memset(&printer_addr, 0, sizeof(printer_addr));

	/*
	** If the address specifies the number of a port to use, use that,
	** otherwise, find out which port we should connect
	** to by looking in the /etc/services database.  It
	** we don't find it, we will use the value 515.
	*/
	{
	char *ptr;

	if((ptr = strchr(address, ':')) != (char*)NULL)
		{
		if((printer_addr.sin_port = htons(atoi(ptr+1))) == 0)
			{
			lpr_error_callback(_("Bad port specification in printer address."));
			return -EXIT_PRNERR_NORETRY_BAD_SETTINGS;
			}

		*ptr = '\0';			/* truncate */
		}
	else						/* If address doesn't specify the port, */
		{
		struct servent *sp;
		if ((sp = getservbyname("printer", "tcp")) != (struct servent *)NULL)
			{
			printer_addr.sin_port = sp->s_port;
			}
		else
			{
			/* Use the default */
			printer_addr.sin_port = htons(515);
			}
		}
	}

	/*
	** If converstion of a dotted address works, use it,
	** otherwise, use gethostbyname() to find the address.
	*/
	if((printer_addr.sin_addr.s_addr = inet_addr(address)) != INADDR_NONE)
		{
		printer_addr.sin_family = AF_INET;
		}
	else
		{
		if((hostinfo = gethostbyname(address)) == (struct hostent *)NULL)
			{
			lpr_error_callback(_("IP address lookup for \"%s\" failed."), address);
			return -EXIT_PRNERR_NO_SUCH_ADDRESS;
			}

		printer_addr.sin_family = hostinfo->h_addrtype;
		memcpy(&printer_addr.sin_addr, hostinfo->h_addr_list[0], hostinfo->h_length);
		}

	/*
	 * Open a TCP/IP socket which we will use to connect
	 * to the LPD server.
	 */
	#ifdef BIND_ACCESS_BUG
	{
	uid_t saved_euid = geteuid();
	seteuid(0);
	#endif

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	#ifdef BIND_ACCESS_BUG
	{ int saved_errno = errno; seteuid(saved_euid); errno = saved_errno; }
	}
	#endif

	if(sockfd == -1)
		{
		lpr_error_callback("socket() failed, errno=%d (%s).", errno, gu_strerror(errno));
		return -EXIT_PRNERR_NORETRY;
		}

	/*
	** If we are root, get a priviledged port number
	** assigned to our socket.
	*/
	{
	uid_t saved_euid = geteuid();
	if(saved_euid == 0 || seteuid(0) != -1)
		{
		u_short lport = (IPPORT_RESERVED - 1);	/* first reserved port */
		struct sockaddr_in sin;					/* address to bind client socket to */
		int ret;								/* return code from bind() */

		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = htonl(INADDR_ANY);

		while(TRUE)
			{
			sin.sin_port = htons(lport);
			if((ret = bind(sockfd, (struct sockaddr *)&sin, sizeof(sin))) != -1)
				break;

			if(errno == EADDRINUSE)		/* If there error is that this port in use, */
				{
				if(--lport >= (IPPORT_RESERVED / 2))
					continue;
				lport = 0;				/* flag for later */
				}

			break;
			}

		/* Switch back before possibly calling lpr_error_callback(). */
		{
		int saved_errno = errno;
		seteuid(saved_euid);
		errno = saved_errno;
		}

		if(lport == 0)
			{
			lpr_error_callback(_("No unused TCP ports available in reserved range."));
			return -EXIT_PRNERR_NORETRY;
			}

		if(ret == -1)
			{
			lpr_error_callback("bind() failed, port=%d, errno=%d (%s)", (int)lport, errno, gu_strerror(errno));
			return -EXIT_PRNERR_NORETRY;
			}
		}
	} /* end of saved_euid context */

	/*
	** Connect the socket to the printer.  Since some
	** systems, such as Linux, fail to implement
	** connect() timeouts, we will do it with alarm().
	*/
	{
	struct sigaction old_handler, new_handler;
	new_handler.sa_handler = sigalrm_handler;
	sigemptyset(&new_handler.sa_mask);
	new_handler.sa_flags =
		#ifdef SA_INTERUPT
		SA_INTERUPT;
		#else
		0;
		#endif
	sigaction(SIGALRM, &new_handler, &old_handler);
	alarm(LPR_CONNECT_TIMEOUT);

	retval = connect(sockfd, (struct sockaddr*) &printer_addr, sizeof(printer_addr));

	alarm(0);
	sigaction(SIGALRM, &old_handler, (struct sigaction *)NULL);
	}

	/* If connect() failed, */
	if(retval == -1)
		{
		switch(errno)
			{
			case ETIMEDOUT:
			case EINTR:
				lpr_error_callback(_("Timeout while trying to connect to lpd server \"%s\"."), address);
				return -EXIT_PRNERR;

			case ECONNREFUSED:
				lpr_error_callback(_("Remote system \"%s\" has refused the connection."), address);
				return -EXIT_PRNERR;

			case EADDRINUSE:
				lpr_error_callback("connect() set errno to EADDRINUSE!\n"
										"(This indicates a bug in libuprint!)");
				return -EXIT_PRNERR_NORETRY;

			default:
				lpr_error_callback("connect() failed, errno=%d (%s)", errno, gu_strerror(errno));
				return -EXIT_PRNERR;
			}
		}

	/* Turn on the socket option which detects dead connections. */
	{
	int truevalue = 1;
	if( setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void*)&truevalue, sizeof(truevalue)) == -1 )
		{
		lpr_error_callback("setsockopt() failed to set SO_KEEPALIVE, errno=%d (%s)", errno, gu_strerror(errno));
		return -EXIT_PRNERR_NORETRY;
		}
	}

	return sockfd;
	} /* end of lpr_make_connection() */

/*
** Send a command to the lpd server.
*/
static int lpr_send_cmd(int fd, const char text[], int length)
	{
	int result;

	if((result = write(fd, text, length)) == -1)
		{
		lpr_error_callback(X_("write() to lpd server failed, errno=%d (%s)"), errno, gu_strerror(errno));
		return -1;
		}

	if(result != length)
		{
		lpr_error_callback(X_("write() to lpd server wrote only %d of %d bytes"), result, length);
		return -1;
		}

	return 0;
	} /* end of lpr_send_cmd() */

/*
** Fetch a 1 byte result code from the RFC-1179 server and return it.
** If an error occurs, -1 is returned.  This routine leaves the 
** printing of error messages up to the caller.
**
** Note: in the RFC-1179 protocol, a result byte of 0 indicates sucess.
*/
static int lpr_response(int sockfd, int timeout)
	{
	char result;
	int retval;
	fd_set sockfd_set;
	struct timeval tv;

	FD_ZERO(&sockfd_set);
	FD_SET(sockfd, &sockfd_set);
	tv.tv_sec = timeout;
	tv.tv_usec = 0;

	if((retval = select(sockfd + 1, &sockfd_set, NULL, NULL, timeout > 0 ? &tv : NULL)) != 1)
		{
		if(retval < 0)
			{
			lpr_error_callback(X_("select() on socket failed, errno=%d (%s)"), errno, gu_strerror(errno));
			}
		else
			{
			lpr_error_callback(_("Timeout while waiting for response from print server."));
			}

		return -1;
		}

	if((retval = read(sockfd, &result, 1)) == -1)
		{
		lpr_error_callback(X_("read() from lpd server failed, errno=%d (%s)"), errno, gu_strerror(errno));
		return -1;
		}

	/* If the read didn't return 1 byte, */
	if(retval != 1)
		{
		lpr_error_callback(X_("read() from lpd server returned %d bytes, 1 byte result code expected"), retval);
		return -1;
		}

	return result;
	} /* end of lpr_response() */

/*
** Return the nodename we should use to identify ourself in
** lpr queue files.
*/
static const char *lpr_nodename()
	{
	const char function[] = "lpr_nodename";
	static struct utsname sysinfo;

	/* Get the name of this system */
	if(uname(&sysinfo) == -1)
		{
		lpr_error_callback("%s(): uname() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
		return (char*)NULL;
		}

	return sysinfo.nodename;
	} /* end of lpr_nodename() */

/*
** Return the next id number.  The string "file" is the full path and
** name of the file to use.  The file is locked, the last id is read,
** and incremented making the new id, and the new id is written back
** to the file, the file is closed, and the new id is returned.
*/
static int lpr_nextid(void)
	{
	const char *myname = "lpr_nextid";
	int fd;				/* file descriptor of id file */
	int tid;			/* for holding id */
	const char *file = LPR_PREVID_FILE;
	char buf[11];
	int len;

	/* If we can open the ID file, */
	if((fd = open(file, O_RDWR)) != -1)
		{
		if(gu_lock_exclusive(fd, TRUE))
			{
			lpr_error_callback("%s(): can't lock \"%s\"", myname, file);
			return -1;
			}

		if((len = read(fd, buf, 10)) == -1)
			{
			lpr_error_callback("%s(): read() failed, errno=%d (%s)", myname, errno, gu_strerror(errno));
			return -1;
			}

		buf[len] = '\0';				/* make buffer ASCIIZ */
		tid = atoi(buf);				/* convert ASCII to binary */

		tid++;							/* add one to it */
		if(tid < 1 || tid > 999)		/* if id unreasonable or too large */
			tid = 1;					/* force it to one */

		lseek(fd, (off_t)0, SEEK_SET);	/* move to start, for write */
		}
	else								/* does not exist */
		{
		tid = 1;						/* start id's at 0 */

		/* Create a new file: */
		if((fd = open(file, O_WRONLY | O_CREAT, UNIX_644)) == -1)
			{
			lpr_error_callback("%s(): can't create \"%s\", errno=%d (%s)", myname, file, errno, gu_strerror(errno));
			return -1;
			}
		}

	snprintf(buf, sizeof(buf), "%d\n", tid);	/* write the new id */
	write(fd, buf, strlen(buf));
	close(fd);									/* and close the id file */

	return tid;
	} /* end of lpr_nextid() */

/*
** Copy stdin to the named file, return the open file
** and set the length.
*/
static int uprint_file_stdin(int *length)
	{
	const char function[] = "uprint_file_stdin";
	char *copybuf;						/* buffer for copy operation */
	int copyfd;							/* file descriptor of temp file */
	int rbytes, wbytes;					/* bytes read and written */

	/* Set length initially to zero. */
	*length = 0;

	{
	char fname[MAX_PPR_PATH];
	snprintf(fname, sizeof(fname), "%s/uprint-%ld-XXXXXX", TEMPDIR, (long)getpid());

	if((copyfd = mkstemp(fname)) == -1)
		{
		lpr_error_callback("%s(): mkstemp() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
		return -1;
		}

	if(unlink(fname) < 0)
		{
		lpr_error_callback("%s(): unlink(\"%s\") failed, errno=%d (%s)", function, fname, errno, gu_strerror(errno));
		return -1;
		}
	}

	/* Copy stdin to a temporary file */
	if((copybuf = (char*)malloc((size_t)16384)) == (void*)NULL)
		{
		lpr_error_callback("%s(): malloc() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
		close(copyfd);
		return -1;
		}

	/* copy until end of file */
	while( (rbytes=read(0, copybuf, 4096)) )
		{
		if(rbytes==-1)
			{
			lpr_error_callback("%s(): read() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
			return -1;
			}

		if( (wbytes=write(copyfd, copybuf, rbytes)) != rbytes )
			{
			if(wbytes == -1)
				lpr_error_callback("%s(): write() failed on tempfile, errno=%d (%s)", function, errno, gu_strerror(errno));
			else
				lpr_error_callback("%s(): disk full while writing tempfile", function);

			return -1;
			}

		*length += rbytes;				/* add to total */
		}

	free(copybuf);						/* free the buffer */

	lseek(copyfd, (off_t)0, SEEK_SET);	/* return to start of file */

	return copyfd;
	} /* end of uprint_file_stdin() */

/*
** Copy the data to the printer.
*/
static int lpr_send_data_file(int source, int sockfd)
	{
	const char function[] = "uprint_send_data_file";
	int just_read, bytes_left, just_written;
	char *ptr;
	char buffer[4096];			/* Buffer for data transfer */

	do	{
		if((just_read=read(source, buffer, sizeof(buffer))) == -1)
			{
			lpr_error_callback("%s(): error reading temp file, errno=%d (%s)", function, errno, gu_strerror(errno));
			return -1;
			}
		bytes_left = just_read;
		ptr = buffer;
		while(bytes_left)
			{
			if( (just_written=write(sockfd, ptr, bytes_left)) == -1 )
				{
				lpr_error_callback("%s(): error writing to socket, errno=%d (%s)", function, errno, gu_strerror(errno));
				return -1;
				}
			ptr += just_written;
			bytes_left -= just_written;
			}
		} while(just_read);

	return 0;
	} /* end of lpr_send_data_file() */

/*
** Send the control file.
*/
static void do_control_file(int sockfd, const char *local_nodename, const char *address_queue, int lpr_queueid, const struct OPTIONS *options)
	{
	char person[LPR_MAX_P+1];			/* name to use with P line in control file */
	char control_file[2048];			/* The actual control file contents. */
	char command[80];					/* staging area for command to LPD */
	int result;

	/* Create a cleaned-up version of the "%%For:" line. */
	{
	int i, c;
	for(i=0; i < LPR_MAX_P && (c = int_cmdline.forline[i]) && c != '@'; i++)
		{
		if(isspace(c))
			person[i] = '_';
		else
			person[i] = c;
		}
	person[i] = '\0';
	}

	/* Build the `control file' */
	snprintf(control_file, sizeof(control_file),
		"H%s\n"							/* host */
		"P%s\n"							/* person */
		"%s%s%s"						/* name for banner page */
		"%c"DF_TEMPLATE"\n"				/* file to print */
		"U"DF_TEMPLATE"\n"				/* file to unlink */
		"N%s\n"							/* file name, to keep System V LP happy */
		"J%s\n",						/* job title (for banner page?) */
				local_nodename,
				person,
				options->banner ? "L" : "",
				options->banner ? person : "",
				options->banner ? "\n" : "",
				options->lpr_typecode, lpr_queueid, local_nodename,
				lpr_queueid, local_nodename,
				int_cmdline.title,
				int_cmdline.title);

	DODEBUG(("control file: %d bytes: \"%s\"", strlen(control_file), control_file));

	/* Tell the server we want to send the control file. */
	snprintf(command, sizeof(command), "\002%d "CF_TEMPLATE"\n", (int)strlen(control_file), lpr_queueid, local_nodename);
	if(lpr_send_cmd(sockfd, command, strlen(command)) == -1)
		exit(EXIT_PRNERR);

	/* Check if response is favourable. */
	if((result = lpr_response(sockfd, TIMEOUT_HANDSHAKE)))
		{
		if(result == -1)		/* elaborate on error message */
			alert(int_cmdline.printer, FALSE, _("(Communication failure while negotiating to send control file.)"));
		else					/* if remote end refuses, */
			alert(int_cmdline.printer, TRUE, _("Remote LPR/LPD system does not have room for control file."));
		exit(EXIT_PRNERR);
		}

	/* Send the control file, including the terminating zero byte. */
	if(lpr_send_cmd(sockfd, control_file, strlen(control_file) + 1) == -1)
		exit(EXIT_PRNERR);

	/* Check if response if favourable. */
	if((result = lpr_response(sockfd, TIMEOUT_HANDSHAKE)))
		{
		if(result == -1)		/* elaborate on read error message */
			alert(int_cmdline.printer, FALSE, _("(Communication failure while sending control file.)"));
		else					/* If remote end says no, */
			alert(int_cmdline.printer, TRUE, _("Remote LPR/LPD queue \"%s\" denies correct receipt of control file."), int_cmdline.address);
		exit(EXIT_PRNERR);
		}
	} /* end of do_control_file() */

/*
** Send a data file in the manner described in RFC 1179.
*/
static void do_data_file(int sockfd, const char *local_nodename, const char *address_queue, int lpr_queueid, int temp_file_fd, int temp_file_length)
	{
	char command[80];
	int result;

	/* Tell the server we want to send the data file. */
	snprintf(command, sizeof(command), "\003%d "DF_TEMPLATE"\n", temp_file_length, lpr_queueid, local_nodename);
	if(lpr_send_cmd(sockfd, command, strlen(command)) == -1)
		exit(EXIT_PRNERR);

	/* Make sure the response is favourable. */
	if((result = lpr_response(sockfd, TIMEOUT_HANDSHAKE)))
		{
		if(result == -1)		/* elaborate on error message */
			alert(int_cmdline.printer, FALSE, _("(Communication failure while negotiating to send data file.)"));
		else
			alert(int_cmdline.printer, TRUE, _("Remote LPR/LPD queue \"%s\" does not have room for data file."), int_cmdline.address);
		exit(EXIT_PRNERR);
		}

	/* Send the data file */
	if(lpr_send_data_file(temp_file_fd, sockfd) == -1)
		exit(EXIT_PRNERR);

	/* Send the zero byte */
	command[0] = '\0';
	if(lpr_send_cmd(sockfd, command, 1) == -1)
		exit(EXIT_PRNERR);

	/* Make sure the server responds with a zero byte. */
	if((result = lpr_response(sockfd, TIMEOUT_PRINT)))
		{
		if(result == -1)		/* elaborate on read error message */
			alert(int_cmdline.printer, FALSE, _("(Communication failure while sending data file.)"));
		else
			alert(int_cmdline.printer, TRUE, _("Remote LPR/LPD queue \"%s\" denies correct receipt of data file."), int_cmdline.address);
		exit(EXIT_PRNERR);
		}

	} /* end of do_data_file() */

/*
** This function sends the data file in chunks.  It reads blocks from
** stdin and sends each as the data file.  It just keeps repeating
** the same data file name.  This will almost certainly not work
** if the receiving end is a spooler, but it seems to work on HP
** JetDirect cards.  This is probably because they don't interpret
** the protocol on a very deep level.
*/
static void do_data_file_chunked(int sockfd, const char *local_nodename, const char *address_queue, int lpr_queueid, int chunk_size)
	{
	char command[80];
	char *buffer;
	int receive_count;
	int transmit_count;
	int result;
	int chunk = 0;

	DODEBUG(("sending data file in %d byte chunks", chunk_size));

	buffer = (char*)gu_alloc(chunk_size, sizeof(char));

	/* This loops until end of input causes us to send an undersize chunk. */
	do	{
		chunk++;
		DODEBUG(("trying to read chunk %d", chunk + 1));

		/* Read a chunk */
		for(receive_count=0; receive_count < chunk_size; receive_count += result)
			{
			if((result = read(0, buffer + receive_count, chunk_size - receive_count)) < 0)
				{
				alert(int_cmdline.printer, TRUE, "read() on stdin failed, errno=%d (%s)", errno, gu_strerror(errno));
				exit(EXIT_PRNERR);
				}
			DODEBUG(("read %d bytes", result));
			if(result == 0)
				break;
			}

		DODEBUG(("chunk %d is %d bytes long", chunk, receive_count));
		if(receive_count == 0) break;

		/* Tell the server we want to send the data file. */
		snprintf(command, sizeof(command), "\003%d "DF_TEMPLATE"\n", receive_count, lpr_queueid, local_nodename);
		if(lpr_send_cmd(sockfd, command, strlen(command)) == -1)
			exit(EXIT_PRNERR);

		/* Make sure the response is favourable. */
		if((result = lpr_response(sockfd, TIMEOUT_HANDSHAKE)))
			{
			if(result == -1)		/* elaborate on error message */
				alert(int_cmdline.printer, FALSE, _("(Communication failure while negotiating to send data file.)"));
			else
				alert(int_cmdline.printer, TRUE, _("Remote LPR/LPD queue \"%s\" does not have room for data file."), int_cmdline.address);
			exit(EXIT_PRNERR);
			}

		/* Send this chunk. */
		for(transmit_count=0; transmit_count < receive_count; transmit_count += result)
			{
			DODEBUG(("writing %d bytes", receive_count - transmit_count));
			if((result = write(sockfd, buffer + transmit_count, receive_count - transmit_count)) < 1)
				{
				if(result == -1)
					alert(int_cmdline.printer, TRUE, "write() to lpd server failed, errno=%d (%s)", errno, gu_strerror(errno));
				else
					alert(int_cmdline.printer, TRUE, "write() to lpd server returned %d", result);
				exit(EXIT_PRNERR);
				}
			DODEBUG(("write() accepted %d bytes", result));
			}

		/* Send the zero byte */
		DODEBUG(("sending zero byte"));
		command[0] = '\0';
		if(lpr_send_cmd(sockfd, command, 1) == -1)
			exit(EXIT_PRNERR);

		/* Make sure the server responds with a zero byte. */
		DODEBUG(("waiting for reply"));
		if((result = lpr_response(sockfd, TIMEOUT_PRINT)))
			{
			if(result == -1)		/* elaborate on read error message */
				alert(int_cmdline.printer, FALSE, _("(Communication failure while sending data file.)"));
			else
				alert(int_cmdline.printer, TRUE, _("Remote LPR/LPD queue \"%s\" denies correct receipt of data file."), int_cmdline.address);
			exit(EXIT_PRNERR);
			}
		DODEBUG(("got reply"));
		} while(receive_count == chunk_size);

	gu_free(buffer);

	DODEBUG(("done sending chunked data file in %d chunk(s)", chunk));
	} /* end of do_data_file_chunked() */

/*
** In this function we say the file is much larger than it is and then
** simply close the connection once we are done sending it.
*/
static void do_data_file_exaggerated(int sockfd, const char *local_nodename, const char *address_queue, int lpr_queueid, int exaggerated_size)
	{
	char command[80];
	char *buffer;
	int receive_count;
	int transmit_count;
	int result;
	int chunk_size = 16384;
	int chunk = 0;
	int actual_size = 0;

	DODEBUG(("sending data file with claimed size of %d bytes", exaggerated_size));

	/* Tell the server we want to send the data file. */
	snprintf(command, sizeof(command), "\003%d "DF_TEMPLATE"\n", exaggerated_size, lpr_queueid, local_nodename);
	if(lpr_send_cmd(sockfd, command, strlen(command)) == -1)
		exit(EXIT_PRNERR);

	/* Make sure the response is favourable. */
	if((result = lpr_response(sockfd, TIMEOUT_HANDSHAKE)))
		{
		if(result == -1)		/* elaborate on error message */
			alert(int_cmdline.printer, FALSE, _("(Communication failure while negotiating to send data file.)"));
		else
			alert(int_cmdline.printer, TRUE, _("Remote LPR/LPD queue \"%s\" does not have room for data file."), int_cmdline.address);
		exit(EXIT_PRNERR);
		}

	buffer = (char*)gu_alloc(chunk_size, sizeof(char));

	/* This loops until end of input causes us to send an undersize chunk. */
	do	{
		chunk++;
		DODEBUG(("trying to read chunk %d", chunk + 1));

		/* Read a chunk */
		for(receive_count=0; receive_count < chunk_size; receive_count += result)
			{
			if((result = read(0, buffer + receive_count, chunk_size - receive_count)) < 0)
				{
				alert(int_cmdline.printer, TRUE, "read() from stdin failed, errno=%d (%s)", errno, gu_strerror(errno));
				exit(EXIT_PRNERR);
				}
			DODEBUG(("read %d bytes", result));
			if(result == 0)
				break;
			}

		DODEBUG(("chunk %d is %d bytes long", chunk, receive_count));
		if(receive_count == 0) break;

		/* We can't go over the exaggerated size! */
		actual_size += receive_count;
		if(actual_size > exaggerated_size)
			{
			alert(int_cmdline.printer, TRUE, "Exaggerated size exceeded!");
			exit(EXIT_JOBERR);
			}

		/* Send this chunk. */
		for(transmit_count=0; transmit_count < receive_count; transmit_count += result)
			{
			DODEBUG(("writing %d bytes", receive_count - transmit_count));
			if((result = write(sockfd, buffer + transmit_count, receive_count - transmit_count)) < 1)
				{
				if(result == -1)
					alert(int_cmdline.printer, TRUE, "write() to lpd server failed, errno=%d (%s)", errno, gu_strerror(errno));
				else
					alert(int_cmdline.printer, TRUE, "write() to lpd server returned %d", result);
				exit(EXIT_PRNERR);
				}
			DODEBUG(("write() accepted %d bytes", result));
			}
		} while(receive_count == chunk_size);

	gu_free(buffer);

	DODEBUG(("done sending exaggerated size data"));
	} /* end of do_data_file_exaggerated() */

/*
** Main function.
** This is called int_main() instead of main() because main() from
** libppr.a wraps this in an exception handler.
*/
int int_main(int argc, char *argv[])
	{
	/* The address broken up into its components: */
	const char *address_queue;			/* before "@" */
	const char *address_host;			/* after "@" */

	/* Values from option keywords: */
	struct OPTIONS options;

	int sockfd;							/* open connection to LPD */
	const char *local_nodename;			/* name of local node */
	int lpr_queueid;					/* remote queue id */
	int temp_file_fd = -1;				/* file descriptor for temp_file_name[] */
	int temp_file_length;				/* length of data file (we must tell recipient) */
	int result;							/* general use function result code */

	/* Shed as much root authority as we can. */
	seteuid(getuid());

	/* Initialize internation messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_INTERFACES, LOCALEDIR);
	textdomain(PACKAGE_INTERFACES);
	#endif

	/* Disable SIGPIPE.  We will catch the error on write(). */
	signal_interupting(SIGPIPE, SIG_IGN);

	/*
	** Copy command line and substitute env variables (used by the gs* interfaces)
	** into the structure called "int_cmdline".
	*/
	int_cmdline_set(argc, argv);

	/* Seperate the host and printer parts of the address. */
	{
	char *p = gu_strdup(int_cmdline.address);
	if(!(address_queue = gu_strsep(&p, "@")) || !(address_host = gu_strsep(&p, "")))
		{
		alert(int_cmdline.printer, TRUE, _("Printer address \"%s\" is invalid, the correct syntax\n"
										"is \"printer@host\"."), int_cmdline.address);
		exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
		}
	}

	/*
	** Parse the options string, searching for name=value pairs.
	*/
	options.banner = FALSE;
	options.lpr_typecode = 'f';
	options.chunk_size = 0;
	options.exaggerated_size = 0;
	options.temp_first = FALSE;
	options.snmp_community = NULL;
	{
	struct OPTIONS_STATE o;
	char name[32];
	char value[32];
	int retval;

	options_start(int_cmdline.options, &o);
	while((retval = options_get_one(&o, name, sizeof(name), value, sizeof(value))) > 0)
		{
		DODEBUG(("option: name=\"%s\", value=\"%s\"", name, value));

		if(strcmp(name, "banner") == 0)
			{
			if(gu_torf_setBOOL(&(options.banner),value) == -1)
				{
				o.error = N_("value must be \"true\" or \"false\"");
				retval = -1;
				break;
				}
			}
		else if(strcmp(name, "lpr_typecode") == 0)
			{
			if(value[0] == 'o' || value[0] == 'f')
				options.lpr_typecode = value[0];
			else
				{
				o.error = N_("value must be \"o\" or \"f\"");
				retval = -1;
				break;
				}
			}
		else if(strcmp(name, "chunk_size") == 0)
			{
			if((options.chunk_size = atoi(value)) < 1)
				{
				o.error = N_("value must be a positive integer");
				retval = -1;
				break;
				}
			}
		else if(strcmp(name, "exaggerated_size") == 0)
			{
			if((options.exaggerated_size = atoi(value)) < 1)
				{
				o.error = N_("value must be a positive integer");
				retval = -1;
				break;
				}
			}
		else if(strcmp(name, "temp_first") == 0)
			{
			if(gu_torf_setBOOL(&(options.temp_first),value) == -1)
				{
				o.error = N_("value must be \"true\" or \"false\"");
				retval = -1;
				break;
				}
			}
		else if(strcmp(name, "snmp_community") == 0)
			{
			if(options.snmp_community)
				gu_free(options.snmp_community);
			options.snmp_community = gu_strdup(value);
			}
		else
			{
			o.error = N_("unrecognized keyword");
			o.index = o.index_of_name;
			retval = -1;
			break;
			}
		}

	/* See if final call to options_get_one() detected an error: */
	if(retval == -1)
		{
		alert(int_cmdline.printer, TRUE, _("Option parsing error:  %s"), gettext(o.error));
		alert(int_cmdline.printer, FALSE, "%s", o.options);
		alert(int_cmdline.printer, FALSE, "%*s^ %s", o.index, "", _("right here"));
		exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
		}
	}

	/* Check for --probe. */
	if(int_cmdline.probe)
		{
		struct sockaddr_in printer_address;
		gu_write_string(1, "%%[ PPR address lookup ]%%\n");
		int_tcp_parse_address(address_host, 0 /* port, doesn't matter*/, &printer_address);
		exit(int_tcp_probe(&printer_address, options.snmp_community));
		}

	/* Check for unsuitable job break methods. */
	if(int_cmdline.jobbreak == JOBBREAK_SIGNAL || int_cmdline.jobbreak == JOBBREAK_SIGNAL_PJL)
		{
		alert(int_cmdline.printer, TRUE,
				_("The jobbreak methods \"signal\" and \"signal/pjl\" are not compatible with\n"
				"the PPR interface program \"%s\"."), int_cmdline.int_basename);
		exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
		}

	/* Check for an unsuitable feedback settting. */
	if(int_cmdline.feedback)
		{
		alert(int_cmdline.printer, TRUE,
			_("The PPR interface program \"%s\" is incapable of sending feedback."),
			int_cmdline.int_basename
			);
		if(strcmp(int_cmdline.printer, "-") != 0)	/* incorrect if not configured printer */
			{
			alert(int_cmdline.printer, FALSE,
				_("Use the command \"ppad feedback %s false\" to correct this problem."),
				int_cmdline.int_basename,
				int_cmdline.printer
				);
			}
		exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
		}

	/*
	** Some printers may timeout while we are copying the temporary file
	** if we do it after connecting.  Therefor, we have the option
	** of doing it now on speculation.
	*/
	if(options.temp_first)
		{
		if((temp_file_fd = uprint_file_stdin(&temp_file_length)) == -1)
			exit(EXIT_PRNERR);
		}

	/* Tell pprdrv we will tell it when we have connected. */
	gu_write_string(1, "%%[ PPR connecting ]%%\n");

	/* Connect to the remote system: */
	if((sockfd = lpr_make_connection(address_host)) < 0)
		exit(-sockfd);

	/* Tell pprdrv that the connexion has gone through. */
	gu_write_string(1, "%%[ PPR connected ]%%\n");

	/* Say we want to send a job: */
	{
	char command[80];
	snprintf(command, sizeof(command), "\002%s\n", address_queue);
	if(lpr_send_cmd(sockfd, command, strlen(command)) == -1)
		exit(EXIT_PRNERR);
	}

	/* Check if the response if favorable: */
	if((result = lpr_response(sockfd, TIMEOUT_HANDSHAKE)))
		{
		if(result == -1)		/* elaborate on error message */
			{
			alert(int_cmdline.printer, FALSE, _("(Communication failure while negotiating to send job.)"));
			exit(EXIT_PRNERR);
			}
		else					/* non-zero byte returned */
			{
			alert(int_cmdline.printer, TRUE, _("Remote LPR/LPD system \"%s\" refuses to accept job for \"%s\"."), address_host, address_queue);
			exit(EXIT_PRNERR_NORETRY_ACCESS_DENIED);
			}
		}

	/* Get our nodename because we need it for the queue file. */
	if((local_nodename = lpr_nodename()) == (char*)NULL)
		exit(EXIT_PRNERR_NORETRY);

	/* Generate a queue id. */
	if((lpr_queueid = lpr_nextid()) == -1)
		exit(EXIT_PRNERR_NORETRY);

	/* Build the control file in memory and send it. */
	do_control_file(sockfd, local_nodename, address_queue, lpr_queueid, &options);

	/*
	** There are currently three ways we can send the data file.
	*/
	if(options.chunk_size > 0)
		{
		do_data_file_chunked(sockfd, local_nodename, address_queue, lpr_queueid, options.chunk_size);
		}
	else if(options.exaggerated_size)
		{
		do_data_file_exaggerated(sockfd, local_nodename, address_queue, lpr_queueid, options.exaggerated_size);
		}
	else
		{
		if(!options.temp_first)
			{
			if((temp_file_fd = uprint_file_stdin(&temp_file_length)) == -1)
				exit(EXIT_PRNERR);
			}
		do_data_file(sockfd, local_nodename, address_queue, lpr_queueid, temp_file_fd, temp_file_length);
		}

	/* Close the connection to the print server. */
	close(sockfd);

	/* If we used a temporary file, close it and thereby delete it. */
	if(temp_file_fd != -1) close(temp_file_fd);

	/* Tell pprdrv that we did our job correctly. */
	exit(EXIT_PRINTED);
	} /* end of main() */

/* end of file */

