/*
** mouse:~ppr/src/filter_dotmatrix/main.c
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

/*
** This is PPR's filter to convert typical dot matrix
** printer code to PostScript.
*/

#include "filter_dotmatrix.h"

/* Name of this program, for error messages. */
const char myname[] = "filter_dotmatrix";

/* Should we print information helpful for getting the filter options right? */
int noisy = FALSE;

/* Should we emmit colour code? */
int colour_ok = FALSE;

/* "Power Up" settings. */
static int initial_perfskip = 0;		/* measured in lines. */

/* Should we use level 2 features? */
int level2 = FALSE;

/* PostScript encoding we are currently using. */
const char *opt_charset = "CP437";

/*
** These constants describe the horizontal and vertical spacing units.
** They are set for 9 pin printers.  If 24 pin printer emulation is
** selected, then they are both changed to 360.
*/
int HORIZONTAL_UNITS = 240;
int VERTICAL_UNITS = 216;

/*
** Factor to convert 72ths or 60ths to VERTICAL_UNITS
** Value is 3 for 9 pin printers, 6 for 24 pin printers.
*/
int VFACTOR = 3;

/*
** The page dimensions and other information for the
** %%DocumentMedia comment:
*/
char *PageSize;											/* "Letter" for letter */
double phys_pu_width;									/* 612.0 for letter */
double phys_pu_height;									/* 792.0 for letter */
double MediaWeight;										/* probably 75.0 */
const char *MediaColor;									/* probably "white" */
const char *MediaType;									/* probably "" */

/*
** Duplex mode to select:
*/
int duplex_mode = DUPLEX_UNDEF;

/*
** These variables describe the page length and width we think
** we are printing on in terms of the above units
*/
int page_width;
int page_length;

/*
** These describe the number of points to shift the page upward
** and to the right.  These shifts are added to any that are
** already in effect.  There will be a 0.25 inch right shift in
** 8in lines are selected.  There is normally a -12/72 downward shift
** to bring the first baseline below the stop of the page.
*/
int xshift = 0;
int yshift = 0;

/*
** This describes the current character spacing and line spacing
** in terms of the above units.
*/
int current_char_spacing;
int current_line_spacing;
int line_feed_direction;				/* 1 or -1 */
int line_spacing_multiplier;			/* 1 or 2 */

/* These variables describe the next desired print possition. */
int xpos, ypos;

/*
** These variables keep track of where the PostScript code thinks we are.
** These variables are used by achieve_position().  There is a reason
** postscript_xpos is a double, I just don't remember precisely what it is.
** It has something to do with rounding errors in justified text.
*/
double postscript_xpos;
int postscript_ypos;

/* This is the current page number for the PostScript comments. */
int current_page = 0;

/* This variable is TRUE if a page is started. */
int in_page = FALSE;

/* Are high bit control codes allowed? */
int upper_controls;

/*
** The printer language uses these to describe the
** current character syle.
*/
int current_charmode;					/* ESC "!" format */
int one_line_expanded;					/* one line expanded mode */
int simple_compressed;					/* SI compressed mode */
int script_mode;						/* Script mode (superscript, subscript). */
int international_char_set;				/* International character set number */
int nlq_mode;							/* Near Letter Quality, FALSE or TRUE */
int nlq_font;							/* Roman or San-Serif */
int justification;						/* Justification mode */
int charset;							/* Italic or IBM */
int extra_dot_spacing;					/* Extra space added to char spacing */
int print_colour;						/* Print colour */

/* We use these to describe what has been propagated. */
int omode_current_charmode = 0;			/* What we have propagated */
int omode_one_line_expanded = FALSE;	/* Have we propogated it to out_hscale? */
int omode_simple_compressed = FALSE;	/* Have we propogated it to out_hscale? */
int omode_script_mode = SCRIPT_NONE;	/* What select_font() has propogated */

/*
** The routine select_font() uses these to describe the
** character style to the output routine.
*/
int out_style;
double out_hscale;
double out_vscale;

