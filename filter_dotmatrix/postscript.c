/*
** mouse:~ppr/src/filter_dotmatrix/postscript.c
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
** Last modified 21 May 2004.
*/

#include "filter_dotmatrix.h"
#include "libppr_font.h"

/*
** These two variables are used for minimizing the amount
** of PostScript code generated by movements.  "ls" is the
** magnitude of the last vertical movement.	 "lm" is the
** position achieved by the last absolute horizontal positioning
** command generated.
*/
static int ls;
static int lm;

/*
** The name of the PostScript encoding which we will use to achieve
** the selected character set.  If the default encoding of all of the
** fonts is OK, then this will be NULL.
*/
static char *encoding_name;

/*
** Here are the font information structures which encoding_to_font()
** will return.
*/
static struct FONT_INFO font_normal;
static struct FONT_INFO font_bold;
static struct FONT_INFO font_oblique;
static struct FONT_INFO font_boldoblique;

/*
** This structure holds all of the information we need to choose 
** appropriate fonts for the encoding and character useage patters.
** Also, by putting this into a structure we have replace a lot
** of cut-past-and-modify code with loops.
*/
struct FONTS {
	const char *command;			/* command we define in setup section to select this font */
	const char *weight;
	const char *slant;
	gu_boolean *uses;				/* value pointed to: does document use this font? */
	gu_boolean *uses_nonascii;		/* value pointed to: does document non-ASCII characters out of this font? */
	gu_boolean *uses_proportional;	/* value pointed to: is used converted to proportional spacing */
	const char *prop_tbl_name;		/* proportional respacing table name */
	struct FONT_INFO *font_info;
   	};

static struct FONTS fonts[4] =
	{
	/*
	 cmd,	weight,		slant,		used,				non-ASCII,					uses propertional,		prop table, 			FONT_INFO */
	{"f",	"normal",	"normal", 	&uses_normal,		&uses_nonascii_normal,		&uses_proportional1,	"Courier",				&font_normal},
	{"fb",	"bold",		"normal", 	&uses_bold,			&uses_nonascii_bold,		&uses_proportional2,	"Courier-Bold",			&font_bold},
	{"fo",	"normal",	"oblique", 	&uses_oblique,		&uses_nonascii_oblique, 	&uses_proportional3,	"Courier-Oblique",		&font_oblique},
	{"fbo",	"bold",		"oblique", 	&uses_boldoblique,	&uses_nonascii_boldoblique,	&uses_proportional4,	"Courier-BoldOblique",	&font_boldoblique}
   	};
    
