/*
** mouse:~ppr/src/templates/main.c
** Copyright 1995--2007, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 9 March 2007.
*/

/*! \file
	\brief skeletal program

	Long description of program
*/

#include "config.h"
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

const char myname[] = "template";

/*
** Command line options:
*/
static const char *option_chars = "";
static const struct gu_getopt_opt option_words[] =
		{
		{"help", 9000, FALSE},
		{"version", 9001, FALSE},
		{(char*)NULL, 0, FALSE}
		} ;

/** Print help
 */
static void help_usage(FILE *outfile)
	{
	fprintf(outfile, _("Usage: %s [switches]\n"), myname);

	fputc('\n', outfile);

	fputs(_("Valid switches:\n"), outfile);

	fputs(_(	"\t--version\n"
				"\t--help\n"), outfile);
	}

/** main entry point
 */
int main(int argc, char *argv[])
	{
	/* Initialize international messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
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
			case 9000:					/* --help */
				help_usage(stdout);
				return EXIT_OK;

			case 9001:					/* --version */
				puts(VERSION);
				puts(COPYRIGHT);
				puts(AUTHOR);
				return EXIT_OK;

			default:					/* other getopt errors or missing case */
				gu_getopt_default(myname, optchar, &getopt_state, stderr);
				return EXIT_SYNTAX;
			}
		}
	if(argc > getopt_state.optind)
		{

		}
	}

	/* Main code continues here */

	} /* end of main() */

/* end of file */
