/*
** mouse:~ppr/src/init_and_cron/cron_run_daily.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 30 June 2000.
*/

#include "before_system.h"
#include <stdio.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

#include "util_exits.h"
#include "version.h"

const char myname[] = "template";

/*
** Command line options:
*/
static const char *option_chars = "";
static const struct gu_getopt_opt option_words[] =
	{
	{"help", 1000, FALSE},
	{"version", 1001, FALSE},
	{"best-time", 1002, TRUE},
	{"touch-file", 1003, TRUE},
	{"script", 1004, TRUE},
	{(char*)NULL, 0, FALSE}
	} ;

/*
** Print help.
*/
void help_switches(FILE *outfile)
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

	    case 1002:			/* --best-time */
		break;

	    case 1003:			/* --touch-file */
		break;

	    case 1004:			/* --script */
		break;

	    default:			/* other getopt errors or missing case */
		gu_getopt_default(myname, optchar, &getopt_state, stderr);
	    	exit(EXIT_SYNTAX);
		break;
    	    }
    	}
    }

    /* Main code continues here */

    return EXIT_OK;
    } /* end of main() */

/* end of file */
