/*
** mouse:~ppr/src/pprd/pprd_mainsup.c
** Copyright 1995--2002, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 9 January 2002.
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

/*
** Handler for signals we don't expect to receive
** like SIGHUP and SIGPIPE.
*/
static void signal_ignore(int sig)
    {
    error("Received %d (%s)", sig, gu_strsignal(sig));
    }

/*
** This is the signal that init will send us
** at system shutdown time.
*/
static void sigterm_handler(int sig)
    {
    state_update("SHUTDOWN");
    fatal(0, "Received SIGTERM, exiting");
    }

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
** Install all of the signal handlers.
*/
void install_signal_handlers(void)
    {
    /* Signal handlers for silly stuff. */
    signal_restarting(SIGPIPE, signal_ignore);
    signal_restarting(SIGHUP, signal_ignore);
    signal_interupting(SIGTERM, sigterm_handler);

    /* Arrange for child termination to be noted. */
    signal_restarting(SIGCHLD, reapchild);

    /* Arrange for alarm clock to tick every TICK_INTERVAL seconds. */
    signal_restarting(SIGALRM, tick);
    } /* end of install_signal_handlers() */

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
** If we are started as root, this is called to
** make sure the spool directories are present
** and have the correct permissions.
*/
void root_create_directories(uid_t euid, gid_t egid)
    {
    /* Make our spool directory and set its permissions correctly. */
    mkdir(VAR_SPOOL_PPR, S_IRWXU);
    chown(VAR_SPOOL_PPR, euid, egid);
    chmod(VAR_SPOOL_PPR, UNIX_755);

    /*
    ** If /var/spool/ppr/queue or /var/spool/ppr/data exist,
    ** will fix their permissions.
    */
    chown(QUEUEDIR, euid, egid);
    chown(DATADIR, euid, egid);
    }

/*
** Create any directories which we expect to be
** able to create as the user "ppr".  This should
** be called after adjust_ids().
*/
void create_work_directories(void)
    {
    mkdir(QUEUEDIR, S_IRWXU);
    chmod(QUEUEDIR, S_IRWXU);
    mkdir(DATADIR, S_IRWXU);	/* This directory especially must */
    chmod(DATADIR, S_IRWXU);	/* be private. */
    mkdir(MOUNTEDDIR, S_IRWXU);
    chmod(MOUNTEDDIR, S_IRWXU);
    mkdir(ALERTDIR, S_IRWXU);
    chmod(ALERTDIR, S_IRWXU);
    mkdir(LOGDIR, UNIX_755);	/* Others should be able to read */
    chmod(LOGDIR, UNIX_755);	/* the log directory. */
    } /* end of ppr_create_directories() */

/*
** This rather complicated bit of code eventually makes
** sure all the user IDs are "ppr" and all the group
** IDs are "ppr".  This program should be setuid "ppr"
** and setgid "ppr".
**
** This code must not call fatal(), fatal() should not be
** used until we are sure the permissions are right,
** otherwise we might be unable to create the log file
** or could create a log file with the wrong ownership.
*/
void adjust_ids(void)
    {
    uid_t uid, euid, correct_euid;
    gid_t gid, egid, correct_egid;
    const char *myname = "pprd";

    /*
    ** Look up the correct uid and gid.
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

    correct_euid = pw->pw_uid;
    correct_egid = gp->gr_gid;

    if(pw->pw_gid != gp->gr_gid)
    	fprintf(stderr, _("%s: Warning: primary group for user \"%s\" is not \"%s\".\n"), myname, USER_PPR, GROUP_PPR);
    }

    /* Read all 4 IDs. */
    uid = getuid(); euid = geteuid(); gid = getgid(); egid = getegid();

#ifndef BROKEN_SETUID_BIT

    /* If setuid root or run by root and not setuid, */
    if(euid == 0 || euid != correct_euid)
	{
	fprintf(stderr, _("%s: Security problem: euid = %u\n"
		"(This program should be setuid \"%s\".)\n"), myname, (unsigned)euid, USER_PPR);
	exit(1);
	}

    /* If setgid root or run with group root and not setgid, */
    if(egid == 0 || egid != correct_egid)
	{
	fprintf(stderr, _("%s: Security problem: egid = %u\n"
		"(This program should be setgid \"%s\".)\n"), myname, (unsigned)egid, GROUP_PPR);
	exit(1);
	}

    /*
    ** If the real user id is initially root, it takes the
    ** opportunity to create /var/spool/ppr if it does not exist.
    */
    if(uid == 0)
	{
	/*
	** Set uid and euid to "root" for the next few operations.
	** The uid of root should allow us to create any directories
	** we want the the gid of root is for no good reason.
	*/
	setuid(0);
	setgid(0);

	/*
	** Create various spool directories and give them
	** the indicated ownership and group.
	*/
	root_create_directories(euid, egid);

	/*
	** Give up root privledges forever.  This sets all three user id's and
	** all three group id's.  This is only possible because we executed
	** setuid(0) and setgid(0) above.  (The man pages for various systems
	** seem to contradict one another on whether setgid(0) is necessary.)
	*/
	setgid(egid);
	setuid(euid);
	}

    /*
    ** This daemon was not started by root.  Make sure it was
    ** started by ppr.  (Actually, we make sure it was started
    ** by the same user as it suid's to.)
    */
    else
	{
	if(uid != euid)
	    {
	    fprintf(stderr, _("%s: Only \"%s\" or \"root\" may start %s.\n"), myname, USER_PPR, myname);
	    exit(1);
	    }

	if(gid != egid)
	    {
	    fprintf(stderr, _("%s: User \"%s\" may only start %s if group is \"%s\".\n"
	    	"(Change 4th field in /etc/passwd for user \"%s\" to \"%d\".)\n"), myname, USER_PPR, myname, GROUP_PPR, USER_PPR, (int)egid);
	    exit(1);
	    }
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
	{(char*)NULL, 0, FALSE}
	} ;

static void help(FILE *out)
    {
    fputs("Usage: pprd [--version] [--help] [--foreground]\n", out);
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

    while( (optchar=ppr_getopt(&getopt_state)) != -1 )
	{
	switch(optchar)
	    {
	    case 1000:		/* --version */
		puts(VERSION);
		puts(COPYRIGHT);
		puts(AUTHOR);
		exit_code = 0;
		break;
	    case 1001:		/* --help */
		help(stdout);
		exit_code = 0;
	        break;
	    case 1002:		/* --foreground */
		*option_foreground = TRUE;
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

