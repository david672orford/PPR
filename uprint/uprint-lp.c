/*
** mouse:~ppr/src/uprint/uprint-lp.c
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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <pwd.h>
#include <errno.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "uprint.h"

extern char *optarg;
extern int optind;

static const char *const myname = "uprint-lp";

void uprint_error_callback(const char *format, ...)
    {
    va_list va;
    fprintf(stderr, "%s: ", myname);
    va_start(va, format);
    vfprintf(stderr, format, va);
    fputc('\n', stderr);
    va_end(va);
    } /* end of uprint_error_callback() */

static const char *option_list = "cd:f:H:mn:o:P:pq:st:T:rwy:";

int main(int argc, char *argv[])
    {
    void *upr;
    struct passwd *pw;
    int c;
    const char *opt_i = (char*)NULL;	/* -i switch */
    const char *opt_H = (char*)NULL;	/* -H switch */

    /* Initialize internation messages library. */
    #ifdef INTERNATIONAL
    setlocale(LC_MESSAGES, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);
    #endif

    /* Trap loops: */
    if(uprint_loop_check(myname) == -1)
    	return 1;

    /*
    ** Create a uprint object.  Notice that we pass the argument
    ** list to the constructor.  If uprint_print() executes
    ** /usr/bin/lp, it will pass it the origional arguments
    ** rather than an reconstructed set.
    */
    if((upr = uprint_new("lp", argc, (const char **)argv)) == (void*)NULL)
	{
	fprintf(stderr, "%s: %s\n", myname, uprint_strerror(uprint_errno));
	return 1;
	}

    /*
    ** Set some defaults:
    */
    uprint_set_show_jobid(upr, TRUE);
    uprint_set_immediate_copy(upr, FALSE);

    /*
    ** Set the ID number and name of the user:
    */
    {
    uid_t uid = getuid();

    if((pw = getpwuid(uid)) == (struct passwd *)NULL)
	{
	fprintf(stderr, _("%s: getpwuid(%ld) failed to find your account\n"), myname, (long int)uid);
	uprint_delete(upr);
	return 1;
	}
    if(pw->pw_name == (char*)NULL)
    	{
    	fprintf(stderr, "%s: strange getpwuid() error, pw_name is NULL\n", myname);
	uprint_delete(upr);
    	return 1;
    	}
    uprint_set_user(upr, uid, -1, pw->pw_name);
    }

    /*
    ** Parse the switches.  Mostly, we will call uprint
    ** member functions.
    */
    while((c = getopt(argc, argv, option_list)) != -1)
	{
	switch(c)
	    {
	    case 'i':		/* request id */
	    	opt_i = optarg;
	    	break;
	    case 'c':		/* copy immediately -- PPR does this anyway */
		uprint_set_immediate_copy(upr, TRUE);
	    	break;
	    case 'd':		/* destination */
		uprint_set_dest(upr, optarg);
	    	break;
	    case 'f':		/* form name */
		uprint_set_form(upr, optarg);
		break;
	    case 'H':		/* special handling */
		opt_H = optarg;
	    	break;
	    case 'm':		/* send mail on normal completion */
		uprint_set_notify_email(upr, TRUE);
	    	break;
	    case 'n':		/* number of copies */
	    	uprint_set_copies(upr, atoi(optarg));
	    	break;
	    case 'o':		/* printer options */
		uprint_set_lp_interface_options(upr, optarg);
	        break;
	    case 'P':		/* page list */
		uprint_set_lp_pagelist(upr, optarg);
	    	break;
	    case 'p':		/* enable notification */

	    	break;
	    case 'q':		/* queue priority */
		uprint_set_priority(upr, atoi(optarg));
	    	break;
	    case 's':		/* suppress messages */
		uprint_set_show_jobid(upr, FALSE);
	    	break;
	    case 'S':		/* character set */
		uprint_set_charset(upr, optarg);
	    	break;
	    case 't':		/* title */
		uprint_set_jobname(upr, optarg);
	    	break;
	    case 'T':		/* content type */
	    	uprint_set_content_type_lp(upr, optarg);
	    	break;
	    case 'r':		/* content type raw */
	    	uprint_set_content_type_lp(upr, "-r");
	    	break;
	    case 'w':		/* write a message at job completion */
		uprint_set_notify_write(upr, TRUE);
	    	break;
	    case 'y':		/* mode list */
		uprint_set_lp_filter_modes(upr, optarg);
	    	break;
	    default:		/* ignore illegal options as */
		break;		/* the real lp does */
	    }
	}

    if(opt_i || opt_H)
    	{
	fprintf(stderr, _("%s: lp -i and -H are not yet implemented\n"), myname);
	uprint_delete(upr);
	return 1;
    	}

    /* If the print destination has not yet been
       determined, determine it now. */
    if(!uprint_get_dest(upr))
	uprint_set_dest(upr, uprint_default_destinations_lp());

    /* If there are file names, tell uprint about them: */
    if(argc > optind)
	uprint_set_files(upr, (const char **)&argv[optind]);

    /* Look for a PPR responder we can use. */
    {
    char *p;
    if((p = getenv("PPR_RESPONDER")))
    	uprint_set_ppr_responder(upr, p);
    if((p = getenv("PPR_RESPONDER_ADDRESS")))
    	uprint_set_ppr_responder_address(upr, p);
    if((p = getenv("PPR_RESPONDER_OPTIONS")))
    	uprint_set_ppr_responder_options(upr, p);
    }

    /* Print the job. */
    {
    int return_code;

    if((return_code = uprint_print(upr, TRUE)) >= 0)
	{
	/* Child ran, nothing more to do. */
	}
    /* Unclaimed queue: */
    else if(uprint_errno == UPE_UNDEST)
	{
	fprintf(stderr, _("%s: Print queue \"%s\" not found.\n"), myname, uprint_get_dest(upr));
	/* This is the code that System V lp uses for unknown queue: */
	return_code = 1;
	}
    /* If queue found but executing program failed, */
    else
	{
	/* An arbitrary value intended to indicated a major failure: */
	return_code = 255;
	}

    /* We are done with the UPRINT structure: */
    uprint_delete(upr);

    return return_code;
    }
    } /* end of main() */

/* end of file */

