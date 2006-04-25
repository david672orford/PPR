/*
** mouse:~ppr/src/ppad/ppad.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 25 April 2006.
*/

/*===========================================================================
** Administration program for PostScript page printers.  This program
** edits the media list and edits printer and group configuration
** files.
<helptopic>
	<name>misc</name>
	<desc>miscelanious command</desc>
</helptopic>
===========================================================================*/

#include "config.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <errno.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "util_exits.h"
#include "ppad.h"
#include "dispatch.h"
#include "dispatch_table.h"
#include "version.h"

/* misc globals */
const char myname[] = "ppad";
int machine_readable = FALSE;
int debug_level = 0;

/*
** Handle fatal errors.
** Print a message and exit.
*/
void fatal(int exitval, const char *message, ...)
	{
	va_list va;

	va_start(va,message);
	gu_utf8_fputs(_("Fatal: "), stderr);
	gu_utf8_vfprintf(stderr,message,va);
	gu_fputwc('\n', stderr);
	va_end(va);

	exit(exitval);
	} /* end of fatal() */

/*
<command helptopics="misc">
	<name><word>remind</word></name>
	<desc>send reminder e-mail about printer problems</desc>
	<args>
	</args>
</command>
*/
int command_remind(const char *argv[])
	{
	write_fifo("n\n");
	return 0;
	} /* command_remind() */

/*
** --help
*/
static void help(void)
	{
	int i;
	const char *switch_list[] =
		{
		N_("-M\tselect machine-readable output"),
		N_("--machine-readable\tsame as -M"),
		N_("--user <username>\trun as if by <username>"),
		N_("-d <n>\tset debug level to <n>"),
		N_("--debug=<n>\tsame as -d"),
		N_("--version\tprint PPR version information"),
		N_("--help\tprint this help message"),
		NULL
		};

	gu_utf8_putline(_("Valid switches:\n"));
	for(i = 0; switch_list[i]; i++)
		{
		const char *p = gettext(switch_list[i]);
		int to_tab = strcspn(p, "\t");
		gu_utf8_printf("    %-20.*s %s\n", to_tab, p, p[to_tab] == '\t' ? &p[to_tab + 1] : "");
		}

	gu_fputwc('\n', stderr);

	{
	const char *args[] = {"help", NULL};
	dispatch(myname, args);
	}

	gu_fputwc('\n', stderr);
	gu_utf8_printf(_("The %s manpage may be viewed by entering this command at a shell prompt:\n"
		"    ppdoc %s\n"), "ppad(1)", "ppad");
	} /* help() */

/*
** interactive mode function
** Return the result code of the last command executed.
**
** In interactive mode, we present a prompt, read command
** lines, and execute them.
*/
static int interactive_mode(void)
	{
	#define MAX_CMD_WORDS 64
	char *ar[MAX_CMD_WORDS+1];	/* argument vector constructed from line[] */
	char *ptr;					/* used to parse arguments */
	unsigned int x;				/* used to parse arguments */
	int errorlevel = 0;			/* return value from last command */

	if( ! machine_readable )
		{
		gu_utf8_putline(_("PPAD, Page Printer Administrator's utility"));
		gu_utf8_putline(VERSION);
		gu_utf8_putline(COPYRIGHT);
		gu_utf8_putline(AUTHOR);
		gu_utf8_putline("");
		gu_utf8_putline(_("Type \"help\" for command list, \"exit\" to quit."));
		gu_utf8_putline("");
		}
	else				/* terse, machine readable banner */
		{
		gu_utf8_putline("*READY\t"VERSION);
		fflush(stdout);
		}

	/*
	** Read input lines until end of file.
	*/
	while((ptr = ppr_get_command("ppad>", machine_readable)))
		{
		/* Skip comments. */
		if(ptr[0] == '#' || ptr[0] == ';')
			continue;

		/*
		** Break the string into white-space separated "words".  A quoted string
		** will be treated as one word.
		*/
		for(x=0; (ar[x] = gu_strsep_quoted(&ptr, " \t", NULL)); x++)
			{
			if(x == MAX_CMD_WORDS)
				{
				gu_utf8_putline(X_("Warning: command buffer overflow!"));	/* temporary code, don't internationalize */
				ar[x] = NULL;
				break;
				}
			}

		/*
		** The variable x will be an index into ar[] which will
		** indicate the first element that has any significance.
		** If the line begins with the word "ppad" will will
		** increment x.
		*/
		x=0;
		if(ar[0] && strcmp(ar[0], "ppad") == 0)
			x++;

		/*
		** If no tokens remain in this command line,
		** go on to the next command line.
		*/
		if(ar[x] == (char*)NULL)
			continue;

		/*
		** If the command is "exit", break out of
		** the line reading loop.
		*/
		if(strcmp(ar[x], "exit") == 0 || strcmp(ar[x], "quit") == 0)
			break;

		/*
		** Call the dispatch() function to execute the command.  If the
		** command is not recognized, dispatch() will return -1.  In that
		** case we print a helpful message and change the errorlevel to
		** zero since -1 is not a valid exit code for a program.
		*/
		if((errorlevel = dispatch(myname, (const char **)&ar[x])) < 0)
			{
			if( ! machine_readable )					/* A human gets english */
				gu_utf8_putline("Try \"help\" or \"exit\".");
			else										/* A program gets a code */
				gu_utf8_putline("*UNKNOWN");

			errorlevel = EXIT_SYNTAX;
			}
		else if(machine_readable)						/* If a program is reading our output, */
			{											/* say the command is done */
			gu_utf8_printf("*DONE\t%d\n ", errorlevel); /* and tell the exit code. */
			}

		if(machine_readable)					/* In machine readable mode output */
			fflush(stdout);						/* is probably a pipe which must be flushed. */
		} /* While not end of file */

	return errorlevel;					/* return result of last command (not counting exit) */
	} /* end of interactive_mode() */

