/*
** mouse:~ppr/src/ppad/dispatch.h
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 18 May 2006.
*/

int dispatch(const char myname[], const char *argv[]);
int dispatch_set_user(const char aclname[], const char new_username[]);
int dispatch_interactive(const char myname[], const char banner[], const char prompt[], gu_boolean machine_readable);

enum COMMAND_NODE_TYPE { COMMAND_NODE_BRANCH, COMMAND_NODE_LEAF };

struct COMMAND_ARG {
	const char *name;
	int flags;
	const char *description;
	};

struct COMMAND_NODE
	{
	const char *name;						/* command keyword */
	enum COMMAND_NODE_TYPE type;			/* command or branch */
	void *value;							/* arguments description or subcommand table */
	const char *description;				/* human-readable description */
	int (*function)(const char *argv[]);	/* command dispatch function */
	unsigned int helptopics;				/* bitmap of helptopics which include this command */
	char *acl;								/* ACL which controls access */
	};

struct COMMAND_HELP
	{
	const char *name;
	const char *description;
	};

/* root of the command dispatch table */
extern struct COMMAND_NODE commands[];
extern struct COMMAND_HELP commands_help_topics[];

/* end of file */
