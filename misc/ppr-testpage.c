/*
** mouse:~ppr/src/misc/ppr-testpage.c
** Copyright 1995--2002, Trinity College Computing Center.
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
** Last modified 7 March 2002.
*/

#include "before_system.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "util_exits.h"
#include "version.h"

const char myname[] = "ppr-testpage";

/*========================================================================
**
========================================================================*/

static void do_header(void)
    {
    printf("\
%%!PS-Adobe-3.0
%%%%Title: PPR Test Page
%%%%Pages: 1
%%%%EndComments

");
    }

static void do_setup(void)
    {
    printf("\
%%%%BeginSetup
%%%%EndSetup

");
    }

static void do_startpage(int num)
    {
    printf("\
%%%%Page: %d %d
save
", num, num);
    }

static void do_endpage(void)
    {
    printf("\
restore
showpage

");
    }

static void do_trailer(void)
    {
    printf("\
%%%%Trailer
%%%%EOF
");
    }

/*========================================================================
** Main and support routines
========================================================================*/

/*
** Command line options:
*/
static const char *option_chars = "";
static const struct gu_getopt_opt option_words[] =
	{
	{"help", 1000, FALSE},
	{"version", 1001, FALSE},
	{(char*)NULL, 0, FALSE}
	} ;

/*
** Print help.
*/
static void help_switches(FILE *outfile)
    {
    fputs(_("Valid switches:\n"), outfile);

    fputs(_(	"\t--version\n"
		"\t--help\n"), outfile);
    }

int main(int argc, char *argv[])
    {
    /* Initialize international messages library. */
    #ifdef INTERNATIONAL
    setlocale(LC_MESSAGES, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);
    #endif

    /* Parse the options. */
    {
    struct gu_getopt_state getopt_state;
    int optchar;
    gu_getopt_init(&getopt_state, argc, argv, option_chars, option_words);
    while((optchar = ppr_getopt(&getopt_state)) != -1)
    	{
    	switch(optchar)
    	    {
	    case 1000:			/* --help */
	    	help_switches(stdout);
	    	exit(EXIT_OK);

	    case 1001:			/* --version */
		puts(VERSION);
		puts(COPYRIGHT);
		puts(AUTHOR);
	    	exit(EXIT_OK);

	    default:			/* other getopt errors or missing case */
		gu_getopt_default(myname, optchar, &getopt_state, stderr);
	    	exit(EXIT_SYNTAX);
		break;
    	    }
    	}
    }

    do_header();

    do_setup();

    do_startpage(1);

    do_endpage();

    do_trailer();

    return 0;
    } /* end of main() */

/* end of file */
