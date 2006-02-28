/*
** mouse:~ppr/src/filter_lp/filter_lp.c
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
** Last modified 28 February 2006.
*/

/*
** This file is a line printer emulation filter for PPR.
*/

#include "config.h"
#include <string.h>
#include <unistd.h>
#include <memory.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <wchar.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "libppr_font.h"
#include "filter_lp.h"

/* The name of this program (for use in error messages) */
const char myname[] = "filter_lp";

/* Set to TRUE if the option "noisy=true" is used. */
int noisy = FALSE;

/* Is it OK to include character codes higher than 127 in the PostScript text? */
gu_boolean eighth_bit_ok = TRUE;

/* global input line buffer */
wchar_t *line;				/* the line itself */
unsigned char *line_attr;	/* bold and underline flags for each character */
int formfeed = FALSE;		/* true if line ended with ff */
int (*readline)(void);		/* pointer to function to read line */

/*
** These values, determined on pass 1, are the number of lines
** and spaces to clip from input text
*/
int top_skip = 1000;		/* minimum blank lines at top (1000=infinity) */
int left_skip = 1000;		/* minimum blanks at begining of line */

/*
** Should we go to a new line if we read a CR?
** This is set to TRUE if we are invoked as "filter_lp_autolf".
** This feature allows us to print Macintosh files.
*/
gu_boolean auto_lf = FALSE;

/*
** Here we find the vital statistics of the medium on which we will
** be printing.  The width and height are in PostScript Units and
** represent the width and height before any rotation for landscape.
**
** PageSize is the string which can be used in
** "%%IncludeFeature: *PageSize x" comments.
*/
char *PageSize;							/* "Letter" for US Letter Size */
double phys_pu_width;					/* 612 for letter */
double phys_pu_height;					/* 792 for letter */
double MediaWeight;						/* probably 75 */
const char *MediaColor;					/* probably "white" */
const char *MediaType;					/* probably "" */
gu_boolean should_specify_medium = FALSE;		/* TRUE if colour, type, or weight is set by option */

/*
** Virtual page dimensions after possible rotation for landscape.
** The units are PostScript units.
*/
double page_width;
double page_height;

/*
** Media and duplex selection stuff.
*/
#define DUPLEX_UNDEF 0
#define DUPLEX_NONE 1
#define DUPLEX_DUPLEX 2
#define DUPLEX_TUMBLE 3
int duplex_mode = DUPLEX_UNDEF;

/* These can be used to force portrait or landscape mode: */
int force_portrait = FALSE;
int force_landscape = FALSE;

/*
** The default minimum margins for portrait and landscape modes.
** All are expressed in PS units.  These may be modified by filter
** options "pmtm=", "pmbm=", etc.
*/
double PMTM = DEFAULT_PMTM;
double PMBM = DEFAULT_PMBM;
double PMLM = DEFAULT_PMLM;
double PMRM = DEFAULT_PMRM;
double LMTM = DEFAULT_LMTM;
double LMBM = DEFAULT_LMBM;
double LMLM = DEFAULT_LMLM;
double LMRM = DEFAULT_LMRM;
double gutter = DEFAULT_GUTTER; /* set by filter option "gutter=" */
double gutter_lr;				/* gutter at left and right */
double gutter_tb;				/* gutter at top and bottom */

/*
** The default number of lines for portrait and landscape.
** The default is used for files which do not contain form feeds.
*/
int pdeflines = DEFAULT_PDEFLINES;
int ldeflines = DEFAULT_LDEFLINES;

/*
** The minimum and maximum number of lines per page
*/
int MAX_LINES = DEFAULT_MAX_LINES;
int MIN_LINES = DEFAULT_MIN_LINES;

/*
** The maximum allowed line length and the minimum number
** of columns to provide on the page.
*/
int MAX_WIDTH = DEFAULT_MAX_WIDTH;
int MIN_COLUMNS = DEFAULT_MIN_COLUMNS;

/*
** A table of dimensions which may be altered by options.  The string
** is the name of the option, the pointer indicates the double
** precision floating point number in which it should store its value.
*/
struct {char *name; double *value;} setable_dimensions[] =
		{
		{ "pmtm", &PMTM },
		{ "pmbm", &PMBM },
		{ "pmlm", &PMLM },
		{ "pmrm", &PMRM },

		{ "lmtm", &PMLM },
		{ "lmbm", &LMBM },
		{ "lmlm", &LMLM },
		{ "lmrm", &LMRM },

		{ "gutter", &gutter },

		{ (char*)NULL, (double*)NULL }
		} ;

/*
** The two values which determine when we switch
** to landscape mode.
*/
int default_landscape_lentrigger;
double default_landscape_asptrigger;
int landscape_lentrigger = 0;
double landscape_asptrigger = 0.0;

/*
** The final margins, in PostScript units
*/
double left_margin;				/* left margin */
double right_margin;			/* right margin */
double top_margin;				/* top margin */
double bottom_margin;			/* !!! possibly unused !!! */

/*
**	Other options.
*/
double pointsize;				/* type size, PS units */
double line_spacing;			/* line spacing centre to centre, PS units */
int landscape;					/* the final determination TRUE or FALSE */
int lines_per_page;				/* the final value */
int TAB_WIDTH = DEFAULT_TAB_WIDTH;

/*
** The normal and bold fonts we will use.
**
** The "fontnormal=" and "fontbold=" options can change these.
** If they are not set manually, they are set automatically,
** with either "Courier" or "IBMCourier" being selected, depending
** on the required character set.
**
** Also, the character width for these fonts as a fraction of the
** height.  The default value is 0.60.
*/
char *font_family = "monospace";
struct FONT_INFO font_normal;
struct FONT_INFO font_bold;
double char_width = DEFAULT_CHAR_WIDTH;
double char_height = DEFAULT_CHAR_HEIGHT;

/* Some pass1 findings: */
int uses_normal = FALSE;				/* <-- never read */
int uses_bold = FALSE;
int uses_nonascii_normal = FALSE;
int uses_nonascii_bold = FALSE;

/* Set the default character set to use.  This is not part
   of the encoding name string, it is a key to be looked
   up in charsets.conf. */
const char *charset = "ISOLatin1";

/*
** Handle fatal errors.
** Print a message and exit.
*/
static void fatal(int exitval, const char message[], ... )
	{
	va_list va;
	va_start(va, message);
	fprintf(stderr, "%s: ", myname);
	vfprintf(stderr, message,va);
	fputc('\n', stderr);
	va_end(va);
	exit(exitval);
	} /* end of fatal() */

/*
** Print a message for non-fatal errors.  The library
** function charset_to_encoding() calls this.
*/
void error(const char message[], ... )
	{
	va_list va;
	va_start(va, message);
	fprintf(stderr, "%s: ", myname);
	vfprintf(stderr, message,va);
	fputc('\n', stderr);
	va_end(va);
	} /* end of error() */