/* The currently active emulation features. */
int emulation;

/* The four alternate graphics modes */
int graphic_mode_K;
int graphic_mode_L;
int graphic_mode_Y;
int graphic_mode_Z;

/* The current tabs in terms of VERTICAL_UNITS and HORIZONTAL_UNITS */
int tabs_vertical[8][16];
int tabs_horizontal[32];
int vertical_tab_channel;

/*
** Margins.  All are expressed in terms of *_UNITS.  If top_margin is
** zero then half of perforation_skip is used instead.
*/
int left_margin;
int right_margin;
int top_margin;			/* always zero for most printers */
int perforation_skip;	/* amount to skip between pages */

/* Automatic line feed and carriage return. */
int auto_cr = TRUE;
int auto_lf = FALSE;

/*
** Handle fatal errors.
** Print a message and exit.
*/
void fatal(int exitval, const char message[], ... )
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
** For non fatal errors (generally from libppr).
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
** Select the correct font if we don't have it already.
** This is done my examining all of the variables that
** reflect the dot-matrix printer state and condensing
** them into:
**
**	out_style
**	out_hscale, out_vscale
**	current_character_spacing
*/
void select_font(void)
	{
	#ifdef DEBUG_COMMENTS
	printf("%% select_font(): current_charmode=%d, one_line_expanded=%s, simple_compressed=%s, script_mode=%d\n",
		current_charmode,
		one_line_expanded ? "TRUE" : "FALSE",
		simple_compressed ? "TRUE" : "FALSE",
		script_mode
		);
	#endif
	if(current_charmode != omode_current_charmode
			|| one_line_expanded != omode_one_line_expanded
			|| simple_compressed != omode_simple_compressed
			|| script_mode != omode_script_mode)
		{
		/* Establish the font style */
		out_style = 0;
		if(current_charmode & (MODE_EMPHASIZED | MODE_DOUBLE_STRIKE))
			{					/* bold */
			out_style |= OSTYLE_BOLD;
			}
		if(current_charmode & MODE_ITALIC)
			{					/* italic */
			out_style |= OSTYLE_OBLIQUE;
			}

		/* Proportional spacing. */
		if(current_charmode & MODE_PROPORTIONAL)
			out_style |= OSTYLE_PROPORTIONAL;

		/* Horizontal scaling */
		out_hscale = 1.0;

		if(current_charmode & MODE_ELITE)
			out_hscale *= FACTOR_ELITE;

		if(current_charmode & MODE_15PITCH)		/* 15 pitch */
			{									/* If compressed, is same as elite compressed */
			if((current_charmode & MODE_CONDENSED) || simple_compressed)
				out_hscale *= FACTOR_ELITE;
			else
				out_hscale *= FACTOR_15PITCH;
			}

		if((current_charmode & MODE_EXPANDED) || one_line_expanded)
			out_hscale *= FACTOR_EXPANDED;


		if(current_charmode & MODE_3X_HORIZONTAL)
			out_hscale *= 3.0;

		if(current_charmode & MODE_4X_HORIZONTAL)
			out_hscale *= 4.0;

		if(current_charmode & MODE_CONDENSED)	/* condensed printing */
			out_hscale *= FACTOR_CONDENSED;

		if(simple_compressed)					/* this one overrides */
			out_hscale=FACTOR_CONDENSED;		/* others */

		/* Vertical scaling */
		out_vscale=1.0;

		if(current_charmode & MODE_2X_VERTICAL)
			{									/* double height */
			out_vscale *= 2.0;					/* lowered baseline */
			out_style |= OSTYLE_FULLDROP;
			}
		else if(current_charmode & MODE_2X_VERTICAL_BASELINE)
			{									/* double height, */
			out_vscale *= 2.0;					/* normal baseline */
			}
		else if(current_charmode & MODE_4X_VERTICAL)
			{									/* quadruple height */
			out_vscale *= 4.0;
			/* !!! what here? */
			}

		/* Superscript and Subscript */
		if(script_mode == SCRIPT_SUB)
			{
			out_vscale*=FACTOR_SCRIPT;
			}
		else if(script_mode == SCRIPT_SUPER)
			{
			out_vscale *= FACTOR_SCRIPT;
			out_style |= OSTYLE_HALFRAISE;
			}

		/* Other special styles */
		if(current_charmode & MODE_UNDERLINE)	/* Underline */
			out_style |= OSTYLE_UNDERLINE;

		/* Set variables so that we know this has gone thru. */
		omode_current_charmode = current_charmode;
		omode_one_line_expanded = one_line_expanded;
		omode_simple_compressed = simple_compressed;
		omode_script_mode = script_mode;

		/* Set current_char_spacing properly */
		current_char_spacing = (int)((double)(HORIZONTAL_UNITS / 10) * out_hscale + 0.5);
		}

	#ifdef DEBUG_COMMENTS
	printf("%% select_font(): out_style=%d, out_hscale=%f, out_vscale=%f\n",out_style,out_hscale,out_vscale);
	#endif
	} /* end of select_font() */

