/*
** mouse:~ppr/src/ppr/ppr_simplify.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 23 May 2001.
*/

/*
** The line input routines for ppr are arranged in a hierarcy.	The higher
** level versions hide more of the unpleasant looking lines and take care
** of copying resources to cache files when necessary as well as possibly
** stripping out resources which we think we can later restore from the cache.
**
** The hierarcy looks like this:
**
** getline_simplify_cache() --> getline_simplify() --> in_getline()
**												   --> copy_data() --> in_getline();
**																   --> in_getc();
*/

#include "before_system.h"
#include <ctype.h>
#include <string.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "ppr.h"
#include "ppr_infile.h"
#include "ppr_gab.h"

int eof_comment_present = FALSE;
int rgrab = 0;							/* 0, 1 or 2 for grabing a resources */
int dsc_comment_number = 0;				/* Will be the same for 1st and cont. lines */

/* These are used to handle %%+ comment continuation. */
static char lastDSC[MAX_CONT + 1] = {0};
static gu_boolean is_continued_line;
static gu_boolean continuation_buffer_overflow;

/*
** Routine to copy an unrecognized comment in line[] to
** the output file.	 If the comment line is an expanded
** continuation line, compress it again.  This routine is
** called from ppr_dscdoc.c but is in here because it
** must `unsimplify' the comment.
*/
void copy_comment(FILE *out)
	{
	if(is_continued_line)				/* if was continuation */
		{								/* comment */
		char *ptr;						/* make it one again */

		ptr = line + strcspn(line, " \t");		/* skip `til space */
		ptr += strspn(ptr, " \t");				/* skip space */

		fprintf(out, "%%%%+ %s\n", ptr);
		}
	else								/* if not continuation comment, */
		{								/* just copy it to the */
		fprintf(out, "%s\n", line);		/* output file */
		}
	} /* end of copy_comment() */

