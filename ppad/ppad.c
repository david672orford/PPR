/*
** mouse:~ppr/src/ppad/ppad.c
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
** Last modified 1 April 2005.
*/

/*
** Administration program for PostScript page printers.  This program
** edits the media database and edits printer and group configuration
** files.
*/

#include "config.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <errno.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "util_exits.h"
#include "ppad.h"
#include "version.h"

/* misc globals */
const char myname[] = "ppad";
int machine_readable = FALSE;
FILE *errors;
int debug_level = 0;
static char *su_user = NULL;

/*
** Handle fatal errors.
** Print a message and exit.
*/
void fatal(int exitval, const char *message, ...)
	{
	va_list va;

	va_start(va,message);
	fputs(_("Fatal: "), errors);
	vfprintf(errors,message,va);
	fputc('\n',errors);
	va_end(va);

	exit(exitval);
	} /* end of fatal() */

/*
** Is the user privileged?  In other words, is the user in the ppad
** access control list?  If the user identity has been changed
** (by the --su switch) since last time this function was called,
** the answer is found again, otherwise a cached answer is returned.
*/
static gu_boolean privileged(void)
	{
	static gu_boolean answer = FALSE;
	static char *answer_username = NULL;

	if(!answer_username || strcmp(su_user, answer_username))
		{
		if(answer_username) gu_free(answer_username);
		answer_username = gu_strdup(su_user);
		answer = user_acl_allows(su_user, "ppad");

		/* This breaks --su for user ppr! */
		#if 0
		/* This special exception comes into play when "make install"
		   is done by a non-root user.  Basically, says that if
		   we are not running setuid, we will let the file system
		   permissions take care of security.  This allows the
		   "ppad media put" command to work during "make install".
		   This is not really a new feature, it simply restores a
		   behavior of the old privileged() implementation.
		   */
		if(!answer && getuid() == geteuid())
		   answer = TRUE;
		#endif
		}

	return answer;
	} /* end of privileged() */

/*
** Set the user who should be considered to be running this program.
** Only privileged users may do this.  Thus, a privileged user may
** become a different privileged user or become an unprivileged user.
** Thus a privileged user can use this feature to drop privledge, but
** not to gain additional access.
**
** Generally, this will be used by servers running under privileged
** user identities.  They will use this so as not to exceed the privledge
** of the user for whom they are acting.
*/
static int su(const char username[])
	{
	if(privileged())
		{
		gu_free(su_user);
		su_user = gu_strdup(username);
		return 0;
		}
	else
		{
		return -1;
		}
	}

/*
** Return TRUE if the user has PPR administrative privledges.
** If not, print an error message and return FALSE.
*/
gu_boolean am_administrator(void)
	{
	if( privileged() )
		{
		return TRUE;
		}
	else
		{
		fputs("You are not allowed to perform the requested operation\n"
				"because you are not a PPR administrator.\n", errors);
		return FALSE;
		}
	} /* end of am_administrator() */

/*
** Print the help screen.
*/
static void help_media(FILE *out)
	{
	int i;
	const char *command_list[] =
		{
		N_("ppad media show <name>"),
		N_("ppad media put <name> <width> <length> <weight> <colour> <type> <banner>"),
		N_("ppad media delete <name>"),
		N_("ppad media export"),
		N_("ppad media import"),
		NULL
		};

	fputs(_("Media Management:\n"), out);
	for(i = 0; command_list[i]; i++)
		{
		const char *p = gettext(command_list[i]);
		fprintf(out, "    %s\n", p);
		}
	} /* end of help_media() */

