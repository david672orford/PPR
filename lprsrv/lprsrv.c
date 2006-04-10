/*
** mouse:~ppr/src/lprsrv/lprsrv.c
** Copyright 1995--2006, Trinity College Computing Center.
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
** Last modified 23 February 2006.
*/

/*
** Berkeley LPR compatible server for PPR and LP on System V Unix.
** There is also partial support for passing jobs to LPR.
*/

#include "config.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "rfc1179.h"
#include "lprsrv.h"
#include "util_exits.h"
#include "version.h"

/*
** This function returns the name of this computer.  This is used in some
** error messages sent to the client.
*/
const char *this_node(void)
	{
	static const char *p = NULL;
	if(!p)
		{
		struct utsname u;
		uname(&u);
		p = gu_strdup(u.nodename);
		}
	return p;
	} /* end of this_node() */

/*
** This function writes a line to the lprsrv log file.  It performs 
** printf()-style formatting.
*/
static void lprsrv_vlog(const char category[], const char format[], va_list va)
	{
	FILE *logfile;
	if((logfile = fopen(LPRSRV_LOGFILE, "a")))
		{
		fprintf(logfile, "%s: %s: %ld: ", category, datestamp(), (long)getpid());
		vfprintf(logfile, format, va);
		fputc('\n', logfile);
		fclose(logfile);
		}
	} /* lprsrv_vlog() */

static void lprsrv_log(const char category[], const char format[], ...)
	{
	va_list va;
	va_start(va, format);
	lprsrv_vlog(category, format, va);
	va_end(va);
	} /* lprsrv_log() */

/*
** Print a debug line in the lprsrv log file.
*/
void debug(const char message[], ...)
	{
	va_list va;
	va_start(va, message);
	lprsrv_vlog("DEBUG", message, va);
	va_end(va);
	} /* end of debug() */

/*
** Print a warning line in the lprsrv log file.
*/
void warning(const char message[], ...)
	{
	va_list va;
	va_start(va, message);
	lprsrv_vlog("WARNING", message, va);
	va_end(va);
	} /* end of warning() */

void uprint_error_callback(const char *format, ...)
	{
	va_list va;
	va_start(va, format);
	lprsrv_vlog("UPRINT", format, va);
	va_end(va);
	} /* end of uprint_error_callback() */

/*=============================================================================
** main() and its support routines:
=============================================================================*/

/*
** Command line options:
*/
static const char *option_chars = "s:A:";
static const struct gu_getopt_opt option_words[] =
		{
		{"arrest-interest-interval", 'A', TRUE},
		{"help", 1000, FALSE},
		{"version", 1001, FALSE},
		{(char*)NULL, 0, FALSE}
		} ;

/*
** Print how to use.  The argument will be either stdout or stderr.
*/
static void help(FILE *outfile)
	{
	fputs(_("Valid switches:\n"), outfile);

	fputs(_("\t-A <seconds>\n"
		"\t--arrest-interest-interval <seconds>\n"
		"\t\t(this switch is passed thru to ppop)\n"
		"\t--version\n"
		"\t\t(print PPR version number)\n"
		"\t--help\n"
		"\t\t(print this message)\n"), outfile);
	}

