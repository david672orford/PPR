/*
** mouse:~ppr/src/ppad/dispatch.h
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
** Last modified 22 February 2006.
*/

int dispatch(const char myname[], const char *argv[]);
int dispatch_set_user(const char aclname[], const char new_username[]);

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
