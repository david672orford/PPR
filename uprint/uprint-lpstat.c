/*
** mouse:~ppr/src/uprint/uprint-lpstat.c
** Copyright 1995--2002, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 17 April 2002.
*/

#include "before_system.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pwd.h>
#include <errno.h>
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

/* This name will appear in certain error messages: */
static const char *const myname = "uprint-lpstat";

void uprint_error_callback(const char *format, ...)
    {
    va_list va;
    fprintf(stderr, "%s: ", myname);
    va_start(va, format);
    vfprintf(stderr, format, va);
    fputc('\n', stderr);
    va_end(va);
    } /* end of uprint_error_callback() */

/*
** Implementation of -a

$ real-lpstat -a x,y,z,dummy
x: unknown printer
y: unknown printer
z: unknown printer
dummy accepting requests since Apr 17 12:12 2002

*/
static void lpstat_a(const char list[])
    {
    FILE *f;
    if((f = fopen(UPRINTREMOTECONF, "r")) != (FILE*)NULL)
    	{
	char line[256];
	while(fgets(line, sizeof(line), f))
	    {
	    if(line[0] == '[')
	    	{
		line[strcspn(line, "]")] = '\0';
		printf("%s accepting requests since Jan 1 00:00 1970\n", line + 1);
	    	}
	    }
	fclose(f);
    	}
    }

/*
** This is a list of the options in the format required by getopt().
** We use the getopt() from the C library rather than PPR's because
** we want to preserve the local system semantics.  Is this a wise
** descision?  I don't know.
**
** Notice that lpstat breaks the POSIX parsing rule that an option
** can either require an argument or not, but can't have an optional
** argument.
**
** The options -a, -c, -f, -o, -p, -S, -u, and -v all take optional
** arguments.  We have to use special code to handle this.
**
** This needs more work!!!
*/
static const char *option_list = "acdDfloprRsStuv";

/*
** This macro handles the messy stuff.
*/
#define OPTIONAL(action)

/*
** And here we tie it all together.
*/
int main(int argc, char *argv[])
    {
    int c;
    uid_t uid;

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
    ** Parse the switches.  Mostly, we will call uprint
    ** member functions.
    */
    while((c = getopt(argc, argv, option_list)) != -1)
	{
	switch(c)
	    {
	    case 'a':		/* acceptance status, optional argument */
		if(optind < argc && argv[optind][0] != '-')
		    {
		    lpstat_a(argv[optind]);
		    optind++;
		    }
		else
		    {
		    lpstat_a(NULL);
		    }
		break;

	    case 'c':		/* list classes and members, optional argument */
		if(optind < argc && argv[optind][0] != '-')
		    {
		    optind++;
		    }
		break;

	    case 'd':		/* report system default destination */
		printf("system default destination: %s\n", uprint_default_destinations_lp());
		return 0;

	    case 'D':		/* include descriptions (-p) */
		break;

	    case 'f':		/* verify forms, optional argument */
		if(optind < argc && argv[optind][0] != '-')
		    {
		    optind++;
		    }
		break;

	    case 'l':		/* long format, no arguments */
		break;

	    case 'o': 		/* output request status, optional argument */
		if(optind < argc && argv[optind][0] != '-')
		    {
		    optind++;
		    }
		break;

	    case 'p':		/* printer status, optional argument */
		if(optind < argc && argv[optind][0] != '-')
		    {
		    optind++;
		    }
		break;

	    case 'r':
		fputs("scheduler is running\n", stdout);
	    	return 0;

	    case 'R':		/* ??? */
		break;

	    case 's':		/* summary status (lots of info) */
		break;

	    case 'S':		/* character set verification, optional argument */
		if(optind < argc && argv[optind][0] != '-')
		    {
		    optind++;
		    }
		break;

	    case 't':		/* -s plus more info */
		break;

	    case 'u':		/* list requests for users, optional argument */
		if(optind < argc && argv[optind][0] != '-')
		    {
		    optind++;
		    }
		break;

	    case 'v':		/* list printers and devices, optional argument */
		if(optind < argc && argv[optind][0] != '-')
		    {
		    optind++;
		    }
		break;

	    default:
		/* fprintf(stderr, _("%s: Syntax error, unrecognized switch: -%c\n"), myname, c); */
		return 1;
	    }
	}

    /*
    ** Flush our stdout buffer before we exec.  This is necessary because
    ** we aren't going to call exit().
    */
    fflush(stdout);

    /* Drop thru to the real lpstat: */
    setuid(0);
    setuid(uid);
    execv(uprint_path_lpstat(), argv);
    fprintf(stderr, "Failed to exec %s, errno=%d (%s)\n", uprint_path_lpstat(), errno, gu_strerror(errno));
    return 1;
    } /* end of main() */

/* end of file */

