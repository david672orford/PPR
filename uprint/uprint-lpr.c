/*
** mouse:~ppr/src/uprint/uprint-lpr.c
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

static const char *const myname = "uprint-lpr";

void uprint_error_callback(const char *format, ...)
    {
    va_list va;
    fprintf(stderr, "%s: ", myname);
    va_start(va, format);
    vfprintf(stderr, format, va);
    fputc('\n', stderr);
    va_end(va);
    } /* end of uprint_error_callback() */

static const char *option_list =
	"cdfglnptvP:hmrs#:1:2:3:4:C:J:T:U:i:w:"	/* BSD */
	"I:jK:N:o:O:x"				/* DEC OSF/1 3.2 */
	;

/* OSF lpr compatible duplex options: */
static struct
    {
    const char *from;
    const char *to;
    } osf_K_xlate[] =
    {
    /* These are the options in queue file format.  The fact that
       "one" and "two" are acceptable to lpr is undocumented. */
    {"one", "one"},
    {"two", "two"},
    {"tumble", "tumble"},
    {"one_sided_duplex", "one_sided_duplex"},
    {"one_sided_tumble", "one_sided_tumble"},
    {"two_sided_simplex", "two_sided_simplex"},

    /* Other documented forms accepted by OSF lpr. */
    {"1", "one"},
    {"one_sided", "one"},
    {"one_sided_simplex", "one"},
    {"2", "two"},
    {"two_sided", "two"},
    {"two_sided_duplex", "two"},
    {"two_sided_tumble", "tumble"},

    {NULL, NULL}
    };

int main(int argc, char *argv[])
    {
    uid_t uid = getuid();
    void *upr;
    struct passwd *pw;
    int c;
    int x;

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
    ** Create a uprint object.
    **
    ** Save the argument array in it in case the uprint
    ** library decides to print the job by executing
    ** the real lpr.
    */
    if((upr = uprint_new("lpr", argc, (const char **)argv)) == (void*)NULL)
	{
	fprintf(stderr, "%s: %s\n", myname, uprint_strerror(uprint_errno));
	uprint_delete(upr);
	return 1;
	}

    /*
    ** Set the UID number and name of the user:
    */
    if( (pw=getpwuid(uid)) == (struct passwd *)NULL)
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

    /*
    ** Set the defaults for a few options:
    */
    uprint_set_immediate_copy(upr, TRUE);

    /*
    ** Parse the switches.  Mostly, we will call uprint
    ** member functions.
    */
    while( (c=getopt(argc, argv, option_list)) != -1 )
	{
	switch(c)
	    {
	    case 'c':		/* CIF */
	    case 'd':		/* DVI */
	    case 'f':
	    case 'g':
	    case 'l':
	    case 'n':
	    case 'p':
	    case 't':
	    case 'v':
	    case 'x':		/* OSF no filtering */
		uprint_set_content_type_lpr(upr, c);
		break;

	    case 'P':
	    	uprint_set_dest(upr, optarg);
	    	break;

	    case 'h':		/* suppress burse page */
		uprint_set_nobanner(upr, TRUE);
	    	break;

	    case 'm':		/* send mail on completion */
		uprint_set_notify_email(upr, TRUE);
		break;

	    case 'r':		/* remove files on completion */
		uprint_set_unlink(upr, TRUE);
	    	break;

	    case 's':		/* use symbolic links */
		uprint_set_immediate_copy(upr, FALSE);
	    	break;

	    case '#':		/* number of copies */
	    	uprint_set_copies(upr, atoi(optarg));
	    	break;

	    case '1':		/* troff fonts */
		uprint_set_troff_1(upr, optarg);
		break;

	    case '2':
		uprint_set_troff_2(upr, optarg);
		break;

	    case '3':
		uprint_set_troff_3(upr, optarg);
		break;

	    case '4':
		uprint_set_troff_4(upr, optarg);
		break;

	    case 'C':		/* class */
		uprint_set_lpr_class(upr, optarg);
		break;

	    case 'J':		/* job name */
		uprint_set_jobname(upr, optarg);
	        break;

	    case 'T':		/* title */
		uprint_set_pr_title(upr, optarg);
	        break;

	    case 'U':		/* user name */
		uprint_set_user(upr, uid, -1, optarg);	/* !!! */
	        break;

	    case 'i':		/* indent */
		uprint_set_indent(upr, optarg);
	        break;

	    case 'w':		/* page width */
		uprint_set_width(upr, optarg);
	        break;

	    case 'I':		/* OSF input tray */
		uprint_set_osf_LT_inputtray(upr, optarg);
		break;

	    case 'j':		/* OSF display job ID */
		uprint_set_show_jobid(upr, TRUE);
		break;

	    case 'K':		/* OSF sides (duplex) */
		{
		int x;
		for(x=0; osf_K_xlate[x].from; x++)
		    {
		    if(strcmp(optarg, osf_K_xlate[x].from) == 0)
		    	{
			uprint_set_osf_K_duplex(upr, osf_K_xlate[x].to);
			break;
		    	}
		    }
		if(! osf_K_xlate[x].from)
		    {
		    fprintf(stderr, _("%s: Invalid -K argument\n"), myname);
		    return 1;
		    }
		break;
		}

	    case 'N':		/* OSF N-Up */
		x = atoi(optarg);
		if(x < 1)
		    {
		    fprintf(stderr, _("%s: Invalid -N argument\n"), myname);
		    return 1;
		    }
		uprint_set_nup(upr, x);
		break;

	    case 'o':		/* OSF output tray */
		uprint_set_osf_GT_outputtray(upr, optarg);
		break;

	    case 'O':		/* OSF orientation */
		uprint_set_osf_O_orientation(upr, optarg);
		break;

	    default:		/* Illegal option */
		break;		/* ignore as BSD lpr seems to */
	    }
	}

    /* If the print destination has not yet been
       determined, determine it now. */
    if(!uprint_get_dest(upr))
	uprint_set_dest(upr, uprint_default_destinations_lpr());

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
    /* No match anywhere: */
    else if(uprint_errno == UPE_UNDEST)
	{
	fprintf(stderr, _("%s: Print queue \"%s\" not found.\n"), myname, uprint_get_dest(upr));
	/* This is the code that BSD lpr uses for unknown queue: */
	return_code = 1;
	}
    /* Printing failed, */
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

