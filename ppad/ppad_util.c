/*
** mouse:~ppr/src/ppad/ppad_util.c
** Copyright 1995--2004, Trinity College Computing Center.
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
** Last modified 3 February 2004.
*/

#include "before_system.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#ifdef HAVE_SPAWN
#include <spawn.h>
#endif
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "ppad.h"
#include "util_exits.h"

/*
** Run ppop to disable a printer before deleting or something like that.
** Return -1 if we fail, otherwise return ppop exit status.
*/
int ppop2(const char *parm1, const char *parm2)
	{
	#ifdef HAVE_SPAWN
	int fd;
	int saved_stdin;
	int saved_stdout;
	int saved_stderr;

	saved_stdin = dup(0);
	saved_stdout = dup(1);
	saved_stderr = dup(2);

	if((fd = open("/dev/null", O_RDWR)) < 0)
		fatal(EXIT_INTERNAL, "failed to open /dev/null");

	dup2(fd,0);
	dup2(fd,1);
	dup2(fd,2);
	if(fd > 2)
		close(fd);

	spawnl(PPOP_PATH, parm1, parm2, (char*)NULL);

	dup2(saved_stdin, 0);
	dup2(saved_stdout, 1);
	dup2(saved_stderr, 2);

	return 0;

	#else						/* use fork() and exec() */
	int wstat;

	switch(fork())
		{
		int fd;

		case -1:				/* fork() failed */
			return -1;
		case 0:					/* child */
			if((fd = open("/dev/null",O_RDWR)) < 0)
				fatal(EXIT_INTERNAL, "failed to open /dev/null");

			dup2(fd,0);
			dup2(fd,1);
			dup2(fd,2);
			if(fd > 2)
				close(fd);
			execl(PPOP_PATH, "ppop", parm1, parm2, (char*)NULL);
			_exit(255);
		default:				/* parent */
			if(wait(&wstat)==-1)		/* if wait failed */
				return -1;
			if(!WIFEXITED(wstat))		/* if ppop didn't exit normally */
				return -1;
			if(WEXITSTATUS(wstat)==255) /* if execl() probably failed */
				return -1;
			return WEXITSTATUS(wstat);
		}
	#endif						/* use spawn() or fork() and exec() */
	} /* end of ppop2() */

/*
** Write a string to the FIFO thru which we communicate
** with pprd.
*/
void write_fifo(const char string[], ... )
	{
	int fifo;
	FILE *FIFO;
	va_list va;

	/* open the fifo */
	#ifdef HAVE_MKFIFO
	if((fifo = open(FIFO_NAME, O_WRONLY | O_NONBLOCK)) < 0)
	#else
	if((fifo = open(FIFO_NAME, O_WRONLY | O_NONBLOCK)) < 0)
	#endif
		fatal(EXIT_NOSPOOLER, _("can't open FIFO, pprd is probably not running"));

	FIFO = fdopen(fifo, "w");

	va_start(va, string);
	vfprintf(FIFO, string, va);
	va_end(va);

	fclose(FIFO);
	} /* end of write_fifo() */

/*
** Make the text of a "Switchset:" line.
**
** This converts the argument vector passed to it into a compressed
** format for placing in the configuration file "Switchset:" line.
**
** Notice that if the first work is "none" then we generate an empty
** switchset.
*/
int make_switchset_line(char *line, const char *argv[])
	{
	int di = 0;

	/* If there are options available to make a new switchset, */
	if(argv[0] && gu_strcasecmp(argv[0], "none"))
		{
		/* process them one at a time. */
		int x;
		for(x=0; argv[x]; )
			{
			if(argv[x][0] != '-')				/* switch must begin with minus */
				 return -1;

			if(di)								/* if not first one, */
				 line[di++] = '|';				/* insert a separator */

			if(argv[x][1] == '-')				/* long option? */
				{
				int c, y;
				for(y=1; (c = argv[x][y]); y++)
					line[di++] = c;
				}
			else								/* short option */
				{
				line[di++] = argv[x][1];		/* copy switch char */

				if(argv[x][2] != '\0')			/* look for lack of separation after switch */
					return -1;
				}

			x++;

			/* If the switch has an argument, copy it too. */
			if(argv[x] && argv[x][0] != '-')
				{
				int c, y;

				if(argv[x-1][1] == '-')			/* if was long, */
					line[di++] = '=';			/* add separator */

				for(y=0; (c = argv[x][y]); y++)
					line[di++] = c;

				x++;
				}
			}
		}

	line[di] = '\0';
	return 0;
	} /* end of make_switchset_line() */