/*
** main server loop,
** dispatch commands
*/
static int real_main(int argc,char *argv[])
	{
	char client_dns_name[MAX_HOSTNAME+1];
	char client_ip[16];
	int client_port;
	struct ACCESS_INFO access_info;

	/* Initialize internation messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	/*
	** Parse the command line options.  We use the parsing routine
	** in libppr.a.  All of the parsing state is kept in the
	** structure getopt_state.
	*/
	{
	int optchar;
	struct gu_getopt_state getopt_state;

	gu_getopt_init(&getopt_state, argc, argv, option_chars, option_words);

	while((optchar=ppr_getopt(&getopt_state)) != -1)
		{
		switch(optchar)
			{
			case 'A':					/* -A or --arrest-interest-interval */
				uprint_arrest_interest_interval = getopt_state.optarg;
				break;

			case 1000:					/* --help */
				help(stdout);
				exit(EXIT_OK);

			case 1001:					/* --version */
				puts(VERSION);
				puts(COPYRIGHT);
				puts(AUTHOR);
				exit(EXIT_OK);

			case '?':					/* help or unrecognized switch */
				fprintf(stderr, _("Unrecognized switch: %s\n"), getopt_state.name);
				help(stderr);
				exit(EXIT_SYNTAX);

			case ':':					/* argument required */
				fprintf(stderr, _("The %s option requires an argument.\n"), getopt_state.name);
				exit(EXIT_SYNTAX);

			case '!':					/* bad aggreation */
				fprintf(stderr, _("Switches, such as %s, which take an argument must stand alone.\n"), getopt_state.name);
				exit(EXIT_SYNTAX);

			case '-':					/* spurious argument */
				fprintf(stderr, _("The %s switch does not take an argument.\n"), getopt_state.name);
				exit(EXIT_SYNTAX);

			default:					/* missing case */
				fprintf(stderr, "Internal error: missing case for -%c option\n", optchar);
				exit(EXIT_INTERNAL);
				break;
			}
		}
	} /* end of command line parsing context */

	DODEBUG_MAIN(("connexion received"));

	/*
	** This must be done before request processing starts.  In practice it is
	** not necessary, but in theory Inetd's only guarantee is that stdin will
	**	be connected to the socket.
	*/
	dup2(0, 1);
	dup2(0, 2);

	/*
	 * Change to a known directory, set a umask (since it is difficult to
	 * know what Inetd will give us, set PPR-related variables in the 
	 * environment, and delete junk from the environment.
	 */
	chdir(LIBDIR);
	umask(PPR_UMASK);
	set_ppr_env();
	prune_env();

	/* Determine the IP address and port of the client. */
	get_client_info(client_dns_name, client_ip, &client_port);
	DODEBUG_MAIN(("connexion is from %s (%s), port %d", client_dns_name, client_ip, client_port));

	/* Search lprsrv.conf and find the information that
	   applies to this node. */
	get_access_settings(&access_info, client_dns_name);

	/* If that information indicates that the client is
	   not allowed to connect, then print and error
	   message and drop the connexion now. */
	if(! access_info.allow)
		gu_Throw(_("Node \"%s\" is not allowed to connect"), client_dns_name);
	if(! access_info.insecure_ports && client_port > 1024)
		gu_Throw(_("Node \"%s\" is not allowed to connect from insecure ports"), client_dns_name);

	/* Do zero or one commands and exit. */
	{
	char line[256];
	if(fgets(line, sizeof(line), stdin))
		{
		line[strcspn(line, "\n\r")] = '\0';

		switch(line[0])
			{
			case 1:									/* ^A */
				DODEBUG_MAIN(("start printer: \"^A%s\"", line+1));
				/* Nothing to do? */
				break;
			case 2:									/* ^B */
				DODEBUG_MAIN(("receive job: \"^B%s\"", line+1));
				do_request_take_job(line+1, client_dns_name, &access_info);
				break;
			case 3:									/* ^C */
				DODEBUG_MAIN(("short queue: \"^C%s\"", line+1));
				do_request_lpq(line);				/* This never returns */
				break;
			case 4:									/* ^D */
				DODEBUG_MAIN(("long queue: \"^D%s\"", line+1));
				do_request_lpq(line);				/* This never returns */
				break;
			case 5:										/* ^E, remove jobs */
				DODEBUG_MAIN(("remove: \"^E%s\"", line+1));
				do_request_lprm(line, client_dns_name, &access_info);
				break;
			case 0:
				gu_Throw("empty command");
			default:								/* what can we do? */
				if(line[0] < ' ')
					gu_Throw("unrecognized command: \"^%c%s\"", line[0]+'@', line+1);
				else
					gu_Throw("unrecognized command: \"%s\"", line);
			}
		} /* end of if fgets() worked */
	} /* end of line reading context */

	return 0;
	} /* end of real_main() */

int main(int argc, char *argv[])
	{
	gu_Try {
		return real_main(argc, argv);
		}
	gu_Catch {
		printf("%s\n", gu_exception);
		lprsrv_log("FATAL", "%s", gu_exception);
		exit(1);
		}
	/* NOREACHED */
	return 255;
	} /* end of main() */

/* end of file */

