/*
** mouse:~ppr/src/pprd/pprd_mainsup.c
** Copyright 1995--2006, Trinity College Computing Center.
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
** Last modified 13 April 2006.
*/

#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "pprd.h"
#include "./pprd.auto_h"
#include "version.h"

/*
** This module contains functions that are called
** only once, from main().
*/

/*
** Create the FIFO for receiving commands from
** ppr, ppop, and ppad.
*/
int open_fifo(void)
	{
	const char function[] = "open_fifo";
	int rfd;

	DODEBUG_STARTUP(("%s()", function));

	unlink(FIFO_NAME);

	/*
	** This is the normal code.  It creates and opens a POSIX FIFO.
	*/
	{
	int wfd;

	if(mkfifo(FIFO_NAME, UNIX_660) < 0)
		fatal(0, "%s(): can't make FIFO, errno=%d (%s)", function, errno, gu_strerror(errno));

	/* Open the read end. */
	while((rfd = open(FIFO_NAME, O_RDONLY | O_NONBLOCK)) < 0)
		fatal(0, "%s(): can't open FIFO for read, errno=%d (%s)", function, errno, gu_strerror(errno));

	/* Open the write end which will prevent EOF on the read end. */
	if((wfd = open(FIFO_NAME, O_WRONLY)) < 0)
		fatal(0, "%s(): can't open FIFO for write, errno=%d (%s)", function, errno, gu_strerror(errno));

	/* Clear the non-block flag for rfd. */
	gu_nonblock(rfd, FALSE);

	/* Set the two FIFO descriptors to close on exec(). */
	gu_set_cloexec(rfd);
	gu_set_cloexec(wfd);
	}

	DODEBUG_STARTUP(("%s(): done", function));

	return rfd;
	} /* end of open_fifo() */

/*
 * Create the UNIX-domain socket for receiving commands from ipp.
 */
int create_unix_socket(void)
	{
	const char function[] = "create_unix_socket";
	int s;
	struct sockaddr_un addr; 

	if((s = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
		gu_Throw(_("%s(): %s() failed, errno=%d (%s)"), function, "socket", errno, strerror(errno));
	
	gu_strlcpy(addr.sun_path, UNIX_SOCKET_NAME, sizeof(addr.sun_path));
	addr.sun_family = AF_UNIX; 
	if(bind(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1)
		gu_Throw(_("%s(): %s() failed, errno=%d (%s)"), function, "bind", errno, strerror(errno));

	if(listen(s, 5) == -1)
		gu_Throw(_("%s(): %s() failed, errno=%d (%s)"), function, "listen", errno, strerror(errno));

	return s;
	} /* end of create_unix_socket() */

/*
** If there is an old log file, rename it.
*/
void rename_old_log_file(void)
	{
	char newname[MAX_PPR_PATH];
	int fh;

	if((fh = open(PPRD_LOGFILE, O_RDONLY)) >= 0)
		{
		close(fh);
		snprintf(newname, sizeof(newname), "%s.old", PPRD_LOGFILE);
		unlink(newname);
		rename(PPRD_LOGFILE,newname);
		}
	} /* end of rename_old_log_file() */

/*========================================================================
** The command line options.
========================================================================*/
static const char *option_description_string = "";

static const struct gu_getopt_opt option_words[] =
		{
		{"version", 1000, FALSE},
		{"help", 1001, FALSE},
		{"foreground", 1002, FALSE},
		{"stop", 1003, FALSE},
		{"debug", 1004, FALSE},
		{(char*)NULL, 0, FALSE}
		} ;

static void help(FILE *out)
	{
	fputs("Usage: pprd [--version] [--help] [--foreground] [--stop] [--debug]\n", out);
	}

static int pprd_stop(void)
	{
	FILE *f;
	int count;
	long int pid;

	if(!(f = fopen(PPRD_LOCKFILE, "r")))
		{
		fprintf(stderr, _("%s: not running\n"), myname);
		return 1;
		}

	count = fscanf(f, "%ld", &pid);

	fclose(f);

	if(count != 1)
		{
		fprintf(stderr, _("%s: failed to read PID from lock file"), myname);
		return 2;
		}

	printf(_("Sending SIGTERM to %s (PID=%ld).\n"), myname, pid);

	if(kill((pid_t)pid, SIGTERM) < 0)
		{
		fprintf(stderr, _("%s: kill(%ld, SIGTERM) failed, errno=%d (%s)\n"), myname, pid, errno, gu_strerror(errno));
		return 3;
		}

	{
	struct stat statbuf;
	printf(_("Waiting while %s shuts down..."), myname);
	while(stat(PPRD_LOCKFILE, &statbuf) == 0)
		{
		printf(".");
		fflush(stdout);
		sleep(1);
		}
	printf("\n");
	}

	printf(_("Shutdown complete.\n"));

	return 0;
	}

/*
** Parse the options:
*/
void parse_command_line(int argc, char *argv[], gu_boolean *option_foreground, gu_boolean *option_debug)
	{
	struct gu_getopt_state getopt_state;
	int optchar;
	int exit_code = -1;

	gu_getopt_init(&getopt_state, argc, argv, option_description_string, option_words);

	while((optchar = ppr_getopt(&getopt_state)) != -1)
		{
		switch(optchar)
			{
			case 1000:			/* --version */
				puts(VERSION);
				puts(COPYRIGHT);
				puts(AUTHOR);
				exit_code = 0;
				break;
			case 1001:			/* --help */
				help(stdout);
				exit_code = 0;
				break;
			case 1002:			/* --foreground */
				*option_foreground = TRUE;
				break;
			case 1003:			/* --stop */
				exit_code = pprd_stop();
				break;
			case 1004:			/* --debug */
				*option_foreground = TRUE;
				*option_debug = TRUE;
				break;
			case '?':
			case ':':
			case '!':
			case '-':
				help(stderr);
				exit_code = 1;
				break;
			default:
				fputs("Missing case in option switch()\n", stderr);
				exit_code = 1;
				break;
			}
		}
	if(exit_code != -1)
		exit(exit_code);
	} /* end of option parsing */

/* end of file */

