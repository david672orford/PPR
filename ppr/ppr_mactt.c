/*
** mouse:~ppr/src/ppr/ppr_mactt.c
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
** The routines in this module handle the placing of Macintosh dual-mode
** fonts in the cache.  These routines are called from ppr_rcache.c.
**
** A Macintosh dual-mode font is a TrueType font which has been converted to
** PostScript.  A Macintosh computer converts a TrueType font to PostScript
** either by generating a type 42 font (a TrueType font encapsulated in a
** PostScript program) or a type 1 font which approximates the origional
** TrueType font.  Versions of the Macintosh LaserWriter driver prior to 8.0
** always generated a font which contained both types of font with code to
** execute only the one which was appropriate for the printer.  LaserWriter 8.x
** downloads only one version in order to save time.
**
** If one type of font is received it is placed in the cache, and this module
** examines it to determine which type it is and modifies the Unix file
** permissions for group and other execute in order to indicate whether it is a
** type 1 or type 42 font.
**
** If at a later date, the other version of the font is received, it is merged
** with the one already in the cache to make a pre-LaserWriter 8.0 style
** dual-mode font.
**
** The various parts of a Macintosh dual-mode font are delimited by comments.
** We use these comments in order to determine which portions are present
** and in order to merge fonts properly.
**
** The printer driver program, pprdrv, contains code to download only the
** necessary portions of a Macintosh dual-mode font.
*/

#include "config.h"
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

#include "ppr.h"
#include "ppr_exits.h"

/* In ppr_main.c */
extern int option_TrueTypeQuery;

/*
** When an incoming document contains a font which is already in the cache,
** this function is called to determine if we want the new one too in order
** to merge them into a Macintosh dual-mode font.
**
** If the mode indicates that this is a Macintosh TrueType font converted to
** PostScript but its flags indicate that we do not yet have both the type 42
** and type 1 parts and the value fed to the -Q switch (TrueTypeQuery)
** suggests that the font we are about to read may contain the missing part,
** then return TRUE.
*/
gu_boolean truetype_more_needed(int current_features)
	{
	#ifdef DEBUG_RESOURCES
	printf("truetype_more_needed(mode=%o) option_TrueTypeQuery=%d\n", current_features, option_TrueTypeQuery);
	#endif

	if(current_features & FONT_MACTRUETYPE )
		{
		if( !(current_features & FONT_TYPE_1) && option_TrueTypeQuery == TT_NONE )
			return TRUE;
		if( !(current_features & FONT_TYPE_42) && (option_TrueTypeQuery==TT_TYPE42 || option_TrueTypeQuery==TT_ACCEPT68K) )
			return TRUE;
		}

	return FALSE;
	} /* end of truetype_more_needed() */

/*
** Set the mode on the indicated cache file to indicate whether it is a
** TrueType font and whether it has type 42 and type 1 components.
**
** If this is an invalid PostScript version of a Macintosh TrueType font
** return -1.  If we return -1, the font will be discarded instead of
** being placed in the cache.  Note that if an attempt is made to print
** a file containing a font which is defective in this way, the document
** will not be printed or will not print correctly (depending on the ProofMode)
** unless ppr is invoked with -S false switch.  This is because if the
** font is stripped out and also discarded from the cache, it has been lost,
** there will be no way to include it in the document.
*/
int truetype_set_fontmode(const char filename[])
	{
	const char function[] = "truetype_set_fontmode";
	char fontline[256];
	mode_t mode;
	FILE *f;
	int beginsfnt_count = 0;

	#ifdef DEBUG_RESOURCES
	printf("truetype_set_fontmode(filename=\"%s\")\n",filename);
	#endif

	/* Start with a basic mode */
	mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

	/* Open the file */
	if((f = fopen(filename,"r")) == (FILE*)NULL)
		fatal(PPREXIT_OTHERERR, "%s(): can't open \"%s\", errno=%d (%s)", function, filename, errno, gu_strerror(errno));

	/* Read all the lines, and if certain ones crop up, set extra bits in mode */
	while(fgets(fontline, sizeof(fontline), f))
		{
		if(strncmp(fontline, "%beginsfnt", 10) == 0)
			{
			mode |= FONT_MODE_TYPE_42;
			mode |= FONT_MODE_MACTRUETYPE;
			beginsfnt_count++;
			}

		if(strncmp(fontline,"%beginType1",11)==0)
			{
			mode |= FONT_MODE_TYPE_1;
			mode |= FONT_MODE_MACTRUETYPE;
			}
		}

	/* Close the font file */
	fclose(f);

	/*
	** If it is a Macintosh font but it is invalid, return -1.
	** This will cause the font to be discarded.  The caller
	** is responsible for issuing a warning message.
	*/
	if(beginsfnt_count > 1 && ( beginsfnt_count < 2 || beginsfnt_count > 3 ))
		{
		#ifdef DEBUG_RESOURCES
		printf("truetype_set_fontmode(): invalid Mac font, beginsfnt_count=%d\n",beginsfnt_count);
		#endif
		return -1;
		}

	/* Change its mode */
	if(chmod(filename,mode) == -1)
		fatal(PPREXIT_OTHERERR, "%s(): chmod(\"%s\", %o) failed, errno=%d (%s)", function, filename, (unsigned)mode, errno, gu_strerror(errno));

	return 0;
	} /* end of truetype_set_fontmode() */

