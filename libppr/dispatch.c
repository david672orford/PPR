/*
** mouse:~ppr/src/ppad/dispatch.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 14 June 2006.
*/

#include "config.h"
#include <string.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "util_exits.h"
#include "dispatch.h"
#include "version.h"

static char *username = NULL;

/*
** Is the last user set with dispatch_set_user() listed in the specified ACL?
** Return TRUE if the operation is allowed.  The answer is cached.
*/
static gu_boolean acl_check(const char aclname[])
	{
	static gu_boolean answer = FALSE;
	static char *answer_username = NULL;
	static char *answer_aclname = NULL;

	if(!answer_username || strcmp(username, answer_username) || strcmp(aclname, answer_aclname))
		{
		gu_free_if(answer_username);
		gu_free_if(answer_aclname);
		answer_username = gu_strdup(username);
		answer_aclname = gu_strdup(aclname);
		answer = user_acl_allows(username, aclname);
		}

	return answer;
	} /* acl_check() */

/** set user for command access checks
 * Inform the command dispatch of the username of the current users
 * so that it can apply access checks.
 */
int dispatch_set_user(const char aclname[], const char new_username[])
	{
	/* If ACL checking is requested and the existing username
	 * isn't in the ACL, refuse. */
	if(aclname && !acl_check(aclname))
		return -1;

	/* It's ok.  Go ahead and accept the new username. */
	gu_free_if(username);
	username = gu_strdup(new_username);
	return 0;
	}

/*
 * Locate a command in the command dispatch table.
 */
static struct COMMAND_NODE *find_command(const char myname[], const char *argv[], int *num_words)
	{
	int argv_index, table_index;
	struct COMMAND_NODE *cmd = commands;

	argv_index = table_index = 0;
	while(cmd[table_index].name && argv[argv_index])
		{
		if(gu_strcasecmp(argv[argv_index], cmd[table_index].name) == 0)
			{
			switch(cmd[table_index].type)
				{
				case COMMAND_NODE_BRANCH:
					argv_index++;					/* move to next word */
					cmd = cmd[table_index].value;	/* take this branch */
					table_index = 0;				/* start at the start of this branch's table */
					continue;						/* skip table_index increment */
				case COMMAND_NODE_LEAF:
					if(num_words)
						*num_words = argv_index+1;
					return &cmd[table_index];
				}
			}
		table_index++;
		}
	
	if(argv_index == 0)				/* didn't get beyond first word */
		gu_utf8_fprintf(stderr, _("%s: unknown sub-command \"%s\"\n"), myname, argv[0]);
	else if(table_index == 0)		/* sub table traversal didn't start. */
		gu_utf8_fprintf(stderr, _("%s: incomplete sub-command \"%s\"\n"), myname, argv[0]);
	else							/* ran off end of sub-table */
		gu_utf8_fprintf(stderr, _("%s: unknown sub-sub-command \"%s %s\"\n"), myname, argv[0], argv[1]);

	return NULL;
	} /* find_command() */

/*
 * Print a description of a command's arguments.
 *  out -- file to print to (stdout for help, stderr when wrong number of parameters)
 *  myname -- name of this program
 *  argv[] -- command word list
 *  index -- length of argv[]
 *  args_template -- description of arguments
 */
static void help_describe_command(FILE *out, const char myname[], const char *argv[], int index, struct COMMAND_ARG *args_template)
	{
	int iii;
	gu_utf8_fprintf(out, _("Usage: %s"), myname);
	for(iii=0; iii < index; iii++)
		gu_utf8_fprintf(out, " %s", argv[iii]);
	for(iii=0; args_template[iii].name; iii++)
		gu_utf8_fprintf(out, (args_template[iii].flags & 1) ? " [<%s>]" : " <%s>", args_template[iii].name);
	gu_utf8_fprintf(out, "\n");
	for(iii=0; args_template[iii].name; iii++)
		gu_utf8_fprintf(out, "    <%s> -- %s\n", args_template[iii].name, args_template[iii].description);
	} /* help_describe_command() */