static void help_printer(FILE *out)
	{
	int i;
    const char *command_list[] =
    	{
		N_("ppad show <printer>"),
		N_("ppad interface <printer> <interface> <address>"),
		N_("ppad delete <printer>"),
		N_("ppad comment <printer> <string>"),
		N_("ppad location <printer> <string>"),
		N_("ppad department <printer> <string>"),
		N_("ppad contact <printer> <string>"),
		N_("ppad options <printer> <quoted_list>"),
		N_("ppad jobbreak <printer> {none, signal,control-d, pjl, signal/pjl}"),
		N_("ppad feedback <printer> {True, False}"),
		N_("ppad codes <printer> {Clean7Bit, Clean8Bit, Binary, TBCP, UNKNOWN, DEFAULT}"),
		N_("ppad rip <printer> [<rip> <driver_output_language> [<rip_options>]]"),
		N_("ppad ppd <printer> <filename>"),
		N_("ppad ppdq <printer>"),
		N_("ppad alerts <printer> <frequency> <method> <address>"),
		N_("ppad frequency <printer> <integer>"),
		N_("ppad flags <printer> {never, no, yes, always} {never, no, yes, always}"),
		N_("ppad charge <printer> {<money>, none} [<money>]"),
		N_("ppad outputorder <printer> {Normal, Reverse, PPD}"),
		N_("ppad bins ppd <printer>"),
		N_("ppad bins add <printer> <bin>"),
		N_("ppad bins delete <printer> <bin>"),
		N_("ppad touch <printer>"),
		N_("ppad switchset <printer> <list>"),
		N_("ppad deffiltopts <printer>"),
		N_("ppad passthru <printer> <list>"),
		N_("ppad ppdopts <printer>"),
		N_("ppad limitpages <printer> <number> <number>"),
		N_("ppad limitkilobytes <printer> <number> <number>"),
		N_("ppad grayok <printer> <boolean>"),
		N_("ppad acls <name> <acl> ..."),
		N_("ppad userparams <name> <list>"),
		N_("ppad pagetimelimit <name> <value>"),
		N_("ppad addon <name> <value>"),
		NULL
    	};

	fputs(_("Printer Management:\n"), out);
	for(i = 0; command_list[i]; i++)
		{
		const char *p = gettext(command_list[i]);
		fprintf(out, "    %s\n", p);
		}
	} /* end of help_printer() */

static void help_group(FILE *out)
	{
	int i;
	const char *command_list[] =
		{
		N_("ppad group show <group>"),
		N_("ppad group members <group> <printer> ..."),
		N_("ppad group add <group> <printer>"),
		N_("ppad group remove <group> <printer>"),
		N_("ppad group delete <group>"),
		N_("ppad group comment <group> <comment>"),
		N_("ppad group rotate <group> {true, false}"),
		N_("ppad group touch <group>"),
		N_("ppad group switchset <printer> <list>"),
		N_("ppad group deffiltopts <group>"),
		N_("ppad group passthru <group> <list>"),
		N_("ppad group acls <name> <acl> ..."),
		N_("ppad group addon <name> <value>"),
		NULL
		};

	fputs(_("Group Management:\n"), out);
	for(i = 0; command_list[i]; i++)
		{
		const char *p = gettext(command_list[i]);
		fprintf(out, "    %s\n", p);
		}
	} /* end of help_group() */

static void help_alias(FILE *out)
	{
	int i;
	const char *command_list[] =
		{
		N_("ppad alias show <alias>"),
		N_("ppad alias forwhat <alias> <queuename>"),
		N_("ppad alias delete <alias>"),
		N_("ppad alias comment <alias> <comment>"),
		N_("ppad alias switchset <alias> <list>"),
		N_("ppad alias passthru <alias> <list>"),
		N_("ppad alias addon <name> <value>"),
		NULL
		};

	fputs(_("Alias Management:\n"), out);
	for(i = 0; command_list[i]; i++)
		{
		const char *p = gettext(command_list[i]);
		fprintf(out, "    %s\n", p);
		}
	} /* end of help_alias() */

static void help_ppdlib(FILE *out)
	{
	int i;
	const char *command_list[] =
		{
		N_("ppad ppdlib query <interface> <address> [<quoted_options_list>]"),
		N_("ppad ppdlib search <pattern>"),
		N_("ppad ppdlib get <name>"),
		NULL
		};

	fputs(_("PPD File Management:\n"), out);
	for(i = 0; command_list[i]; i++)
		{
		const char *p = gettext(command_list[i]);
		fprintf(out, "    %s\n", p);
		}
	
	} /* end of help_ppdlib() */

static void help_other(FILE *out)
	{
	fputs("New printer default:\n"
		"ppad new alerts <frequency> <method> <address>\n", out);

	fprintf(out, "Dispatch reminder email to user \"%s\":\n"
		"ppad remind\n", USER_PPR);
	} /* end of help_other() */