/*
** Attempt to combine the old and new Macintosh TrueType fonts to form
** one which has both type 1 and type 42 components.
**
** The file named by "old" is both a source file and the name for the
** final product.  The file named by "new" is a temporary file which
** we must delete before we return or exit.
*/
static void mactt_copy(char *line, int linelen, FILE *in, FILE *out, const char stop[], char *clean1, char *clean2)
	{
	int neq;

	do	{
		fputs(line,out);
		neq = strcmp(line,stop);
		if( fgets(line, linelen, in) == (char*)NULL )
			{
			unlink(clean1);
			unlink(clean2);
			fatal(PPREXIT_OTHERERR,"ppr_mactt.c: mactt_copy(): unexpected EOF while copying til \"%s\"",stop);
			}
		} while(neq);

	} /* end of _copy() */

static void mactt_discard(char *line, int len, FILE *in, const char stop[], char *clean1, char *clean2)
	{
	int neq;

	do	{
		neq = strcmp(line,stop);
		if( fgets(line, len, in) == (char*)NULL )
			{
			unlink(clean1);
			unlink(clean2);
			fatal(PPREXIT_OTHERERR,"ppr_mactt.c: mactt_discard(): unexpected EOF while discarding til \"%s\"",stop);
			}
		} while(neq);

	} /* end of _discard() */

