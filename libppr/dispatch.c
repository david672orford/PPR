/*
** mouse:~ppr/src/ppad/dispatch.c
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
** Last modified 17 February 2006.
*/

#include "config.h"
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "util_exits.h"
#include "dispatch.h"

/*
 * Print a description of a commands arguments.
 */
static void help_describe(FILE *out, const char myname[], const char *argv[], int index, struct COMMAND_ARG *args_template)
	{
	int iii;
	gu_utf8_fprintf(out, _("Usage: %s"), myname);
	for(iii=0; iii < index; iii++)
		gu_utf8_fprintf(out, " %s", argv[iii]);
	for(iii=0; args_template[iii].name; iii++)
		gu_utf8_fprintf(out, (args_template[iii].flags & 1) ? " [<%s>]" : " <%s>", args_template[iii].name);
	gu_utf8_fprintf(out, "\n");
	for(iii=0; args_template[iii].name; iii++)
		gu_utf8_fprintf(out, "    %s -- %s\n", args_template[iii].name, args_template[iii].description);
	} /* help_describe() */

/*
 * The built-in command "help"
 */
static int help(const char myname[], const char *argv[])
	{
	struct COMMAND_NODE *cmd = commands;
	int iii;
	if(argv[0] && argv[1])
		{
		}
	else if(argv[0])
		{
		}
	else
		{
		gu_utf8_printf(_("Usage: %s help <command>\n"), myname);
		gu_utf8_puts(_("Help topics:\n"));
		for(iii=0; cmd[iii].name; iii++)
			{
			if(cmd[iii].type == COMMAND_NODE_BRANCH)
				gu_utf8_printf("    %s\n", cmd[iii].name);
			}
		}
	return 0;
	} /* command_help() */

/*
 * Called from dispatch(), this function attempts to execute the command function.
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
				help_describe(stderr, myname, argv, index, args_template);
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
				help_describe(stderr, myname, argv, index, args_template);
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

int dispatch(const char myname[], const char *argv[])
	{
	struct COMMAND_NODE *cmd = commands;
	int argv_index, table_index;

	if(!argv[0])
		{
		gu_utf8_fprintf(stderr, _("%s: no sub-command, try \"ppad help\"\n"), myname);
		return EXIT_SYNTAX;
		}

	if(gu_strcasecmp(argv[0], "help") == 0)
		return help(myname, &argv[1]);
	
	argv_index = table_index = 0;
	cmd=commands;
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
					return invoke_command(myname, argv, argv_index+1, &cmd[table_index]);
				}
			}
		table_index++;
		}
		
	/* command was not recognized, give help */
	if(argv_index == 0)				/* didn't get beyond first word */
		gu_utf8_fprintf(stderr, _("%s: unknown sub-command \"%s\", try \"ppad help\"\n"), myname, argv[0]);
	else if(table_index == 0)		/* sub table traversal didn't start. */
		gu_utf8_fprintf(stderr, _("%s: incomplete sub-command \"%s\", try \"ppad help %s\"\n"), myname, argv[0], argv[0]);
	else							/* ran off end of sub-table */
		gu_utf8_fprintf(stderr, _("%s: unknown sub-sub-command \"%s %s\", try \"ppad help %s\"\n"), myname, argv[0], argv[1], argv[0]);

	return EXIT_SYNTAX;
	} /* end of dispatch() */

/* end of file */