/*
** Copy data between %%Begin(End)Data: or %%Begin(End)Binary: comments.
**
** This function is only called by getline_simplify().
** It is broken out to make the logic easier to follow.
**
** This function is called with a "%%BeginData:" line
** in line[].  When it returns, the "%%BeginData:" will
** have been copied to the output, possibly along with
** other lines.	 There will be a fresh line in the buffer.
** Hopefully, this fresh line will be an "%%EndData" line.
*/
static void copy_data(void)
	{
	int c;
	long int number, countdown;

	if(option_gab_mask & GAB_STRUCTURE_NESTING)
		printf("nesting: %s\n", line);

	/* Write the %%BeginData: comment to the output.  If we are caching
	   a resource now, then put in the cache file too. */
	fprintf(text, "%s\n", line);
	if(rgrab) fprintf(cache_file, "%s\n", line);

	/* Syntax check */
	if(!tokens[1] || !tokens[2] || !tokens[3])
		{
		warning(WARNING_SEVERE, _("Insufficent number of arguments on \"%%%%BeginData:\" line"));
		in_getline();
		return;
		}

	/* Read the number of items to be copied. */
	sscanf(tokens[1], "%ld", &number);
	countdown = number;

	/* Is the data measured in lines? */
	if(strcmp(tokens[3], "Lines") == 0)
		{
		/*
		** Read the required number of lines and send them to the "-text"
		** file and possibly to a cache file too.  Notice that we can
		** handle lines which are too long for line[].
		*/
		while(!in_eof() && countdown--)
			{
			in_getline();
			if(rgrab)
				{
				fwrite(line, sizeof(unsigned char), line_len, cache_file);
				if(!line_overflow) fputc('\n', cache_file);
				}
			fwrite(line, sizeof(unsigned char), line_len, text);
			if(line_overflow)
				countdown++;
			else
				fputc('\n', text);
			}

		/*
		** If the "%%BeginData:" comment was correct, then the corresponding
		** "%%EndData" comment should be on the next line.	This code isn't
		** precisely correct because it won't notice trailing garbage, but
		** that shouldn't be a problem.
		*/
		if(in_eof())
			{
			warning(WARNING_SEVERE, _("\"%%%%BeginData:\" block ran off end of file after %ld of %ld lines"), (number - countdown), number);
			}
		else
			{
			in_getline();
			if(strncmp(line, "%%EndData", 9) && strncmp(line, "%%EndBinary", 11))
				warning(WARNING_SEVERE, _("\"%%%%BeginData:\" said %ld lines, but no \"%%%%EndData\" found there"), number);
			}
		}

	/* Is the data measured in bytes? */
	else if(strcmp(tokens[3], "Bytes") == 0)
		{
		int x;
		/* Length of the string "%%End" */
		#define LENGTH_END 5
		char temp[LENGTH_END];
		long int countup = 0;

		/* Copy the required number of bytes. */
		if(rgrab)
			{
			while(countdown-- && (c = in_getc()) != EOF)
				{
				fputc(c, text);
				fputc(c, cache_file);
				countup++;
				}
			}
		else
			{
			while(countdown-- && (c = in_getc()) != EOF)
				{
				fputc(c, text);
				countup++;
				}
			}

		/* Read a 5 byte sample of what follows. */
		for(x=0; x < LENGTH_END; x++)
			{
			if((c = in_getc()) == EOF)
				{
				warning(WARNING_SEVERE, _("\"%%%%BeginData:\" block ran off end of file at byte %ld of %ld"), countup, number);
				break;
				}
			temp[x] = c;
			countup++;
			}

		/* If the "%%End" isn't found, keep copying until we see it. */
		if(c != EOF && memcmp(temp, "%%End", LENGTH_END))
			{
			warning(WARNING_SEVERE, _("\"%%%%BeginData:\" said %ld bytes, but no \"%%%%EndData\" found there"), number);

			do	{
				fputc(temp[0], text);
				if(rgrab) fputc(temp[0], cache_file);

				for(x=1; x < LENGTH_END; x++)
					temp[x - 1] = temp[x];

				if((c = in_getc()) == EOF)
					{
					warning(WARNING_SEVERE, _("Ran off end of file after %ld bytes while searching for \"%%%%End\""), countup);
					x--;
					break;
					}
				temp[LENGTH_END - 1] = c;
				countup++;
				} while(memcmp(temp, "%%End", LENGTH_END));


			}

		if(c != EOF)
			warning(WARNING_SEVERE, _("Found \"%%%%End\" after copying %ld additional bytes"), (countup - number));

		/* Push the sample back. */
		for(x--; x >= 0; x--)
			in_ungetc(temp[x]);

		/* Leave "%%EndData" in line[]. */
		in_getline();
		}

	/* Is the unit size unrecognized? */
	else
		{
		warning(WARNING_SEVERE, _("Unrecognized data type \"%s\" in \"%%%%BeginData:\""), tokens[3]);
		in_getline();
		}

	if(option_gab_mask & GAB_STRUCTURE_NESTING)
		printf("nesting: end of data?\n");
	} /* end of copy_data() */

/*
** This function is called by getline_simplify().
** It is broken out to make the logic easier to follow.
** It is called whenever a %%+ comment is encountered.	It
** tries to replace the "%%+" with a keyword saved from the
** last %%[A-Z] comment.  This function returns TRUE if it suceeds.
*/
static gu_boolean simplify_continuation(void)
	{
	/* If continuation buffer empty, */
	if(lastDSC[0] == '\0')
		{
		warning(WARNING_SEVERE, _("Attempt to continue non-existent DSC comment"));
		return FALSE;
		}
	/* If continuation buffer overflowed, */
	else if(continuation_buffer_overflow)
		{
		warning(WARNING_SEVERE, _("Continued DSC comment \"%s...\" overflows buffer"), lastDSC);
		return FALSE;
		}
	/* If line is in continuation buffer, */
	else
		{
		char tempstr[MAX_TOKENIZED + MAX_CONT + 1];
		int x = 3;
		while(isspace(line[x])) /* eat up space after %%+ */
			x++;
		line_len = snprintf(tempstr, sizeof(tempstr), "%%%%%s %s", lastDSC, &line[x]);
		strcpy(line, tempstr);	/* then expand to full comment */
		return TRUE;
		}
	} /* end of simplify_continuation() */

