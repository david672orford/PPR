/*
** mouse:~ppr/src/interfaces/lpr.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell and Damian Ivereigh.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 10 May 2001.
*/

/*
** This interface sends the print job to another computer
** using the Berkeley LPR protocol.
*/

#include "before_system.h"
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
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "interface.h"
#include "libppr_int.h"
#include "uprint.h"

/* Possible enable debugging.  The debuging output goes to
   a file in /var/spool/ppr/logs. */
#if 0
#define DODEBUG(a) int_debug a
#else
#define DODEBUG(a)
#endif

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
    } ;

/*
** This function is called by libuprint when it wants to print
** an error message.
*/
void uprint_error_callback(const char *format, ...)
    {
    va_list va;
    alert(int_cmdline.printer, TRUE, "The PPR interface program \"%s\" failed for the following reason:", int_cmdline.int_basename);
    va_start(va, format);
    valert(int_cmdline.printer, FALSE, format, va);
    va_end(va);
    } /* end of uprint_error_callback() */

/*
** Send the control file.
*/
static void do_control_file(int sockfd, const char *local_nodename, const char *address_queue, int lpr_queueid, const struct OPTIONS *options)
    {
    char person[LPR_MAX_P+1];		/* name to use with P line in control file */
    char control_file[2048];		/* The actual control file contents. */
    char command[80];			/* staging area for command to LPD */
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
    	"H%s\n"				/* host */
    	"P%s\n"				/* person */
	"%s"				/* name for banner page */
    	"%c"DF_TEMPLATE"\n"		/* file to print */
    	"U"DF_TEMPLATE"\n"		/* file to unlink */
 	"N%s\n",			/* file name, to keep System V LP happy */
    		local_nodename,
		person,
    		options->banner ? "Lsomebody\n" : "",
		options->lpr_typecode, lpr_queueid, local_nodename,
    		lpr_queueid, local_nodename,
    		int_cmdline.jobname);

    DODEBUG(("control file: %d bytes: \"%s\"", strlen(control_file), control_file));

    /* Tell the server we want to send the control file. */
    snprintf(command, sizeof(command), "\002%d "CF_TEMPLATE"\n", (int)strlen(control_file), lpr_queueid, local_nodename);
    if(uprint_lpr_send_cmd(sockfd, command, strlen(command)) == -1)
    	int_exit(EXIT_PRNERR);

    /* Check if response is favourable. */
    if((result = uprint_lpr_response(sockfd, TIMEOUT_HANDSHAKE)))
    	{
	if(result == -1)	/* elaborate on error message */
	    alert(int_cmdline.printer, FALSE, _("(Communication failure while negotiating to send control file.)"));
	else			/* if remote end refuses, */
	    alert(int_cmdline.printer, TRUE, _("Remote LPR/LPD system does not have room for control file."));
    	int_exit(EXIT_PRNERR);
    	}

    /* Send the control file, including the terminating zero byte. */
    if(uprint_lpr_send_cmd(sockfd, control_file, strlen(control_file) + 1) == -1)
    	int_exit(EXIT_PRNERR);

    /* Check if response if favourable. */
    if((result = uprint_lpr_response(sockfd, TIMEOUT_HANDSHAKE)))
    	{
	if(result == -1)	/* elaborate on read error message */
	    alert(int_cmdline.printer, FALSE, _("(Communication failure while sending control file.)"));
	else			/* If remote end says no, */
	    alert(int_cmdline.printer, TRUE, _("Remote LPR/LPD queue \"%s\" denies correct receipt of control file."), int_cmdline.address);
    	int_exit(EXIT_PRNERR);
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
    if(uprint_lpr_send_cmd(sockfd, command, strlen(command)) == -1)
    	int_exit(EXIT_PRNERR);

    /* Make sure the response is favourable. */
    if((result = uprint_lpr_response(sockfd, TIMEOUT_HANDSHAKE)))
    	{
	if(result == -1)	/* elaborate on error message */
	    alert(int_cmdline.printer, FALSE, _("(Communication failure while negotiating to send data file.)"));
	else
	    alert(int_cmdline.printer, TRUE, _("Remote LPR/LPD queue \"%s\" does not have room for data file."), int_cmdline.address);
    	int_exit(EXIT_PRNERR);
    	}

    /* Send the data file */
    if(uprint_lpr_send_data_file(temp_file_fd, sockfd) == -1)
    	int_exit(EXIT_PRNERR);

    /* Send the zero byte */
    command[0] = '\0';
    if(uprint_lpr_send_cmd(sockfd, command, 1) == -1)
    	int_exit(EXIT_PRNERR);

    /* Make sure the server responds with a zero byte. */
    if((result = uprint_lpr_response(sockfd, TIMEOUT_PRINT)))
    	{
	if(result == -1)	/* elaborate on read error message */
            alert(int_cmdline.printer, FALSE, _("(Communication failure while sending data file.)"));
	else
	    alert(int_cmdline.printer, TRUE, _("Remote LPR/LPD queue \"%s\" denies correct receipt of data file."), int_cmdline.address);
    	int_exit(EXIT_PRNERR);
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
		int_exit(EXIT_PRNERR);
	    	}
	    DODEBUG(("read %d bytes", result));
	    if(result == 0)
	    	break;
	    }

	DODEBUG(("chunk %d is %d bytes long", chunk, receive_count));
	if(receive_count == 0) break;

        /* Tell the server we want to send the data file. */
        snprintf(command, sizeof(command), "\003%d "DF_TEMPLATE"\n", receive_count, lpr_queueid, local_nodename);
        if(uprint_lpr_send_cmd(sockfd, command, strlen(command)) == -1)
            int_exit(EXIT_PRNERR);

        /* Make sure the response is favourable. */
        if((result = uprint_lpr_response(sockfd, TIMEOUT_HANDSHAKE)))
            {
            if(result == -1)        /* elaborate on error message */
                alert(int_cmdline.printer, FALSE, _("(Communication failure while negotiating to send data file.)"));
            else
                alert(int_cmdline.printer, TRUE, _("Remote LPR/LPD queue \"%s\" does not have room for data file."), int_cmdline.address);
            int_exit(EXIT_PRNERR);
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
		int_exit(EXIT_PRNERR);
	    	}
	    DODEBUG(("write() accepted %d bytes", result));
            }

	/* Send the zero byte */
	DODEBUG(("sending zero byte"));
        command[0] = '\0';
        if(uprint_lpr_send_cmd(sockfd, command, 1) == -1)
            int_exit(EXIT_PRNERR);

        /* Make sure the server responds with a zero byte. */
	DODEBUG(("waiting for reply"));
        if((result = uprint_lpr_response(sockfd, TIMEOUT_PRINT)))
            {
            if(result == -1)        /* elaborate on read error message */
                alert(int_cmdline.printer, FALSE, _("(Communication failure while sending data file.)"));
            else
                alert(int_cmdline.printer, TRUE, _("Remote LPR/LPD queue \"%s\" denies correct receipt of data file."), int_cmdline.address);
            int_exit(EXIT_PRNERR);
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
    if(uprint_lpr_send_cmd(sockfd, command, strlen(command)) == -1)
        int_exit(EXIT_PRNERR);

    /* Make sure the response is favourable. */
    if((result = uprint_lpr_response(sockfd, TIMEOUT_HANDSHAKE)))
        {
        if(result == -1)        /* elaborate on error message */
            alert(int_cmdline.printer, FALSE, _("(Communication failure while negotiating to send data file.)"));
        else
            alert(int_cmdline.printer, TRUE, _("Remote LPR/LPD queue \"%s\" does not have room for data file."), int_cmdline.address);
        int_exit(EXIT_PRNERR);
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
		int_exit(EXIT_PRNERR);
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
	    int_exit(EXIT_JOBERR);
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
		int_exit(EXIT_PRNERR);
	    	}
	    DODEBUG(("write() accepted %d bytes", result));
            }
	} while(receive_count == chunk_size);

    gu_free(buffer);

    DODEBUG(("done sending exaggerated size data"));
    } /* end of do_data_file_exaggerated() */

