/*
** mouse:~ppr/src/misc/ppr-xgrant.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 11 November 2001.
*/

/*
** This program should get setuid "ppr".  It reads the invoking user's
** .Xauthority file and the DISPLAY environment variable and grants
** PPR permission to connect X clients to that display.
*/

#include "before_system.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <pwd.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

#include "util_exits.h"

int main(int argc, char *argv[])
    {
    char *display;
    int fds[2];
    pid_t childpid1, childpid2;
    struct passwd *pass;
    int ret, wstat;
    char xauth_path[MAX_PPR_PATH];
    char ppr_xauthority_file[MAX_PPR_PATH];

    /* Initialize international messages library. */
    #ifdef INTERNATIONAL
    setlocale(LC_MESSAGES, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);
    #endif

   if(geteuid() != 0)
	{
	fprintf(stderr, _("%s: This program must be setuid root!\n"), argv[0]);
	return EXIT_INTERNAL;
	}

    /*
    ** Make sure the environment variable "DISPLAY" is defined.
    ** If it is not, xauth will not know what to export.
    */
    if((display = getenv("DISPLAY")) == (char*)NULL)
    	{
    	fprintf(stderr, _("%s: DISPLAY not set.\n"), argv[0]);
    	return EXIT_SYNTAX;
    	}

    /*
    ** We need to know ppr's uid so that we can run
    ** the second xauth program instance as ppr.
    */
    if((pass = getpwnam(USER_PPR)) == (struct passwd*)NULL)
    	{
    	fprintf(stderr, _("%s: Can't find user \"ppr\" in password database.\n"), argv[0]);
    	return EXIT_INTERNAL;
    	}

    /*
    ** We have to make sure xauth will be able to read the Xauthority file.
    ** If this program is invoked as the result of "su -", then xauth will
    ** keep trying to open an Xauthority file which it probably doesn't have
    ** permission to open.  After about 10 seconds it will timeout,
    ** but it is still anoying.
    */
    {
    char *p;
    struct stat statbuf;
    if((p = getenv("XAUTHORITY")))
	{
	if(stat(p, &statbuf) != -1)
	    {
	    if(statbuf.st_uid != getuid())
		{
                fprintf(stderr, _("%s: The file \"%s\" is not yours, doing nothing.\n"), argv[0], p);
		return EXIT_DENIED;
		}
	    }
	}
    }

    printf(_("Granting PPR permission to connect X clients to display \"%s\".\n"), display);

    ppr_fnamef(xauth_path, "%s/xauth", XWINBINDIR);
    ppr_fnamef(ppr_xauthority_file, "%s/Xauthority", RUNDIR);

    /* Create the pipe that will connect the to instances of xauth. */
    if(pipe(fds) < 0)
    	{
    	fprintf(stderr, "%s: pipe() failed, errno=%d (%s)\n", argv[0], errno, gu_strerror(errno) );
    	return EXIT_INTERNAL;
    	}

    /* Fork a process for the reading end of the pipe. */
    if((childpid2 = fork()) == -1)
    	{
    	fprintf(stderr, "%s: fork() failed, errno=%d (%s)\n", argv[0], errno, gu_strerror(errno) ) ;
    	return EXIT_INTERNAL;
    	}

    /*
    ** This is the first child process.  In this one we will run xauth to add to
    ** ppr's .Xauthority file.  We must specify the path to the .Xauthority
    ** file since the default (which is determined using the HOME environment
    ** variable) is not suitable.
    */
    if(childpid2 == 0)
    	{
	dup2(fds[0], 0);
	close(fds[1]);

	setuid(pass->pw_uid);	/* become ppr */

    	execl(xauth_path, "xauth", "-f", ppr_xauthority_file, "nmerge", "-", (char*)NULL);

    	fprintf(stderr, "%s: child: exec(\"%s\", ...) failed, errno=%d (%s)\n", argv[0], xauth_path, errno, gu_strerror(errno) );
	exit(242);
    	}

    /* Launch a file for the writing end of the pipe. */
    if((childpid1 = fork()) < 0)
    	{
    	fprintf(stderr, "%s: fork() failed, errno=%d (%s)\n", argv[0], errno, gu_strerror(errno) ) ;
    	return EXIT_INTERNAL;
    	}

    /*
    ** The second child process must run xauth too, but must
    ** have its stdout connected to the write end of
    ** the pipe.  It must also become the user so that
    ** the user's .Xauthority file may be read.
    */
    if(childpid1 == 0)
	{
	dup2(fds[1], 1);
	close(fds[0]);

	setuid(getuid());	/* become the user */

	execl(xauth_path, "xauth", "nextract", "-", display, (char*)NULL);

	fprintf(stderr, "%s: exec(\"%s\", ...) failed, errno=%d, (%s)\n", argv[0], xauth_path, errno, gu_strerror(errno) );
	exit(242);
	}

    /* If we don't close these, it will confuse the children. */
    close(fds[0]);
    close(fds[1]);

    /*
    ** Wait for the children to exit.
    */
    while((ret = wait(&wstat)) != -1 || errno != ECHILD)
    	{
	if( ret == -1 )
	    {
	    fprintf(stderr, "%s: wait() failed, errno=%d (%s)\n", argv[0], errno, gu_strerror(errno) );
	    return EXIT_INTERNAL;
	    }

	if( WIFEXITED(wstat) )
	    {
	    if( WEXITSTATUS(wstat) != 0 )
	    	{
	    	if(ret == childpid1)
	    	    fprintf(stderr, "%s: The user's xauth exited with code %d.\n", argv[0], WEXITSTATUS(wstat) );
		else if(ret == childpid2)
		    fprintf(stderr, "%s: PPR's xauth exit with code %d.\n", argv[0], WEXITSTATUS(wstat) );
		else
		    fprintf(stderr, "%s: Internal error, unknown process %ld exited.\n", argv[0], (long)ret);
	    	}
	    }
	else
	    {
	    if(WIFSIGNALED(wstat))
		fprintf(stderr, "%s: Child %ld was killed by %s.\n", argv[0], (long)ret, gu_strsignal(WTERMSIG(wstat)) );
	    if(WCOREDUMP(wstat))
	    	fprintf(stderr, "%s: Child %ld dumped core.\n", argv[0], (long)ret);
	    return EXIT_INTERNAL;
	    }
    	}

    return EXIT_OK;
    } /* end of main() */

/* end of file */