void truetype_merge_fonts(char *fontname, char *oldfont, char *newfont)
	{
	const char function[] = "truetype_merge_fonts";
	char fname[MAX_PPR_PATH];
	FILE *f1, *f2, *out;
	char f1line[1024], f2line[1024];
	const mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH | FONT_MODE_MACTRUETYPE | FONT_MODE_TYPE_1 | FONT_MODE_TYPE_42;

	#ifdef DEBUG_RESOURCES
	printf("%s(oldfont=\"%s\", newfont=\"%s\")\n", function, oldfont, newfont);
	#endif

	/* Create a name for a temporary file to hold the merged font. */
	ppr_fnamef(fname, "%s.temp%ld", oldfont, (long int)getpid());

	/* Open the temporary file for output. */
	if((out = fopen(fname, "w")) == (FILE*)NULL)
		fatal(PPREXIT_OTHERERR,"%s(): can't open \"%s\" for write, errno=%d (%s)", function, fname, errno, gu_strerror(errno));

	/* Open the old font for input.	 (We will call this the first font.) */
	if((f1 = fopen(oldfont, "r")) == (FILE*)NULL)
		fatal(PPREXIT_OTHERERR,"%s(): can't open \"%s\" for read, errno=%d (%s)", function, oldfont, errno, gu_strerror(errno));

	/* Open the newly received font for input.	(We will call this the second font.) */
	if((f2 = fopen(newfont,"r")) == (FILE*)NULL)
		fatal(PPREXIT_OTHERERR, "%s(): can't open \"%s\" for read, errno=%d (%s)", function, newfont, errno, gu_strerror(errno));

	/*
	** Copy the first font up to, but not including, the
	** first "%begin" line.
	**
	** Since the first font is already in the cache and has
	** a mode which indicates that it is a Macintosh TrueType
	** font converted to PostScript, it had better have at least
	** one "%begin" line before EOF.  If it does not, the cache
	** is corrupt.  This should never happen, so we make it a
	** fatal error.
	*/
	while(TRUE)
		{
		if( fgets(f1line,sizeof(f1line),f1) == (char*)NULL )
			{
			unlink(fname);
			unlink(newfont);
			fatal(PPREXIT_OTHERERR, "%s(): cache file \"%s\" isn't a Mac TT font, corrupt or has wrong mode", function, oldfont);
			}

		if(strncmp(f1line, "%begin", 6) == 0)
			break;

		fputs(f1line,out);
		}

	/*
	** Discard lines from the second font up to the first "%begin" line.
	** Presumably, these lines will be identical in both fonts.
	**
	** If this font proves not to have any "%begin" lines then it is
	** not a Macintosh TrueType font converted to PostScript, abort the
	** merge operation.
	*/
	while(TRUE)
		{
		if(fgets(f2line,sizeof(f2line),f2) == (char*)NULL)
			{
			warning(WARNING_SEVERE, _("Merge into \"%s\" aborted because new font is not Mac TrueType"), oldfont);
			unlink(fname);
			unlink(newfont);
			return;
			}

		if(strncmp(f2line, "%begin", 6) == 0)
			break;
		}

	/*
	** If the first file has a "sfnt" section, copy it and
	** discard the same section from the second file if it has it too.
	**
	** Otherwise, copy from the second font.
	**
	** This is the section that defines the body of the type 42 font.
	*/
	if(strcmp(f1line,"%beginsfnt\n") == 0)
		{
		#ifdef DEBUG_RESOURCES
		printf("%s(): copying sfnt section from first font\n", function);
		#endif

		mactt_copy(f1line, sizeof(f1line), f1, out, " %endsfnt\n", fname, newfont);

		if(strcmp(f2line,"%beginsfnt\n") == 0)
			{
			#ifdef DEBUG_RESOURCES
			printf("%s(): discarding sfnt section from second font\n", function);
			#endif
			mactt_discard(f2line,sizeof(f2line),f2," %endsfnt\n",fname,newfont);
			}
		}
	else if(strcmp(f2line,"%beginsfnt\n") == 0)
		{
		#ifdef DEBUG_RESOURCES
		printf("%s(): copying sfnt section from second font\n", function);
		#endif
		mactt_copy(f2line, sizeof(f2line), f2, out, " %endsfnt\n", fname, newfont);
		}
	else
		{
		#ifdef DEBUG_RESOURCES
		printf("%s(): neither font has an sfnt section\n", function);
		#endif
		warning(WARNING_SEVERE, _("No sfnt section found while merging Mac font \"%s\", merger aborted"),fontname);
		unlink(fname);
		unlink(newfont);
		return;
		}

	/*
	** If the first file has a "sfntBC" section, copy it and
	** discard the same section from the second file if it has it too.
	**
	** Otherwise, copy from the second font.
	**
	** If neither font has it, generate one from `memory'.
	**
	** The "sfntBC" section is used by the 68K TrueType rasterizer.  The
	** macintosh will only download it if the answer to the "*TTRasterizer"
	** query is "Accept68K".
	*/
	if(strcmp(f1line, "%beginsfntBC\n") == 0)
		{
		#ifdef DEBUG_RESOURCES
		printf("truetype_merge_fonts(): copying sfntBC section from first font\n");
		#endif

		mactt_copy(f1line,sizeof(f1line),f1,out," %endsfntBC\n",fname,newfont);

		if( strcmp(f2line,"%beginsfntBC\n") == 0)
			{
			#ifdef DEBUG_RESOURCES
			printf("%s(): discarding sfntBC section from second font\n", function);
			#endif
			mactt_discard(f2line,sizeof(f2line),f2," %endsfntBC\n", fname, newfont);
			}
		}
	else if(strcmp(f2line, "%beginsfntBC\n") == 0)
		{
		#ifdef DEBUG_RESOURCES
		printf("%s(): copying sfntBC section from second font\n", function);
		#endif

		mactt_copy(f2line, sizeof(f2line), f2, out, " %endsfntBC\n", fname, newfont);
		}
	else
		{
		#ifdef DEBUG_RESOURCES
		printf("%s(): no sfntBC section in either font, no matter\n", function);
		#endif
		}

	/*
	** If the first file has a "sfntsdef" section, copy it and discard the
	** same section from the second file if it has it too.  Otherwise,
	** copy from the second font.
	*/
	if( strcmp(f1line,"%beginsfntdef\n") == 0 )
		{
		#ifdef DEBUG_RESOURCES
		printf("%s(): copying sfntdef section from first font\n", function);
		#endif

		mactt_copy(f1line,sizeof(f1line),f1,out," %endsfntdef\n",fname,newfont);

		if(strcmp(f2line,"%beginsfntdef\n") == 0)
			{
			#ifdef DEBUG_RESOURCES
			printf("%s(): discarding sfntdef section from second font\n", function);
			#endif
			mactt_discard(f2line, sizeof(f2line), f2, " %endsfntdef\n", fname, newfont);
			}
		}
	else if(strcmp(f2line,"%beginsfntdef\n") == 0)
		{
		#ifdef DEBUG_RESOURCES
		printf("%s(): copying sfntdef section from second font\n", function);
		#endif

		mactt_copy(f2line,sizeof(f2line), f2, out, " %endsfntdef\n", fname, newfont);
		}
	else
		{
		#ifdef DEBUG_RESOURCES
		printf("%s(): no sfntdef section in either font\n", function);
		#endif
		warning(WARNING_SEVERE, _("No sfntdef section found while merging Mac TT font \"%s\", merger aborted"), fontname);
		unlink(fname);
		unlink(newfont);
		return;
		}

	/*
	** If the first font has a "Type1" section, copy it and discard
	** any similiar section from the second file.
	**
	** Otherwise, try to copy it from the second font.
	*/
	if( strcmp(f1line,"%beginType1\n") == 0 )
		{
		#ifdef DEBUG_RESOURCES
		printf("%s(): copying Type1 section from first font\n", function);
		#endif

		mactt_copy(f1line, sizeof(f1line), f1, out, " %endType1\n", fname, newfont);

		if(strcmp(f2line, "%beginType1\n") == 0)
			{
			#ifdef DEBUG_RESOURCES
			printf("%s(): discarding Type1 section from second font\n", function);
			#endif
			mactt_discard(f2line, sizeof(f2line), f2, " %endType1\n", fname, newfont);
			}
		}
	else if(strcmp(f2line,"%beginType1\n") == 0)
		{
		#ifdef DEBUG_RESOURCES
		printf("%s(): copying Type1 section from second font\n", function);
		#endif
		mactt_copy(f2line,sizeof(f2line),f2,out," %endType1\n",fname,newfont);
		}
	else
		{
		#ifdef DEBUG_RESOURCES
		printf("%s(): neither font has a Type1 section\n", function);
		#endif
		unlink(fname);
		unlink(newfont);
		warning(WARNING_SEVERE, _("No Type 1 section found while merging Mac TT font \"%s\", merge aborted"), fontname);
		return;
		}

	/* Copy the tail of the first file. */
	do	{
		fputs(f1line,out);
		} while( fgets(f1line,sizeof(f1line),f1) != (char*)NULL );

	/* Close all of the open files. */
	fclose(out);
	fclose(f1);
	fclose(f2);

	/* Set the mode of the newly created font cache file. */
	if(chmod(fname,mode) < 0)
		fatal(PPREXIT_OTHERERR, _("%s(): chmod(\"%s\", %o) failed, errno=%d (%s)"), function, fname, (unsigned)mode, errno, gu_strerror(errno) );

	/* Remove the old font cache file. */
	if(unlink(oldfont) < 0)
		fatal(PPREXIT_OTHERERR, _("%s(): unlink(\"%s\") failed, errno=%d (%s)"), function, oldfont,errno,gu_strerror(errno));

	/* Move the new font cache file into place. */
	if(rename(fname,oldfont) < 0)
		fatal(PPREXIT_OTHERERR, _("%s(): rename(\"%s\", \"%s\") failed, errno=%d (%s)"), function, fname, oldfont, errno, gu_strerror(errno));

	/* Remove the temporary file which was the newly received font. */
	if(unlink(newfont) < 0)
		fatal(PPREXIT_OTHERERR, _("%s(): unlink(\"%s\") failed, errno=%d (%s)"), function, newfont, errno, gu_strerror(errno));
	} /* end of truetype_merge_fonts() */

/* end of file */