/*
** Print a switchset line in an inteligable form.
** In other words, it uncompresses a "Switchset:" line.
**
** This function is in this module becuase it is called
** by both ppad_printer.c and ppad_group.c.
*/
void print_switchset(char *switchset)
	{
	int x, count;				/* line index */
	char optchar;
	char *argument;				/* character's argument */
	int len;

	len = strlen(switchset);
	for(count=x=0; x < len; count++,x++)			/* move thru the line */
		{
		optchar = switchset[x++];					/* get an option character */

		argument = &switchset[x];					/* The stuff after the switch */
		argument[strcspn(argument, "|\n")] = '\0';	/* NULL terminate */

		if(count > 0)
			putchar(' ');

		/* if a long option, argument is actually option=argument */
		if(optchar == '-')
			{
			char *p;
			if((p = strchr(argument, '=')) && strcspn(p+1, " \t=*") != strlen(p+1))
				printf("--%.*s='%s'", (int)strcspn(argument, "="), argument, p+1);
			else
				printf("--%s", argument);
			}

		/* short option with no argument */
		else if(argument[0] == '\0')
			{
			printf("-%c", optchar);
			}

		/* short option with argument, */
		else
			{
			if(strcspn(argument, " \t=*") != strlen(argument))	/* if contains spaces, */
				printf("-%c '%s'", optchar, argument);			/* embed optarg in quotes */
			else												/* not spaces, */
				printf("-%c %s", optchar, argument);			/* no quotes */
			}

		x += strlen(argument);			/* move past this one */
		}								/* (x++ in for() will move past the NULL) */
	} /* end of print_switchset() */

/*
** Print some text.  Wrap the text to 80 columns.  Indent subsequent
** lines by 4 spaces.  Understand that start_column is the column
** the cursor is when this is called.  (0 is the first column.)
*/
int print_wrapped(const char *text, int starting_column)
	{
	int x;
	int opts_len;
	int out_len = starting_column;
	int word_len;

	opts_len = strlen(text);					/* determine total length of options */

	for(x=0; x < opts_len; x++)						/* while options remain */
		{
		word_len = strcspn(&text[x], " \t"); /* how long is this element? */

		if((out_len+word_len+1) >= 80)				/* If leading space and element */
			{										/* will not fit, */
			PUTS("\n    ");							/* start a new line. */
			out_len = 4 + word_len;
			}
		else										/* Otherwise, */
			{										/* add to this one */
			if(x) putchar(' ');
			out_len++;								/* a space */
			out_len += word_len;					/* and the element. */
			}

		while(word_len--)							/* Print the element. */
			{
			putchar(text[x++]);
			}
		}

	return out_len;
	} /* end of print_wrapped() */

/*
** Build a space separated list from the argument vector provided.
** Note that this will return a NULL pointer if argv[] has zero
** members or they are all empty strings.
*/
char *list_to_string(const char *argv[])
	{
	char *string = NULL;				/* pointer to dynamically allocated buffer */
	int string_len = 0;					/* space in buffer used so far */
	int string_space = 0;				/* currently allocated size of buffer */
	int x;

	for(x=0; argv[x]; x++)				/* loop through the strings to be concatentated */
		{
		int word_len, newlen;

		word_len = strlen(argv[x]);		/* how big is this 'word'? */

		if(word_len == 0)				/* ommit empty 'words' */
			continue;

		newlen = string_len + word_len;			/* how long with this new 'word'? */
		if(string_len)							/* if not the first 'word', allow room for a separating space */
			newlen++;

		if((newlen + 1) > string_space)			/* If necessary, enlarge the memory block */
			{
			string_space += 256;
			string = (char*)gu_realloc(string, string_space, sizeof(char));
			}

		if(string_len)							/* if not the first word, append a space first */
			string[string_len++] = ' ';
		strcpy(&string[string_len], argv[x]);	/* append the word */

		string_len = newlen;					/* adopt the new length */
		}

	/* Chop the memory block down to the size actually used. */
	if(string)
		string = (char*)gu_realloc(string, (string_len + 1), sizeof(char));

	return string;
	} /* end of list_to_string */

/*
 * This function takes a value of gu_exception_code and converts it to 
 * an exit code from util_exits.h.
 */
int exception_to_exitcode(int exception_code)
	{
	switch(exception_code)
		{
		case EEXIST:
			return EXIT_NOTFOUND;
		default:
			return EXIT_INTERNAL;
		}
	}

/* end of file */
