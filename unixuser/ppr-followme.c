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
** Last modified 13 March 2003.
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
** Command line options:
*/
static const char *option_chars = "";
static const struct gu_getopt_opt option_words[] =
	{
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

    fputs(_(	"\t--version\n"
		"\t--help\n"), outfile);
    }

int main(int argc, char *argv[])
    {
    int i;
    struct passwd *pw;
    char *responder = NULL, *responder_address = NULL, *responder_options = NULL;

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
	    case 9000:			/* --help */
	    	help_usage(stdout);
	    	return EXIT_OK;

	    case 9001:			/* --version */
		puts(VERSION);
		puts(COPYRIGHT);
		puts(AUTHOR);
	    	return EXIT_OK;

	    default:			/* other getopt errors or missing case */
		gu_getopt_default(myname, optchar, &getopt_state, stderr);
	    	return EXIT_SYNTAX;
    	    }
    	}
    i = getopt_state.optind;
    }

    /* We need the username since it is the key into followme.db. */
    if(!(pw = getpwuid(getuid())))
	{
	fprintf(stderr, X_("%s: getpwuid(%ld) failed, you are a non-user\n"), myname, (long)getuid());
	return EXIT_INTERNAL;
	}

    /* Read up to three command line options. */
    if((argc - i) >= 1)
	responder = argv[i + 0];
    if((argc - i) >= 2)
	responder_address = argv[i + 1];
    if((argc - i) >= 3)
	responder_options = argv[i + 2];
    if((argc - i) >= 4)
	{
	help_usage(stderr);
	return EXIT_SYNTAX;
	}

    /* Now we supply defaults for any that weren't specified. */

    if(!responder)
	responder = "write";

    if(!responder_address)
	{
	if(strcmp(responder, "xwin") == 0)
	    responder_address = ":0.0";
	else if(strcmp(responder, "pprpopup") == 0)
	    asprintf(&responder_address, "%s@localhost", pw->pw_name);
	else
	    responder_address = pw->pw_name;
	}

    if(!responder_options)
	if(!(responder_options = getenv("PPR_RESPONDER_OPTIONS")))
	    responder_options = "";

    /* Detect settings that could subvert the user's intentions. */
    {
    char *p;
    if((p = getenv("PPR_RESPONDER")) && strcmp(p, "followme") != 0)
	fprintf(stderr, _("Warning: PPR_RESPONDER is set to %s\n"), p);
    if((p = getenv("PPR_RESPONDER_ADDRESS")) && strcmp(p, pw->pw_name) != 0)
	fprintf(stderr, _("Warning: PPR_RESPONDER_ADDRESS is set to %s\n"), p);
    }

    /* Show the user what we have decided on. */
    printf("responder=%s, responder-address=%s, responder-options=\"%s\"\n", responder, responder_address, responder_options);

    /* Save it in followme.db. */
    if(write_record(pw->pw_name, responder, responder_address, responder_options) == -1)
	return EXIT_INTERNAL;   

    /* The xwin responder won't work unless we give it X display access. */
    if(strcmp(responder, "xwin") == 0)
	{
	gu_runl(myname, stderr, HOMEDIR"/bin/ppr-xgrant", NULL);
	}

    return EXIT_OK;
    } /* end of main() */

/* end of file */