/*
** This function is called by getline_simplify().
** It is broken out to make the logic easier to follow.
** It is called whenever an %! comment is encountered.
*/
static void do_bang(void)
	{
	int len = line_len;
	while(len-- > 0 && isspace(line[len])) line_len = len;
	line[line_len] = '\0';
	tokenize();

	if(strncmp(line, "%!PS-Adobe-", 11) == 0)
		{
		/* Is this an EPS header in the middle of the document, */
		if(tokens[1] != (char*)NULL && strncmp(tokens[1], "EPSF-", 5) == 0
						&& nest_level() == 0
						&& outermost_current() != OUTERMOST_HEADER_COMMENTS)
			{
			/* Is the hack to handle this turned on? */
			if(qentry.opts.hacks & HACK_BADEPS)
				{
				char temp[80];

				warning(WARNING_PEEVE, _("Unenclosed EPS file, applying badeps hack"));
				line[79] = '\0';		/* guard against temp[] overflow */
				strcpy(temp, line);
				strcpy(line, "%%BeginDocument:\n");
				strcat(line, temp);
				line_len = strlen(line);
				nest_push(NEST_BADEPS, tokens[1]);
				}
			else
				{
				warning(WARNING_SEVERE, _("Unenclosed EPS file, try -H badeps\n"
						"\t(Expect additional semi-spurious warnings due to this problem.)"));
				}
			}
		}
	} /* end of do_bang() */

/*
** Get a line, handling included documents and marked
** data sections indicated by %%Begin(End)Data.
**
** This function is only called by getline_simplify_cache().
*/
static void getline_simplify(void)
	{
	/* Call the basic line input routine in ppr_infile.c. */
	in_getline();

	again:

	/* Set a core dump trap.  (Avoid masking certain errors.) */
	tokens[0] = tokens[1] = tokens[2] = NULL;

	/* If this is not a comment, get out quickly. */
	if(line[0] != '%')
		{
		lastDSC[0] = '\0';		/* clear comment continuation memory */
		return;
		}

	/* If this line is too long to be a valid DSC comment,
	   don't attempt to do anything with it. */
	if(line_len > MAX_TOKENIZED)
		return;

	/* If it is a %! (such as "%!PS-Adobe-3.0),
	   there is a subroutine to handle it. */
	if(line[1] == '!')
		{
		lastDSC[0] = '\0';
		do_bang();
		return;
		}

	/* If this is a %% comment, it is probably DSC, prepare to handle it. */
	if(line[1] == '%')			/* line[0] provent to be '%' */
		{
		int len = line_len;
		while(len-- > 0 && isspace(line[len])) line_len = len;
		line[line_len] = '\0';

		/* It is a DSC comment if the 3rd character is a capital letter. */
		if(isalpha(line[2]) && isupper(line[2]))
			{
			int len, colonpos;

			/* Is not a %%+ line. */
			is_continued_line = FALSE;
			dsc_comment_number++;

			len = strcspn(&line[2], " \t");
			colonpos = strcspn(&line[2], ":");

			/* See RBp 630. */
			if((len - colonpos) > 1)
				{
				len = colonpos + 1;
				warning(WARNING_PEEVE, _("Space missing after keyword \"%.*s\""), len + 2, line);
				}

			continuation_buffer_overflow = FALSE;
			if(len > MAX_CONT)
				{
				len = MAX_CONT;
				continuation_buffer_overflow = TRUE;
				}

			strncpy(lastDSC, &line[2], len);
			lastDSC[len] = '\0';
			} /* end of if comment begins with uppercase letter */

		/* Or is it a dsc comment continuation? */
		else if(line[2] == '+')
			{
			/* Replace the + with the saved keyword. */
			is_continued_line = simplify_continuation();
			}

		/* Must be some other kind of %% comment. */
		else
			{
			lastDSC[0] = '\0';
			tokenize();
			return;
			}

		/* Break line into words: */
		tokenize();

		/* Upgrade certain old DSC comments to DSC version 3.0 comments. */
		old_DSC_to_new();

		/*
		** This switch statment will act on certain comments which
		** have global significance.
		**
		** If an embedded document is marked by comments, simple
		** increment the nesting level at the start and decrement
		** it at the end.
		**
		** In the case of resources, we must operate the cache
		** machinery and we must make notes of those resources
		** which appear in the document.
		*/
		switch(line[2])
			{
			case 'B':
				if(strcmp(tokens[0], "%%BeginDocument:") == 0)
					{									/* begin document */
					nest_push(NEST_DOC, tokens[1]);		/* raises our parsing level */
					break;
					}
				if(strcmp(tokens[0], "%%BeginData:") == 0)
					{							/* Just copy enclosed data */
					copy_data();				/* (copy_data() takes care of caching) */
					goto again;
					}
				if(strcmp(tokens[0], "%%BeginResource:") == 0)
					{											/* Note the resource in the table */
					resource(REREF_REALLY_SUPPLIED, tokens[1], 2);
																/* (resource() must be ready for NULL resources) */
					nest_push(NEST_RES, tokens[2]);				/* this is a new level */
					if(nest_level() == 1)						/* if 1st level resource, */
						begin_resource();						/* tell the cache machinery */
					break;
					}
				break;

			case 'I':
				if(strcmp(tokens[0], "%%IncludeResource:") == 0)
					{
					resource(REREF_INCLUDE, tokens[1], 2);
					break;
					}
				break;

			case 'E':
				if(strcmp(line, "%%EndDocument") == 0)
					{							/* end document lowers it */
					if(nest_level() == 0)
						warning(WARNING_SEVERE, "\"%%%%EndDocument\" without \"%%%%BeginDocument:\"");
					else
						nest_pop(NEST_DOC);		/* assuming it was raised by */
					break;						/* BeginDocument */
					}
				if(strcmp(line, "%%EndResource") == 0)
					{							/* If end of resource, */
					if(nest_level() == 0)		/* If no level to end, */
						{						/* get very angry. */
						warning(WARNING_SEVERE, "\"%%%%EndResource\" without \"%%%%BeginResource\"");
						}
					else
						{
						if(nest_level() == 1)	/* If first level resource, */
							end_resource();		/* tell cache machinery. */
						nest_pop(NEST_RES);		/* Drop down a level. */
						}
					break;
					}
				if(strcmp(line, "%%EOF") == 0)
					{
					if(nest_level() == 0)				/* If it is the one which */
						{								/* ends the document, */
						eof_comment_present = TRUE;		/* set flag for -Z switch. */
						break;
						}
					/*
					** If we are in an EPS document which did not begin
					** with %%BeginDocument then add %%EndDocument.
					*/
					if( (qentry.opts.hacks & HACK_BADEPS) && nest_inermost_type() == NEST_BADEPS )
						{
						nest_pop(NEST_BADEPS);
						strcat(line, "\n%%EndDocument");
						break;
						}
					break;
					}
				break;
			} /* end of if comment begins with upper case */
		} /* end of if "%%" */

	/* It is a comment not defined in the DSC, but PPR might still
	   read it, just no processing here. */
	else
		{
		tokenize();
		}
	} /* end of getline_simplify() */

