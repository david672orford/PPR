/*
** mouse:~ppr/src/libppr/readppd.c
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
** Last modified 23 January 2004.
*/

/*+ \file

This module contains functions for opening and reading lines from PPD
files.  Includes are handled automatically.

*/

#include "before_system.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#ifdef HAVE_ZLIB
#include <zlib.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "util_exits.h"


/** given a PPD filename or product string, find the file and returns its full path
*/
char *ppd_find_file(const char ppdname[])
	{
	FILE *f;
	char *line = NULL;
	int line_len = 256;
	char *p;
	char *f_description, *f_filename;
	char *filename = NULL;
	
	if(ppdname[0] == '/')
		return gu_strdup(ppdname);

	if(!(f = fopen(PPD_INDEX, "r")))
		gu_Throw("Can't open \"%s\", errno=%d (%s)", PPD_INDEX, errno, gu_strerror(errno));

	while((p = line = gu_getline(line, &line_len, f)))
		{
		if((f_description = gu_strsep(&p,":"))
				&& (f_filename = gu_strsep(&p,":"))
			)
			{
			if(strcmp(f_description, ppdname) == 0)
				{
				filename = gu_strdup(f_filename);
				break;
				}
			}
		}

	if(line)
		gu_free(line);
	
	if(filename)
		return filename;

	gu_asprintf(&filename, "%s/%s", PPDDIR, ppdname);
	return filename;
	}

/** decode PPD file quoted string
 *
 * Take initial_segment and possibly subsequent lines readable with ppd_readline()
 * (until one of them ends with a quote) and assemble them into a PCS.
 */
void *ppd_finish_quoted_string(char *initial_segment)
	{
	char *p = initial_segment;
	gu_boolean end_quote = FALSE;
	int len;
	void *text = gu_pcs_new();

	do	{
		len = strlen(p);
		if(len > 0 && p[len-1] == '"')
			{
			p[len-1] = '\0';
			end_quote = TRUE;
			}
		if(gu_pcs_length(&text) > 0)
			gu_pcs_append_char(&text, '\n');
		gu_pcs_append_cstr(&text, p);
		} while(!end_quote && (p = ppd_readline()));
	return text;
	} /* end of ppd_finish_quoted_string() */

/** Decode PPD file QuotedValue
 *
 * Edit a string in place, decoding hexadecimal substrings.  The length of the
 * resulting string (which will never be longer) is returned.  QuotedValues with hexadecimal
 * substrings are described in the "PostScript Printer File Format Specification"
 * version 4.0 on page 5.
 */
int ppd_decode_QuotedValue(char *p)
	{
	int len;
	int si, di;

	len = strlen(p); 
	
	for(si=di=0; si < len; si++)
		{
		if(p[si] == '<')
			{
			int count = 0;
			int c, high_nibble = 0;

			si++;
			while((c = p[si]) && c != '>')
				{
				if(c >= '0' && c <= '9')
					c -= '0';
				else if(c >= 'a' && c <= 'f')
					c -= ('a' + 10);
				else if(c >= 'A' && c <= 'F')
					c -= ('A' + 10);
				else
					continue;

				if(count % 2 == 0)
					high_nibble = c;
				else
					p[di++] = (high_nibble << 4) + c;
				
				count++;
				}
			}
		else
			{
			p[di++] = p[si];
			}
		}
	
	return di;
	} /* end of ppd_decode_QuotedValue() */

/*============================================================================*/

static int nest;						/* current PPD nesting level */
static char *fname[MAX_PPD_NEST];		/* list of names of open PPD files */
static char *line = (char*)NULL;		/* storate for the current line */
static FILE *saved_errors = NULL;		/* STDIO file to send error messages to */
#ifdef HAVE_ZLIB
static gzFile f[MAX_PPD_NEST];
#else
static FILE *f[MAX_PPD_NEST];
#endif