/*
** This function reads a line from standard input.  The line is
** stored in line, its attributes in line_attr.  The length is
** returned.  On EOF, -1 is returned.  For form-feeds, 0 is
** returned in formfeed is TRUE.  If auto_lf is TRUE then
** CR is considered to be CRLF.
*/
static int readline_normal(void)
	{
	int count;							/* current possition in line */
	int maxcount;
	int c;
	int x;
	static int formfeed_pending = FALSE;

	formfeed = FALSE;					/* clear the global ff flag */

	if(formfeed_pending)
		{
		formfeed_pending = FALSE;
		formfeed = TRUE;
		return 0;
		}

	/* Clear the line buffer.  Using memset() instead of a loop
	 * cuts the program run time in half!
	 */
	memset(line, 0, (MAX_WIDTH+TAB_WIDTH+1) * sizeof(wchar_t));
	memset(line_attr, 0, MAX_WIDTH+TAB_WIDTH);

	maxcount = count = 0;						/* count is ptr into */
	while(TRUE)									/* line array */
		{
		if(count > maxcount) maxcount = count;

		if(count >= MAX_WIDTH)					/* break very long lines */
			break;

		switch(c = fgetc(stdin))				/* examine the character */
			{
			case EOF:							/* If physical end of file, */
				if(maxcount == 0)				/* if buffer empty, */
					return -1;					/* return -1 now. */
				goto break_break;				/* otherwise defer to next call. */
			case 0:								/* ignore NULLs in input */
				continue;
			case 8:								/* if backspace */
				count--;
				continue;
			case 9:								/* if tab */
				x=TAB_WIDTH-(count%TAB_WIDTH);	/* compute the number */
				while(x--)						/* of spaces we must advance */
					{
					if(line[count] == '\0')
						line[count] = ' ';
					count++;
					}
				continue;
			case 13:							/* If carriage return, */
				if(!auto_lf)					/* If auto line feed, */
					{							/* fall thru to lf. */
					count = 0;					/* otherwise, return to */
					continue;					/* far left column. */
					}
			case 10:						/* if line feed */
				goto break_break;			/* end of line */
			case 12:						/* if formfeed */
				if(count)					/* if it at the end of a line, */
					formfeed_pending=TRUE;	/* don't return flag until next call, */
				else						/* otherwise, */
					formfeed=TRUE;			/* set a flag now */
				goto break_break;			/* any case, consider it end of line */
			case '_':						/* if underscore */
				line_attr[count] |= ATTR_UNDERLINE;
				if(line[count] == '\0')			/* if underlined nothing, */
					line[count] = ' ';			/* make it a space */
				count++;
				continue;
			case ' ':							/* if space */
				if(line[count] == '\0')			/* spaces are non-destructive */
					line[count] = ' ';
				count++;
				continue;
			default:
				if(line[count] == c)					/* if already there, */
					line_attr[count++] |= ATTR_BOLD;	/* make it bold */
				else									/* otherwise, */
					line[count++] = c;					/* store the character */
				continue;
			}
		}
	break_break:

	/* Remove trailing spaces. */
	count = maxcount;
	while(count-- && line[count] == ' ' && line_attr[count] == 0)
		{
		line[count] = '\0';
		maxcount = count;
		}

	return maxcount;
	} /* end of readline_normal() */

/*
 * A UTF-* version of readline_normal().  Presently 
 * it differs from readline_normal() only in that it
 * calls fgetc_utf_8() instead of fgetc(), but that
 * must change to support BIDI and character composition.
 */
static int readline_utf_8(void)
	{
	int count;							/* current possition in line */
	int maxcount;
	int c;
	int x;
	static int formfeed_pending = FALSE;

	formfeed = FALSE;					/* clear the global ff flag */

	if(formfeed_pending)
		{
		formfeed_pending = FALSE;
		formfeed = TRUE;
		return 0;
		}

	/* Clear the line buffer.  Using memset() instead of a loop
	 * cuts the program run time in half!
	 */
	memset(line, 0, (MAX_WIDTH+TAB_WIDTH+1) * sizeof(wchar_t));
	memset(line_attr, 0, MAX_WIDTH+TAB_WIDTH);

	maxcount = count = 0;						/* count is ptr into */
	while(TRUE)									/* line array */
		{
		if(count > maxcount) maxcount = count;

		if(count >= MAX_WIDTH)					/* break very long lines */
			break;

		switch(c = gu_utf8_fgetwc(stdin))		/* examine the character */
			{
			case EOF:							/* If physical end of file, */
				if(maxcount == 0)				/* if buffer empty, */
					return -1;					/* return -1 now. */
				goto break_break;				/* otherwise defer to next call. */
			case 0:								/* ignore NULLs in input */
				continue;
			case 8:								/* if backspace */
				count--;
				continue;
			case 9:								/* if tab */
				x=TAB_WIDTH-(count%TAB_WIDTH);	/* compute the number */
				while(x--)						/* of spaces we must advance */
					{
					if(line[count] == '\0')
						line[count] = ' ';
					count++;
					}
				continue;
			case 13:							/* If carriage return, */
				if(!auto_lf)					/* If auto line feed, */
					{							/* fall thru to lf. */
					count = 0;					/* otherwise, return to */
					continue;					/* far left column. */
					}
			case 10:						/* if line feed */
				goto break_break;			/* end of line */
			case 12:						/* if formfeed */
				if(count)					/* if it at the end of a line, */
					formfeed_pending=TRUE;	/* don't return flag until next call, */
				else						/* otherwise, */
					formfeed=TRUE;			/* set a flag now */
				goto break_break;			/* any case, consider it end of line */
			case '_':						/* if underscore */
				line_attr[count] |= ATTR_UNDERLINE;
				if(line[count] == '\0')			/* if underlined nothing, */
					line[count] = ' ';			/* make it a space */
				count++;
				continue;
			case ' ':							/* if space */
				if(line[count] == '\0')			/* spaces are non-destructive */
					line[count] = ' ';
				count++;
				continue;
			default:
				if(line[count] == c)					/* if already there, */
					line_attr[count++] |= ATTR_BOLD;	/* make it bold */
				else									/* otherwise, */
					line[count++] = c;					/* store the character */
				continue;
			}
		}
	break_break:

	/* Remove trailing spaces. */
	count = maxcount;
	while(count-- && line[count] == ' ' && line_attr[count] == 0)
		{
		line[count] = '\0';
		maxcount = count;
		}

	return maxcount;
	} /* end of readline_utf_8() */

/*
** When in FORTRAN carriage control mode, this function
** is used instead of readline_normal().
*/
static int readline_fortran(void)
	{
	int c
		#ifdef GNUC_HAPPY
		= 0
		#endif
		;
	static int column = 0;		/* used to tell when to look for control byte */
	static int linenum = 0;		/* used to suppress initial line page break */
	int len;

	formfeed = FALSE;

	if(column++ == 0)
		{
		switch(c = fgetc(stdin))
			{
			case EOF:					/* most likely place to catch EOF */
				column = linenum = 0;
				return -1;

			case ' ':					/* space does nothing */
				break;

			case '1':					/* 1 starts new page */
				if(linenum > 0)
					{
					formfeed = TRUE;
					return 0;
					}
				break;

			default:
				break;
			}
		}

	memset(line, 0, (MAX_WIDTH+TAB_WIDTH+1) * sizeof(wchar_t));
	memset(line_attr, 0, MAX_WIDTH+TAB_WIDTH);

	for(len=0; len < MAX_WIDTH && (c = fgetc(stdin)) != EOF && c != '\n'; column++)
		{
		switch(c)
			{
			case '\r':			/* ignore carriage returns */
				break;

			case '\t':			/* if tab, move to next tab stop */
				{
				int x = TAB_WIDTH - (len % TAB_WIDTH);
				while(x--)
					line[len++] = ' ';
				}
				break;

			default:			/* just copy other characters */
				line[len++] = c;
				break;
			}
		}

	if(c == '\n')
		column = 0;

	/* Remove trailing spaces and terminate line. */
	while(len > 0 && line[len-1] == ' ')
		len--;
	line[len] = '\0';

	linenum++;
	return len;
	} /* end of readline_fortran() */