/*
 * Called from dispatch(), this function attempts to execute the command function.
 *  myname[]	-- name of this program
 *  argv[] -- arguments to this program
 *  index -- position in argv[] of first argument to subcommand
 *  cmd -- command table entry for subcomand to be invoked
 */
static int invoke_command(const char myname[], const char *argv[], int index, struct COMMAND_NODE *cmd)
	{
	int iii;
	gu_boolean repeat = FALSE;
	struct COMMAND_ARG *args_template = (struct COMMAND_ARG*)cmd->value;

	for(iii=0; TRUE; iii++)
		{
		if(!argv[index+iii])		/* if we ran out of arguments, */
			{
			/* If this one is not optional, */
			if(args_template[iii].name && (args_template[iii].flags & 1) == 0)
				{
				gu_utf8_fprintf(stderr, _("%s: too few arguments\n"), myname);
				help_describe_command(stderr, myname, argv, index, args_template);
				return EXIT_SYNTAX;
				}
			break;
			}
		/* if ran out of template, */
		if(!args_template[iii].name)
			{
			if(!repeat)
				{
				gu_utf8_fprintf(stderr, _("%s: too many arguments\n"), myname);
				help_describe_command(stderr, myname, argv, index, args_template);
				return EXIT_SYNTAX;
				}
			break;
			}
		if((args_template[iii].flags & 2) != 0)		/* if repeat, */
			{
			repeat = TRUE;	
			}
		}

	/* Looks good, lets dispatch! */
	return (cmd->function)(&argv[index]);
	} /* invoke_command() */

/*
 * Print a list of the commands which match a given "help topic".
 *  index -- position of help topic in commands_help_topics[]
 *           (-1 means show all)
 */
static void help_topic_display(int index)
	{
	struct {					/* stack of command tables that we are traversing */
		struct COMMAND_NODE *cmd;
		int table_index;
		} stack[10];
	int sp = 0;					/* stack pointer */

	if(index == -1)
		gu_utf8_printf(_("All commands:\n"));
	else
		gu_utf8_printf(_("Commands related to topic %s:\n"), commands_help_topics[index].name);
	
	stack[0].cmd = commands;	/* start with the root command table */
	stack[0].table_index = 0;
	sp = 0;
	while(TRUE)
		{
		if(!stack[sp].cmd[stack[sp].table_index].name)
			{
			sp--;
			stack[sp].table_index++;
			}
		if(sp < 0)
			break;
		switch(stack[sp].cmd[stack[sp].table_index].type)
			{
			case COMMAND_NODE_BRANCH:
				if(++sp == 10)
					gu_Throw("help_topics(): stack overflow");
				/* take this branch */
				stack[sp].cmd = stack[sp-1].cmd[stack[sp-1].table_index].value;
				stack[sp].table_index = 0;
				continue;					/* skip table_index increment */
			case COMMAND_NODE_LEAF:
				if(index == -1 || stack[sp].cmd[stack[sp].table_index].helptopics & (1<<index))
					{
					int iii;
					for(iii=0; iii <= sp; iii++)
						gu_utf8_printf(" %s", stack[iii].cmd[stack[iii].table_index].name);
					gu_utf8_printf(" -- %s\n", stack[sp].cmd[stack[sp].table_index].description);
					}		
				break;
			}
		stack[sp].table_index++;
		}
	} /* help_topic_display() */

/*
 * The built-in command "help"
 */
