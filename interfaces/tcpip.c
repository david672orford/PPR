/*
** mouse:~ppr/src/interfaces/tcpip.c
** Copyright 1995--2002, Trinity College Computing Center.
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
** Last revised 5 September 2002.
*/

/*
** This interface program uses raw TCP/IP to communicating with devices such 
** as HP Jetdirect cards or Extended Systems Pocket Print Servers.
**
** There are at least two variants of this protocol, known as SocketAPI and
** AppSocket.  These are described at:
**	<http://www.lprng.com/LPRng-HOWTO-Multipart/socketapi.htm>
**	<http://www.lprng.com/LPRng-HOWTO-Multipart/appsocket.htm>
**
** JetDirect cards appear to conform to SocketAPI.  The author of this code
** (David Chappell) doesn't have a printer that implements AppSocket, but
** from a reading of the above documents he believes that this the following
** options will set a queue to speak AppSocket instead of SocketAPI:
**
** $ ppad myprn jobbreak newinterface
** $ ppad myprn options appsocket_status_interval=15
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
#include "cexcept.h"

/* Change the zero below to a one and recompile to turn on debugging. */
#if 0
#define DEBUG 1
#define DODEBUG(a) int_debug a
#else
#undef DEBUG
#define DODEBUG(a)
#endif

/* We use exception handling in this program.  All exceptions are character pointers. */
define_exception_type(const char *);
static struct exception_context the_exception_context[1];

