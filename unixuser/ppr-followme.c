/*
** mouse:~ppr/src/unixuser/ppr-followme.c
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
** Last modified 31 July 2003.
*/

#include "before_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "util_exits.h"
#include "version.h"

const char myname[] = "ppr-followme";

const char dbdir[] = VAR_SPOOL_PPR"/followme.db";

/*
** This function creates the record which is later picked up by ppr-respond.
*/
static int write_record(const char username[], const char responder[], const char responder_address[], const char responder_options[])
	{
	FILE *f;
	char fname[MAX_PPR_PATH];

	ppr_fnamef(fname, "%s/%s", dbdir, username);
	if(!(f = fopen(fname, "w")))
		{
		fprintf(stderr, _("%s: fopen(\"%s\", \"w\") failed, errno=%d (%s)\n"), myname, fname, errno, gu_strerror(errno));
		return -1;
		}

	fprintf(f, "%s %s \"%s\"\n", responder, responder_address, responder_options);

	fclose(f);

	return 0;
	}

/*
** This function reads one's existing followme record and prints it.
*/
static int do_show(const char username[])
	{
	FILE *f;

	{
	char fname[MAX_PPR_PATH];
	ppr_fnamef(fname, "%s/%s", dbdir, username);
	if(!(f = fopen(fname, "r")))
		{
		if(errno == ENOENT)
			{
			printf(_("The user \"%s\" is not registered with followme.\n"), username);
			return EXIT_NOTFOUND;
			}
		else
			{
			fprintf(stderr, _("%s: fopen(\"%s\", \"r\") failed, errno=%d (%s)\n"), myname, fname, errno, gu_strerror(errno));
			return EXIT_INTERNAL;
			}
		}
	}

	{
	char *responder = NULL, *responder_address = NULL, *responder_options = NULL;
	{
	char *line = NULL;
	int line_space_available = 80;

	if(!(line = gu_getline(line, &line_space_available, f)) || gu_sscanf(line, "%S %S %Q", &responder, &responder_address, &responder_options) != 3)
		{
		fprintf(stderr, _("%s: invalid followme registration record\n"), myname);
		return EXIT_INTERNAL;
		}

	gu_free(line);
	}

	printf("%s %s \"%s\"\n", responder, responder_address, responder_options);
	gu_free(responder);
	gu_free(responder_address);
	gu_free(responder_options);
	}

	return EXIT_OK;
	}

/*
** This takes up to three command line arguments and writes them to a
** followme record.
*/
static int do_set(char username[], int argc, char *argv[], int i)
	{
	char *responder = NULL, *responder_address = NULL, *responder_options = NULL;

	/* Read up to three command line options. */
	if((argc - i) >= 1)
		responder = argv[i + 0];
	if((argc - i) >= 2)
		responder_address = argv[i + 1];
	if((argc - i) >= 3)
		responder_options = argv[i + 2];

	/* Now we supply defaults for any that weren't specified. */

	if(!responder)
		{
		if(getenv("DISPLAY"))
			responder = "xwin";
		else
			responder = "write";
		}

	if(!responder_address)
		{
		if(strcmp(responder, "xwin") == 0)
			{
			if(!(responder_address = getenv("DISPLAY")))
				responder_address = ":0.0";
			}
		else if(strcmp(responder, "pprpopup") == 0)
			gu_asprintf(&responder_address, "%s@localhost", username);
		else
			responder_address = username;
		}

	if(!responder_options)
		if(!(responder_options = getenv("PPR_RESPONDER_OPTIONS")))
			responder_options = "";

	/* Detect settings that could subvert the user's intentions. */
	{
	char *p;
	if((p = getenv("PPR_RESPONDER")) && strcmp(p, "followme") != 0)
		fprintf(stderr, _("Warning: PPR_RESPONDER is set to %s\n"), p);
	if((p = getenv("PPR_RESPONDER_ADDRESS")) && strcmp(p, username) != 0)
		fprintf(stderr, _("Warning: PPR_RESPONDER_ADDRESS is set to %s\n"), p);
	}

	/* Show the user what we have decided on. */
	printf("%s %s \"%s\"\n", responder, responder_address, responder_options);

	/* Save it in followme.db. */
	if(write_record(username, responder, responder_address, responder_options) == -1)
		return EXIT_INTERNAL;

	/* The xwin responder won't work unless we give it X display access. */
	if(strcmp(responder, "xwin") == 0)
		{
		gu_runl(myname, stderr, HOMEDIR"/bin/ppr-xgrant", NULL);
		}

	return EXIT_OK;
	} /* end of do_set() */

/*
** Command line options:
*/
static const char *option_chars = "";
static const struct gu_getopt_opt option_words[] =
		{
		{"show", 1000, FALSE},
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

	fprintf(outfile, _("       %s [responder [address [\"options\"]]]\n"), myname);

	fputc('\n', outfile);

	fputs(_("Valid switches:\n"), outfile);

	fputs(_("\t--show\n"), outfile);

	fputs(_("\t--version\n"
						"\t--help\n"), outfile);
	}

int main(int argc, char *argv[])
	{
	int i;
	struct passwd *pw;
	gu_boolean opt_show = FALSE;

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
			case 1000:					/* --show */
				opt_show = TRUE;
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

	/* We have no use for more than 4 non-switch arguments. */
	if((argc - i) >= 4)
		{
		help_usage(stderr);
		return EXIT_SYNTAX;
		}

	/* We need the username since it is the key into followme.db. */
	if(!(pw = getpwuid(getuid())))
		{
		fprintf(stderr, X_("%s: getpwuid(%ld) failed, you are a non-user\n"), myname, (long)getuid());
		return EXIT_INTERNAL;
		}

	if(opt_show)
		return do_show(pw->pw_name);
	else
		return do_set(pw->pw_name, argc, argv, i);

	} /* end of main() */

/* end of file */