/*
** Reset the printer.
** Hard or soft reset.
** (NEC P6 has both.)
*/
void reset(int hard)
	{
	/* Clear the line buffer */
	buffer_delete(0);

	/* Return to 10CPI */
	if(hard)
		{
		current_charmode=MODE_PICA;
		one_line_expanded=FALSE;
		simple_compressed=FALSE;
		script_mode=SCRIPT_NONE;		/* no super/subscript */
		international_char_set=0;		/* Set international character set to USA */
		nlq_mode=FALSE;
		charset=CHARSET_EXTENDED;
		nlq_font=NLQ_ROMAN;
		extra_dot_spacing=0;			/* No extra inter-character spacing */
		print_colour=COLOUR_BLACK;
		}

	/* Set select font variables to default values. */
	out_style=0;
	out_hscale=1.0;
	out_vscale=1.0;

	/* Restore default vertical tab channel */
	vertical_tab_channel = 0;

	/* Reset all tabs */
	reset_tabs();

	/* Reset the form length */
	page_length = (int)(phys_pu_height / INCH * (double)VERTICAL_UNITS + 0.5);
	page_width = (int)(phys_pu_width / INCH * (double)HORIZONTAL_UNITS + 0.5);

	/* Reset the spacing */
	current_char_spacing = HORIZONTAL_UNITS/10; /* 10 CPI */
	current_line_spacing = VERTICAL_UNITS/6;	/* 6 LPI */
	line_feed_direction = 1;					/* forward */
	line_spacing_multiplier = 1;				/* normal */

	/* Clear margins */
	left_margin = 0;
	right_margin = page_width;
	perforation_skip = initial_perfskip * (HORIZONTAL_UNITS/10);
	top_margin = 0;

	/* return the graphics modes to their defaults. */
	graphic_mode_K = GRAPHICS_60;
	graphic_mode_L = GRAPHICS_120a;
	graphic_mode_Y = GRAPHICS_120b;
	graphic_mode_Z = GRAPHICS_240;
	} /* end of reset */

/*
** Reset at top of page. The argument "consumed" is the amount of space
** already eaten up at the start of the page by an uncompleted vertical
** movement command.
*/
static void page_reset(int consumed)
	{
	/* move to top of page */
	if(top_margin == 0)									/* If we don't use top margin, */
		ypos = page_length - (perforation_skip/2);		/* top margin is half of perforation skip. */
	else												/* If explicit top magin, */
		ypos = page_length - top_margin;				/* use it. */

	ypos -= consumed;

	/* Move to left margin */
	xpos = left_margin;
	} /* end of page_reset() */