static const char *option_chars = "Md:";
static const struct gu_getopt_opt option_words[] =
		{
		{"machine-readable", 'M', FALSE},
		{"debug", 'd', TRUE},
		{"help", 1000, FALSE},
		{"version", 1001, FALSE},
		{"user", 1002, TRUE},
		{(char*)NULL, 0, FALSE}
		} ;

/*
** Main function.
**
** This depends on the argv[] array being terminated by
** a NULL character pointer.
*/
int main(int argc, char *argv[])
	{
	int optchar;
	struct gu_getopt_state getopt_state;

	/* Initialize internation messages library. */
	gu_locale_init(argc, argv, PACKAGE, LOCALEDIR);

	umask(PPR_UMASK);		/* so config files come out with desired mode */

	/* Figure out the user's name and make it the initial value for --user. */
	{
	struct passwd *pw;
	uid_t uid = getuid();
	if((pw = getpwuid(uid)) == (struct passwd *)NULL)
		{
		gu_utf8_fprintf(stderr, "%s: getpwuid(%ld) failed, errno=%d (%s)\n", myname, (long)uid, errno, gu_strerror(errno));
		return EXIT_INTERNAL;
		}
	dispatch_set_user(NULL, pw->pw_name);
	}

	/*
	** Parse any dash options which appear before the subcommand.
	*/
	gu_getopt_init(&getopt_state, argc, argv, option_chars, option_words);
	while((optchar = ppr_getopt(&getopt_state)) != -1)
		{
		switch(optchar)
			{
			case 'M':							/* machine readable */
				machine_readable = TRUE;
				/* send error messages to stdout */
				/*stderr = stdout;*/
				dup2(1,2);
				break;

			case 'd':							/* debug */
				if(strspn(getopt_state.optarg, "0123456789") != strlen(getopt_state.optarg))
					{
					gu_utf8_fprintf(stderr, _("%s: debug level must be a positive integer\n"), myname);
					exit(EXIT_SYNTAX);
					}
				debug_level = atoi(getopt_state.optarg);
				break;

			case 1000:							/* --help */
				help();
				exit(EXIT_OK);

			case 1001:							/* --version */
				if(machine_readable)
					{
					gu_utf8_putline(SHORT_VERSION);
					}
				else
					{
					gu_utf8_putline(VERSION);
					gu_utf8_putline(COPYRIGHT);
					gu_utf8_putline(AUTHOR);
					}
				exit(EXIT_OK);

			case 1002:							/* --user */
				if(dispatch_set_user("ppad", getopt_state.optarg) == -1)
					{
					gu_utf8_fprintf(stderr, _("%s: you are not allowed to use --user\n"), myname);
					exit(EXIT_DENIED);
					}
				break;

			default:
				gu_getopt_default(myname, optchar, &getopt_state, stderr);
				exit(EXIT_SYNTAX);
				break;
			}
		}

	/*
	** If there is a command, dispatch it, otherwise
	** invoke interactive mode.
	*/
	if(getopt_state.optind < argc)
		return dispatch(myname, (const char **)&argv[getopt_state.optind]);
	else
		return interactive_mode();

	} /* end of main() */

/* end of file */