/*
** Main function.
*/
int main(int argc, char *argv[])
    {
    /* The address broken up into its components: */
    const char *address_queue;		/* before "@" */
    const char *address_host;		/* after "@" */

    /* Values from option keywords: */
    struct OPTIONS options;

    int sockfd;				/* open connection to LPD */
    const char *local_nodename;		/* name of local node */
    int lpr_queueid;			/* remote queue id */
    int temp_file_fd = -1;		/* file descriptor for temp_file_name[] */
    int temp_file_length;		/* length of data file (we must tell recipient) */
    int result;				/* general use function result code */

    /* Shed as much root authority as we can. */
    seteuid(getuid());

    /* Initialize internation messages library. */
    #ifdef INTERNATIONAL
    setlocale(LC_MESSAGES, "");
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

    /* Check for unsuitable job break methods. */
    if(int_cmdline.jobbreak == JOBBREAK_SIGNAL || int_cmdline.jobbreak == JOBBREAK_SIGNAL_PJL)
    	{
    	alert(int_cmdline.printer, TRUE,
    		_("The jobbreak methods \"signal\" and \"signal/pjl\" are not compatible with\n"
    		"the PPR interface program \"%s\"."), int_cmdline.int_basename);
    	int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
    	}

    /* Check for an unsuitable feedback settting. */
    if(int_cmdline.feedback)
    	{
    	alert(int_cmdline.printer, TRUE,
    		_("The PPR interface program \"%s\" does not support bidirectional\n"
    		"communication.  Use the command \"ppad feedback %s false\" to\n"
    		"correct this problem."), int_cmdline.int_basename, int_cmdline.printer);
	int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
	}

    /* Seperate the host and printer parts of the address. */
    {
    char *p = gu_strdup(int_cmdline.address);
    if(!(address_queue = gu_strsep(&p, "@")) || !(address_host = gu_strsep(&p, "")))
	{
    	alert(int_cmdline.printer, TRUE, _("Printer address \"%s\" is invalid, the correct syntax\n"
    					"is \"printer@host\"."), int_cmdline.address);
	int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
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
	    int x;
            if((x = gu_torf(value)) == ANSWER_UNKNOWN)
                {
	    	o.error = N_("value must be \"true\" or \"false\"");
                retval = -1;
                break;
                }
	    options.banner = x ? TRUE : FALSE;
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
	    int x;
	    if((x = gu_torf(value)) == ANSWER_UNKNOWN)
		{
	    	o.error = N_("value must be \"true\" or \"false\"");
                retval = -1;
                break;
		}
	    options.temp_first = x ? TRUE : FALSE;
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
    	int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
    	}
    }

    /*
    ** Some printers may timeout while we are copying the temporary file
    ** if we do it after connecting.  Therefor, we have the option
    ** of doing it now on speculation.
    */
    if(options.temp_first)
        {
	if((temp_file_fd = uprint_file_stdin(&temp_file_length)) == -1)
            int_exit(EXIT_PRNERR);
        }

    /* Tell pprdrv we will tell it when we have connected. */
    gu_write_string(1, "%%[ PPR connecting ]%%\n");

    /* Connect to the remote system: */
    if((sockfd = uprint_lpr_make_connection(address_host)) == -1)
	{
	if(uprint_errno == UPE_TEMPFAIL)
	    int_exit(EXIT_PRNERR);
	if(uprint_errno == UPE_BADSYS)
	    int_exit(EXIT_PRNERR_NO_SUCH_ADDRESS);
	if(uprint_errno == UPE_BADARG)
	    int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
	int_exit(EXIT_PRNERR_NORETRY);
	}

    /* Tell pprdrv that the connexion has gone through. */
    gu_write_string(1, "%%[ PPR connected ]%%\n");

    /* Say we want to send a job: */
    {
    char command[80];
    snprintf(command, sizeof(command), "\002%s\n", address_queue);
    if(uprint_lpr_send_cmd(sockfd, command, strlen(command)) == -1)
    	int_exit(EXIT_PRNERR);
    }

    /* Check if the response if favorable: */
    if((result = uprint_lpr_response(sockfd, TIMEOUT_HANDSHAKE)))
	{
	if(result == -1)	/* elaborate on error message */
	    alert(int_cmdline.printer, FALSE, _("(Communication failure while negotiating to send job.)"));
	else			/* non-zero byte returned */
	    alert(int_cmdline.printer, TRUE, _("Remote LPR/LPD system \"%s\" refuses to accept job for \"%s\"."), address_host, address_queue);
	int_exit(EXIT_PRNERR_NORETRY_ACCESS_DENIED);
	}

    /* Get our nodename because we need it for the queue file. */
    if((local_nodename = uprint_lpr_nodename()) == (char*)NULL)
    	int_exit(EXIT_PRNERR_NORETRY);

    /* Generate a queue id. */
    if((lpr_queueid = uprint_lpr_nextid()) == -1)
    	int_exit(EXIT_PRNERR_NORETRY);

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
	    	int_exit(EXIT_PRNERR);
	    }
	do_data_file(sockfd, local_nodename, address_queue, lpr_queueid, temp_file_fd, temp_file_length);
	}

    /* Close the connection to the print server. */
    close(sockfd);

    /* If we used a temporary file, close it and thereby delete it. */
    if(temp_file_fd != -1) close(temp_file_fd);

    /* Tell pprdrv that we did our job correctly. */
    int_exit(EXIT_PRINTED);
    } /* end of main() */

/* end of file */