/*
** This subroutine is called at the top of the document.
*/
void top_of_document(void)
	{
	gu_boolean need_proportional_procset = FALSE;
    {
    int i;
    struct ENCODING_INFO encoding;
    gu_boolean no_substitute;

	/* Look up the selected charset in order to find the cooresponding
	   PostScript encoding name.
	   */
	if(charset_to_encoding(opt_charset, &encoding) < 0)
		fatal(10, _("charset \"%s\" is unknown"), opt_charset);

	/* Here we find an encoding-appropriate font in each of the required
	   styles.
	   */
    no_substitute = FALSE;
    for(i=0; i<4; i++)
		{
		/* If this font is used at all, */
		if(*fonts[i].uses)
			{
			if(encoding_to_font(encoding.encoding, "monospace", fonts[i].weight, fonts[i].slant, "normal", fonts[i].font_info) < 0)
				fatal(10, _("no font available for charset %s, weight %s, slant %s"), opt_charset, fonts[i].weight, fonts[i].slant);

			/* If the document needs one or more non-ASCII characters from this font
			   or the font doesn't have a substitute for documents with only ASCII
			   characters, then we will not substitute for any font.
			   */
			if(*fonts[i].uses_nonascii || !fonts[i].font_info->ascii_subst_font)
				no_substitute = TRUE;

			if(*fonts[i].uses_proportional)
				need_proportional_procset = TRUE;
			}
		}

	/* If we haven't decided we can't substitute and the encoding is ASCII compatible,
	   then go with the ASCII substitutes.
	   */
	if(!no_substitute && encoding.encoding_ascii_compatible)
		{
		for(i=0; i<4; i++)
			{
			fonts[i].font_info->font_psname = fonts[i].font_info->ascii_subst_font;
			}
		encoding_name = NULL;
		}
	else
		{
		encoding_name = gu_strdup(encoding.encoding);
		}
	}
	
	puts("%!PS-Adobe-3.0");
	puts("%%Creator: PPR dotmatrix printer emulator");
	puts("%%Pages: (atend)");
	puts("%%DocumentData: Clean7Bit");
	gu_psprintf("%%%%DocumentNeededResources: procset %s\n", DOTMATRIX);

	/* If we need graphics routines, mention the procedure sets that contain
	   them.  We will show where to insert them later.
	   */
	if(uses_graphics)
		{
		if(level2)
			fputs("%%+ procset "DOTMATRIXG2"\n",stdout);
		else
			fputs("%%+ procset "DOTMATRIXG1"\n",stdout);
		}

	/* If we will have to re-encode the font, mention the resources needed to do it.
	   Again, we will say where to include them later.
	   */
	if(encoding_name)
		{
		gu_psprintf("%%%%+ procset %s\n", REENCODE);
		gu_psprintf("%%%%+ encoding %s\n", encoding_name);
		}

	/* Do we need to proportional-spacing conversion procset? */
	if(need_proportional_procset)
		gu_psprintf("%%%%+ procset %s\n", NEWMETRICS);

	/* Emmit requirement comments for those fonts we will actually use.
	  */
	{
    int i;
    for(i=0; i<4; i++)
    	{
		if(*fonts[i].uses)
			gu_psprintf("%%%%+ font %s\n", fonts[i].font_info->font_psname);

		/* This will never be set if .uses isn't.  We are bug hunting. */
		if(*fonts[i].uses_proportional)
			gu_psprintf("%%%%+ procset "METRICSEPSON"\n", i+1);			
    	}
	}

	/* If colour required, name that proceedure set. */
	if(uses_colour)
		puts("%%+ procset "COLOUR);

	/*
	** Name the document's requirements:
	*/
	fputs("%%Requirements:", stdout);
	if(uses_colour)
		fputs(" color", stdout);
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

	/*
	** If level 2 PostScript required, say so.  Level 2 features
	** are only required if we have generated compressed graphics.
	*/
	gu_psprintf("%%%%LanguageLevel: %d\n",(level2 && uses_graphics) ? 2 : 1);

	/* Describe the media we have formatted for: */
	gu_psprintf("%%%%DocumentMedia: lpform %f %f %f %s (%s)\n",
		phys_pu_width, phys_pu_height, MediaWeight, MediaColor, MediaType);

	puts("%%EndComments");
	puts("");

	/*
	** In the prolog, we insert proceedure sets and encodings.
	** Begin by downloading the proceedure set with dot matrix
	** printer emulation proceedures.
	*/
	puts("%%BeginProlog");
	gu_psprintf("%%%%IncludeResource: procset %s\n", DOTMATRIX);

	/* If we will be printing graphics, emmit the routines here. */
	if(uses_graphics)
		{
		if(level2)
			gu_psprintf("%%%%IncludeResource: procset %s\n", DOTMATRIXG2);
		else
			gu_psprintf("%%%%IncludeResource: procset %s\n", DOTMATRIXG1);
		}

	/*
	** If a character set other than Standard was selected and non-ASCII
	** characters are used in at least one of the four fonts, then
	** download the required encoding and a proceedure to re-encode
	** a font.
	*/
	if(encoding_name)
		{
		gu_psprintf("%%%%IncludeResource: procset %s\n", REENCODE);
		gu_psprintf("%%%%IncludeResource: encoding %s\n", encoding_name);
		}

	/*
	** If we are using proportional spacing, download
	** the proportional font metrics procedure sets.
	*/
	if(need_proportional_procset)
		gu_psprintf("%%%%IncludeResource: procset %s\n", NEWMETRICS);
	{
    int i;
    for(i=0; i<4; i++)
    	{
		if(*fonts[i].uses_proportional)
			gu_psprintf("%%%%IncludeResource: procset "METRICSEPSON"\n", i+1);
    	}
	}

	/* If colour needed, download that proceedure set. */
	if(uses_colour)
		gu_psprintf("%%%%IncludeResource: procset %s\n", COLOUR);

	fputs("%%EndProlog\n\n",stdout);

	/*
	** In the setup section, we can execute things.
	** We begin by putting our dictionary on the top
	** of the stack.
	*/
	puts("%%BeginSetup");

	/* Select the page size we want. */
	gu_psprintf("%%%%IncludeFeature: *PageSize %s\n", PageSize);

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

	/*
	** Get the Dotmatrix emulation dictionary ready to go:
	*/
	puts("pprdotmatrix begin");
	puts("pprdotmatrix_init");

	/*
	** If we will use Pinwriter 6 units, change some of
	** the scaling variables.
	*/
	if(emulation & EMULATION_24PIN_UNITS)
		{
		puts("/xfactor 360 72 div def");		/* convert horizontal units to PostScript units */
		puts("/yfactor 360 72 div def");		/* same for vertical units */
		puts("/bunit 60 def");					/* printer's basic unit */
		}

	/*
	** If we are emulating a narrow carriage printer,
	** we must arrange to have things shifted 0.25
	** inch to the right.
	*/
	if(emulation & EMULATION_8IN_LINE)
		puts("/xshift 0.25 inch def");

	/*
	** If an x or y shift has been specified, emmit it.
	*/
	if(xshift)
		gu_psprintf("/xshift xshift %d add def\n", xshift);
	if(yshift)
		gu_psprintf("/yshift yshift %d add def\n", yshift);

	gu_psprintf("\n");

	/* Download all the fonts we need. */
	{
	int i;
	for(i=0; i<4; i++)
		{
		if(*fonts[i].uses)
			gu_psprintf("%%%%IncludeResource: font %s\n", fonts[i].font_info->font_psname);
		if(*fonts[i].uses_nonascii)
			gu_psprintf("/%s /%s /%s ReEncode\n", fonts[i].font_info->font_psname, fonts[i].font_info->font_psname, encoding_name);
		if(*fonts[i].uses)
			gu_psprintf("/%s /%s findfont 12 scalefont def\n", fonts[i].command, fonts[i].font_info->font_psname);
		if(*fonts[i].uses_proportional)
			{
			gu_psprintf("/%s /PS%s MetricsEpson_%s NewMetrics\n", fonts[i].font_info->font_psname, fonts[i].font_info->font_psname, fonts[i].prop_tbl_name);
			gu_psprintf("/p%s /PS%s findfont 12 scalefont def\n", fonts[i].command, fonts[i].font_info->font_psname);
			}
		gu_psprintf("\n");
		}
	}
	
	/*
	** If there are any normal characters, it is ok to select
	** normal characters as the default.
	*/
	if(uses_normal)
		fputs("f 1 sf\n", stdout);

	fputs("%%EndSetup\n\n", stdout);
	} /* end of top_of_document() */