/*=======================================================================
** Make the first pass through the input file.
** On this pass we will determine the length of the longest line and
** the longest page and how big the built in left and top margins
** are if they exist.
=======================================================================*/
static void pass1(void)
	{
	int max_plen = 0;			/* maximum page length */
	int max_len = 0;			/* maximim line length */
	double min_tm, min_bm, min_lm, min_rm;
	int default_lines_per_page;

	int line_count = 0;			/* lines so far on this page */
	int in_body = FALSE;		/* TRUE if beyond leading blank lines */
	int has_formfeeds = FALSE;	/* uses formfeeds */

	int len;					/* length of this line */
	int leadlen;				/* length of leading space, this line */

	int columns;				/* number of columns, used in calculations */

	int x, c;

	const wchar_t one_space[] = {' ', '\0'};

	while((len = (*readline)()) >= 0)
		{
		line_count++;			/* count lines since start of doc or last formfeed */

		if(len > max_len)		/* keep max line length */
			max_len = len;

		if(len > 0)								/* if non-blank line */
			{									/* and has less */
			leadlen = wcsspn(line, one_space);	/* leading space than previous, */
			if(leadlen < left_skip)				/* then record this small leading */
				left_skip = leadlen;			/* space value */

			if(!in_body)
				{
				int this_top_skip = line_count - 1;
				if(this_top_skip < top_skip)
					top_skip = this_top_skip;
				in_body = TRUE;
				}
			}

		/* Scan line to see which fonts will be needed. */
		for(x=0; x < len; x++)
			{
			c=line[x];
			if(line_attr[x] & ATTR_BOLD)
				{
				uses_bold = TRUE;

				if(c < ' ' || c > '~')
					uses_nonascii_bold = TRUE;
				}
			else
				{
				uses_normal = TRUE;

				if(c < ' ' || c > '~')
					uses_nonascii_normal = TRUE;
				}
			}

		if(formfeed)					/* if formfeed, look back on page */
			{							/* (formfeed is a global.) */
			has_formfeeds = TRUE;
			line_count--;				/* a formfeed isn't a real line */

			if(line_count > max_plen)	/* did we set a new record for */
				max_plen = line_count;	/* page length? */

			line_count = 0;				/* reset line count */
			in_body = 0;				/* new page, not in body */
			}
		} /* end of while() loop */

	if(line_count > max_plen)			/* do this for the */
		max_plen = line_count;			/* final page even if no FF */

	if(left_skip == 1000)				/* in case document has */
		left_skip = 0;					/* no non-blank lines */

	/*-----------------------------------------------------------------
	** Now that we have gathered this information, use it to set the
	** printer options.
	-----------------------------------------------------------------*/

	/* Printed columns is length of longest line minus the smalllest leading whitespace. */
	columns = max_len - left_skip;

	/* Compute printable columns to provide, subject to minimum requirment. */
	/* columns = (columns - (2 * left_skip)) > MIN_COLUMNS ? columns : (MIN_COLUMNS - (2 * left_skip)); */
	columns = (columns + (2 * left_skip)) > MIN_COLUMNS ? columns : (MIN_COLUMNS - (2 * left_skip));

	/*
	** If size of longest page is known, set printable lines per page
	** to it, otherwise, set to zero so default will be used.
	*/
	if(!has_formfeeds || (max_plen - top_skip) > MAX_LINES)
		{						/* If no detectable page breaks or way too long, */
		lines_per_page = 0;		/* defer selection of default size. */
		top_skip = 0;
		}
	else						/* If page breaks, use document */
		{						/* lines or minimun lines. */
		lines_per_page = max_plen-top_skip>MIN_LINES?max_plen-top_skip:MIN_LINES;
		}

	if(noisy)
		{
		int landscape_suggested = FALSE;

		fprintf(stderr, _("Longest line is %d columns wide, shortest leading whitespace is %d columns wide.\n"), max_len, left_skip);

		if(columns != (max_len - left_skip))
			fprintf(stderr, _("Constraints force columns from %d to %d.\n"), (max_len - left_skip), columns);

		if(lines_per_page != 0)
			{
			fprintf(stderr, _("Longest page is %d lines long.\n"), lines_per_page);
			fprintf(stderr, _("Input file's aspect ratio: %.2f (landscape_asptrigger=%.2f)\n"),
						((double)columns/(double)lines_per_page),
						landscape_asptrigger);
			if( ((double)columns/(double)lines_per_page) > landscape_asptrigger )
				landscape_suggested = TRUE;
			}
		else
			{
			fputs(_("Not one FF found, page length and asp[ect ratio] unknown\n"), stderr);
			if(columns > landscape_lentrigger)
				landscape_suggested = TRUE;
			}

		if(landscape_suggested)
			fputs(_("Input file is probably landscape.\n"), stderr);
		else
			fputs(_("Input file is probably portrait.\n"), stderr);

		if(force_landscape)
			fputs(_("User has forced landscape mode.\n"), stderr);

		if(force_portrait)
			fputs(_("User has forced portrait mode.\n"), stderr);

		fputs("\n", stderr);
		}

	/*
	** If the longest line is longer than lanscape_lentrigger
	** and the lines per page is undetermined or the aspect
	** ratio justifies landscape mode, use landscape mode.
	**
	** (If the number of lines per page is unknown,
	** lines_per_page will be zero.)
	**
	** There are also force clauses in there.  If force_landscape
	** is true, nothing else matters.  If force_portrait is true,
	** nothing that would suggest landscape mode matters.
	*/
	if( force_landscape || ( ! force_portrait
				&& columns > landscape_lentrigger
				&& (lines_per_page==0 || ((double)columns/(double)lines_per_page) > landscape_asptrigger) ) )
		{
		if(noisy)
			fputs(_("Landscape mode selected.\n"), stderr);

		landscape = TRUE;
		page_width = phys_pu_height;
		page_height = phys_pu_width;
		min_lm = LMLM;			/* landscape minimum margins */
		min_rm = LMRM;
		min_tm = LMTM;
		min_bm = LMBM;
		default_lines_per_page = ldeflines;
		gutter_lr = 0.0;
		gutter_tb = gutter;
		}
	else
		{
		if(noisy)
			fputs(_("Portrait mode selected.\n"), stderr);

		landscape = FALSE;
		page_width = phys_pu_width;
		page_height = phys_pu_height;
		min_lm = PMLM;			/* portrait minimum margins */
		min_rm = PMRM;
		min_tm = PMTM;
		min_bm = PMBM;
		default_lines_per_page = pdeflines;
		gutter_lr = gutter;
		gutter_tb = 0.0;
		}

	/*
	** If tumble duplex mode is selected, move the gutter from
	** the left and rigth to the top and bottom or vice-versa.
	**
	** If neither duplex nor simplex was explicitly selected,
	** do don't know where to put the gutter so we will
	** not have one.
	*/
	if(duplex_mode == DUPLEX_TUMBLE)
		{
		double temp;
		temp = gutter_tb;
		gutter_tb = gutter_lr;
		gutter_lr = temp;
		}
	else if(duplex_mode == DUPLEX_UNDEF)
		{
		gutter_tb = 0.0;
		gutter_lr = 0.0;
		}

	/* If we defered assigning the default lines per page, do it now. */
	if(lines_per_page == 0)
		lines_per_page = default_lines_per_page;

	/*
	** If a left skip is employed, use it to compute the
	** desired left and right margins, if not, use the
	** default left and right margins.
	*/
	if(left_skip)
		{
		left_margin = right_margin =
						page_width * (left_skip / (columns + (2 * left_skip)));
		if(left_margin < min_lm )
			left_margin = min_lm;
		if(right_margin < min_rm )
			right_margin = min_rm;
		}
	else
		{
		left_margin = min_lm;
		right_margin = min_rm;
		}

	/* Determine the top and bottom margins, by default or computation. */
	if(!has_formfeeds || top_skip == 0)			/* if no page breaks, */
		{
		top_margin = min_tm;
		bottom_margin = min_bm;
		}
	else
		{
		bottom_margin = top_margin =
			(double)(top_skip*2)/(double)((top_skip*2)+lines_per_page)*page_height;
		if(top_margin < min_tm)
			top_margin = min_tm;
		if(bottom_margin < min_bm)
			bottom_margin = min_bm;
		}

	/*
	** The line spacing in PostScript Units is equal to the number
	** of PostScript Units in the vertical printable area divided
	** by the number of lines per page.
	*/
	line_spacing = (page_height-top_margin-bottom_margin-gutter_tb)/lines_per_page;

	/*
	** The pointsize is equal to the horizontal printable area
	** divided between the columns divided by the character width
	** factor and converted to points.
	*/
	pointsize = (page_width-left_margin-right_margin-gutter_lr)/columns/char_width;
	if(noisy)
		fprintf(stderr, "pointsize = %.1f\n", pointsize);

	/*
	** If the above computation yielded a point size which is
	** too large for the line spacing, then change the point size
	** to what we feel is the largest the line spacing allows.
	*/
	if(pointsize > (line_spacing * char_height) )
		{
		pointsize = line_spacing * char_height;
		if(noisy)
			fprintf(stderr, "pointsize reduced to %.1f because line_spacing is %.1f\n",pointsize,line_spacing);
		}

	/* If simplex mode is selected, turn the gutter into a bigger
	   constant left or top margin. */
	if(duplex_mode == DUPLEX_NONE)
		{
		left_margin += gutter_lr;
		gutter_lr = 0.0;
		top_margin += gutter_tb;
		gutter_tb = 0.0;
		}

	} /* end of pass1() */

/*====================================================================
** PASS2 and its support routines
====================================================================*/

