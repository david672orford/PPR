/*
** mouse:~ppr/src/www/ppr-passwd.c
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
** Last modified 20 February 2003.
*/

#include "before_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "gu_md5.h"
#include "global_defines.h"
#include "util_exits.h"
#include "version.h"

const char myname[] = "ppr-passwd";
const char htpasswd[] = CONFDIR"/htpasswd";
const char htpasswd_new[] = CONFDIR"/htpasswd_new";
const char realm[] = "printing";

static int body(FILE *rfile, FILE *wfile, const char username[], const char realm[]);
static int new_password(FILE *wfile, const char username[], const char realm[]);
static void md5_digest(char *buffer, const char username[], const char realm[], const char password[]);

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
static void help_usage(FILE *outfile)
    {
    fputs(_("Usage: ppr-passwd <username>\n"), outfile);

    fputc('\n', outfile);

    fputs(_("Valid switches:\n"), outfile);

    fputs(_(	"\t--version\n"
		"\t--help\n"), outfile);
    }

int main(int argc, char *argv[])
    {
    const char *username;
    int rfd, wfd;
    FILE *rfile, *wfile;

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
	    	help_usage(stdout);
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
    if(argc > getopt_state.optind)
	{
	username = argv[getopt_state.optind];
	}
    else
	{
	struct passwd *pw;
	if(!(pw = getpwuid(getuid())))
	    {
	    fprintf(stderr, _("%s: getpwuid(%ld) failed, errno=%d (%s)\n"), myname, (long)getuid(), errno, gu_strerror(errno));
	    return EXIT_NOTFOUND;
	    }
	username = gu_strdup(pw->pw_name);
	}
    }

    if((rfd = open(htpasswd, O_RDWR)) == -1)
	{
	fprintf(stderr, _("%s: can't open \"%s\", errno=%d (%s)\n"), myname, htpasswd, errno, gu_strerror(errno));
	return EXIT_INTERNAL;
	}

    if(gu_lock_exclusive(rfd, TRUE) == -1)
	{
	fprintf(stderr, _("%s: failed to get exclusive lock on \"%s\", errno=%d (%s).\n"), myname, htpasswd, errno, gu_strerror(errno));
	return EXIT_INTERNAL;
	}

    if((wfd = open(htpasswd_new, (O_WRONLY | O_CREAT | O_EXCL), UNIX_640)) == -1)
	{
	fprintf(stderr, _("%s: can't create \"%s\", errno=%d (%s)\n"), myname, htpasswd_new, errno, gu_strerror(errno));
	return EXIT_INTERNAL;
	}

    if(!(rfile = fdopen(rfd, "r")) || !(wfile = fdopen(wfd, "w")))
	{
	fprintf(stderr, _("%s: fdopen() failed, errno=%d (%s)\n"), myname, errno, gu_strerror(errno));
	return EXIT_INTERNAL;
	}

    {
    int ret;
    ret = body(rfile, wfile, username, realm);

    if(fclose(wfile))
	{
	fprintf(stderr, _("%s: fclose() failed, errno=%d (%s)\n"), myname, errno, gu_strerror(errno));
	ret = -1;
	}

    if(fclose(rfile))
	{
	fprintf(stderr, _("%s: fclose() failed, errno=%d (%s)\n"), myname, errno, gu_strerror(errno));
	ret = -1;
	}

    if(ret)
	{
	if(unlink(htpasswd_new) == -1)
	    {
	    fprintf(stderr, _("%s: unlink(\"%s\") failed, errno=%d (%s)\n"), myname, htpasswd_new, errno, gu_strerror(errno));
	    }
	}

    else
	{
	if(rename(htpasswd_new, htpasswd) == -1)
	    {
	    fprintf(stderr, _("%s: rename(\"%s\", \"%s\") failed, errno=%d (%s)\n"), myname, htpasswd_new, htpasswd, errno, gu_strerror(errno));
	    ret = -1;
	    }
	}

    if(ret == 1)
    	return EXIT_SYNTAX;
    if(ret == -1)
	return EXIT_INTERNAL;
    }

    printf(_("Password updated sucessfully.\n"));
    return EXIT_OK;
    } /* end of main() */

static int body(FILE *rfile, FILE *wfile, const char username[], const char realm[])
    {
    char *line = NULL;
    int line_available = 80;
    gu_boolean found = FALSE;
    char *p, *f1, *f2, *f3;
    int linenum = 0;
    int ret = -1;

    while((p = line = gu_getline(line, &line_available, rfile)))
	{
	linenum++;
	if(!(f1 = gu_strsep(&p,":")) || !(f2 = gu_strsep(&p,":")) || !(f3 = gu_strsep(&p, ":")))
	    {
	    fprintf(stderr, _("%s: line %d of %s is invalid, aborting\n"), myname, linenum, htpasswd);
	    break;
	    }
	if(strcmp(f1, username) == 0 && strcmp(f2, realm) == 0)
	    {
	    if((ret = new_password(wfile, username, realm)))
	    	break;
	    found = TRUE;
	    }
	else
	    {
	    if(fprintf(wfile, "%s:%s:%s\n", f1, f2, f3) < 0)
		{
		fprintf(stderr, "%s: fprintf() failed\n", myname);
		break;
		}
	    }
	}

    if(line)
	gu_free(line);

    else if(!found)
	{
	ret = new_password(wfile, username, realm);
	}

    return ret;
    }

static int new_password(FILE *wfile, const char username[], const char realm[])
    {
    char prompt[80];
    char *pass_ptr = NULL;
    char crypted[33];

    snprintf(prompt, sizeof(prompt), _("Enter new password for %s: "), username);

    while(!pass_ptr)
	{
	if(!(pass_ptr = getpass(prompt)))
	    return -1;
	if(strlen(pass_ptr) == 0)
	    return 1;
	if(strlen(pass_ptr) < 6)
	    {
	    printf(_("That password is too short.  Please pick another.\n"));
	    pass_ptr = NULL;
	    continue;
	    }
	break;
	}

    md5_digest(crypted, username, realm, pass_ptr);

    if(fprintf(wfile, "%s:%s:%s\n", username, realm, crypted) < 0)
	{
	fprintf(stderr, "%s: fprintf() failed\n", myname);
	return -1;
	}

    return 0;
    }

static void md5_digest(char *buffer, const char username[], const char realm[], const char password[])
    {
    md5_state_t state;
    md5_byte_t digest[16];
    int i;

    md5_init(&state);
    md5_append(&state, (const md5_byte_t *)username, strlen(username));
    md5_append(&state, (const md5_byte_t *)":", strlen(":"));
    md5_append(&state, (const md5_byte_t *)realm, strlen(realm));
    md5_append(&state, (const md5_byte_t *)":", strlen(":"));
    md5_append(&state, (const md5_byte_t *)password, strlen(password));
    md5_finish(&state, digest);

    for(i = 0; i < 16; i++)
	snprintf(&buffer[i * 2], 3, "%02x", digest[i]);
    }

/* end of file */
