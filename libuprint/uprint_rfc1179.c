/*
** mouse:~ppr/src/libuprint/uprint_rfc1179.c
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
** Last modified 19 February 2003.
*/

#include "before_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "uprint.h"

/*
** TIMEOUT_HANDSHAKE is the time to wait on operations that
** should be completed immediately, such as stating the desired
** queue name.  TIMEOUT_PRINT is the amount of time to wait for
** a response to the zero byte sent at the end of a data file.
** TIMEOUT_PRINT can be set separately because some printer
** Ethernet boards don't respond right away.
*/
#define TIMEOUT_HANDSHAKE 30
#define TIMEOUT_PRINT 30

void uprint_error_callback(const char *format, ...)
    {
    va_list va;
    fprintf(stderr, "uprint_errno: %d\n", uprint_errno);
    fprintf(stderr, "uprint_error_callback: ");
    va_start(va, format);
    vfprintf(stderr, format, va);
    fputc('\n', stderr);
    va_end(va);
    } /* end of uprint_error_callback() */

static int do_print(int sockfd, const char server_node[], char *argv[])
    {
    const char function[] = "do_lpr";
    char command[64];
    int code;
    const char *local_nodename = argv[0];
    int lpr_queueid = atoi(argv[1]);
    const char *printer = argv[2];
    const char *control_file = argv[3];
    char **files_list = &argv[4];
    const char *filename;
    int x;

    /* Say we want to send a job: */
    snprintf(command, sizeof(command), "\002%s\n", printer);
    if(uprint_lpr_send_cmd(sockfd, command, strlen(command)) == -1)
	{
	return -1;
	}

    /* Check if the response if favorable: */
    if((code = uprint_lpr_response(sockfd, TIMEOUT_HANDSHAKE)) == -1)
	{
	uprint_error_callback(_("(Connection lost while negotiating to send job.)"));
	return -1;
	}
    else if(code)
	{
	uprint_errno = UPE_DENIED;
	uprint_error_callback(_("Remote LPR/LPD system \"%s\" refuses to accept job for \"%s\" (%d)."), server_node, printer, code);
	return -1;
	}

    /* Tell the other end that we want to send the control file: */
    snprintf(command, sizeof(command), "\002%d cfA%03d%s\n", (int)strlen(control_file), lpr_queueid, local_nodename);
    if(uprint_lpr_send_cmd(sockfd, command, strlen(command)) == -1)
	{
	return -1;
	}

    /* Check if response is favourable. */
    if((code = uprint_lpr_response(sockfd, TIMEOUT_HANDSHAKE)) == -1)
	{
	uprint_error_callback(_("(Connection lost while negotiating to send control file failed.)"));
	return -1;
	}
    else if(code)
    	{
	uprint_errno = UPE_TEMPFAIL;
    	uprint_error_callback(_("Remote LPR/LPD system does not have room for control file."));
    	return -1;
    	}

    /* Send the control file. */
    if(uprint_lpr_send_cmd(sockfd, control_file, strlen(control_file)) == -1)
    	return -1;

    /* Send the zero byte */
    command[0] = '\0';
    if(uprint_lpr_send_cmd(sockfd, command, 1) == -1)
    	return -1;

    /* Check if response if favourable. */
    if((code = uprint_lpr_response(sockfd, TIMEOUT_HANDSHAKE)) == -1)
	{
	uprint_error_callback(_("(Connection lost while transmitting control file.)"));
	return -1;
	}
    else if(code)
    	{
	uprint_errno = UPE_TEMPFAIL;
    	uprint_error_callback(_("Remote LPR/LPD system \"%s\" denies correct receipt of control file."), server_node);
	uprint_lpr_send_cmd(sockfd, "\1\n", 2);		/* cancel the job */
    	return -1;
    	}

    for(x = 0; (filename = files_list[x]) != (const char *)NULL; x++)
	{
	int pffd = -1;	/* file being printed */
	int df_length;	/* length of file being printed */

	/* STDIN */
	if(strcmp(filename, "-") == 0)
	    {
	    /* copy stdin to a temporary file */
	    if((pffd = uprint_file_stdin(&df_length)) == -1)
		{
		/* Failed, cancel the job: */
		uprint_lpr_send_cmd(sockfd, "\1\n", 2);
		return -1;
		}
	    }

	/* Disk file */
	else
	    {
	    struct stat statbuf;

	    /* Try */
	    uprint_errno = UPE_NONE;
	    do  {
		/* Try to open the file to be printed: */
		if((pffd = open(filename, O_RDONLY)) == -1)
		    {
		    uprint_errno = UPE_NOFILE;
		    uprint_error_callback(_("Can't open \"%s\", errno=%d (%s)."), filename, errno, gu_strerror(errno));
		    break;
		    }

		/* Use fstat() to determine the file's length: */
		if(fstat(pffd, &statbuf) == -1)
		    {
		    uprint_errno = UPE_INTERNAL;
		    uprint_error_callback("%s(): fstat() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
		    break;
		    }
	    	df_length = (int)statbuf.st_size;
	        } while(FALSE);

	    /* Catch */
	    if(uprint_errno != UPE_NONE)
	    	{
		/* Cancel the job: */
		uprint_lpr_send_cmd(sockfd, "\1\n", 2);
		return -1;
	    	}
	    } /* end of if disk file */

	/* Ask permission to send the data file: */
	snprintf(command, sizeof(command), "\003%d dfA%03d.%03d%s\n", df_length, lpr_queueid, x, local_nodename);
	if(uprint_lpr_send_cmd(sockfd, command, strlen(command)) == -1)
	    {
	    close(pffd);
	    return -1;
	    }

	/* Check if response is favourable: */
	if((code = uprint_lpr_response(sockfd, TIMEOUT_HANDSHAKE)) == -1)
	    {
	    uprint_error_callback(_("(Connection lost while negotiating to send data file.)"));
	    close(pffd);				/* couldn't read response */
	    return -1;
	    }
        else if(code)
	    {
	    uprint_error_callback(_("Remote LPR/LPD system \"%s\" does not have room for data file."), server_node);
	    uprint_errno = UPE_TEMPFAIL;
	    close(pffd);				/* close print file */
	    uprint_lpr_send_cmd(sockfd, "\1\n", 2);	/* cancel the job */
	    return -1;
	    }

	/* Send and close the data file and
	   catch any error in sending the data file. */
	{
	int ret = uprint_lpr_send_data_file(pffd, sockfd);
	close(pffd);
	if(ret == -1)
	    return -1;
	}

	/* Send the zero byte */
	command[0] = '\0';
	if(uprint_lpr_send_cmd(sockfd, command, 1) == -1)
	    return -1;

	/* Check if response if favourable. */
	if((code = uprint_lpr_response(sockfd, TIMEOUT_PRINT)) == -1)
	    {
	    uprint_error_callback(_("(Connection lost while sending data file.)"));
	    return -1;
	    }
	else if(code)
	    {
	    uprint_error_callback(_("Remote LPR/LPD system \"%s\" denies correct receipt of data file."), server_node);
	    uprint_errno = UPE_TEMPFAIL;
	    uprint_lpr_send_cmd(sockfd, "\1\n", 2);	/* cancel the job */
	    return -1;
	    }
	} /* end of for each file */

    return 0;
    }

static int do_command(int sockfd, char *argv[])
    {
    const char function[] = "do_command";
    int x;
    char temp[512];

    /* Transmit the command: */
    if(uprint_lpr_send_cmd(sockfd, argv[0], strlen(argv[0])) == -1)
	return -1;

    /* Copy from the socket to stdout until the connexion
       is closed by the server. */
    while((x = read(sockfd, temp, sizeof(temp))) > 0)
	{
	write(1, temp, x);
	}

    /* If there was an error, */
    if(x == -1)
	{
	uprint_errno = UPE_TEMPFAIL;
	uprint_error_callback("%s(): read() from remote system failed, errno=%d (%s)", function, errno, gu_strerror(errno));
	return -1;
	}

    return 0;
    }

int main(int argc, char *argv[])
    {
    int ret = 0;
    int sockfd;
    const char *subcommand, *server_node;

    /* Initialize international messages library. */
    #ifdef INTERNATIONAL
    setlocale(LC_MESSAGES, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);
    #endif

    if(argc < 4)
    	{
	uprint_error_callback("too few arguments");
	return 1;
    	}

    if(geteuid() != 0)
	{
	uprint_error_callback("warning: %s is not setuid root", argv[0]);
	}

    subcommand = argv[1];
    server_node = argv[2];
    
    if((sockfd = uprint_lpr_make_connection_with_failover(server_node)) == -1)
    	return 1;

    /* Drop setuid root privledges. */
    {
    uid_t real_uid = getuid();

    if(setreuid(real_uid, real_uid) == -1)
	{
	uprint_error_callback("setreuid(%ld, %ld) failed, errno=%d (%s)", (long)real_uid, (long)real_uid, errno, gu_strerror(errno));
    	return 1;
    	}

    /* Be paranoid.  If the real user isn't root, then the ability to setuid(0) should be gone. */
    if(real_uid != 0 && setuid(0) != -1)
	{
	uprint_error_callback("setuid(0) didn't fail");
    	return 1;
    	}
    }

    if(strcmp(subcommand, "print") == 0)
	{
	ret = do_print(sockfd, server_node, &argv[3]);
	}
    else if(strcmp(subcommand, "command") == 0)
	{
	ret = do_command(sockfd, &argv[3]);
	}
    else
	{
	uprint_error_callback("unknown subcommand: %s", subcommand);
	return 1;
	}

    close(sockfd);

    return (ret == -1) ? 1 : 0;
    } /* end of main() */

/* end of file */