/*
** Do a line feed of the specified number of VERTICAL_UNITS
*/
void line_feed(int how_far)
	{
	empty_buffer();		/* per Epson manual */

	ypos-=how_far;		/* Move down specified amount */

	#ifdef DEBUG_COMMENTS
	printf("%% line feed is %d units long, resulting ypos=%d\n",how_far,ypos);
	printf("%% top_margin=%d, perforation_skip=%d\n",top_margin,perforation_skip);
	#endif

	/* Move to next page if necessary. */
	if(ypos < (top_margin ? perforation_skip : (perforation_skip/2)))
		{
		bottom_of_page();		/* close out this page */
		in_page=FALSE;			/* no page currently in progress */

		if(ypos < 0)			/* !!! This is close to correct */
			page_reset(-ypos);
		else
			page_reset(0);

		#ifdef DEBUG_COMMENTS
		printf("%% new page, ypos=%d\n",ypos);
		#endif
		}
	} /* end of line_feed() */

/* This routine is the main loop. */
static void process_input(void)
	{
	int c;

	while( (c=input()) != EOF )
		{
		switch(c)
			{
			case 7:						/* beep */
				#ifdef DEBUG_COMMENTS
				puts("% beep (ignored)");
				#endif
				break;					/* ignore */
			case 8:						/* backspace */
				#ifdef DEBUG_COMMENTS
				puts("% backspace");
				#endif
				empty_buffer();			/* <-- per epson manual */
				xpos-=current_char_spacing;
				if(xpos==0) xpos=0;
				break;
			case 9:						/* horizontal tab */
				#ifdef DEBUG_COMMENTS
				puts("% tab");
				#endif
				empty_buffer();			/* <-- per epson manual */
				horizontal_tab();
				break;
			case 10:					/* line feed */
				#ifdef DEBUG_COMMENTS
				puts("% line feed");
				#endif
				line_feed(current_line_spacing*line_feed_direction*line_spacing_multiplier);
				if(auto_cr)
					goto carriage_return;
				break;
			case 11:					/* vertical tab */
				#ifdef DEBUG_COMMENTS
				puts("% vertical tab");
				#endif
				empty_buffer();			/* <-- per epson manual */
				vertical_tab();
				break;
			case 12:					/* form feed */
				#ifdef DEBUG_COMMENTS
				puts("% form feed");
				#endif
				empty_buffer();			/* <-- per epson manual */
				if(!in_page)
					top_of_page();
				bottom_of_page();
				in_page = FALSE;
				page_reset(0);
				break;
			case 13:					/* <-- carriage return */
				#ifdef DEBUG_COMMENTS
				puts("% carriage return");
				#endif
				empty_buffer();			/* per epson manual */
				if(auto_lf)				/* Auto line feed? */
					line_feed(current_line_spacing*line_feed_direction*line_spacing_multiplier);
				carriage_return:		/* for auto_cr */
				xpos=left_margin;
				one_line_expanded=FALSE;
				select_font();			/* font might have changed */
				#ifdef DEBUG_COMMENTS
				printf("%% carriage return result:	xpos=%d, ypos=%d\n",xpos,ypos);
				#endif
				break;
			case 14:					/* one line expanded mode */
				one_line_expanded=TRUE;
				select_font();
				break;
			case 15:			/* condensed mode on */
				empty_buffer(); /* per epson manual */
				simple_compressed=TRUE;
				select_font();
				break;
			case 17:			/* printer active */
				break;			/* just ignore */
			case 18:			/* condensed mode off */
				simple_compressed=FALSE;
				select_font();
				break;
			case 19:			/* printer inactive */
				break;			/* not implemented */
			case 20:			/* expanded mode off */
				one_line_expanded=FALSE;
				select_font();
				break;
			case 24:			/* cancel buffer */
				buffer_delete(0);
				break;
			case 27:			/* start of ESC code */
				escape();
				break;
			case 28:			/* start of NEC FS code */
				fs();
				break;
			case 127:			/* delete last text character in buffer */
				buffer_delete(1);
				break;
			default:			/* emulate printer line buffer */
				if(!in_page)
					{
					top_of_page();
					in_page = TRUE;
					}
				buffer_add(c);
				break;
			}
		} /* end of while */

	/* close out any unfinished page */
	if(in_page)
		bottom_of_page();
	} /* end of process_input() */

