/*
** mouse:~ppr/src/libuprint/lpr_connect.c
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
** Last modified 12 February 2003.
*/

#include "before_system.h"
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#ifndef INADDR_NONE
#define INADDR_NONE -1
#endif
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "interface.h"
#include "uprint.h"
#include "uprint_private.h"

static void sigalrm_handler(int sig)
    { }

/*
** Make the connection to the printer.
** Return the file descriptor.
*/
int uprint_lpr_make_connection(const char address[])
    {
    int sockfd;				/* handle of socket we will connect to LPR server */
    struct sockaddr_in printer_addr;	/* internet address of LPR server */
    struct hostent *hostinfo;		/* return value of gethostbyname() */
    int retval;

    /* Check for any obvious syntax error in the address */
    if(strpbrk(address, " \t"))
    	{
    	uprint_error_callback(_("Spaces and tabs are not allowed in LPR hostnames."));
	uprint_errno = UPE_BADARG;
    	return -1;
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
	    uprint_error_callback(_("Bad port specification in printer address."));
	    uprint_errno = UPE_BADARG;
	    return -1;
	    }

	*ptr = '\0';		/* truncate */
	}
    else			/* If address doesn't specify the port, */
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
	    uprint_error_callback(_("IP address lookup for \"%s\" failed."), address);
	    uprint_errno = UPE_BADSYS;
	    return -1;
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
	uprint_error_callback("socket() failed, errno=%d (%s).", errno, gu_strerror(errno));
	uprint_errno = UPE_INTERNAL;
	return -1;
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
	struct sockaddr_in sin;			/* address to bind client socket to */
	int ret;                		/* return code from bind() */

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);

	while(TRUE)
	    {
	    sin.sin_port = htons(lport);
	    if((ret = bind(sockfd, (struct sockaddr *)&sin, sizeof(sin))) != -1)
	        break;

	    if(errno == EADDRINUSE)	/* If there error is that this port in use, */
		{
		if(--lport >= (IPPORT_RESERVED / 2))
		    continue;
		lport = 0;		/* flag for later */
		}

	    break;
	    }

	/* Switch back before possibly calling uprint_error_callback(). */
	{
	int saved_errno = errno;
	seteuid(saved_euid);
	errno = saved_errno;
	}

	if(lport == 0)
	    {
            uprint_errno = UPE_INTERNAL;
            uprint_error_callback(_("No unused TCP ports available in reserved range."));
	    return -1;
	    }

	if(ret == -1)
	    {
	    uprint_errno = UPE_INTERNAL;
	    uprint_error_callback("bind() failed, port=%d, errno=%d (%s)", (int)lport, errno, gu_strerror(errno));
	    return -1;
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
		uprint_error_callback(_("Timeout while trying to connect to lpd server \"%s\"."), address);
		uprint_errno = UPE_TEMPFAIL;
		return -1;

	    case ECONNREFUSED:
		uprint_error_callback(_("Remote system \"%s\" has refused the connection."), address);
		uprint_errno = UPE_TEMPFAIL;
		return -1;

	    case EADDRINUSE:
		uprint_error_callback("connect() set errno to EADDRINUSE!\n"
					"(This indicates a bug in libuprint!)");
		uprint_errno = UPE_INTERNAL;
		return -1;

	    default:
		uprint_error_callback("connect() failed, errno=%d (%s)", errno, gu_strerror(errno));
		uprint_errno = UPE_TEMPFAIL;
		return -1;
	    }
	}

    /* Turn on the socket option which detects dead connections. */
    {
    int truevalue = 1;
    if( setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void*)&truevalue, sizeof(truevalue)) == -1 )
    	{
    	uprint_error_callback("setsockopt() failed to set SO_KEEPALIVE, errno=%d (%s)", errno, gu_strerror(errno));
	uprint_errno = UPE_INTERNAL;
	return -1;
    	}
    }

    return sockfd;
    } /* end of uprint_lpr_make_connection() */

/*
** Send a command to the lpd server.
*/
int uprint_lpr_send_cmd(int fd, const char text[], int length)
    {
    int result;

    if((result = write(fd, text, length)) == -1)
    	{
    	uprint_error_callback(X_("write() to lpd server failed, errno=%d (%s)"), errno, gu_strerror(errno));
	uprint_errno = UPE_TEMPFAIL;
    	return -1;
    	}

    if(result != length)
    	{
    	uprint_error_callback(X_("write() to lpd server wrote only %d of %d bytes"), result, length);
	uprint_errno = UPE_TEMPFAIL;
    	return -1;
    	}

    return 0;
    } /* end of uprint_lpr_send_cmd() */

/*
** Fetch a 1 byte result code from the RFC-1179 server and return it.
** If an error occurs, uprint_errno is set and -1 is returned.  This
** routine leaves the printing of error messages up to the caller.
**
** Note: in the RFC-1179 protocol, a result byte of 0 indicates sucess.
*/
int uprint_lpr_response(int sockfd, int timeout)
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
	    uprint_errno = UPE_INTERNAL;
	    uprint_error_callback(X_("select() on socket failed, errno=%d (%s)"), errno, gu_strerror(errno));
	    }
	else
	    {
	    uprint_errno = UPE_TEMPFAIL;
	    uprint_error_callback(_("Timeout while waiting for response from print server."));
	    }

	return -1;
	}

    if((retval = read(sockfd, &result, 1)) == -1)
    	{
	uprint_errno = UPE_INTERNAL;
	uprint_error_callback(X_("read() from lpd server failed, errno=%d (%s)"), errno, gu_strerror(errno));
    	return -1;
    	}

    /* If the read didn't return 1 byte, */
    if(retval != 1)
    	{
	uprint_errno = UPE_INTERNAL;
	uprint_error_callback(X_("read() from lpd server returned %d bytes, 1 byte result code expected"), retval);
    	return -1;
    	}

    return result;
    } /* end of uprint_lpr_response() */

/* end of file */
