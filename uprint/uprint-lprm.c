/*
** mouse:~ppr/src/uprint/uprint-lprm.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 14 February 2000.
*/

#include "before_system.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pwd.h>
#include <string.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

#include "uprint.h"

extern char *optarg;
extern int optind;

static const char *const myname = "uprint-lprm";

void uprint_error_callback(const char *format, ...)
    {
    va_list va;
    fprintf(stderr, "%s: ", myname);
    va_start(va, format);
    vfprintf(stderr, format, va);
    fputc('\n', stderr);
    va_end(va);
    } /* end of uprint_error_callback() */

static const char *option_list = "P:";

int main(int argc, char *argv[])
    {
    uid_t uid;
    struct passwd *pw;
    int c;
    char *user;
    const char *queue = (const char *)NULL;

    /* Initialize internation messages library. */
    #ifdef INTERNATIONAL
    setlocale(LC_MESSAGES, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);
    #endif

    /* Trap loops: */
    if(uprint_loop_check(myname) == -1)
    	return 1;

    /* Set euid to real id for more safety: */
    if(uprint_re_uid_setup(&uid, NULL) == -1)
	{
	if(uprint_errno == UPE_SETUID)
	    fprintf(stderr, _("%s: this program must be setuid root\n"), myname);
    	return 1;
    	}

    /*
    ** Look up the name of the user who invoked this program
    ** and save it in a variable for future reference.
    */
    if((pw = getpwuid(uid)) == (struct passwd *)NULL)
	{
	fprintf(stderr, _("%s: getpwuid(%ld) failed to find your account\n"), myname, (long int)uid);
	return 1;
	}
    if(pw->pw_name == (char*)NULL)
    	{
    	fprintf(stderr, "%s: strange getpwuid() error, pw_name is NULL\n", myname);
    	return 1;
    	}
    user = pw->pw_name;

    /*
    ** Parse the switches.  Mostly, we will call uprint
    ** member functions.
    */
    while((c = getopt(argc, argv, option_list)) != -1)
	{
	switch(c)
	    {
	    case 'P':		/* printer */
		queue = optarg;
	    	break;

	    default:
		/* fprintf(stderr, _("%s: Syntax error, unrecognized switch: -%c\n"), myname, c); */
		return 1;
	    }
	}

    /* If the print destination has not yet been
       determined, determine it now. */
    if(queue == (char*)NULL)
	if((queue = getenv("PRINTER")) == (char*)NULL)
	    queue = uprint_default_destinations_lp();

    /* Handle that funny stuff with an argument of "-": */
    if(argc > optind && strcmp(argv[optind], "-") == 0)
    	{
	/* If root, use special incantation for all jobs of all users: */
	if(uid == 0)
	    {
	    argv[optind] = (char*)NULL;
	    user = "-all";
	    }
	/* If not root, "-" is an alias for the user name: */
	else
	    {
	    argv[optind] = user;
	    }
    	}

    {
    int return_code;

    /* Delete the jobs: */
    if((return_code = uprint_lprm(uid, user, (const char *)NULL, queue, (const char **)&argv[optind], TRUE)) >= 0)
    	{
	/* Child ran, nothing more to do. */
	}
    /* Unclaimed: */
    else if(uprint_errno == UPE_UNDEST)
	{
	fprintf(stderr, _("%s: Print queue \"%s\" not found.\n"), myname, queue);
	/* This is the exit code that BSD lprm uses for unknown queue: */
	return_code = 1;
	}
    /* Other failure: */
    else
	{
	/* An arbitrary value intended to indicated a major failure: */
	return_code = 255;
	}

    return return_code;
    }
    } /* end of main() */

/* end of file */