/*
** Read command line parameters, if any, and call process_input().
** Usage: filter_dotmatrix 'option1...optionN' _printer_ _title_
*/
int main(int argc, char *argv[])
	{
	int explicit_pins = FALSE;			/* did uses request 24 pin? */

	/* Initialize international messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	/* Read the default pagesize from the PPR config file. */
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

	/* Set default emulation mode */
	emulation = 0;						/* was once set to EMULATION_8IN_LINE */

	/* Process the options. */
	if(argc >= 2)
		{
		struct OPTIONS_STATE o;
		char name[32], value[64];
		int rval;

		/* Process all the segments of the options string. */
		options_start(argv[1], &o);
		while((rval = options_get_one(&o, name, sizeof(name), value, sizeof(value))) == 1)
			{
			/*---------------------------------------------
			** Set the debuging mode on or off
			**-------------------------------------------*/
			if(strcmp(name, "noisy") == 0)
				{
				if( (noisy = gu_torf(value)) == ANSWER_UNKNOWN )
					filter_options_error(1, &o, _("Value must be boolean."));
				}

			/*---------------------------------------------
			** Is it ok to emmit colour code?
			**-------------------------------------------*/
			else if(strcmp(name, "colour") == 0 || strcmp(name, "color") == 0)
				{
				if((colour_ok = gu_torf(value)) == ANSWER_UNKNOWN)
					filter_options_error(1, &o, _("Value must be boolean."));
				}

			/*---------------------------------------------
			** Select a dot matrix printer to emulate.
			**-------------------------------------------*/
			else if(strcmp(name, "emulation") == 0)
				{
				/* Basic Epson emulation is the default. */
				if(gu_strcasecmp(value, "epson") == 0)
					{
					emulation &= ~EMULATION_CONFLICTING_IBM;
					emulation &= ~EMULATION_P6_INTERPRETATION;
					}

				/*
				** If the user choses "proprinter", set flag to
				** enable all those conflicting proprinter commands.
				*/
				else if(gu_strcasecmp(value, "proprinter") == 0)
					{
					emulation |= EMULATION_CONFLICTING_IBM;
					emulation &= EMULATION_P6_INTERPRETATION;
					}

				/*
				** If the user chooses "p6", select 24 pin emulation
				** with P6 interpretation of varying commands.
				*/
				else if(gu_strcasecmp(value, "p6") == 0)
					{
					emulation = EMULATION_24PIN_UNITS | EMULATION_P6_INTERPRETATION;
					}

				/* No other emulations are supported. */
				else
					{
					filter_options_error(1, &o, _("Unrecognized emulation."));
					}
				}

			/*-------------------------------------------
			** Ask for a certain duplex mode
			**-----------------------------------------*/
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
					filter_options_error(1, &o, _("Valid \"duplex=\" values are \"none\", \"undef\", \"tumble\", and \"notumble\""));
				}

			/*------------------------------------------
			** Set a non-default page size:
			**----------------------------------------*/
			else if(strcmp(name, "pagesize") == 0)
				{
				if(pagesize(value, &PageSize, &phys_pu_width, &phys_pu_height, NULL) == -1)
					filter_options_error(1, &o, _("Value is not among those allowed."));
				}

			/*-------------------------------------------
			** MediaType for "%%Media:" comment:
			**-----------------------------------------*/
			else if(strcmp(name, "mediatype") == 0)
				{
				if(is_unsafe_ps_name(value))
					filter_options_error(1, &o, _("Value contains illegal characters."));
				MediaType = gu_strdup(value);
				}

			/*-------------------------------------------
			** MediaColor for "%%Media:" comment:
			**------------------------------------------*/
			else if(strcmp(name, "mediacolour") == 0 || strcmp(name, "mediacolor") == 0)
				{
				if(is_unsafe_ps_name(value))
					filter_options_error(1, &o, _("Value contains illegal characters."));
				MediaColor = gu_strdup(value);
				}

			/*--------------------------------------------
			** MediaWeight for "%%Media:" comment:
			**------------------------------------------*/
			else if(strcmp(name, "mediaweight") == 0)
				{
				MediaWeight = atof(value);
				}

			/*--------------------------------------------
			** Set the default perforation skip:
			**------------------------------------------*/
			else if(strcmp(name, "perfskip") == 0)
				{
				if((initial_perfskip = atoi(value)) < 0 || initial_perfskip > 65)
					{
					filter_options_error(1, &o, _("Value is unreasonable."));
					}
				}

			/*-----------------------------------------------------
			** Select whether to emulation a 9 or 24 pin printer.
			**---------------------------------------------------*/
			else if(strcmp(name, "pins") == 0)
				{
				int x;

				explicit_pins=TRUE;

				x = atoi(value);

				if(x==9)
					emulation &= ~EMULATION_24PIN_UNITS;
				else if(x==24)
					emulation |= EMULATION_24PIN_UNITS;
				else
					{
					filter_options_error(1, &o, _("Only 9 and 24 are valid values."));
					}
				}

			/*----------------------------------------------
			** Select the character set.
			**--------------------------------------------*/
			else if(strcmp(name, "charset") == 0)
				{
				opt_charset = gu_strdup(value);
				}

			/*---------------------------------------------
			** Select a LangaugeLevel for PostScript
			---------------------------------------------*/
			else if(strcmp(name, "level")==0)
				{
				int x = atoi(value);
				if(x == 1)
					level2 = FALSE;
				else if(x >= 2)
					level2 = TRUE;
				else
					{
					filter_options_error(1, &o, _("Value must be a positive integer."));
					}
				}

			/*----------------------------------------------
			** Possibly select an 8 inch line
			----------------------------------------------*/
			else if(strcmp(name, "narrowcarriage") == 0)
				{
				switch( gu_torf(value) )
					{
					case ANSWER_TRUE:
						emulation |= EMULATION_8IN_LINE;
						break;
					case ANSWER_FALSE:
						emulation &= ~EMULATION_8IN_LINE;
						break;
					case ANSWER_UNKNOWN:
					default:
						filter_options_error(1, &o, _("Value must be boolean."));
					}
				}

			/*----------------------------------------------
			** Look for x or y shift
			**--------------------------------------------*/
			else if(strcmp(name,"xshift")==0)
				xshift = atoi(value);
			else if(strcmp(name,"yshift")==0)
				yshift = atoi(value);

			/*----------------------------------------------
			** If noisy is TRUE tell what options we are
			** ignoring.
			**--------------------------------------------*/
			else if(noisy)
				{
				fprintf(stderr, _("Ignoring option \"%s=%s\".\n"), name, value);
				}

			} /* end of while loop for each name=value pair */

		/*
		** If options_get_one() failed, print the error message.
		*/
		if(rval == -1)
			{
			filter_options_error(1, &o, gettext(o.error));
			}

		} /* end of if options exist */

	/* Reset the "printer" for the dry run. */
	reset(TRUE);						/* complete printer reset */
	page_reset(0);						/* set variables for top of page */

	/* Do the first pass. */
	pass1();

	/* If the user did not manually set pins= and we saw 24 pin commands, */
	/* change to 24 pin mode now. */
	if(!explicit_pins && uses_24pin_commands)
		emulation |= EMULATION_24PIN_UNITS;

	/* Rewind the input file. */
	rewind_input();

	/* Make adjustments if we have decided to use 24pin units. */
	if(emulation & EMULATION_24PIN_UNITS)
		{
		HORIZONTAL_UNITS=360;
		VERTICAL_UNITS=360;
		VFACTOR=6;
		}

	/* Reset the printer again. */
	reset(TRUE);
	page_reset(0);

	/* Emmit the PostScript header. */
	top_of_document();

	/* Do the real work. */
	process_input();

	/* Emmit the PostScript trailer. */
	bottom_of_document();

	/* Tell ppr that filtering went well. */
	return 0;
	} /* end of document */

/* end of file */

