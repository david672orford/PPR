/*
** mouse:~ppr/src/pprdrv/pprdrv_mactt.c
** Copyright 1995--2005, Trinity College Computing Center.
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
** Last modified 29 March 2005.
*/

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "interface.h"
#include "pprdrv.h"

/*
** send_font_mactt()
**
** Download only those portions of the Mac TrueType font which are necessary.
** What is necessary depends on whether the printer has a TTRasteriser and
** what type it is.
*/

static void _copy(const char fname[], char *tline, int tline_len, FILE *cache_file, char *stop)
	{
	int ne;

	do	{
		printer_puts(tline);
		ne = strcmp(tline,stop);
		if(!fgets(tline,tline_len,cache_file))
			fatal(EXIT_PRNERR_NORETRY, _("Dual-type font \"%s\" has no \"%.*s\"."), fname, (int)strcspn(stop,"\n"), stop);
		} while( ne );

	} /* end of _copy() */

static void _discard(const char fname[], char *tline, int tline_len, FILE *cache_file, char *stop)
	{
	int ne;

	do	{
		ne = strcmp(tline,stop);
		if(!fgets(tline, tline_len, cache_file))
			fatal(EXIT_PRNERR_NORETRY, _("Dual-type font \"%s\" has no \"%.*s\"."), fname, (int)strcspn(stop,"\n"), stop);
		} while( ne );

	} /* end of _discard() */

static void _skipblanklines(const char fname[], char *tline, int tline_len, FILE *cache_file)
	{
	while(tline[strspn(tline, " \t\r\n")] == '\0')
		{
		if(!fgets(tline, tline_len, cache_file))
			return;
		}
	} /* end of _skipblanklines() */

/*
 * Send the needed parts of the indicated font file to the printer.
 */
void send_font_mactt(const char filename[])
	{
	FILE *cache_file;
	char tline[2048];			/* seems generous since those samples examined where not longer than 256 */
	int type42_sent = FALSE;

	/* Open the cache file for read. */
	if((cache_file = fopen(filename, "r")) == (FILE*)NULL)
		fatal(EXIT_JOBERR, _("Can't open resource file \"%s\", errno=%d (%s)"), filename, errno, gu_strerror(errno));

	/* Copy up to the first "%begin" line. */
	while(fgets(tline,sizeof(tline),cache_file) && !lmatch(tline,"%begin") )
		printer_puts(tline);

	/* 
	 * If this is the "%beginsfnt" section, copy or discard it.
	 * (The sfnt section is the type 42 version of the font.)
	 */
	if(strcmp(tline,"%beginsfnt\n") == 0)
		{
		/* If a TrueType rasterizer was previously found to be available, */
		if(printer.type42_ok)
			{
			_copy(filename,tline,sizeof(tline),cache_file," %endsfnt\n");
			type42_sent = TRUE;
			}
		else
			{
			_discard(filename,tline,sizeof(tline),cache_file," %endsfnt\n");
			}

		_skipblanklines(filename, tline, sizeof(tline), cache_file);

		/*
		** If the TrueDict compatibility BuildChar procedure comes next
		** and we need it copy it.  If we don't need it (if we will not be
		** printing any TrueType fonts or the printer has a built-in TT
		** rasterizer), discard it.  If we need it and it is absent,
		** insert one from `memory'.
		*/
		if(strcmp(tline,"%beginsfntBC\n") == 0)
			{
			if(type42_sent && Features.TTRasterizer == TT_ACCEPT68K)
				_copy(filename,tline,sizeof(tline),cache_file," %endsfntBC\n");
			else
				_discard(filename,tline,sizeof(tline),cache_file," %endsfntBC\n");
			}
		else if(type42_sent && Features.TTRasterizer == TT_ACCEPT68K)
			{
			printer_puts(
				"%beginsfntBC\n"
				"truedictknown type42known not and ( %endsfntBC)exch fcheckload\n"
				"/TrueState 271 string def\n"
				"TrueDict begin sfnts save 72 0 matrix defaultmatrix dtransform dup mul\n"
				"exch dup mul add sqrt cvi 0 72 matrix defaultmatrix dtransform dup mul\n"
				"exch dup mul add sqrt cvi 3 -1 roll restore TrueState initer end\n"
				"/BuildChar{exch begin Encoding 1 index get CharStrings dup 2 index known\n"
				"{exch}{exch pop /.notdef}ifelse get dup xcheck{currentdict systemdict\n"
				"begin begin exec end end}{exch pop TrueDict begin /bander load cvlit\n"
				"exch TrueState render end}ifelse end} bind def\n"
				"\n %endsfntBC\n"
				);
			}

		_skipblanklines(filename, tline, sizeof(tline), cache_file);

		/* Sanity check, make sure the sfntdef section is next. */
		if(strcmp(tline, "%beginsfntdef\n"))
			fatal(EXIT_PRNERR_NORETRY, _("Dual-Type font \"%s\" has no sfntdef section."), filename);

		/* Copy or discard the "sfntdef" section */
		if(type42_sent)
			_copy(filename,tline,sizeof(tline),cache_file," %endsfntdef\n");
		else
			_discard(filename,tline,sizeof(tline),cache_file," %endsfntdef\n");
		}

	_skipblanklines(filename, tline, sizeof(tline), cache_file);

	/* If a type1 font comes next, copy or discard it. */
	if(strcmp(tline,"%beginType1\n") == 0)
		{
		if(! type42_sent)
			{
			printer_puts(tline);						/* Write out the comment line */
			fgets(tline,sizeof(tline),cache_file);		/* Load the test line */
			printer_printf("%%PPR %s",tline);			/* Write it commented out */
			fgets(tline,sizeof(tline),cache_file);		/* load next line */
			_copy(filename,tline,sizeof(tline),cache_file," %endType1\n");
			}
		else
			{
			_discard(filename,tline,sizeof(tline),cache_file," %endType1\n");
			}
		}

	/* Copy the tail end of the file */
	do	{
		printer_puts(tline);
		} while(fgets(tline,sizeof(tline),cache_file));

	/* close the cache file */
	fclose(cache_file);
	} /* end of send_font_mactt() */

/* end of file */