/*
** This routine is called from ppd_open() and from ppd_readline()
** whenever an "*Include:" line is encountered.
*/
static int _ppd_open(const char name[])
	{
	const char function[] = "_ppd_open";

	if(++nest >= MAX_PPD_NEST)			/* are we too deep? */
		{
		if(saved_errors)
			{
			fprintf(saved_errors, "PPD files nested too deeply:\n"
				"\t\"%s\" included by:\n", name);
			}
		for(nest--; nest >= 0; nest--)
			{
			fclose(f[nest]);
			if(saved_errors)
				fprintf(saved_errors, "\t\"%s\"%s\n", fname[nest], nest ? " included by:" : "");
			gu_free(fname[nest]);
			}
		return EXIT_BADDEST;
		}

	/*
	** If the PPD file name begins with a slash, use it as is,
	** otherwise, prepend PPDDIR to it unless it is an included
	** file in which case we prepend the directory of the
	** including file.
	*/
	if(name[0] == '/')
		{
		fname[nest] = gu_strdup(name);
		}
	else
		{
		char *dirend;

		if(nest == 0)
			{
			if(saved_errors)
				fprintf(saved_errors, "%s(): assertion failed\n", function);
			return EXIT_INTERNAL;
			}

		/* Get the offset of the last "/" in the previous path.
		   This should never fail.  If it does it is an
		   internal error. */
		if(!(dirend = strrchr(fname[nest-1], '/')))
			{
			if(saved_errors)
				fprintf(saved_errors, "%s(): internal error\n", function);
			return EXIT_INTERNAL;
			}

		/* Build the path in malloced memory. */
		gu_asprintf(&fname[nest], "%.*s/%s", (dirend - fname[nest-1]), fname[nest-1], name);
		}

	/* Open the PPD file for reading. */
	#ifdef HAVE_ZLIB
	if(!(f[nest] = gzopen(fname[nest], "r")))
	#else
	if(!(f[nest] = fopen(fname[nest], "r")))
	#endif
		{
		if(saved_errors)
			fprintf(saved_errors, "PPD file \"%s\" does not exist.\n", fname[nest]);
		gu_free(fname[nest--]);
		for( ; nest >= 0; nest--)
			{
			fclose(f[nest]);
			if(saved_errors)
				fprintf(saved_errors, "\tincluded by: \"%s\"\n", fname[nest]);
			gu_free(fname[nest]);
			}
		return EXIT_BADDEST;
		}

	return EXIT_OK;
	} /* end of ppd_open() */

/** Open the indicated PPD file.

This function opens the indicated PPD file and sets up internal structures so that it can
be read by calling ppd_readline().  If we can't open it, print an error
message and return an appropriate exit code.

The errors parameter is a stdio file object to write error messages to.  If it is NULL,
none will be written.

On success, EXIT_OK is returned.

*/
int ppd_open(const char ppdname[], FILE *errors)
	{
	const char function[] = "ppd_open";
	char *filename;
	int retval;

	/*
	** These functions use static storage.  Only one instance
	** is allowed at a time.
	*/
	if(line)
		{
		if(errors)
			fprintf(errors, "%s(): already open\n", function);
		return EXIT_INTERNAL;
		}

	saved_errors = errors;		/* for use by _ppd_open() */
	nest = -1;

	filename = ppd_find_file(ppdname);
	if((retval = _ppd_open(filename)) == EXIT_OK)
		line = (char*)gu_alloc(MAX_PPD_LINE+2, sizeof(char));
	gu_free(filename);

	return retval;
	} /* end of ppd_open() */

/** Read the next line from the PPD file

Returns a pointer to the next line from the PPD file.  If you want to save anything in the line,
make a copy of it since it will be overwritten on the next call.
If we have reached the end
of the file, return (char*)NULL.
Comment lines are skipt and include files are transparently followed.

*/
char *ppd_readline(void)
	{
	int len;

	if(!line)							/* guard against usage error */
		return (char*)NULL;

	while(nest >= 0)
		{
		#ifdef HAVE_ZLIB
		while(!gzgets(f[nest], line, MAX_PPD_LINE+2))
		#else
		while(!fgets(line, MAX_PPD_LINE+2, f[nest]))
		#endif
			{
			#ifdef HAVE_ZLIB
			gzclose(f[nest]);
			#else
			fclose(f[nest]);
			#endif
			gu_free(fname[nest]);		/* free the stored file name */
			if(--nest < 0)				/* if we just closed the last file, */
				{
				gu_free(line);			/* free the line buffer */
				line = (char*)NULL;		/* leave a sign that there is no file open */
				return (char*)NULL;		/* and report end of file */
				}
			}

		/* If this is a comment line, skip it. */
		if(strncmp(line, "*%", 2) == 0)
			continue;

		/* Remove all trailing whitespace, including carriage return and line feed. */
		len = strlen(line);
		while(--len >= 0 && isspace(line[len]))
			line[len] = '\0';

		/* Skip blank lines. */
		if(line[0] == '\0')
			continue;

		/* If this is an "*Include:" line, open a new file. */
		if(strncmp(line, "*Include:", 9) == 0)
			{
			char *ptr;
			int ret;

			ptr = &line[9];
			ptr += strspn(ptr, " \t\"");				/* find name start */
			ptr[strcspn(ptr,"\"")] = '\0';				/* terminate name */

			if((ret = _ppd_open(ptr)))
				{
				gu_free(line);
				line = (char*)NULL;
				return (char*)NULL;
				}

			continue;
			}

		return line;
		}

	return (char*)NULL;
	} /* end of ppd_readline() */