/*
** This structure is used to store the result of parsing the name=value pairs
** in the interface options parameter on our command line.
*/
struct OPTIONS
    {
    int connect_timeout;
    int sndbuf_size;
    int refused_retries;
    gu_boolean refused_engaged;
    char *snmp_community;
    int idle_status_interval;
    int snmp_status_interval;
    int appsocket_status_interval;
    int sleep;
    gu_boolean use_shutdown;
    };

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
** aborted if the parse fails or a DNS lookup fails.
*/
static void parse_address(const char address[], struct sockaddr_in *printer_addr)
    {
    char *ptr;
    struct hostent *hostinfo;

    /* Clear the printer address structure. */
    memset(printer_addr, 0, sizeof(printer_addr));

    /* Parse the address into host and port. */
    if(strpbrk(address, " \t") != (char*)NULL)
    	{
    	alert(int_cmdline.printer, TRUE, _("Spaces and tabs not allowed in a TCP/IP printer address."
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
    printer_addr->sin_port = htons(atoi(ptr+1));

    /* Terminate the host part so that the port number will be ignored for now. */
    *ptr = '\0';

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
    *ptr = ':';
    } /* end of parse_address() */

/*
** Make the connection to the printer.
** Return the file descriptor.
*/
static int open_connexion(const char address[], struct sockaddr_in *printer_addr, struct OPTIONS *options, void (*status_function)(void *), void *status_obj)
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
	alarm(options->connect_timeout);
	retval = connect(sockfd, (struct sockaddr*)printer_addr, sizeof(struct sockaddr_in));
	alarm(0);
	DODEBUG(("connect() returned %d", retval));

	/* If a timeout occured, */
	if(sigalrm_caught)
	    {
	    alert(int_cmdline.printer, TRUE, _("Printer \"%s\" is not responding.\n"
					    "(Aborted after connect() blocked for %d seconds.)"), address, options->connect_timeout);
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
    } /* end of open_connexion() */

/*
** Explain why reading from or writing to the TCP connnection to the printer failed.
*/
static void explain_error_in_context(int error_number)
    {
    switch(error_number)
    	{
	case EIO:
	    alert(int_cmdline.printer, TRUE, _("Connection to printer lost."));
	    break;
    	default:
	    alert(int_cmdline.printer, TRUE, _("TCP/IP communication failed, errno=%d (%s)."), error_number, gu_strerror(error_number));
	    break;
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
    int error_code;
    int n1, n2;
    unsigned int n3;

    if(gu_snmp_get(snmp_obj, &error_code,
		"1.3.6.1.2.1.25.3.2.1.5.1", GU_SNMP_INT, &n1,
    		"1.3.6.1.2.1.25.3.5.1.1.1", GU_SNMP_INT, &n2,
                "1.3.6.1.2.1.25.3.5.1.2.1", GU_SNMP_BIT, &n3,
    		NULL) < 0)
	{
	alert(int_cmdline.printer, TRUE, "gu_snmp_get() failed, error_code=%d", error_code);
	return;
    	}

    /* This will be picked up by pprdrv. */
    printf("%%%%[ PPR SNMP: %d %d %08x ]%%%%\n", n1, n2, n3);
    fflush(stdout);

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

static void *appsocket_status_open(unsigned int ip_address, int port, const char **error_str)
    {
    const char *e;
    struct sockaddr_in server_ip;
    struct sockaddr_in my_ip;
    #warning Expect spurious warnings on next line.
    int fd;
    struct appsocket *p;

    Try {
        memset(&server_ip, 0, sizeof(server_ip));
        server_ip.sin_family = AF_INET;
        memcpy(&server_ip.sin_addr, &ip_address, sizeof(ip_address));
        server_ip.sin_port = htons(port);

        memset(&my_ip, 0, sizeof(my_ip));
        my_ip.sin_family = AF_INET;
        my_ip.sin_addr.s_addr = htonl(INADDR_ANY);
        my_ip.sin_port = htons(0);

        if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
            Throw("socket() failed, errno=%d (%s)");

	Try {
            if(bind(fd, (struct sockaddr *)&my_ip, sizeof(my_ip)) < 0)
                Throw("bind() failed, errno=%d (%s)");

            if(connect(fd, (struct sockaddr *)&server_ip, sizeof(server_ip)) < 0)
                Throw("connect() failed, errno=%d (%s)");
	    }
	Catch(e)
	    {
	    int saved_errno = errno;
	    close(fd);
	    errno = saved_errno;
	    Throw(e);
	    }
	}
    Catch(e)
	{
	if(error_str)
	    *error_str = e;
	return NULL;
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
    const char *e;
    
    /* If we get "connection refused" on recv(), don't try again. */
    if(obj->refused)
    	return;

    /* Send and resent the request until we get a response or the retries
       are exhausted. */
    Try {
        int attempt;
	fd_set rfds;
	struct timeval timeout;
	int len;
	
        for(attempt=0; attempt < 5; attempt++)
            {
	    /* Send the request (an empty UDP packet). */
	    DODEBUG(("sending..."));
            if(send(obj->socket, "", 0, 0) < 0)
                Throw("send() failed, errno=%d (%s)");

	    /* Wait up to 1 second for a response. */
	    FD_ZERO(&rfds);
	    FD_SET(obj->socket, &rfds);
	    timeout.tv_sec = 1;
	    timeout.tv_usec = 0;
	    if(select(obj->socket + 1, &rfds, NULL, NULL, &timeout) < 0)
		Throw("select() failed, errno=%d (%s)");

	    /* If there was nothing to read, start next iteration
	       (which will result in a resend. */
	    if(!FD_ISSET(obj->socket, &rfds))
	    	continue;

	    /* Receive the packet. */
	    if((len = recv(obj->socket, obj->result, sizeof(obj->result), 0)) < 0)
		{
		if(errno == ECONNREFUSED)
		    obj->refused = TRUE;
		Throw("recv() failed, errno=%d (%s)");
		}
	    DODEBUG(("Got %d bytes", len));
	    printf("%%%%[ %.*s ]%%%%\n", len, obj->result);
	    return;
	    }

	/* If we reach here, there was no valid response. */
	Throw("timeout");
        }
    Catch(e)
	{
	char temp[80];
	gu_snprintf(temp, sizeof(temp), e, errno, gu_strerror(errno));
	printf("appsocket_status(): %s\n", temp);
	}
    }

static void appsocket_status_close(void *p)
    {
    struct appsocket *obj = (struct appsocket *)p;
    close(obj->socket);
    gu_free(obj);
    }

/*=========================================================================
** Tie it all together.
=========================================================================*/
int main(int argc, char *argv[])
    {
    struct OPTIONS options;
    int sockfd;
    struct sockaddr_in printer_address;

    options.refused_retries = 5;
    options.sndbuf_size = 0;			/* size for SO_SNDBUF, 0 means don't set it */
    options.sleep = 0;				/* time to sleep() after printing */
    options.connect_timeout = 20;		/* connexion timeout in seconds */
    options.idle_status_interval = 0;		/* frequency of ^T transmission */
    options.snmp_status_interval = 0;
    options.appsocket_status_interval = 0;
    options.refused_engaged = TRUE;
    options.snmp_community = NULL;
    options.use_shutdown = FALSE;

    /* Initialize internation messages library. */
    #ifdef INTERNATIONAL
    setlocale(LC_MESSAGES, "");
    bindtextdomain(PACKAGE_INTERFACES, LOCALEDIR);
    textdomain(PACKAGE_INTERFACES);
    #endif

    /* Process the command line and leave the results in
       a global structure called "int_cmdline". */
    int_cmdline_set(argc, argv);

    DODEBUG(("============================================================"));
    DODEBUG(("tcpip printer=\"%s\", address=\"%s\", options=\"%s\", jobbreak=%d, feedback=%d, codes=%d, jobname=\"%s\", routing=\"%s\", forline=\"%s\", barbarlang=\"%s\"",
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
    		"the PPR interface program \"%s\"."), int_cmdline.int_basename);
    	int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
    	}

    /* If feedback is on, and control-d handshaking is on, turn on the ^T stuff. */
    if(int_cmdline.feedback && int_cmdline.jobbreak == JOBBREAK_CONTROL_D)
	options.idle_status_interval = 15;

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
	** The delay after closing connection, before exiting.
	*/
	if(strcmp(name, "sleep") == 0)
	    {
	    if((options.sleep = atoi(value)) < 0)
	    	{
	    	o.error = N_("value must be 0 or a positive integer");
	    	retval = -1;
	    	break;
	    	}
	    }
	/*
	** Size for TCP/IP send buffer
	*/
	else if(strcmp(name, "sndbuf_size") == 0)
	    {
	    if((options.sndbuf_size = atoi(value)) < 1)
	    	{
		o.error = N_("value must be a positive integer");
		retval = -1;
		break;
	    	}
	    }
	/*
	** Connect timeout
	*/
	else if(strcmp(name, "connect_timeout") == 0)
	    {
	    if((options.connect_timeout = atoi(value)) < 1)
	    	{
		o.error = N_("value must be a positive integer");
		retval = -1;
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
	    	o.error = N_("value must be 0 or a positive integer");
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
		o.error = N_("value must be 0 or a positive integer");
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
		o.error = N_("value must be 0 or a positive integer");
		retval = -1;
		break;
	    	}
	    }
	/*
	** refused=engaged
	** refused=error
	*/
	else if(strcmp(name, "refused") == 0)
	    {
	    if(strcmp(value, "engaged") == 0)
	    	options.refused_engaged = TRUE;
	    else if(strcmp(value, "error") == 0)
	    	options.refused_engaged = FALSE;
	    else
	    	{
		o.error = N_("value must be \"engaged\" or \"error\"");
		retval = -1;
		break;
	    	}
	    }
	/*
	** refused_retries
	*/
	else if(strcmp(name, "refused_retries") == 0)
	    {
	    if((options.refused_retries = atoi(value)) < 0)
	    	{
		o.error = N_("value must be 0 or a positive integer");
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
	DODEBUG(("barbarlang=\"%s\", setting idle_status_interval to 0", int_cmdline.barbarlang));
    	}

    /* Describe the options in the debuging output. */
    DODEBUG(("sleep=%d, connect_timeout=%d, sndbuf_size=%d, idle_status_interval=%d",
	options.sleep,
    	options.connect_timeout,
    	options.sndbuf_size,
    	options.idle_status_interval));

    /* Parse the printer address and do a DNS lookup if necessary. */
    parse_address(int_cmdline.address, &printer_address);

    {
    void (*status_function)(void *) = NULL;
    void *status_obj = NULL;
    int status_interval = 0;
    int error_code;
    const char *error_str;
    
    if(options.snmp_status_interval > 0)
    	{
       	if(!(status_obj = gu_snmp_open(printer_address.sin_addr.s_addr, options.snmp_community, &error_code)))
	    {
	    alert(int_cmdline.printer, TRUE, "gu_snmp_open() failed, error_code=%d", error_code);
	    return EXIT_PRNERR;
	    }
	status_function = snmp_status;
	status_interval = options.snmp_status_interval;
	}
    else if(options.appsocket_status_interval > 0)
	{
	if(!(status_obj = appsocket_status_open(printer_address.sin_addr.s_addr, ntohs(printer_address.sin_port) + 1, &error_str)))
	    {
	    char temp[80];
	    gu_snprintf(temp, sizeof(temp), error_str, errno, gu_strerror(errno));
	    alert(int_cmdline.printer, TRUE, "appsocket_status_open() failed, %s", temp);
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
    sockfd = open_connexion(int_cmdline.address, &printer_address, &options, status_function, status_obj);
    gu_write_string(1, "%%[ PPR connected ]%%\n");

    /* Disable SIGPIPE.  We will catch the error on write(). */
    signal_interupting(SIGPIPE, SIG_IGN);

    /* Copy stdin to the printer. */
    int_copy_job(sockfd,				/* connection to printer */
	options.idle_status_interval,			/* how often to send control-T */
	explain_error_in_context,			/* error printing function */
	options.use_shutdown ? do_shutdown : NULL,	/* EOJ function */
	status_function, status_obj, status_interval	/* SNMP or AppSocket UDP status function */
	);

    /* Close the connection */
    close(sockfd);

    if(options.snmp_status_interval > 0)
	gu_snmp_close(status_obj);
    else if(options.appsocket_status_interval > 0)
    	appsocket_status_close(status_obj);
    }

    /*
    ** Pocket print servers have been known to reject a new
    ** connection for a few seconds after closing the previous one.
    ** If more than one job is in the queue at one time, this can result
    ** in every other print attempt producing a fault.  This
    ** problem is minor and can go unnoticed, but we will have
    ** an option to sleep for a specified number of seconds
    ** after closing the connection.
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