static int help(const char myname[], const char *argv[])
	{
	if(!argv[0])
		{
		int iii;
		gu_utf8_printf(_("Usage: %s help <topic>\n"), myname);
		gu_utf8_printf(_("       %s help <command>\n"), myname);
		gu_utf8_puts(_("Help topics:\n"));
		gu_utf8_printf(_("    %s -- %s\n"), _("all"), _("show all commands"));
		for(iii=0; commands_help_topics[iii].name; iii++)
			gu_utf8_printf(_("    %s -- %s\n"), _(commands_help_topics[iii].name), _(commands_help_topics[iii].description));
		return EXIT_OK;
		}

	if(gu_strcasecmp(argv[0], "all") == 0)
		{
		help_topic_display(-1);
		return EXIT_OK;
		}

	/* Look for a formal help topic */
	{
	int iii;
	for(iii=0; commands_help_topics[iii].name; iii++)
		{
		if(gu_strcasecmp(commands_help_topics[iii].name, argv[0]) == 0)
			{
			help_topic_display(iii);
			return EXIT_OK;
			}
		}
	}

	/* see if it is a command */
	{
	struct COMMAND_NODE *cmd;
	int num_words;
	if((cmd = find_command(myname, argv, &num_words)))
		{
		help_describe_command(stdout, myname, argv, num_words, (struct COMMAND_ARG*)cmd->value);
		return EXIT_OK;
		}
	}

	gu_utf8_fprintf(stderr, _("%s: no such help topic or command\n"), myname);
	return EXIT_NOTFOUND;
	} /* help() */

/** Dispatch command using dispatch table
 *
 * Dispatch the command in argv[] using the dispatch table in the global
 * variable commands[].
 *
 * Return -1 if the command was not found, the exit code if it was found
 * and run.
 */
int dispatch(const char myname[], const char *argv[])
	{
	struct COMMAND_NODE *cmd = commands;
	int num_words;

	if(!argv[0])
		{
		gu_utf8_fprintf(stderr, _("%s: no sub-command, try \"%s help\"\n"), myname, myname);
		return EXIT_SYNTAX;
		}

	if(gu_strcasecmp(argv[0], "help") == 0)
		return help(myname, &argv[1]);

	/* Look up the command in the dispatch table. */	
	if(!(cmd = find_command(myname, argv, &num_words)))
		return -1;

	if(cmd->acl && !acl_check(cmd->acl))
		{
		gu_utf8_fprintf(stderr, _("%s: permission denied because user \"%s\" is not in PPR ACL \"%s\"\n"), myname, username, cmd->acl);
		return EXIT_DENIED;
		}
	
	return invoke_command(myname, argv, num_words, cmd);
	} /* end of dispatch() */

/** Dispatch commands interactively
 * In interactive mode, we present a prompt, read command lines, break them
 * into words and dispatch them by passing them to dispatch().
 *
 * Return the result code of the last command executed.
*/
int dispatch_interactive(const char myname[], const char banner[], const char prompt[], gu_boolean machine_readable)
	{
	#define MAX_CMD_WORDS 64
	const char *ar[MAX_CMD_WORDS+1];	/* argument vector constructed from line[] */
	char *ptr;					/* used to parse arguments */
	unsigned int x;				/* used to parse arguments */
	int errorlevel = 0;			/* return value from last command */

	if( ! machine_readable )
		{
		gu_utf8_putline(banner);
		gu_utf8_putline(VERSION);
		gu_utf8_putline(COPYRIGHT);
		gu_utf8_putline(AUTHOR);
		gu_utf8_putline("");
		gu_utf8_putline(_("Type \"help\" for command list, \"exit\" to quit."));
		gu_utf8_putline("");
		}
	else				/* terse, machine readable banner */
		{
		gu_utf8_printf("*READY\t%s\n", SHORT_VERSION);
		fflush(stdout);
		}

	/*
	** Read input lines until end of file.
	*/
	while((ptr = ppr_get_command(prompt, machine_readable)))
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
		** If the line begins with the name of this command we will
		** increment x in order to skip it.
		*/
		x=0;
		if(ar[0] && strcmp(ar[0], myname) == 0)
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
		if((errorlevel = dispatch(myname, &ar[x])) < 0)
			{
			if(machine_readable)				/* A human gets english */
				gu_utf8_putline("*UNKNOWN");
			errorlevel = EXIT_SYNTAX;
			}
		else if(machine_readable)						/* If a program is reading our output, */
			{											/* say the command is done */
			gu_utf8_printf("*DONE\t%d\n", errorlevel);	/* and tell the exit code. */
			}

		if(machine_readable)					/* In machine readable mode output */
			fflush(stdout);						/* is probably a pipe which must be flushed. */
		} /* While not end of file */

	return errorlevel;					/* return result of last command (not counting exit) */
	} /* end of interactive_mode() */

/* end of file */
