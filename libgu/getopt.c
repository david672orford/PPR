/*
** mouse:~ppr/src/libppr/getopt.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 29 June 2000.
*/

/*
** This module implements a special version of getopt().  This getopt()
** can handle both POSIX-style switches and GNU-style long switches.
*/

#include "before_system.h"
#include <string.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

/* #include "global_structs.h" */

/*
** This function is called once at the start of argument processing.  The
** first argument is a pointer to a structure which will hold the state
** or processing between calls to ppr_getopt().
*/
void gu_getopt_init(struct gu_getopt_state *state, int argc, char *argv[], const char *opt_chars, const struct gu_getopt_opt *opt_words)
	{
	state->argc = argc;
	state->argv = argv;
	state->opt_chars = opt_chars;
	state->opt_words = opt_words;

	state->optind = 1;
	state->x = state->len = 0;
	state->name = (char*)NULL;
	state->putback = NULL;
	} /* end of getopt_init() */

/*
** Error returns:
** '?'	unrecognized option
** ':'	missing argument
** '!'	illegal aggretation
** '-'	spurious argument
*/
int ppr_getopt(struct gu_getopt_state *state)
	{
	char *p;
	size_t plen;
	int i;

	if(state->putback)
		{
		*(state->putback) = '=';
		state->putback = NULL;
		}

	/* If not in an old-style switch, look for the next one, */
	if(!state->x || state->x >= state->len)
		{
		if(state->optind >= state->argc)
			return -1;

		p = state->argv[state->optind];
		if(p[0] == '-' && p[1] != '-' && p[1] != '\0')
			{
			state->x = 1;
			state->len = strlen(p);
			state->optind++;
			}
		else
			{
			state->x = state->len = 0;
			}
		}

	/* If in an old-style switch now, */
	if(state->x)
		{
		int c;
		char *p2;

		/* Point to the switch: */
		p = state->argv[state->optind - 1];

		/* The switch character: */
		c = p[state->x++];

		/* Build the switch name in the scratch space: */
		snprintf(state->scratch, sizeof(state->scratch), "-%c", c);
		state->name = state->scratch;

		/* Look for the current character in the list: */
		if((p2 = strchr(state->opt_chars, c)))
			{
			if(p2[1] == ':')	/* if it requires an argument, */
				{
				/* Switches with arguments mustn't be grouped
				   and there must be a space before the argument. */
				if(state->x != 2 || p[state->x] != '\0')
					return '!';

				/* If none left, return a colon as an error indication, */
				if(state->optind >= state->argc)
					return ':';

				/* Take the next argument as the argument to this switch: */
				state->optarg = state->argv[state->optind++];
				}

			return c;
			}
		else					/* Unknown switch */
			{
			return '?';
			}
		}

	/* Out of arguments? */
	/* if(state->optind >= state->argc)
		return -1; */

	/* Need a handy pointer */
	p = state->argv[state->optind];

	/* Is it a non-option argument such as "filename" or "-"? */
	if(p[0] != '-' || p[1] != '-')
		return -1;

	/* We will be consuming this argument. */
	state->optind++;

	/* Breaker? */
	if(strcmp(p, "--") == 0)
		return -1;

	/* Look up long option: */
	plen = strcspn(p+2, "=");
	for(i=0; state->opt_words[i].name; i++)
		{
		if(plen == strlen(state->opt_words[i].name) && strncmp(p+2, state->opt_words[i].name, plen) == 0)
			{
			state->name = p;					/* The full name with the -- before it */

			/* If it requires an argument, */
			if(state->opt_words[i].needsarg)
				{
				if(p[2+plen] == '=')					/* If argument is in save argv[] member */
					{									/* separated by an equals sign, */
					state->putback = p + 2 + plen;
					*(state->putback) = '\0';			/* terminate name */

					state->optarg = p + 2 + plen + 1;	/* point past double hyphen, name, and equals sign */
					}
				else									/* if in next argv[], */
					{
					if(state->optind >= state->argc)	/* if there isn't one, */
						return ':';						/* protest. */

					state->optarg = state->argv[state->optind++];
					}
				}
			else								/* doesn't take an option */
				{
				if(p[2+plen] == '=')			/* If there is one, */
					return '-';					/* protest. */
				}

			/* Return the code number for this argument: */
			return state->opt_words[i].code;
			}
		}

	state->name = p;
	return '?';
	} /* end of ppr_getopt() */

/*
** This can be used for the default case of the switch()
** statment.
*/
void gu_getopt_default(const char myname[], int optchar, const struct gu_getopt_state *getopt_state, FILE *errors)
	{
	fprintf(errors, "%s: ", myname);

	switch(optchar)
		{
		case '?':					/* help or unrecognized switch */
			fprintf(errors, _("Unrecognized switch: %s\n\n"), getopt_state->name);
			break;

		case ':':					/* argument required */
			fprintf(errors, _("The %s option requires an argument.\n"), getopt_state->name);
			break;

		case '!':					/* bad aggreation */
			fprintf(errors, _("Switches, such as %s, which take an argument must stand alone.\n"), getopt_state->name);
			break;

		case '-':					/* spurious argument */
			fprintf(errors, _("The %s switch does not take an argument.\n"), getopt_state->name);
			break;

		default:					/* missing case */
			fprintf(errors, X_("Missing case %d in switch dispatch switch()\n"), optchar);
			break;
		}
	} /* end of gu_getopt_default() */

/* end of file */