/*
** Send the filter_lp procedure set.
*/
static void our_procset(void)
	{
	/* bind and define */
	fputs("/d{bind def}bind def\n",stdout);

	/* start page */
	fputs("/sp{save\n page 2 mod 1 eq{/y y gut_tb sub def}if}def\n",stdout);

	/* end page */
	fputs("/ep{restore showpage}def\n",stdout);

	/* new line */
	fputs("/n{/y y yspace sub def /m 0 def}d\n",stdout);

	/* specified number of newlines */
	fputs("/nx{/y exch yspace mul neg y add def /m 0 def}d\n",stdout);

	/* show */
	fputs("/s{m 0 eq{/m 1 def lm indent ptsize width mul mul add\n"
		"    page 2 mod 1 eq{gut_lr add}if\n"	/* possibly add gutter width */
		"    y moveto}if\n"
		" show}d\n",stdout);

	/* show and newline */
	fputs("/p{s n}d\n",stdout);

	/* change indent */
	fputs("/i{/indent exch def}d\n",stdout);

	/* select bold font */
	fputs("/b{BFont setfont}d\n",stdout);

	/* select regular font */
	fputs("/r{RFont setfont}d\n",stdout);

	/* show and select bold font */
	fputs("/a{s BFont setfont}d\n",stdout);

	/* show and select regular font */
	fputs("/q{s RFont setfont}d\n",stdout);

	/* show and skip specified number of spaces */
	fputs("/t{exch s ptsize mul width mul currentpoint "
		"3 1 roll add exch moveto}d\n",stdout);

	/* numbers of spaces between 4 and 29 */
	fputs("/A{4 t}d /B{5 t}d /C{6 t}d /D{7 t}d /E{8 t}d /F{9 t}d\n",stdout);
	fputs("/G{10 t}d /H{11 t}d /I{12 t}d /J{13 t}d /K{14 t}d\n",stdout);
	fputs("/L{15 t}d /M{16 t}d /N{17 t}d /O{18 t}d /P{19 t}d\n",stdout);
	fputs("/Q{20 t}d /R{21 t}d /S{22 t}d /T{23 t}d /U{24 t}d\n",stdout);
	fputs("/V{25 t}d /W{26 t}d /X{27 t}d /Y{28 t}d /Z{29 t}d\n",stdout);

	/* underline */
	fputs("/u{newpath exch ptsize mul width mul lm add "
		"page 2 mod 1 eq{gut_lr add}if "		/* possibly add gutter width */
		"y ptsize 0.25 mul sub "
		"moveto "
		"ptsize mul width mul 0 rlineto "
		"stroke}d\n",stdout);

	} /* end of our_procset() */

/*
** write the prolog to standard output
*/
static gu_boolean prolog(void)
	{
	const char *newencoding, *newencoding_normal, *newencoding_bold;
	struct ENCODING_INFO encoding;

	/* Look up the selected charset: */
	if(charset_to_encoding(charset, &encoding) < 0)
		fatal(10, "charset \"%s\" is unknown", charset);

	/* Which PostScript encoding will we use for normal-weight and bold text?
	 * The choices are the fonts default encoding (represented by NULL) and
	 * encoding.encoding determined above.  If we set either to 
	 * encoding.encoding we set newencoding too so that we will know to
	 * load the needed resources. */
	newencoding = newencoding_normal = newencoding_bold = NULL;

	/* Select a normal (non-bold) font which supports the necessary
	 * encoding and is in the requested font family.
	 *
	 * Note that if the user has already selected the font we will
	 * make the unwarranted assumption that he knew what he was doing. :)
	 * If the user has selected the font, then we make no assumptions
	 * about its encoding and reencode it to our satisfaction.
	 */
	if(font_normal.font_psname)
		{
		newencoding = newencoding_normal = encoding.encoding;
		}
	else
		{
		if(encoding_to_font(encoding.encoding, font_family, "normal", "normal", "normal", &font_normal) < 0)
			{
			fatal(10, "Can't find normal style font in family \"%s\"\n"
				"\tfor charset \"%s\".", font_family, charset);
			}

		/* If ASCII won't do, */
		if(uses_nonascii_normal || !encoding.encoding_ascii_compatible)
			{
			/* If font's default encoding isn't correct, */
			if(strcmp(encoding.encoding, font_normal.font_encoding))
				newencoding = newencoding_normal = encoding.encoding;
			}
		/* If ASCII encoding ok and there is a substitute font
		   for this case, use it in stead. */
		else if(font_normal.ascii_subst_font)
			{
			font_normal.font_psname = font_normal.ascii_subst_font;
			}
		}

	/*
	** Do the same for the bold font.
	*/
	if(font_bold.font_psname)
		{
		newencoding = newencoding_normal = encoding.encoding;
		}
	else if(uses_bold)
		{
		if(encoding_to_font(encoding.encoding, font_family, "bold", "normal", "normal", &font_bold) < 0)
			{
			fatal(10, "Can't find bold style font in family \"%s\"\n"
				"\tfor charset \"%s\".", font_family, charset);
			}

		if(uses_nonascii_bold || !encoding.encoding_ascii_compatible)
			{
			if(strcmp(encoding.encoding, font_bold.font_encoding))
				newencoding = newencoding_bold = encoding.encoding;
			}
		else if(font_bold.ascii_subst_font)
			{
			font_bold.font_psname = font_bold.ascii_subst_font;
			}
		}

	if(noisy)
		{
		fprintf(stderr, "fontnormal = \"%s\", fontbold = \"%s\"\n", font_normal.font_psname, font_bold.font_psname);
		fprintf(stderr, "Encoding = \"%s\"\n", encoding.encoding);
		}

	/* We haven't implemented a Clean7Bit encoding for UNICODE. */
	if(newencoding && strcmp(newencoding, "PPR-UNICODE") == 0 && !eighth_bit_ok)
		fatal(10, "No 7 bit encoding for UNICODE implemented.");
	
	/* Start the PostScript output. */
	fputs("%!PS-Adobe-3.0\n", stdout);
	fputs("%%Creator: PPR Line Printer Emulator\n", stdout);
	gu_psprintf("%%%%DocumentData: %s\n", ((uses_nonascii_normal || uses_nonascii_bold) && eighth_bit_ok) ? "Clean8Bit" : "Clean7Bit");
	gu_psprintf("%%%%LanguageLevel: %d\n", (newencoding && strcmp(newencoding, "PPR-UNICODE") == 0) ? 2 : 1);	
	fputs("%%Pages: (atend)\n", stdout);

	/*
	** Assume that Courier will always be required, name Courier-Bold
	** only if it is needed.
	*/
	gu_psprintf("%%%%DocumentNeededResources: font %s\n", font_normal.font_psname);
	if(uses_bold)
		gu_psprintf("%%%%+ font %s\n", font_bold.font_psname);

	/*
	** If the encoding matters because we have non-ASCII characters,
	** Then say the ReEncode proceedure set is required as well as the
	** encoding in question.
	*/
	if(newencoding)
		{
		if(strcmp(newencoding, "PPR-UNICODE") == 0)
			{
			fputs("%%+ procset (TrinColl-PPR-UNICODE) 1.0 0\n", stdout);
			}
		else
			{
			fputs("%%+ procset (TrinColl-PPR-ReEncode) 1.1 0\n", stdout);
			gu_psprintf("%%%%+ encoding %s\n", newencoding);
			}
		}

	/*
	** Possibly indicate what kind of paper we want:
	*/
	if(should_specify_medium)
		{
		gu_psprintf("%%%%DocumentMedia: lpform %f %f %f %s (%s)\n",
			phys_pu_width, phys_pu_height, MediaWeight, MediaColor, MediaType);
		}

	/*
	** We already know which orientation we will be using,
	** so put in a comment.
	*/
	if(landscape)
		fputs("%%Orientation: Landscape\n", stdout);
	else
		fputs("%%Orientation: Portrait\n", stdout);

	/*
	** If we require duplex, say so.
	*/
	fputs("%%Requirements:", stdout);
	switch(duplex_mode)
		{
		case DUPLEX_DUPLEX:
			fputs(" duplex", stdout);
			break;
		case DUPLEX_TUMBLE:
			fputs(" duplex(tumble)", stdout);
			break;
		}
	fputc('\n', stdout);

	fputs("%%EndComments\n", stdout);

	/*---------------------------------------------
	** Send some proceedure sets
	---------------------------------------------*/
	fputs("%%BeginProlog\n", stdout);

	/*
	** If we are not using StandardEncoding (which every PostScript
	** interpreter has) we have to download an re-encoding routine.
	*/
	if(newencoding)
		{
		if(strcmp(newencoding, "PPR-UNICODE") == 0)
			{
			fputs("%%IncludeResource: procset (TrinColl-PPR-UNICODE) 1.0 0\n", stdout);
			}
		else
			{
			fputs("%%IncludeResource: procset (TrinColl-PPR-ReEncode) 1.1 0\n", stdout);
			gu_psprintf("%%%%IncludeResource: encoding %s\n", newencoding);
			}
		}

	/* Send the filter_lp procedure set. */
	our_procset();

	fputs("%%EndProlog\n\n", stdout);

	/*--------------------------------------------
	** execute some of those proceedures
	** in order to get ready to print
	**------------------------------------------*/
	fputs("%%BeginSetup\n",stdout);

	/* Select the page size we want. */
	gu_psprintf(
		"[ {\n"
		"%%%%IncludeFeature: *PageSize %s\n"
		"} stopped {(*Pagesize %s failed.\\n)print} if cleartomark\n",
		PageSize, PageSize);

	/* Set duplex mode if we have been asked to set one: */
	switch(duplex_mode)
		{
		case DUPLEX_NONE:
			fputs("%%IncludeFeature: *Duplex None\n", stdout);
			break;
		case DUPLEX_DUPLEX:
			fputs("%%IncludeFeature: *Duplex DuplexNoTumble\n", stdout);
			break;
		case DUPLEX_TUMBLE:
			fputs("%%IncludeFeature: *Duplex DuplexTumble\n", stdout);
			break;
		}

	/* Download our fonts. */
	gu_psprintf("%%%%IncludeResource: font %s\n", font_normal.font_psname);
	if(uses_bold)
		gu_psprintf("%%%%IncludeResource: font %s\n", font_bold.font_psname);

	/* Re-encode those we must. */
	if(newencoding && strcmp(newencoding, "PPR-UNICODE") != 0)
		{
		if(newencoding_normal)
			gu_psprintf("/%s /%s /%s ReEncode\n", font_normal.font_psname, font_normal.font_psname, newencoding_normal);
		if(newencoding_bold)
			gu_psprintf("/%s /%s /%s ReEncode\n", font_bold.font_psname, font_bold.font_psname, newencoding_bold);
		}

	/*
	** We must set the point size variable before we can
	** select the fonts.
	*/
	gu_psprintf("/ptsize %f def\n", pointsize);

	/*
	** Find, scale, and save the fonts.
	*/
	gu_psprintf("/RFont /%s findfont ptsize scalefont def\n", font_normal.font_psname);
	if(uses_bold)
		gu_psprintf("/BFont /%s findfont ptsize scalefont def\n", font_bold.font_psname);

	/*
	** Set a lot of PostScript variables.
	*/
	gu_psprintf("/width %f def\n", char_width);
	gu_psprintf("/yspace %f def\n", line_spacing);	/* vertical spacing */
	gu_psprintf("/lm %f def\n", left_margin);		/* left margin in inches */
	gu_psprintf("/tm %f def\n", top_margin);		/* top margin in inches */
	gu_psprintf("/gut_lr %f def\n", gutter_lr);		/* gutter width */
	gu_psprintf("/gut_tb %f def\n", gutter_tb);		/* gutter width */
	gu_psprintf("/ph %f def\n", page_height);		/* page height in inches */
	fputs("/y ph tm sub ptsize 2 div sub def\n", stdout);
		/* y at top minus top margin minus half the point size */
	fputs("/m 0 def\n", stdout);				/* no moveto yet */
	fputs("0 i\n", stdout);						/* zero indent */
	fputs("r\n", stdout);						/* select roman font (we need a default) */
	fputs("ptsize 35 div setlinewidth\n", stdout);
	fputs("%%EndSetup\n\n", stdout);

	if(newencoding && strcmp(newencoding, "PPR-UNICODE") == 0)
		return TRUE;
	else
		return FALSE;
	} /* end of prolog() */