/*
** This subroutine is called at the bottom of the document.
*/
void bottom_of_document(void)
	{
	puts("%%Trailer");
	puts("end % pprdotmatrix");
	gu_psprintf("%%%%Pages: %d\n",current_page);
	puts("%%EOF");
	} /* end of bottom_of_document() */

/*
** This subroutine is called at the top of each page.
*/
void top_of_page(void)
	{
	/* increment page number */
	current_page++;

	/* current point has been lost */
	postscript_xpos = postscript_ypos = -1;

	/* Colour setting has been lost. */
	postscript_print_colour=COLOUR_BLACK;

	/* emmit appropriate postscript code to start a page */
	gu_psprintf("%%%%Page: %d %d\n",current_page,current_page);
	puts("%%BeginPageSetup");
	puts("bp");
	puts("%%EndPageSetup");

	/* Rest the line output code */
	buffer_top_of_page_reset();

	/* Reset the line spacing compression variables */
	lm = ls = 0;
	} /* end of top_of_page() */

/*
** This subroutine is called at the bottom of each page.
*/
void bottom_of_page(void)
	{
	puts("%%PageTrailer");
	puts("ep");
	puts("");
	} /* end of bottom_of_page() */

/*
** Issue PostScript code to aquire the position named in
** xpos and ypos.
*/
void achieve_position(void)
	{
	#ifdef DEBUG_COMMENTS
	if(xpos != postscript_xpos || ypos != postscript_ypos)
		gu_psprintf("%% xpos=%d, ypos=%d, lm=%d, ls=%d, postscript_xpos=%f, postscript_ypos=%d\n",
				xpos, ypos, lm, ls, postscript_xpos, postscript_ypos);
	#endif

	/* If both x and y must change, */
	if(xpos != postscript_xpos && ypos != postscript_ypos)
		{
		/* If it is a CRLF, */
		if(xpos == lm && ypos == (postscript_ypos - ls))
			{
			fputs("n\n",stdout);
			}
		else
			{
			gu_psprintf("%d %d mxy\n", xpos, ypos);
			lm = xpos;
			ls = postscript_ypos - ypos;
			}
		}

	/* If only y must change, */
	else if( ypos != postscript_ypos )
		{
		gu_psprintf("%d my\n", ypos);
		ls = postscript_ypos - ypos;
		}

	/* If only x must change, */
	else if( xpos != postscript_xpos )
		{
		int movement = xpos - postscript_xpos;

		if(movement > 0 && movement < HORIZONTAL_UNITS)
			gu_psprintf("%d m\n", movement);
		else
			gu_psprintf("%d mx\n", xpos);
		}

	/* We believe at one of the above did the job. */
	postscript_xpos = xpos;
	postscript_ypos = ypos;
	} /* end of achieve_position() */

/* end of file */