/*
** Print help information on switches.
*/
static void help(FILE *out)
	{
	int i;
	const char *switch_list[] =
		{
		N_("-M\tselect machine-readable output"),
		N_("--machine-readable\tsame as -M"),
		N_("-d <n>\tset debug level to <n>"),
		N_("--debug=<n>\tsame as -d"),
		N_("--version\tprint PPR version information"),
		N_("--help\tprint this help message"),
		NULL
		};
	const char *command_list[] =
		{
		N_("ppad help printer"),
		N_("ppad help group"),
		N_("ppad help alias"),
		N_("ppad help media"),
		N_("ppad help ppdlib"),
		N_("ppad help other"),
		NULL
		};

	fputs(_("Valid switches:\n"), out);
	for(i = 0; switch_list[i]; i++)
		{
		const char *p = gettext(switch_list[i]);
		int to_tab = strcspn(p, "\t");
		fprintf(out, "    %-20.*s %s\n", to_tab, p, p[to_tab] == '\t' ? &p[to_tab + 1] : "");
		}

	fputc('\n', out);

	fputs(_("Additional help topics:\n"), out);
	for(i = 0; command_list[i]; i++)
		{
		const char *p = gettext(command_list[i]);
		fprintf(out, "    %s\n", p);
		}

	fputc('\n', out);
	fprintf(out, _("The %s manpage may be viewed by entering this command at a shell prompt:\n"
		"    ppdoc %s\n"), "ppad(1)", "ppad");
	} /* end of help() */

