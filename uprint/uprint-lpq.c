/*
** mouse:~ppr/src/uprint/uprint-lpq.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 30 July 1999.
*/

#include "before_system.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pwd.h>
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
static const char *const myname = "uprint-lpq";

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
** In main() we parse the options and call the
** uprint library routine to list a queue.
*/
static const char *option_list = "P:l";

int main(int argc, char *argv[])
    {
    int c;
    uid_t uid;
    const char *queue = (const char *)NULL;
    int format = 0;

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
	    case 'P':		/* printer */
		queue = optarg;
	    	break;

	    case 'l':		/* long format */
	    	format = 1;
	    	break;

	    default:
		/* fprintf(stderr, _("%s: Syntax error, unrecognized switch: -%c\n"), myname, c); */
		return 1;
	    }
	}

    /* If the print destination has not yet been
       determined, determine it now. */
    if(queue == (const char *)NULL)
	if((queue = getenv("PRINTER")) == (char*)NULL)
	    queue = uprint_default_destinations_lpr();

    {
    int return_code;

    /* Print the queue. */
    if((return_code = uprint_lpq(uid, "???", queue, format, (const char **)&argv[optind], TRUE)) != -1)
	{
	/* Child ran, nothing more to do. */
	}
    /* Unclaimed: */
    else if(uprint_errno == UPE_UNDEST)
	{
	fprintf(stderr, _("%s: Print queue \"%s\" not found.\n"), myname, queue);
	/* This is the exit code that BSD lpq uses for unknown queue: */
	return_code = 1;
	}
    /* Command failed: */
    else
	{
	/* An arbitrary value intended to indicated a major failure: */
	return_code = 255;
	}

    return return_code;
    }
    } /* end of main() */

/* end of file */

