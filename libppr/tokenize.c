/*
** mouse:~ppr/src/libppr/tokenize.c
** Copyright 1995, 1999, Trinity College Computing Center
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 14 July 1999.
*/

#include "before_system.h"
#include "gu.h"
#include "global_defines.h"

/* The line buffer provided by our caller: */
extern char line[];
extern int line_len;

/* Our globals.	 The caller refers to tokens[] */
static char tokenized_line[MAX_TOKENIZED+2];	/* room for line broken into tokens */
char *tokens[MAX_TOKENS+1];						/* array of ptrs to tokens */
int tokens_count;

void tokenize(void)
	{
	int sptr, dptr, tptr;		/* source pointer, destionation ptr, token ptr */
	int qlevel;					/* quote level */
	int lastc = '\0';			/* the previous character */
	int windex;					/* chars stored for this word */
	#ifdef APPLE_QUOTE
	int alevel = 0;				/* Apple quote level */
	#endif

	if(line_len > MAX_TOKENIZED)
		libppr_throw(EXCEPTION_BADUSAGE, "tokenize", "line too long");

	windex = sptr = dptr = tptr = qlevel = 0;

	/* loop until end of line */
	while(line[sptr])
		{
		/* record this token pointer */
		tokens[tptr++] = &tokenized_line[dptr];

		/* copy this word */
		while(line[sptr] &&				/* if not end of line and */
				#ifdef APPLE_QUOTE
				( qlevel || alevel ||
				#else
				( qlevel ||
				#endif
					(line[sptr] != ' ' && line[sptr] != '\t') ) )
			{
			if(windex == 0 && line[sptr] == '(' && lastc != '\\')
				{						/* if PostScript opening quote, */
				if(++qlevel == 1)		/* delete opening "(" */
					{
					sptr++;
					continue;
					}
				}
			else if(qlevel && line[sptr] == ')' && lastc != '\\')
				{
				if(--qlevel == 0)		/* delete closing ")" */
					{
					sptr++;
					continue;
					}
				}

			#ifdef APPLE_QUOTE
			else if(qlevel == 0 && line[sptr] == '"')
				{
				if(windex == 0)					/* if 1st char */
					{
					alevel++;					/* raise Apple quote level */
					sptr++;						/* and discard the quote */
					continue;
					}
				else if( alevel )				/* if in Apple quote mode, */
					{
					alevel--;					/* lower Apple quote level */
					sptr++;						/* and discard quote mark */
					continue;
					}
				}
			#endif

			lastc = tokenized_line[dptr++] = line[sptr++];
			windex++;

			/* RBII page 630, space optional since common error. */
			if(tptr == 1 && lastc == ':')
				break;
			} /* this loop breaks at end of word */

		tokenized_line[dptr++] = '\0';			/* terminate extracted word */
		lastc = windex = 0;

		/* eat white space */
		while(line[sptr] == ' ' || line[sptr] == '\t')
			sptr++;						/* eat up whitespace */

		/* limit to size of tokens array */
		if(tptr == MAX_TOKENS)			/* if no more room, */
			break;						/* stop now */

		} /* this loop breaks at end of line */

	tokens_count = tptr;

	while(tptr < MAX_TOKENS)			/* make all remaining token pointers */
		tokens[tptr++] = (char*)NULL;	/* null pointers */
	}

/* end of file */