/*
** Command dispatcher.
*/
static int dispatch(const char *argv[])
	{
	/* media commands */
	if(gu_strcasecmp(argv[0], "media") == 0 && argv[1])
		{
		if(gu_strcasecmp(argv[1], "show") == 0)
			return media_show(&argv[2]);
		if(gu_strcasecmp(argv[1], "put") == 0)
			return media_put(&argv[2]);
		if(gu_strcasecmp(argv[1], "delete") == 0)
			return media_delete(&argv[2]);
		if(gu_strcasecmp(argv[1], "export") == 0)
			return media_export();
		if(gu_strcasecmp(argv[1], "import") == 0)
			return media_import(&argv[2]);
		}

	/* new printer default */
	if(gu_strcasecmp(argv[0], "new") == 0 && argv[1])
		{
		if(gu_strcasecmp(argv[1], "alerts") == 0)
			return printer_new_alerts(&argv[2]);
		}

	/* Remind (nag) command. */
	if(gu_strcasecmp(argv[0], "remind") == 0)
		{
		write_fifo("n\n");
		return 0;
		}

	/* Help commands */
	if(gu_strcasecmp(argv[0], "help") == 0)
		{
		if(argv[1] == (char*)NULL)
			help(stdout);
		else if(gu_strcasecmp(argv[1], "printer") == 0)
			help_printer(stdout);
		else if(gu_strcasecmp(argv[1], "group") == 0)
			help_group(stdout);
		else if(gu_strcasecmp(argv[1], "alias") == 0)
			help_alias(stdout);
		else if(gu_strcasecmp(argv[1], "media") == 0)
			help_media(stdout);
		else if(gu_strcasecmp(argv[1], "ppdlib") == 0)
			help_ppdlib(stdout);
		else if(gu_strcasecmp(argv[1], "other") == 0)
			help_other(stdout);
		else
			{
			help(errors);
			return EXIT_SYNTAX;
			}
		return EXIT_OK;
		}

	/* alias commands */
	if(gu_strcasecmp(argv[0], "alias") == 0 && argv[1])
		{
		if(gu_strcasecmp(argv[1], "show") == 0)
			return alias_show(&argv[2]);
		if(gu_strcasecmp(argv[1], "forwhat") == 0)
			return alias_forwhat(&argv[2]);
		if(gu_strcasecmp(argv[1], "delete") == 0)
			return alias_delete(&argv[2]);
		if(gu_strcasecmp(argv[1], "comment") == 0)
			return alias_comment(&argv[2]);
		if(gu_strcasecmp(argv[1], "switchset") == 0)
			return alias_switchset(&argv[2]);
		if(gu_strcasecmp(argv[1], "passthru") == 0)
			return alias_passthru(&argv[2]);
		if(gu_strcasecmp(argv[1], "addon") == 0)
			return alias_addon(&argv[2]);
		}

	/* group commands */
	if(gu_strcasecmp(argv[0], "group") == 0 && argv[1])
		{
		if(gu_strcasecmp(argv[1], "show") == 0)
			return group_show(&argv[2]);
		if(gu_strcasecmp(argv[1], "comment") == 0)
			return group_comment(&argv[2]);
		if(gu_strcasecmp(argv[1], "rotate") == 0)
			return group_rotate(&argv[2]);
		if(gu_strcasecmp(argv[1], "members") == 0)
			return group_members_add(&argv[2], FALSE);
		if(gu_strcasecmp(argv[1], "add") == 0)
			return group_members_add(&argv[2], TRUE);
		if(gu_strcasecmp(argv[1], "remove") == 0)
			return group_remove(&argv[2]);
		if(gu_strcasecmp(argv[1], "delete") == 0)
			return group_delete(&argv[2]);
		if(gu_strcasecmp(argv[1], "touch") == 0)
			return group_touch(&argv[2]);
		if(gu_strcasecmp(argv[1], "switchset") == 0)
			return group_switchset(&argv[2]);
		if(gu_strcasecmp(argv[1], "deffiltopts") == 0)
			return group_deffiltopts(&argv[2]);
		if(gu_strcasecmp(argv[1], "passthru") == 0)
			return group_passthru(&argv[2]);
		if(gu_strcasecmp(argv[1], "acls") == 0)
			return group_acls(&argv[2]);
		if(gu_strcasecmp(argv[1], "addon") == 0)
			return group_addon(&argv[2]);
		}

	/* PPD library commands */
	if(gu_strcasecmp(argv[0], "ppdlib") == 0 && argv[1])
		{
		if(gu_strcasecmp(argv[1], "query") == 0)
			return ppdlib_query(&argv[2]);
		if(gu_strcasecmp(argv[1], "search") == 0)
			return ppdlib_search(&argv[2]);
		if(gu_strcasecmp(argv[1], "get") == 0)
			return ppdlib_get(&argv[2]);
		}

	/* printer commands */
	if(gu_strcasecmp(argv[0], "show") == 0)
		return printer_show(&argv[1]);
	if(gu_strcasecmp(argv[0], "comment") == 0)
		return printer_comment(&argv[1]);
	if(gu_strcasecmp(argv[0], "location") == 0)
		return printer_location(&argv[1]);
	if(gu_strcasecmp(argv[0], "department") == 0)
		return printer_department(&argv[1]);
	if(gu_strcasecmp(argv[0], "contact") == 0)
		return printer_contact(&argv[1]);
	if(gu_strcasecmp(argv[0], "interface") == 0)
		return printer_interface(&argv[1]);
	if(gu_strcasecmp(argv[0], "options") == 0)
		return printer_options(&argv[1]);
	if(gu_strcasecmp(argv[0], "jobbreak") == 0)
		return printer_jobbreak(&argv[1]);
	if(gu_strcasecmp(argv[0], "feedback") == 0)
		return printer_feedback(&argv[1]);
	if(gu_strcasecmp(argv[0], "codes") == 0)
		return printer_codes(&argv[1]);
	if(gu_strcasecmp(argv[0], "rip") == 0)
		return printer_rip(&argv[1]);
	if(gu_strcasecmp(argv[0], "ppd") == 0)
		return printer_ppd(&argv[1]);
	if(gu_strcasecmp(argv[0], "ppdq") == 0)
		return printer_ppdq(&argv[1]);
	if(gu_strcasecmp(argv[0], "alerts") == 0)
		return printer_alerts(&argv[1]);
	if(gu_strcasecmp(argv[0], "frequency") == 0)
		return printer_frequency(&argv[1]);
	if(gu_strcasecmp(argv[0], "flags") == 0)
		return printer_flags(&argv[1]);
	if(gu_strcasecmp(argv[0], "outputorder") == 0)
		return printer_outputorder(&argv[1]);
	if(gu_strcasecmp(argv[0], "charge") == 0)
		return printer_charge(&argv[1]);
	if((gu_strcasecmp(argv[0], "bins") == 0) && argv[1])
		{
		if(gu_strcasecmp(argv[1], "set") == 0)
			return printer_bins_set_or_add(FALSE, &argv[2]);
		if(gu_strcasecmp(argv[1], "add") == 0)
			return printer_bins_set_or_add(TRUE, &argv[2]);
		if(gu_strcasecmp(argv[1], "ppd") == 0)
			return printer_bins_ppd(&argv[2]);
		if(gu_strcasecmp(argv[1], "delete") == 0)
			return printer_bins_delete(&argv[2]);
		}
	if(gu_strcasecmp(argv[0], "delete") == 0)
		return printer_delete(&argv[1]);
	if(gu_strcasecmp(argv[0], "touch") == 0)
		return printer_touch(&argv[1]);
	if(gu_strcasecmp(argv[0], "switchset") == 0)
		return printer_switchset(&argv[1]);
	if(gu_strcasecmp(argv[0], "deffiltopts") == 0)
		return printer_deffiltopts(&argv[1]);
	if(gu_strcasecmp(argv[0], "passthru") == 0)
		return printer_passthru(&argv[1]);
	if(gu_strcasecmp(argv[0], "ppdopts") == 0)
		return printer_ppdopts(&argv[1]);
	if(gu_strcasecmp(argv[0], "limitpages") == 0)
		return printer_limitpages(&argv[1]);
	if(gu_strcasecmp(argv[0], "limitkilobytes") == 0)
		return printer_limitkilobytes(&argv[1]);
	if(gu_strcasecmp(argv[0], "grayok") == 0)
		return printer_grayok(&argv[1]);
	if(gu_strcasecmp(argv[0], "acls") == 0)
		return printer_acls(&argv[1]);
	if(gu_strcasecmp(argv[0], "userparams") == 0)
		return printer_userparams(&argv[1]);
	if(gu_strcasecmp(argv[0], "pagetimelimit") == 0)
		return printer_pagetimelimit(&argv[1]);
	if(gu_strcasecmp(argv[0], "addon") == 0)
		return printer_addon(&argv[1]);

	return -1;
	} /* end of dispatch() */

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
		puts(_("PPAD, Page Printer Administrator's utility"));
		puts(VERSION);
		puts(COPYRIGHT);
		puts(AUTHOR);
		puts("");
		puts(_("Type \"help\" for command list, \"exit\" to quit."));
		puts("");
		}
	else				/* terse, machine readable banner */
		{
		puts("*READY\t"VERSION);
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
				puts(X_("Warning: command buffer overflow!"));	/* temporary code, don't internationalize */
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
		if((errorlevel = dispatch((const char **)&ar[x])) == -1)
			{
			if( ! machine_readable )					/* A human gets english */
				puts("Try \"help\" or \"exit\".");
			else										/* A program gets a code */
				puts("*UNKNOWN");

			errorlevel = EXIT_SYNTAX;
			}
		else if(machine_readable)				/* If a program is reading our output, */
			{									/* say the command is done */
			printf("*DONE\t%d\n ", errorlevel); /* and tell the exit code. */
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
		{"su", 1002, TRUE},
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
	int retval;
	struct gu_getopt_state getopt_state;

	/* Initialize internation messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	errors = stderr;			/* set default */

	umask(PPR_UMASK);

	/* Figure out the user's name and make it the initial value for su_user. */
	{
	struct passwd *pw;
	uid_t uid = getuid();
	if((pw = getpwuid(uid)) == (struct passwd *)NULL)
		{
		fprintf(errors, "%s: getpwuid(%ld) failed, errno=%d (%s)\n", myname, (long)uid, errno, gu_strerror(errno));
		exit(EXIT_INTERNAL);
		}
	su_user = gu_strdup(pw->pw_name);
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
				errors = stdout;				/* send error messages to stdout */
				break;

			case 'd':							/* debug */
				debug_level = atoi(getopt_state.optarg);
				break;

			case 1000:							/* --help */
				help(stdout);
				exit(EXIT_OK);

			case 1001:							/* --version */
				if(machine_readable)
					{
					puts(SHORT_VERSION);
					}
				else
					{
					puts(VERSION);
					puts(COPYRIGHT);
					puts(AUTHOR);
					}
				exit(EXIT_OK);

			case 1002:							/* --su */
				su(getopt_state.optarg);
				break;

			default:
				gu_getopt_default(myname, optchar, &getopt_state, errors);
				exit(EXIT_SYNTAX);
				break;
			}
		}

	/*
	** If there is a command, dispatch it, otherwise
	** invoke interactive mode.
	*/
	if(getopt_state.optind < argc)
		retval = dispatch((const char **)&argv[getopt_state.optind]);
	else
		retval = interactive_mode();

	/* command was not recognized, give help */
	if(retval == -1)
		{
		fprintf(errors, _("%s: unknown sub-command \"%s\", try \"ppad help\"\n"), myname, argv[getopt_state.optind]);
		retval = EXIT_SYNTAX;
		}

	return retval;
	} /* end of main() */

/* end of file */

