/*
** mouse:~ppr/src/cron/ppr-index.c
** Copyright 1995--2005, Trinity College Computing Center.
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
** Last modified 7 January 2005.
*/

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "util_exits.h"
#include "version.h"

const char myname[] = "ppr-index";

const char *indexes[] = {
	"fonts",
	"ppds",
	"filters",
	"windrivers",
	NULL
	};

static int do_index(const char name[], gu_boolean delete_index, gu_boolean skip_missing);

/*
** Command line options:
*/
static const char *option_chars = "";
static const struct gu_getopt_opt option_words[] = {
	{"delete", 1000, FALSE},
	{"help", 9000, FALSE},
	{"version", 9001, FALSE},
	{(char*)NULL, 0, FALSE}
	} ;

/*
** Print help.
*/
static void help_usage(FILE *outfile)
	{
	fprintf(outfile, _("Usage: %s [switches]\n"), myname);

	fputc('\n', outfile);

	fputs(_("Valid switches:\n"), outfile);

	fputs(_(	"\t--delete\n"), outfile);

	fputs(_(	"\t--version\n"
				"\t--help\n"), outfile);
	}

int main(int argc, char *argv[])
	{
	gu_boolean opt_delete = FALSE;
	int i;
	int ret = EXIT_OK;

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
			case 1000:					/* --delete */
				opt_delete = TRUE;
				break;

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
	i = getopt_state.optind;
	}

	/* Main code continues here */
	if(i < argc)
		{
		for( ;i < argc; i++)
			{
			int i2;
			for(i2 = 0; indexes[i2]; i2++)
				{
				if(strcmp(indexes[i2], argv[i]) == 0)
					break;
				}
			if(!indexes[i2])
				{
				fprintf(stderr, "%s: there is no index called \"%s\"\n", argv[0], argv[i]);
				ret = EXIT_BADDEST;
				break;
				}
			if((ret = do_index(argv[i], opt_delete, FALSE)) != EXIT_OK)
				{
				break;
				}
			}
		}
	else
		{
		for(i = 0; indexes[i]; i++)
			{
			if((ret = do_index(indexes[i], opt_delete, TRUE)) != EXIT_OK)
				break;
			}
		}

	if(ret == -1)
		return EXIT_INTERNAL;
	else
		return ret;	   
	} /* end of main() */

static int do_index(const char name[], gu_boolean delete_index, gu_boolean skip_missing)
	{
	char fname[MAX_PPR_PATH];
	ppr_fnamef(fname, "%s/lib/index%s", HOMEDIR, name);
	if(skip_missing)
		{
		if(access(fname, X_OK) == -1)
			return EXIT_OK;
		}
	return gu_runl(myname, stderr, fname, delete_index ? "--delete" : NULL, NULL);
	}

/* end of file */
