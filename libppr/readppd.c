/*
** mouse:~ppr/src/libppr/readppd.c
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
** Last modified 3 April 2006.
*/

/*+ \file

This module contains functions for opening and reading lines from PPD
files.  Includes are handled automatically.

*/

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#ifdef HAVE_ZLIB
#include <zlib.h>
#endif
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

#define MAX_PPD_NEST 10
#define MAX_PPD_LINE 256

struct PPDOBJ {
	int magic;
	int nest;						/* current nesting level */
	char *fname[MAX_PPD_NEST];		/* list of names of open PPD files */
	char line[MAX_PPD_LINE+2];		/* storage for the current line */
	#ifdef HAVE_ZLIB
	gzFile f[MAX_PPD_NEST];
	#else
	FILE *f[MAX_PPD_NEST];
	#endif
	};

/** given a PPD filename or product string, find the file and returns its full path
 *
 * If the return value is not NULL, it points to allocated memory.
*/
char *ppd_find_file(const char ppdname[])
	{
	FILE *f;
	char *line = NULL;
	int line_len = 256;
	char *p;
	char *f_description, *f_filename;
	char *filename = NULL;
	
	/* If it is an absolute path, our services aren't needed.  But return a copy of
	 * the name since the caller expects to receive a block of heap memory.
	 */
	if(ppdname[0] == '/')
		return gu_strdup(ppdname);

	if(!(f = fopen(PPD_INDEX, "r")))
		gu_Throw("can't open \"%s\", errno=%d (%s)", PPD_INDEX, errno, gu_strerror(errno));

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

	gu_free_if(line);
	fclose(f);

	/* If we found it in the PPD index, we are done. */	
	if(filename)
		return filename;

	/* If we reach here, fall back to the old assumption that the name 
	 * is the name of a file in /usr/share/ppr/ppd/.
	 */
	gu_asprintf(&filename, "%s/%s", PPDDIR, ppdname);
	return filename;
	} /* end of ppd_find_file() */

static void ppdobj_open(struct PPDOBJ *self, const char ppdname[])
	{
	const char function[] = "ppdobj_open";

	if((self->nest + 1) >= MAX_PPD_NEST)			/* are we too deep? */
		gu_Throw("PPD files nested too deeply (to %d levels)", self->nest);

	if(self->nest < 0)			/* if first level, */
		{
		self->nest = -1;		/* <-- we are paranoid */
		self->fname[self->nest + 1] = ppd_find_file(ppdname);
		}
	else if(ppdname[0] == '/')	/* include absolute path */
		{
		self->fname[self->nest + 1] = gu_strdup(ppdname);
		}
	else						/* include relative path */
		{
		char *ptr;

		/* We should always be able to find the final slash. */
		if(!(ptr = strrchr(self->fname[self->nest], '/')))
			gu_Throw("%s(): assertion failed\n", function);

		/* Build the path in malloced memory. */
		gu_asprintf(&(self->fname[self->nest + 1]), "%.*s/%s", (int)(ptr - self->fname[self->nest]), self->fname[self->nest], ppdname);
		}

	self->nest++;

	/* Open the PPD file for reading. */
	#ifdef HAVE_ZLIB
	if(!(self->f[self->nest] = gzopen(self->fname[self->nest], "r")))
	#else
	if(!(self->f[self->nest] = fopen(self->fname[self->nest], "r")))
	#endif
		{
		if(self->nest > 0)
			gu_Throw(_("can't open PPD file \"%s\" (included from \"%s\"), errno=%d (%s)"), self->fname[self->nest], self->fname[self->nest - 1], errno, gu_strerror(errno));
		else
			gu_Throw(_("can't open PPD file \"%s\", errno=%d (%s)"), self->fname[self->nest], errno, gu_strerror(errno));
		}
	} /* ppdobj_open() */

PPDOBJ ppdobj_new(const char ppdname[])
	{
	struct PPDOBJ *self = gu_alloc(1, sizeof(struct PPDOBJ));
	self->magic = 0x4210;
	self->nest = -1;
	gu_Try {
		ppdobj_open(self, ppdname);
		}
	gu_Catch {
		ppdobj_free(self);
		gu_ReThrow();
		}
	return self;
	}

void ppdobj_free(PPDOBJ self)
	{
	if(self->magic != 0x4210)
		gu_Throw("not a PPDOBJ");

	/* close any files which may still be open */
	while(self->nest >= 0)
		{
		if(self->f[self->nest])		/* watch out for open failures! */
			{
			#ifdef HAVE_ZLIB
			gzclose(self->f[self->nest]);
			#else
			fclose(self->f[self->nest]);
			#endif
			}
		gu_free(self->fname[self->nest]);
		self->nest--;
		}

	gu_free(self);
	}

char *ppdobj_readline(PPDOBJ self)
	{
	int len;
	char *ptr;

	if(self->magic != 0x4210)
		gu_Throw("not a PPDOBJ");

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
			ptr[strcspn(ptr, "\"")] = '\0';			/* terminate name */
			ppdobj_open(self, ptr);
			continue;
			}

		return self->line;
		}

	return (char*)NULL;
	} /* ppdobj_readline() */

/*============================================================================*/

/** read in a PPD file quoted string
 *
 * Take initial_segment and possibly subsequent lines readable with ppd_readline()
 * (until one of them ends with a quote) and assemble them into a PCS.
 */
void *ppd_finish_quoted_string(PPDOBJ self, char *initial_segment)
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
		} while(!end_quote && (p = ppdobj_readline(self)));
	return text;
	} /* end of ppd_finish_quoted_string() */

/** read in a PPD file QuotedValue and decode it
*/
char *ppd_finish_QuotedValue(PPDOBJ self, char *initial_segment)
	{
	void *pcs = ppd_finish_quoted_string(self, initial_segment);
	char *p = gu_pcs_get_editable_cstr(&pcs);
	gu_pcs_truncate(&pcs, ppd_decode_QuotedValue(p));
	return gu_pcs_free_keep_cstr(&pcs);
	}

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

/* end of file */