/*
** This function gets the next line of input, though %%Begin(End)Data
** and that which is between them are never returned because they are
** silently copied thru by getline_simplify().	This is the only
** function which calls getline_simplify().
*/
void getline_simplify_cache(void)
	{
	/*
	** Call lower level part.
	*/
	getline_simplify();

	/*
	** If rgrab is non-zero, stash the line in
	** the resource cache.
	*/
	if(rgrab)
		{
		if(rgrab == 2)	/* If 2nd or subsequent line, */
			{			/* (1st line is "%%BeginResource:".) */
						/* then write it to the cache file. */
			fwrite(line, sizeof(unsigned char), line_len, cache_file);

			if(!line_overflow)					/* If that was a whole line, */
				fputc('\n', cache_file);		/* then terminate it. */
			}
		else									/* If first line, */
			{
			rgrab = 2;							/* just note that we saw the 1st line. */
			}
		}
	} /* end of getline_simplify_cache() */

/*
** This is like getline_simplify_cache() above but it
** never returns a line that is part of a nested structure.
*/
void getline_simplify_cache_hide_nest(void)
	{
	getline_simplify_cache();
	while(!in_eof() && nest_level() > 0)
		{
		fwrite(line, sizeof(unsigned char), line_len, text);
		if(!line_overflow) fputc('\n', text);
		getline_simplify_cache();
		}
	} /* end of getline_simplify_cache_hidenest() */

/*
** This is like getline_simplify_cache_hidenest above but it
** never returns a line of PostScript code, only comments.
*/
void getline_simplify_cache_hide_nest_hide_ps(void)
	{
	getline_simplify_cache_hide_nest();
	while(!in_eof() && line[0] != '%')
		{
		fwrite(line, sizeof(unsigned char), line_len, text);
		if(!line_overflow) fputc('\n', text);
		getline_simplify_cache();
		}
	} /* end of getline_simplify_cache_hide_nest_hide_ps() */

/* end of file */