/*
** a few globals for the next few routines
*/
int font;					/* 0=roman, 1=bold */
gu_boolean font_changed;	/* true if font changed at start of this segment */
int indent;					/* spaces to indent next line */

/*
** start a page
*/
static void startpage(int page)
	{
	gu_psprintf("%%%%Page: %d %d\n", page, page);
	puts("%%BeginPageSetup");

	gu_psprintf("/page %d def\nsp\n", page);
	if(landscape)								/* landscape */
		{
		gu_psprintf("90 rotate 0 %f neg translate\n", phys_pu_width);
		}

	puts("%%EndPageSetup");
	font = 0;
	indent = 0;
	} /* end of startpage() */

/*
** End a page.
*/
static void endpage(void)
	{
	fputs("ep\n\n", stdout);
	} /* end of endpage() */

/*
** If this line is underlined, write an underline line.
** If the whole line was spaces and we underlined it with a command,
** return zero, otherwise return -1.
*/
static int underline(int skip)
	{
	wchar_t *cptr = &line[skip];
	unsigned char *aptr = &line_attr[skip];
	int index;
	int ulstart=-1;
	int ul=0;						/* TRUE if not just blanks underlined */
	int x;
	int retval=0;					/* start with assumption it is all spaces */
	int c;

	index=0;							/* find underline segments */
	do	{								/* by moving thru the whole line */
		if((c = cptr[index]) && c!=' ') /* if this is not a space, then */
			retval=-1;					/* it is not all spaces */

		if( aptr[index] & ATTR_UNDERLINE )		/* (NULL will have no attribs) */
			{									/* If underlined in any way, */
			if(c != ' ')						/* If not a space, set a flag which */
				ul=1;							/* says that underline is not all spaces */
			if(ulstart==-1)						/* If not started yet, */
				ulstart=index;					/* start here. */
			}
		else						/* otherwise, */
			{
			if(ulstart!=-1)			/* if underline just ended, */
				{
				if(ul)				/* if not all spaces */
					{				/* print an underlining command */
					gu_psprintf("%d %d u\n", ulstart, index-ulstart);
					}
				else				/* if all spaces, */
					{
					if( (index-ulstart) > 7 )
						{			/* if rather long, */
						gu_psprintf("%d %d u\n", ulstart, index-ulstart);
						}
					else			/* if short, */
						{			/* convert to underscores */
						for(x=ulstart;x<index;x++)
							cptr[x]='_';
						retval=-1;	/* and force the line state to "not all spaces" */
						}
					}
				ulstart=-1;			/* reset underline starting point */
				ul=0;				/* reset not all spaces flag */
				}
			}

		} while(cptr[index++]);		/* this is right! */

	return retval;					/* return an indication as to whether it is all spaces */
	} /* end of underline() */