/*============================================================================*/

struct PPDOBJ {
	int nest;						/* current nesting level */
	char *fname[MAX_PPD_NEST];		/* list of names of open PPD files */
	char line[MAX_PPD_LINE+2];		/* storage for the current line */
	#ifdef HAVE_ZLIB
	gzFile f[MAX_PPD_NEST];
	#else
	FILE *f[MAX_PPD_NEST];
	#endif
	};

static void ppdobj_open(struct PPDOBJ *self, const char ppdname[])
	{
	const char function[] = "ppdobj_open";

	if((self->nest + 1) >= MAX_PPD_NEST)			/* are we too deep? */
		gu_Throw("PPD files nested too deeply");

	if(nest < 0)			/* if first level, */
		{
		self->nest = -1;	/* <-- we are paranoid */
		self->fname[self->nest + 1] = ppd_find_file(ppdname);
		}
	else if(ppdname[0] == '/')
		{
		self->fname[self->nest +1] = gu_strdup(ppdname);
		}
	else					/* if an include file, */
		{
		char *dirend;

		/* We should always be able to find the final slash. */
		if(!(dirend = strrchr(self->fname[self->nest], '/')))
			gu_Throw("%s(): internal error\n", function);

		/* Build the path in malloced memory. */
		gu_asprintf(&(self->fname[self->nest + 1]), "%.*s/%s", (dirend - self->fname[self->nest]), self->fname[self->nest], ppdname);
		}

	self->nest++;

	/* Open the PPD file for reading. */
	#ifdef HAVE_ZLIB
	if(!(f[nest] = gzopen(self->fname[self->nest], "r")))
	#else
	if(!(f[nest] = fopen(self->fname[self->nest], "r")))
	#endif
		{
		gu_Throw("PPD file \"%s\" does not exist.\n", self->fname[self->nest]);
		}
	}

void *ppdobj_new(const char ppdname[])
	{
	struct PPDOBJ *self = gu_alloc(1, sizeof(struct PPDOBJ));
	self->nest = -1;
	gu_Try {
		ppdobj_open(self, ppdname);
		}
	gu_Catch {
		ppdobj_delete(self);
		gu_ReThrow();
		}
	return (void*)self;
	}

void ppdobj_delete(void *p)
	{
	struct PPDOBJ *self = p;

	/* close any files which may still be open */
	while(self->nest >= 0)
		{
		if(self->f[self->nest])		/* watch out for open failures! */
			{
			#ifdef HAVE_ZLIB
			gzclose(f[nest]);
			#else
			fclose(f[nest]);
			#endif
			}
		gu_free(self->fname[self->nest]);
		self->nest--;
		}

	gu_free(p);
	}

char *ppdobj_readline(void *p)
	{
	struct PPDOBJ *self = p;
	int len;
	char *ptr;

	while(self->nest >= 0)
		{
		#ifdef HAVE_ZLIB
		if(!gzgets(self->f[self->nest], self->line, MAX_PPD_LINE+2))
		#else
		if(!fgets(self->line, MAX_PPD_LINE+2, self->f[self->nest]))
		#endif
			{
			#ifdef HAVE_ZLIB
			gzclose(self->f[self->nest]);
			#else
			fclose(self->f[self->nest]);
			#endif
			gu_free(self->fname[self->nest]);		/* free the stored file name */

			self->nest--;
			continue;
			}

		/* If this is a comment line, skip it. */
		if(strncmp(self->line, "*%", 2) == 0)
			continue;

		/* Remove all trailing whitespace, including carriage return and line feed. */
		len = strlen(self->line);
		while(--len >= 0 && isspace(self->line[len]))
			self->line[len] = '\0';

		/* Skip blank lines. */
		if(self->line[0] == '\0')
			continue;

		/* If this is an "*Include:" line, open a new file. */
		if((ptr = lmatchp(self->line, "*Include:")))
			{
			ptr += strspn(ptr, "\"");				/* find name start */
			ptr[strcspn(ptr,"\"")] = '\0';			/* terminate name */

			ppdobj_open(self, ptr);
			continue;
			}

		return self->line;
		}

	return (char*)NULL;
	}

/* end of file */
