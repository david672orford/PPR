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
** Last modified 16 February 2006.
*/

/*
** Berkeley LPR compatible server for PPR and LP on System V Unix.
** There is also partial support for passing jobs to LPR.
*/

#include "before_system.h"
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
#include "lprsrv.h"
#include "util_exits.h"
#include "version.h"
#include "uprint.h"

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
** This routine is called by fatal(), debug(), etc.
** It writes a line to the lprsrv log file.
*/
static void lprsrv_vlog(const char category[], const char atfunction[], const char format[], va_list va)
	{
	FILE *logfile;
	if((logfile = fopen(LPRSRV_LOGFILE, "a")) != NULL)
		{
		fprintf(logfile, "%s: %s: %ld: ", category, datestamp(), (long)getpid());
		if(atfunction)
			fprintf(logfile, "%s(): ", atfunction);
		vfprintf(logfile, format, va);
		fputc('\n', logfile);
		fclose(logfile);
		}
	} /* lprsrv_vlog() */

static void lprsrv_log(const char category[], const char atfunction[], const char format[], ...)
	{
	va_list va;
	va_start(va, format);
	lprsrv_vlog(category, atfunction, format, va);
	va_end(va);
	} /* lprsrv_log() */

/*
** Print an error message and abort.
*/
void fatal(int exitcode, const char message[], ... )
	{
	va_list va;
	va_start(va, message);

	lprsrv_vlog("FATAL", NULL, message, va);

	fputs("lprsrv: ", stdout);
	vfprintf(stdout, message, va);
	fputc('\n', stdout);

	va_end(va);

#ifdef STANDALONE
	if(am_standalone_parent)
		unlink(LPRSRV_LOCKFILE);
#endif

	exit(exitcode);
	} /* end of fatal() */

/*
** Print a debug line in the lprsrv log file.
*/
void debug(const char message[], ...)
	{
	va_list va;
	va_start(va, message);
	lprsrv_vlog("DEBUG", NULL, message, va);
	va_end(va);
	} /* end of debug() */

/*
** Print a warning line in the lprsrv log file.
*/
void warning(const char message[], ...)
	{
	va_list va;
	va_start(va, message);
	lprsrv_vlog("WARNING", NULL, message, va);
	va_end(va);
	} /* end of warning() */

void uprint_error_callback(const char *format, ...)
	{
	va_list va;
	va_start(va, format);
	lprsrv_vlog("UPRINT", NULL, format, va);
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
		#ifdef STANDALONE
		{"standalone-port", 's', TRUE},
		#endif
		{(char*)NULL, 0, FALSE}
		} ;

/*
** Print how to use.  The argument will be either stdout or stderr.
*/
static void help(FILE *outfile)
	{
	fputs(_("Valid switches:\n"), outfile);

#ifdef STANDALONE
	fputs(_("\t-s <port>\n"
		"\t--standalone-port <port>\n"
		"\t\t(run standalone, bind to specified TCP port)\n"), outfile);
#endif

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
	const char function[] = "main";
	char client_dns_name[MAX_HOSTNAME+1];
	char client_ip[16];
	int client_port;
	struct ACCESS_INFO access_info;
	#ifdef STANDALONE
	int standalone_port = 0;
	#endif

	/* Initialize internation messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	/*
	** Change to ppr's home directory.  That way we know
	** where our core dumps will go. :-)
	*/
	chdir(HOMEDIR);

	/*
	** Set the umask since it is difficult to predict
	** what umask we will inherit.
	*/
	umask(PPR_UMASK);

	/*
	** Clean up the environement.
	*/
	set_ppr_env();
	prune_env();

	/*
	** It is essential that this program run as "root" if it is to act as the user.
	*/
	if(getuid())
		fatal(1, _("lprsrv must run as root"));

	/*
	** Switch the effective UID to that of the PPR user in order to gain a 
	** little safety but leave the real UID as root so that we can bind
	** to priveledged ports (in standalone mode) or become users in order
	** to print jobs for them.
	*/
	{
	struct passwd *user_ppr;

	if((user_ppr = getpwnam(USER_PPR)) == (struct passwd *)NULL)
		fatal(1, _("%s(): getpwnam(\"%s\") failed, errno=%d (%s)"), function, USER_PPR, errno, gu_strerror(errno));

	if(setgroups(0, &user_ppr->pw_gid) == -1)
		fatal(1, _("%s(): setgroups() failed, errno=%d (%s)"), function, errno, gu_strerror(errno));

	if(setegid(user_ppr->pw_gid) == -1)
		fatal(1, _("%s(): setegid(%ld) failed, errno=%d (%s)"), function, (long)user_ppr->pw_gid, errno, gu_strerror(errno));

	if(seteuid(user_ppr->pw_uid) == -1)
		fatal(1, _("%s(): seteuid(%ld) failed, errno=%d (%s)"), function, (long)user_ppr->pw_uid, errno, gu_strerror(errno));
	}

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
			case 's':
				#ifdef STANDALONE
				{
				if(strspn(getopt_state.optarg, "0123456789") == strlen(getopt_state.optarg))
					{
					standalone_port = atoi(getopt_state.optarg);
					}
				else
					{
					if((standalone_port = port_name_lookup(getopt_state.optarg)) == -1)
						{
						fprintf(stderr, _("Unknown port name: %s"), getopt_state.optarg);
						exit(EXIT_SYNTAX);
						}
					}
				}
				break;
				#else
				fputs(_("Standalone mode code not present.\n"), stderr);
				exit(EXIT_SYNTAX);
				#endif

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

	/*
	** If we should run in standalone mode, do it now.
	** The parent will never return from this function call
	** but the children will.
	*/
	#ifdef STANDALONE
	if(standalone_port)
		run_standalone(standalone_port);
	#endif

	DODEBUG_MAIN(("connexion received"));

	/*
	** This must be done first thing!
	** INETD's only guarantee is that
	** stdin will be connected to the socket.
	*/
	dup2(0, 1);
	dup2(0, 2);

	get_client_info(client_dns_name, client_ip, &client_port);
	DODEBUG_MAIN(("connexion is from %s (%s), port %d", client_dns_name, client_ip, client_port));

	/* Search lprsrv.conf and find the information that
	   applies to this node. */
	get_access_settings(&access_info, client_dns_name);

	/* If that information indicates that the client is
	   not allowed to connect, then print and error
	   message and drop the connexion now. */
	if(! access_info.allow)
		fatal(1, _("Node \"%s\" is not allowed to connect"), client_dns_name);
	if(! access_info.insecure_ports && client_port > 1024)
		fatal(1, _("Node \"%s\" is not allowed to connect from insecure ports"), client_dns_name);

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
				fatal(1, "empty command");
			default:								/* what can we do? */
				if(line[0] < ' ')
					fatal(1, "unrecognized command: \"^%c%s\"", line[0]+'@', line+1);
				else
					fatal(1, "unrecognized command: \"%s\"", line);
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
		lprsrv_log("exception", NULL, "%s", gu_exception);
		exit(1);
		}
	/* NOREACHED */
	return 255;
	} /* end of main() */

/* end of file */