/*
** Send a line, properly formated to stdout.
** This routine should not be called with a blank line.
*/
static void output_line(int skip, gu_boolean unicode)
	{
	wchar_t *cptr=&line[skip];
	unsigned char *aptr = &line_attr[skip];
	int len;
	int started;
	int newindent=0;
	int c;

	if((len = wcsspn(cptr, L" ")) != indent)	/* If number of leading spaces */
		{										/* is not equal to the current */
		gu_psprintf("%d i", len);				/* indent, then change current */
		indent=len;								/* indent */
		newindent=-1;							/* set flag so space can be */
		}										/* added if "b" or "r" used */
	cptr+=len;									/* now that that is done, */
	aptr+=len;									/* eat up the indent spaces */

	started=0;
	while((c = *cptr))							/* Take the next character. */
		{
		if((len = wcsspn(cptr, L" ")) > 3)		/* If it begins a run of more than 3 spaces, */
			{
			if(len>29)							/* If too long to abreviate, */
				gu_psprintf(")%d t(", len);		/* then write it out long */
			else								/* if short enough */
				gu_psprintf(")%c(", len-4+'A');	/* abreviate to a single letter. */
			cptr+=len;
			aptr+=len;
			continue;
			}

		/* Does the font change from bold to regular or vice-verse? */
		if(c != ' ')					/* spaces have no font */
			{
			if(*aptr & ATTR_BOLD)		/* if this character is bold */
				{
				if(font==0)				/* if font is currently roman, */
					{
					font=1;				/* change it to bold */
					font_changed=TRUE;
					}
				}
			else						/* if this character is roman */
				{
				if(font==1)				/* if font is currently bold, */
					{
					font=0;				/* change it to roman */
					font_changed=TRUE;
					}
				}
			}

		if(font_changed)
			{
			if(started)					/* if we are in a string */
				{
				if(font)				/* do a show and font change */
					fputs(")a(",stdout);
				else
					fputs(")q(",stdout);
				}
			else						/* if not in a string yet, */
				{
				if(newindent)			/* if x i used, */
					fputs(" ",stdout);	/* we must add a space */
				if(font)
					fputs("b",stdout);
				else
					fputs("r",stdout);
				}
			font_changed=FALSE;
			}
		newindent=0;		  			 /* after started, newident is meaningless */

		if(!started)
			{
			fputc('(',stdout);
			started=1;
			}

		switch(c)
			{
			case '(':					/* proceed (, ), and \ with \ */
			case ')':
			case '\\':
				fputc('\\', stdout);
				fputc(c, stdout);
				break;
			default:
				if(c < ' ' || c == 127)			/* ASCII control characters */
					gu_psprintf("\\%o", c);
				else if(c < 127)				/* ASCII printing characters */
					fputc(c, stdout);
				else
					{
					if(unicode)
						{
						/* This may not be the most compact code, but it should involve 
						 * fewer instructions for the lower code points.
						 */
						if(c < 0x00000800)
							{
							fputc((c >> 6)   | 0xC0, stdout);
							fputc((c & 0x3F) | 0x80, stdout);
							}
						else if(c < 0x00010000)
							{
							fputc( (c >> 12)         | 0xE0, stdout);
							fputc(((c >>  6) & 0x3F) | 0x80, stdout);
							fputc( (c        & 0x3F) | 0x80, stdout);
							}
						else if(c < 0x00200000)
							{
							fputc(( c >> 18)         | 0xF0, stdout);
							fputc(((c >> 12) & 0x3F) | 0x80, stdout);
							fputc(((c >>  6) & 0x3F) | 0x80, stdout);
							fputc(( c        & 0x3F) | 0x80, stdout);
							}
						else if(c < 0x04000000)
							{
							fputc(( c >> 24)         | 0xF8, stdout);
							fputc(((c >> 18) & 0x3F) | 0x80, stdout);
							fputc(((c >> 12) & 0x3F) | 0x80, stdout);
							fputc(((c >>  6) & 0x3F) | 0x80, stdout);
							fputc(( c        & 0x3F) | 0x80, stdout);
							}
						else /* actually < 0x80000000 */
							{
							fputc((c >> 30)          | 0xFC, stdout);
							fputc(((c >> 24) & 0x3F) | 0x80, stdout);
							fputc(((c >> 18) & 0x3F) | 0x80, stdout);
							fputc(((c >> 12) & 0x3F) | 0x80, stdout);
							fputc(((c >>  6) & 0x3F) | 0x80, stdout);
							fputc(( c        & 0x3F) | 0x80, stdout);
							}
						}
					else
						{
						if(eighth_bit_ok)
							fputc(c, stdout);
						else
							gu_psprintf("\\%o", c);
						}
					}
				break;
			}

		cptr++; aptr++;
		} /* end of while loop */

	if(started)							/* this if probably is not necessary */
		fputs(")p\n",stdout);
	} /* end of output_line() */

/*
** Write the document trailer to standard output.
*/
static void trailer(int pagecount)
	{
	fputs("%%Trailer\n",stdout);
	gu_psprintf("%%%%Pages: %d\n", pagecount);
	fputs("%%EOF\n", stdout);
	}

/*
** do the second pass on the input file
*/
static void pass2(void)
	{
	int current_page = 0;		/* page we are on */
	int linen, linen2;			/* current line number */
	int nlpend = 0;				/* count of newlines pending */
	char writ = FALSE;			/* true if anything written on this page */
	int len = 0;
	gu_boolean unicode;

	unicode = prolog();			/* emmit the PostScript prolog */

	/* the main loop */
	while(len >= 0)				/* until end of file */
		{
		linen = linen2 = 0;		/* reset line count */
		nlpend = 0;				/* pending newlines don't matter any more */

		current_page++;

		while((len = (*readline)()) >= 0 && ! formfeed)
			{							/* read lines `til end of page or file */
			if((++linen) <= top_skip)	/* skip lines at top */
				continue;				/* (just throw them away) */

			linen2++;			/* this count doesn't take in top skip lines */

			if(len == 0)		/* if line is blank */
				{				/* (ie, length of zero), */
				nlpend++;
				}				/* mearly add it to pending count */
			else				/* otherwise, print it */
				{
				if(writ==FALSE) /* if page isn't started yet, start it */
					{
					startpage(current_page);
					writ = TRUE;
					}
				if(nlpend)		/* send all pending newlines */
					{
					if(nlpend==1)						/* use "n" command for */
						fputs("n\n", stdout);			/* a single newline, */
					else								/* "nx" command for */
						gu_psprintf("%d nx\n", nlpend);	/* multiple newline */
					nlpend = 0;
					}

				if(underline(left_skip))				/* underline it if needed */
					output_line(left_skip, unicode);	/* if line has characters, print them */
				else									/* otherwise, */
					nlpend++;							/* write it up as a blank line */
				}

			if(linen2 == lines_per_page)		/* end page */
				break;							/* if page filled */
			}					/* while not EOF and not formfeed */

		if(writ)				/* if page is not blank */
			{
			endpage();			/* send page closing lines */
			writ = FALSE;		/* and reset writ */
			}
		else
			{
			current_page--;		/* otherwise suppress blank page */
			}

		}						/* this loop ends when file does */
	trailer(current_page);		/* emmit the PostScript trailer */
	} /* end of pass2() */

