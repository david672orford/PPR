/*
** mouse:~ppr/src/uprint/uprint-cancel.c
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

static const char *const myname = "uprint-cancel";

void uprint_error_callback(const char *format, ...)
    {
    va_list va;
    fprintf(stderr, "%s: ", myname);
    va_start(va, format);
    vfprintf(stderr, format, va);
    fputc('\n', stderr);
    va_end(va);
    } /* end of uprint_error_callback() */

static const char *option_list = "";

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

    setuid(0);
    setuid(uid);
    execv(uprint_path_cancel(), argv);
    fprintf(stderr, "Can't exec \"%s\", errno=%d (%s)\n", uprint_path_cancel(), errno, gu_strerror(errno));
    return 1;

    /*
    ** Parse the switches.  Mostly, we will call uprint
    ** member functions.
    */
    while((c = getopt(argc, argv, option_list)) != -1)
	{
	switch(c)
	    {
	    default:
		fprintf(stderr, _("%s: Syntax error, unrecognized switch: -%c\n"), myname, c);
		return 1;
	    }
	}

    return 0;
    } /* end of main() */

/* end of file */
