/*
** mouse:~ppr/src/pprd/pprd_mainsup.c
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
** Last modified 7 March 2003.
*/

#include "before_system.h"
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
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "pprd.h"
#include "./pprd.auto_h"
#include "version.h"

/*
** This module contains functions that are called
** only once, from main().
*/

static const char myname[] = "pprd";

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

#ifdef HAVE_MKFIFO
	/*
	** This is the normal code.  It creates and opens a POSIX FIFO.
	*/
	{
	int wfd;

	if(mkfifo(FIFO_NAME, S_IRUSR | S_IWUSR) < 0)
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

#else
	/*
	** This is the substitute code for broken systems.  It creates an ordinary
	** file which will grow without bounds.
	*/
	if((rfd = open(FIFO_NAME, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR)) == -1)
		fatal(0, "open_fifo(): can't open(), errno=%d (%s)", errno, gu_strerror(errno));
	gu_set_cloexec(rfd);
#endif

	DODEBUG_STARTUP(("%s(): done", function));

	return rfd;
	} /* end of open_fifo() */

/*
** Create the lock file which ensures only one pprd at a time.
*/
void create_lock_file(void)
	{
	int lockfilefd;
	char temp[10];
	if((lockfilefd = open(PPRD_LOCKFILE, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) < 0)
		fatal(100, "can't open \"%s\", errno=%d (%s)", PPRD_LOCKFILE, errno, gu_strerror(errno));
	if(gu_lock_exclusive(lockfilefd, FALSE))
		fatal(100, "pprd already running");
	snprintf(temp, sizeof(temp), "%ld\n", (long)getpid());
	write(lockfilefd, temp, strlen(temp));
	gu_set_cloexec(lockfilefd);
	lockfile_created = TRUE;
	} /* end of create_lock_file() */

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

/*
** This funtions makes sure that all of the user IDs are "ppr" and all
** the group IDs are "ppr".  For this to work, pprd must be setuid "ppr"
** and setgid "ppr".  (Though of course it will also work if run by
** with as ppr:ppr.)
**
** This code must not call fatal(), fatal() should not be
** used until we are sure the permissions are right,
** otherwise we might be unable to create the log file
** or could create a log file with the wrong ownership.
*/
void adjust_ids(void)
	{
	uid_t uid, euid, ppr_uid;
	gid_t gid, egid, ppr_gid;

	/*
	** Look up the correct uid and gid we should be running under.
	*/
	{
	struct passwd *pw;
	struct group *gp;

	if((pw = getpwnam(USER_PPR)) == NULL)
		{
		fprintf(stderr, _("%s: The user \"%s\" doesn't exist.\n"), myname, USER_PPR);
		exit(1);
		}

	if((gp = getgrnam(GROUP_PPR)) == NULL)
		{
		fprintf(stderr, _("%s: The group \"%s\" doesn't exist.\n"), myname, GROUP_PPR);
		exit(1);
		}

	if(pw->pw_gid != gp->gr_gid)
		fprintf(stderr, _("%s: Warning: primary group for user \"%s\" is not \"%s\".\n"), myname, USER_PPR, GROUP_PPR);

	ppr_uid = pw->pw_uid;
	ppr_gid = gp->gr_gid;
	}

	/*
	** Read all 4 IDs.
	*/
	uid = getuid();
	euid = geteuid();
	gid = getgid();
	egid = getegid();

	/*
	** All this is the start of a block that can be disabled for
	** MS-Windows 95 any anything else that doesn't have real users
	** and groups.
	*/
	#ifndef BROKEN_SETUID_BIT
	
	/*
	** Make sure the permissions and setuid/setgid are right.
	*/
	if(euid != ppr_uid)
		{
		fprintf(stderr, _("%s: Security problem: euid = %ld\n"
				"(This program should be setuid %ld (%s).)\n"), myname, (long)euid, (long)ppr_uid, USER_PPR);
		exit(1);
		}

	if(egid != ppr_gid)
		{
		fprintf(stderr, _("%s: Security problem: egid = %ld\n"
				"(This program should be setgid %ld (%s).)\n"), myname, (long)euid, (long)ppr_gid, GROUP_PPR);
		exit(1);
		}

	/*
	** Make sure the invoker is authorized.
	*/
	if(uid != ppr_uid && uid != 0)
		{
		fprintf(stderr, _("%s: Only \"%s\" or \"root\" may start %s.\n"), myname, USER_PPR, myname);
		exit(1);
		}

	/*
	** If we were run by root, initialize our auxiliary groups.
	*/
	#ifdef HAVE_INITGROUPS
	if(uid == 0)
		{
		seteuid(0);
		if(initgroups(USER_PPR, ppr_gid) == -1)
			{
			fprintf(stderr, _("%s: setgroups(\"%s\", %ld) failed, errno=%d (%s)\n"), myname, USER_PPR, (long)ppr_gid, errno, gu_strerror(errno));
			exit(1);
			}
		}
	#endif

	/*
	** Relinquish any root privledge we may have.  This is difficult because not all 
	** systems have precisely the same semantics with regard to when the saved IDs 
	** are set.
	*/

	/* MacOS 10.2 must not act as its manpage suggest since setuid(ppr_uid) doesn't work except for root. */
	seteuid(0);

	/* MacOS 10.2 manpage (which is probably the BSD manpage)suggests that this will 
		set the saved IDs. */
	if(setgid(ppr_gid) == -1)
		{
		fprintf(stderr, _("%s: setgid(%ld) failed, errno=%d (%s)\n"), myname, (long)ppr_gid, errno, gu_strerror(errno));
		exit(1);
		}
	if(setuid(ppr_uid) == -1)
		{
		fprintf(stderr, _("%s: setuid(%ld) failed, errno=%d (%s)\n"), myname, (long)ppr_uid, errno, gu_strerror(errno));
		exit(1);
		}

	/* Linux manpage suggests that this will set the saved IDs. */
	if(setreuid(ppr_uid, ppr_uid) == -1)
		{
		fprintf(stderr, _("%s: setreuid(%ld, %ld) failed, errno=%d (%s)\n"), myname, (long)ppr_uid, (long)ppr_uid, errno, gu_strerror(errno));
		exit(1);
		}
	if(setregid(ppr_gid, ppr_gid) == -1)
		{
		fprintf(stderr, _("%s: setregid(%ld, %ld) failed, errno=%d (%s)\n"), myname, (long)ppr_gid, (long)ppr_gid, errno, gu_strerror(errno));
		exit(1);
		}

	/* Make sure the system semantics haven't bitten us. */
	if(setuid(0) != -1)
		{
		fprintf(stderr, _("%s: setuid(0) did not fail!\n"), myname);
		exit(1);
		}

	#endif /* BROKEN_SETUID_BIT */
	} /* end of adjust_ids() */

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
		{(char*)NULL, 0, FALSE}
		} ;

static void help(FILE *out)
	{
	fputs("Usage: pprd [--version] [--help] [--foreground]\n", out);
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
void parse_command_line(int argc, char *argv[], int *option_foreground)
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