/*
** Usage: filter_lp 'option1...optionN' _printer_ _title_
**        filter_lp_autolf 'option1...optionN' _printer_ _title_
** The _printer_ and _title_ options are ignored.
*/
int main(int argc, char *argv[])
	{
	const char *my_basename;

	/* Initialize internation messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	/* Locate the filename portion of this programs name. */
	if((my_basename = strrchr(argv[0], '/')))
		my_basename++;
	else
		my_basename = argv[0];

	/*
	** If we are invoked as "filter_lp_autolf" then
	** we interpret carriage return and carriage return
	** and line feed.  This is for printing files with
	** Macintosh line termination.
	*/
	if(strcmp(my_basename, "filter_lp_autolf") == 0)
		auto_lf = TRUE;

	/*
	** Set to NULL so we will know of fontnormal= or
	** fontbold= option is used.
	*/
	font_normal.font_psname = NULL;
	font_normal.font_encoding = "";
	font_normal.ascii_subst_font = "";
	font_bold.font_psname = NULL;
	font_bold.font_encoding = "";
	font_bold.ascii_subst_font = "";

	/*
	** Read the default pagesize from the PPR config file.
	*/
	{
	const char file[] = PPR_CONF;
	const char section[] = "internationalization";
	const char key[] = "defaultmedium";
	const char *error_message;

	error_message = gu_ini_scan_list(file, section, key,
		GU_INI_TYPE_NONEMPTY_STRING, &PageSize,
		GU_INI_TYPE_POSITIVE_DOUBLE, &phys_pu_width,
		GU_INI_TYPE_POSITIVE_DOUBLE, &phys_pu_height,
		GU_INI_TYPE_NONNEG_DOUBLE, &MediaWeight,
		GU_INI_TYPE_STRING, &MediaColor,
		GU_INI_TYPE_STRING, &MediaType,
		GU_INI_TYPE_END);

	if(error_message)
		{
		fprintf(stderr, _("%s: %s\n"
						"\twhile attempting to read \"%s\"\n"
						"\t\t[%s]\n"
						"\t\t%s =\n"),
				myname, gettext(error_message), file, section, key);
		exit(1);
		}
	}

	/*
	** Process the options.  The options, of course, are a
	** series of name=value pairs.
	*/
	if(argc >= 2)
		{
		struct OPTIONS_STATE o;
		char name[32], value[64];
		int rval;

		/* Handle all the name=value pairs. */
		options_start(argv[1], &o);
		while((rval = options_get_one(&o, name,sizeof(name),value,sizeof(value))) == 1)
			{
			/* Turn on debugging. */
			if(strcmp(name, "noisy") == 0)
				{
				if(gu_torf_setBOOL(&noisy,value) == -1)
					filter_options_error(1, &o, _("Value for option \"%s=\" must be boolean."), "noisy");
				}

			/* change the character set */
			else if(strcmp(name, "charset") == 0)
				{
				charset = gu_strdup(value);
				}

			/* set a non-default page size */
			else if(strcmp(name, "pagesize") == 0)
				{
				if(pagesize(value, &PageSize, &phys_pu_width, &phys_pu_height, NULL) == -1)
					filter_options_error(1, &o, _("Value for option \"%s=\" is invalid."), "pagesize");
				}

			/* force a page width (number of columns) */
			else if(strcmp(name, "width") == 0)
				{
				if((MAX_WIDTH = atoi(value)) < 10 || MAX_WIDTH > 1000)
					filter_options_error(1, &o, _("Unreasonable value for option \"%s=\"."), "width");
				}

			/* set a minimum number of columns */
			else if(strcmp(name, "mincolumns") == 0)
				{
				if((MIN_COLUMNS = atoi(value)) <= 0)
					filter_options_error(1, &o, _("Unreasonable value for option \"%s=\"."), "mincolumns");
				}

			/* force the page length */
			else if(strcmp(name, "length") == 0)
				{
				/* not implemented */
				}

			/* line length which triggers landscape printing */
			else if(strcmp(name, "landscape_lentrigger") == 0)
				{
				if((landscape_lentrigger = atoi(value)) < 0)
					filter_options_error(1, &o, _("Unreasonable value for option \"%s=\"."), "landscape_lentrigger");
				}

			/* ratio of length the height which must also be exceeded */
			else if(strcmp(name, "landscape_asptrigger") == 0)
				{
				landscape_asptrigger = atof(value);
				}

			/* The font family (as defined in fonts.conf) to use: */
			else if(strcmp(name, "fontfamily") == 0)
				{
				font_family = gu_strdup(value);
				}

			/* font for printing normal text */
			else if(strcmp(name, "fontnormal") == 0)
				{
				if( is_unsafe_ps_name(value) )
					filter_options_error(1, &o, _("Value of option \"%s=\" contains illegal characters."), "fontnormal");

				font_normal.font_psname = gu_strdup(value);
				}

			/* font for printing bold text */
			else if(strcmp(name, "fontbold") == 0)
				{
				if(is_unsafe_ps_name(value))
					filter_options_error(1, &o, _("Value of option \"%s=\" contains illegal characters."), "fontbold");

				font_bold.font_psname = gu_strdup(value);
				}

			/* The font width as a fraction of the point size */
			else if(strcmp(name, "charwidth") == 0)
				{
				if((char_width	= atof(value)) < 0.1 || char_width > 3.0)
					filter_options_error(1, &o, _("Unreasonable value for option \"%s=\"."), "charwidth");
				}

			/* Font line spacing as a fraction of the point size, normally 1.0. */
			else if(strcmp(name, "charheight") == 0)
				{
				if((char_height = atof(value)) < 0.1 || char_height > 3.0)
					filter_options_error(1, &o, _("Unreasonable value for option \"%s=\"."), "charheight");
				}

			/* MediaType for "%%Media:" comment */
			else if(strcmp(name, "mediatype") == 0)
				{
				if(is_unsafe_ps_name(value))
					filter_options_error(1, &o, _("Value of option \"%s=\" contains illegal characters."), "mediatype");

				MediaType = gu_strdup(value);
				should_specify_medium = TRUE;
				}

			/* MediaColor for "%%Media:" comment */
			else if(strcmp(name, "mediacolour") == 0 || strcmp(name, "mediacolor") == 0)
				{
				if(is_unsafe_ps_name(value))
					filter_options_error(1, &o, _("Value of option \"%s=\" contains illegal characters."), "mediacolour");

				MediaColor = gu_strdup(value);
				should_specify_medium = TRUE;
				}

			/* MediaWeight for "%%Media:" comment */
			else if(strcmp(name, "mediaweight") == 0)
				{
				MediaWeight = atof(value);
				should_specify_medium = TRUE;
				}

			/* Duplex mode to request */
			else if(strcmp(name, "duplex") == 0)
				{
				if(gu_strcasecmp(value,"undef")==0)
					duplex_mode = DUPLEX_UNDEF;
				else if(gu_strcasecmp(value,"none")==0)
					duplex_mode = DUPLEX_NONE;
				else if(gu_strcasecmp(value,"tumble")==0)
					duplex_mode = DUPLEX_TUMBLE;
				else if(gu_strcasecmp(value,"notumble")==0)
					duplex_mode = DUPLEX_DUPLEX;
				else
					filter_options_error(1, &o, _("Valid \"duplex=\" values are \"none\", \"undef\", \"tumble\", and \"notumble\"."));
				}

			/* Tab width */
			else if(strcmp(name, "tabwidth") == 0)
				{
				if((TAB_WIDTH = atoi(value)) < 1)
					filter_options_error(1, &o, _("Unreasonable value for option \"%s=\"."), "tabwidth");
				}

			/* Default lines per page, portrait and landscape modes */
			else if(strcmp(name, "pdeflines") == 0)
				{
				if((pdeflines = atoi(value)) <= 0)
					filter_options_error(1, &o, _("Unreasonable value for option \"%s=\"."), "pdeflines");
				}
			else if(strcmp(name, "ldeflines") == 0)
				{
				if((ldeflines = atoi(value)) <= 0)
					filter_options_error(1, &o, _("Unreasonable value for option \"%s=\"."), "ldeflines");
				}

			/* Minimum and maximum lines per page */
			else if(strcmp(name, "minlines") == 0)
				{
				if((MIN_LINES = atoi(value)) <= 0)
					filter_options_error(1, &o, _("Unreasonable value for option \"%s=\"."), "minlines");
				}
			else if(strcmp(name, "maxlines") == 0)
				{
				if((MAX_LINES = atoi(value)) <= 0)
					filter_options_error(1, &o, _("Unreasonable value for option \"%s=\"."), "maxlines");
				}
			else if(strcmp(name, "orientation") == 0)
				{
				if(strcmp(value, "portrait") == 0)
					{
					force_portrait = TRUE;
					force_landscape = FALSE;
					}
				else if(strcmp(value, "landscape") == 0)
					{
					force_portrait = FALSE;
					force_landscape = TRUE;
					}
				else if(strcmp(value, "auto") == 0)
					{
					force_portrait = FALSE;
					force_landscape = FALSE;
					}
				else
					{
					filter_options_error(1, &o, _("Valid values for orientation are \"portrait\", \"landscape\", and \"auto\"."));
					}
				}

			/* Settable dimensions */
			else
				{
				int x;

				for(x=0; setable_dimensions[x].name; x++)
					{
					if(strcmp(name,setable_dimensions[x].name) == 0)
						{
						if( (*(setable_dimensions[x].value) = convert_dimension(value)) < 0 )
							{
							filter_options_error(1, &o, _("Value for dimension \"%s\" is invalid."), name);
							}
						break;
						}
					}

				/* Possibly tell about things we are ignoring */
				if(noisy)
					{
					if(setable_dimensions[x].name == (char*)NULL)
						fprintf(stderr, _("Ignoring option \"%s=%s\".\n"), name, value);
					}

				}

			} /* end of options loop */

		/*
		** If options_get_one() detected an error, print it now.
		*/
		if(rval == -1)
			{
			filter_options_error(1, &o, gettext(o.error));
			exit(1);
			}

		} /* end of if there are options */

	/* If we are running in noisy mode, describe many of the options. */
	if(noisy)
		{
		fprintf(stderr, "pagesize=%.2fpsu x %.2fpsu\n", phys_pu_width, phys_pu_height);

		fprintf(stderr, "Min portrait margins: pmlm=%.2fpsu, pmrm=%.2fpsu, pmtm=%.2fpsu pmbm=%.2fpsu\n",
				PMLM, PMRM, PMTM, PMBM);

		fprintf(stderr, "Min landscape margins: lmlm=%.2fpsu, lmrm=%.2fpsu, lmtm=%.2fpsu lmbm=%.2fpsu\n",
				LMLM, LMRM, LMTM, LMBM);

		fprintf(stderr,"Default page lengths: pdeflines = %d, ldeflines = %d\n",
				pdeflines, ldeflines);

		fprintf(stderr,"Page length limits: maxlines = %d, minlines = %d\n",
				MAX_LINES, MIN_LINES);

		fprintf(stderr, "Page width limits: maxwidth = %d, mincolumns = %d\n",
				MAX_WIDTH, MIN_COLUMNS);

		fprintf(stderr, "char_width = %.2f, char_height = %.2f\n", char_width, char_height);

		if(landscape_lentrigger != 0)
			fprintf(stderr, "landscape_lentrigger = %d\n", landscape_lentrigger);
		else
			fputs("landscape_lentrigger = default_landscape_lentrigger\n", stderr);

		if(landscape_asptrigger != 0.0)
			fprintf(stderr, "landscape_asptrigger = %.2f\n", landscape_asptrigger);
		else
			fputs("landscape_asptrigger = default_landscape_asptrigger\n", stderr);

		fprintf(stderr, "Charset: %s\n", charset);

		fputs("\n", stderr);
		}

	/*
	 * Select the proper readline function according to the
	 * format of the input file.
	*/
	if(strcmp(my_basename, "filter_fortran") == 0)
		readline = readline_fortran;
	else if(strcasecmp(charset, "UTF-8") == 0)
		readline = readline_utf_8;
	else
		readline = readline_normal;

	/*
	** Compute the default landscape_lentrigger and
	** landscape_asptrigger.
	*/
		{
		double portrait_length, portrait_width;
		double landscape_length, landscape_width;
		double portrait_ideal_asp;
		double landscape_ideal_asp;
		double portrait_ideal_len;
		double landscape_ideal_len;

		/*
		** The printable width in portrait mode is the page width minus
		** the left and right margins.  If we know that we are
		** printing in simplex, the gutter is added to the left
		** margin.  If we are printing in duplex mode with long
		** edge binding (no tumble) then it is added alternately
		** to the left and right margin.
		*/
		portrait_width = phys_pu_width - PMLM - PMRM;
		if(duplex_mode != DUPLEX_UNDEF && duplex_mode != DUPLEX_TUMBLE)
			portrait_width -= gutter;

		/*
		** The printable length in portrait mode is the page length
		** minus the top and bottom margins.  If we are doing duplex
		** with short edge binding then the gutter which will be
		** alternately at the top and bottom must be subtracted too.
		*/
		portrait_length = phys_pu_height - PMTM - PMRM;
		if(duplex_mode == DUPLEX_TUMBLE)
			portrait_length -= gutter;

		/* Same idea for landscape width. */
		landscape_width = phys_pu_height - LMLM - LMRM;
		if(duplex_mode == DUPLEX_TUMBLE)
			landscape_width -= gutter;

		/* Same idea for landscape length. */
		landscape_length = phys_pu_width - LMTM - LMRM;
		if(duplex_mode != DUPLEX_TUMBLE && duplex_mode != DUPLEX_UNDEF)
			landscape_length -= gutter;

		/*
		** For each orientation, the ideal aspect ratio is the
		** one which leaves the characters as close as possible to
		** their intended line spacing.
		*/
		portrait_ideal_asp = (portrait_width/portrait_length) / (char_width/char_height);
		landscape_ideal_asp = (landscape_width/landscape_length) / (char_width/char_height);

		/*
		** The trigger point should be half way between the two
		** ideal aspect ratios.
		*/
		default_landscape_asptrigger = (portrait_ideal_asp + landscape_ideal_asp) / 2.0;

		/*
		** The ideal line length for each orientation is the one which yields
		** the font design line spacing at the default number of lines.
		*/
		portrait_ideal_len = (double)pdeflines * portrait_ideal_asp;
		landscape_ideal_len = (double)ldeflines * landscape_ideal_asp;

		/*
		** Again, the trigger point should be half way in between.
		*/
		default_landscape_lentrigger = (int)(((portrait_ideal_len + landscape_ideal_len) / 2.0) + 0.5);

		if(noisy)
			{
			fprintf(stderr, _("Portrait printable area: %.2fpsu x %.2fpsu\n"), portrait_width, portrait_length);
			fprintf(stderr, _("Landscape printable area: %.2fpsu x %.2fpsu\n"), landscape_width, landscape_length);
			fprintf(stderr, _("Portrait ideal asp (columns/lines): %.2f\n"), portrait_ideal_asp);
			fprintf(stderr, _("Landscape ideal asp (columns/lines): %.2f\n"), landscape_ideal_asp);
			fprintf(stderr, _("Average: default_landscape_asptrigger=%.2f\n"), default_landscape_asptrigger);
			fprintf(stderr, _("Portrait ideal len for pdeflines=%d: %.2f\n"), pdeflines, portrait_ideal_len);
			fprintf(stderr, _("Landscape ideal len for ldeflines=%d: %.2f\n"), ldeflines, landscape_ideal_len);
			fprintf(stderr, _("Average: default_landscape_lentrigger=%d\n"), default_landscape_lentrigger);
			fputc('\n', stderr);
			}
		}

	/*
	** If either landscape_asptrigger or landscape_lentrigger
	** is undefined, use the default.
	*/
	if(landscape_asptrigger == 0.0)
		landscape_asptrigger = default_landscape_asptrigger;
	if(landscape_lentrigger == 0)
		landscape_lentrigger = default_landscape_lentrigger;

	/*
	** Check options for unworkable option combinations.
	*/
	if(MAX_LINES < MIN_LINES)
		fatal(1, _("maxlines is less than minlines"));
	if(pdeflines < MIN_LINES)
		fatal(1, _("pdeflines is less than minlines"));
	if(pdeflines > MAX_LINES)
		fatal(1, _("pdeflines is greater than maxlines"));
	if(ldeflines < MIN_LINES)
		fatal(1, _("ldeflines is less than minlines"));
	if(ldeflines > MAX_LINES)
		fatal(1, _("ldeflines is greater than maxlines"));
	if((PMLM + PMRM + gutter) >= phys_pu_width)
		fatal(1, _("pmlm, pmrm, gutter, and current pagesize leave no space for text"));
	if((PMTM + PMBM) >= phys_pu_height)
		fatal(1, _("pmtm, pmbm, and current pagesize leave no space for text"));
	if((LMLM + LMRM) >= phys_pu_height)
		fatal(1, _("lmlm, lmrm, and current pagesize leave no space for text"));
	if((LMTM + LMBM + gutter) >= phys_pu_width)
		fatal(1, _("lmtm, lmbm, gutter, and current pagesize leave no space for text"));

	/* If in noisy mode, point out certain odd conditions. */
	if(noisy)
		{
		if(MIN_COLUMNS >= landscape_lentrigger)
			{
			fprintf(stderr, _("Warning:	 mincolumns is greater than landscape_lentrigger,\n"
				"\t	 files w/out FF will always be printed in landscape.\n"));
			}
		else if(MAX_WIDTH < landscape_lentrigger)
			{
			fprintf(stderr, _("Warning:	 maxwidth is less than lanscape_lentrigger,\n"
				"\t	 files w/out FF will never be printed in landscape.\n"));
			}
		if(MAX_LINES == MIN_LINES)
			{
			fprintf(stderr, _("Warning:	 maxlines equals minlines, therefore page length is fixed.\n"));
			}
		}

	/* Create the line input buffer */
	line = gu_alloc((MAX_WIDTH+TAB_WIDTH+1), sizeof(wchar_t));
	line_attr = gu_alloc((MAX_WIDTH+TAB_WIDTH), sizeof(unsigned char));

	/* Make the first pass over the input file, analyzing the input. */
	pass1();

	if(fseek(stdin,0L,SEEK_SET))		/* rewind the input */
		{
		fprintf(stderr, "%s: stdin must be seekable\n", myname);
		exit(1);
		}

	/* Second pass, generate the PostScript output. */
	pass2();

	return 0;
	} /* end of main() */

/* end of file */
