/*
** mouse:~ppr/src/filter_dotmatrix/escape.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 6 July 1999.
*/

/*
** This file is part of PPR's dot matrix printer emulator.
** This module is responsible for interpreting ESC and FS codes.
**
** The functions in the module compile into two versions according to
** whether PASS1 is defined or not.
*/

#include <stdio.h>
#include <ctype.h>
#include "filter_dotmatrix.h"

/*
** A macro to get a new character, handling EOF.
*/
#define NEWC {if( (c=input()) == EOF ) return;}

/*
** Handle escape codes.
*/
#ifdef PASS1
void escape_pass1(void)
#else
void escape(void)
#endif
	{
	int c;								/* The current character */
	int mode;							/* Graphics mode or some such thing */
	int length;							/* Length of object such as graphic */

	NEWC;								/* Get the character after the ESC. */

	switch(c)							/* act on it */
		{
		case 14:						/* NEC one line expanded */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% NEC one line expanded\n");
			#endif
			one_line_expanded=TRUE;
			select_font();
			#endif
			break;
		case 15:						/* NEC simple compressed */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% NEC simple compressed\n");
			#endif
			simple_compressed=TRUE;
			select_font();
			#endif
			break;
		case 25:						/* Set cut-sheet option */
			NEWC;						/* ignore */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% cut sheet option = %d (ignored)\n",c);
			#endif
			#endif
			break;
		case 32:						/* set additional spacing */
			NEWC;						/* (NEC P6) */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% extra_dot_spacing=%d\n",c);
			#endif
			extra_dot_spacing=c;
			#endif
			break;
		case '!':						/* Select Master Print mode */
			NEWC;
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% Master print mode: %d\n",c);
			#endif
			#endif
			current_charmode=c;			/* both passes */
			#ifndef PASS1
			select_font();
			#endif
			break;
		case '#':						/* Cancel control of 8th bit */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% cancel control of 8th bit\n");
			#endif
			#endif
			set8th=0;
			clear8th=0xFF;
			break;
		case '$':						/* move to absolute position */
			NEWC;						/* get position in 60ths of an inch */
			#ifndef PASS1
			length=c;
			#endif
			NEWC;
			#ifndef PASS1
			length+=c*256;
			xpos=length * 6;

			#ifdef DEBUG_COMMENTS
			printf("%% move to horizontal position %d/60ths, xpos=%d\n", length, xpos);
			#endif

			#endif
			break;
		case '%':						/* Activate character set */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("%% Activate character set (ignored)");
			#endif
			#endif
			NEWC;						/* ignore for now */
			NEWC;
			break;
		case '&':						/* Define user characters */
			NEWC;						/* eat zero */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% define user characters (ignored)\n");
			#endif

			/* missing code */

			#endif
			break;
		case '*':						/* Graphics mode */
			NEWC;						/* get the graphics mode to enter */
			mode=c;
			NEWC;						/* get the number of columns */
			length=c;					/* to expect as a 16 bit number */
			NEWC;
			length+=c*256;

			#ifdef PASS1
			eat_graphic(mode,PINS_8or24,length);
			#else
			#ifdef DEBUG_COMMENTS
			printf("%% graphics mode %d, %d columns\n",mode,length);
			#endif
			graphic(mode,PINS_8or24,length);
			#endif
			break;
		case '+':				/* set line spacing to n/360ths of an inch */
			NEWC;				/* (Panasonic PX-P1124) */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% line spacing: %d/360ths inch (24 pin)\n",c);
			#endif
			current_line_spacing=c;
			#endif
			break;
		case '-':						/* Underline on/off */
			NEWC;
			#ifndef PASS1
			switch(c)
				{
				case 0:
				case '0':
					#ifdef DEBUG_COMMENTS
					puts("% underline off");
					#endif
					current_charmode &= ~MODE_UNDERLINE;
					break;
				case 1:
				case '1':
					#ifdef DEBUG_COMMENTS
					puts("% underline on");
					#endif
					current_charmode|=MODE_UNDERLINE;
					break;
				default:
					#ifdef DEBUG_COMMENTS
					puts("% invalid underline command (ESC -)");
					#endif
					fprintf(stderr,"Invalid ESC - command\n");
					break;
				}
			select_font();				/* comes under the heading of fonts */
			#endif
			break;
		case '/':						/* Vertical tab channel */
			NEWC;
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% switch to vertical tab channel %d\n",c);
			#endif
			#endif
			if(c < 8)
				vertical_tab_channel=c;
			else
				fprintf(stderr,"Illegal value in ESC /\n");
			break;
		case '0':						/* 1/8 inch spacing */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% 1/8th inch line spacing");
			#endif
			current_line_spacing=VERTICAL_UNITS / 8;
			#endif
			break;
		case '1':						/* 7/72 inch spacing (what about NEC?) */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% 7/72ths inch line spacing");
			#endif
			current_line_spacing=(7 * VFACTOR);
			#endif
			break;
		case '2':						/* 1/6 inch spacing */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% 1/6th inch line spacing");
			#endif
			current_line_spacing=VERTICAL_UNITS / 6;
			#endif
			break;
		case '3':						/* n/216 inch spacing */
			NEWC;						/* or n/180 */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% %d/216th inch line spacing\n",c);
			#endif
			if(emulation & EMULATION_24PIN_UNITS)
				current_line_spacing=c*2;
			else
				current_line_spacing=c;;
			#endif
			break;
		case '4':						/* Italic on */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% italic on");
			#endif
			#endif

			current_charmode|=MODE_ITALIC; /* needed for both passes */

			#ifndef PASS1
			select_font();
			#endif
			break;
		case '5':						/* Italic off */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% italic off");
			#endif
			#endif

			current_charmode&=(~MODE_ITALIC); /* needed for both passes */

			#ifndef PASS1
			select_font();
			#endif
			break;
		case '6':						/* disable high bit controls */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% disable high bit controls");
			#endif
			#endif
			upper_controls=FALSE; /* (Epson FX-850, not LX-80 manual) */
			break;
		case '8':						/* disable paper out */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% disable paper out sensor (ignored)");
			#endif
			#endif
			break;
		case '9':						/* enable paper out */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% enable paper out sensor (ignored)");
			#endif
			#endif
			break;						/* ignore */
		case ':':						/* copy ROM characters to RAM */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% copy ROM characters to RAM (ignored)");
			#endif
			#endif
			NEWC;
			NEWC;
			NEWC;
			break;
		case '<':						/* one line unidirectional mode */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% one line unidirectional mode (ignored)");
			#endif
			#endif
			break;
		case '=':						/* set eight bit to 0 */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% set eight bit to 0");
			#endif
			#endif
			set8th=0;
			clear8th=0x7F;
			break;
		case '>':						/* set eight bit to 1 */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% set eight bit to 1");
			#endif
			#endif
			set8th=0x80;
			clear8th=0;
			break;
		case '?':						/* Define graphics modes */
			NEWC;
			#ifndef PASS1

			#ifdef DEBUG_COMMENTS
			puts("% re-define graphics mode");
			#endif

			switch(c)
				{
				case 'K':
					NEWC;
					graphic_mode_K=c;
					break;
				case 'L':
					NEWC;
					graphic_mode_L=c;
					break;
				case 'Y':
					NEWC;
					graphic_mode_Y=c;
					break;
				case 'Z':
					NEWC;
					graphic_mode_Z=c;
					break;
				default:
					NEWC;
					fprintf(stderr,"Invalid ESC ? command\n");
					break;
				}

			#endif
			break;
		case '@':						/* Reset printer */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% reset printer");
			#endif

			/* NEC interpretation calls for a soft reset */
			if(emulation & EMULATION_P6_INTERPRETATION)
				reset(FALSE);
			else
				reset(TRUE);

			#endif
			break;
		case 'A':						/* VMI to n/72ths (n/60ths, NEC) */
			NEWC;
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% VMI = %d/72ths\n",c);
			#endif
			current_line_spacing=(c * VFACTOR);
			#endif
			break;
		case 'B':						/* set vertical tabs */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% set vertical tabs, channel 0");
			#endif
			#endif
			vertical_tabs_set(0);
			break;
		case 'C':						/* set form length to N lines */
			NEWC;						/* or N inches */
			if(c)						/* if c is not zero */
				{						/* it is lines */
				#ifndef PASS1
				if(c < 128)
					page_length=current_line_spacing * c;
				else
					fprintf(stderr,"Illegal number of lines in ESC C\n");

				#ifdef DEBUG_COMMENTS
				printf("%% form length to %d lines\n",c);
				#endif
				#endif
				}
			else
				{
				NEWC;
				#ifndef PASS1
				if(c < 23)
					page_length=c * VERTICAL_UNITS;
				else
					fprintf(stderr,"Illegal number of inches in ESC C 0\n");

				#ifdef DEBUG_COMMENTS
				printf("%% form length to %d inches\n",c);
				#endif
				#endif
				}
			/* we will not reset to top of form */
			break;
		case 'D':						/* set horizontal tabs */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% set horizontal tabs");
			#endif
			#endif
			horizontal_tabs_set();
			break;
		case 'E':						/* emphasized mode on (we will allow with elite and compressed) */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% emphasized mode on");
			#endif
			#endif

			current_charmode|=MODE_EMPHASIZED;	/* needed for both passes */

			#ifndef PASS1
			select_font();
			#endif
			break;
		case 'F':						/* emphasized mode off */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% emphasized mode off");
			#endif
			#endif

			current_charmode&=(~MODE_EMPHASIZED); /* needed for both passes */

			#ifndef PASS1
			select_font();
			#endif
			break;
		case 'G':						/* double strike mode on */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% double strike mode on");
			#endif
			#endif

			current_charmode|=MODE_DOUBLE_STRIKE;		/* both passes */

			#ifndef PASS1
			select_font();
			#endif
			break;
		case 'H':						/* double strike mode off */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% double strike mode off");
			#endif
			#endif

			current_charmode&=(~MODE_DOUBLE_STRIKE);	/* both passes */

			#ifndef PASS1
			select_font();
			#endif
			break;
		case 'J':						/* Immediate line feed of n/216ths */
			NEWC;						/* or possibly, n/180ths inch */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% immediate line feed of %d/216ths inch\n",c);
			#endif

			if(emulation & EMULATION_24PIN_UNITS)
				line_feed(c*2);			/* convert 180ths to 360ths */
			else
				line_feed(c);
			#endif
			break;
		case 'K':						/* single density graphics mode */
			NEWC;
			length=c;
			NEWC;
			length+=c*256;

			#ifdef PASS1
			eat_graphic(graphic_mode_K,PINS_8or24,length);
			#else
			#ifdef DEBUG_COMMENTS
			printf("%% single-density graphics (%d), %d columns\n",graphic_mode_K,length);
			#endif
			graphic(graphic_mode_K,PINS_8or24,length);
			#endif
			break;
		case 'L':						/* low-speed double-density graphics */
			NEWC;
			length=c;
			NEWC;
			length+=c*256;

			#ifdef PASS1
			eat_graphic(graphic_mode_L,PINS_8or24,length);
			#else
			#ifdef DEBUG_COMMENTS
			printf("%% low-speed double-density graphics (%d), %d columns\n",graphic_mode_L,length);
			#endif
			graphic(graphic_mode_L,PINS_8or24,length);
			#endif
			break;
		case 'M':						/* elite mode */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% elite mode");
			#endif
			current_charmode|=MODE_ELITE;
			select_font();
			#endif
			break;
		case 'N':				/* perforation skip to N lines */
			NEWC;
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% perforation skip: %d line(s)\n",c);
			#endif

			if(c < 128 && c > 1)
				perforation_skip=c*current_line_spacing;
			else
				fprintf(stderr,"Illegal value for ESC N\n");
			#endif
			break;
		case 'O':				/* turns perforation skip mode off */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% perforation skip off");
			#endif
			perforation_skip=0;
			#endif
			break;
		case 'P':				/* elite mode off */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% Elite off");
			#endif
			current_charmode&=(~MODE_ELITE);
			select_font();
			#endif
			break;
		case 'Q':				/* set right margin */
			NEWC;
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% right margin: %d\n",c);
			#endif
			right_margin=c * current_char_spacing;
			#endif
			break;
		case 'R':				/* set international character set */
			NEWC;
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% international character set: %d\n",c);
			#endif

			if(c <= 10)
				international_char_set=c;
			else
				fprintf(stderr,"Illegal value for ESC R\n");
			#endif
			break;
		case 'S':				/* Turns script mode on */
			NEWC;
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% script mode: %c\n",c);
			#endif

			switch(c)
				{
				case 0:			/* superscript */
				case '0':
					script_mode=SCRIPT_SUPER;
					break;
				case 1:			/* subscript */
				case '1':
					script_mode=SCRIPT_SUB;
					break;
				default:
					fprintf(stderr,"Invalid ESC S %d command\n",c);
					break;
				}
			select_font();
			#endif
			break;
		case 'T':				/* Turns script mode off */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% script mode off");
			#endif

			script_mode=SCRIPT_NONE;
			select_font();
			#endif
			break;
		case 'U':				/* Turn unidirectional mode on or off */
			NEWC;				/* ignore this command */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% unidirectional mode: %d (ignored)\n",c);
			#endif
			#endif
			break;
		case 'V':				/* repeats data in input buffer */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% repeat data (ignored)");
			#endif
			#endif

			/* code missing */

			break;
		case 'W':				/* Turns expanded mode on or off */
			NEWC;
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% expanded mode: %d\n",c);
			#endif

			switch(c)
				{
				case 0:			/* off */
					current_charmode &= ~MODE_EXPANDED;
					break;
				case 1:
					current_charmode |= MODE_EXPANDED;
					break;
				}
			select_font();
			#endif
			break;
		case 'Y':				/* High speed double-density graphics */
			NEWC;
			length=c;
			NEWC;
			length+=c * 256;

			#ifdef PASS1
			eat_graphic(graphic_mode_Y,PINS_8or24,length);
			#else
			#ifdef DEBUG_COMMENTS
			printf("%% high-speed double-density graphics (%d), %d columns\n",graphic_mode_Y,length);
			#endif
			graphic(graphic_mode_Y,PINS_8or24,length);
			#endif
			break;
		case 'Z':				/* Quadruple-density graphics */
			NEWC;
			length=c;
			NEWC;
			length+=c * 256;

			#ifdef PASS1
			eat_graphic(graphic_mode_Z,PINS_8or24,length);
			#else
			#ifdef DEBUG_COMMENTS
			printf("%% quadruple-density graphics (%d), %d columns\n",graphic_mode_Z,length);
			#endif
			graphic(graphic_mode_Z,PINS_8or24,length);
			#endif
			break;
		case '^':				/* 9 pin graphics */
			NEWC;
			mode=c;
			NEWC;
			length=c;
			NEWC;
			length+=c * 256;

			#ifdef PASS1
			eat_graphic(mode,PINS_9,length);
			#else
			#ifdef DEBUG_COMMENTS
			printf("%% 9 pin graphics (%d), %d columns\n",mode,length);
			#endif
			graphic(mode,PINS_9,length);
			#endif

			break;
		case 'a':				/* NLQ justification */
			NEWC;
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% NLQ justification: %d\n",c);
			#endif
			justification=c;
			#endif
			break;
		case 'b':				/* Set vertical tabs */
			NEWC;				/* get channel number */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% set vertical tabs, channel %d\n",c);
			#endif
			#endif
			if(c > 7)
				fprintf(stderr,"Invalid vertical tab channel in ESC b command\n");
			else
				vertical_tabs_set(c);
			break;
		case 'e':				/* Set horizontal or vertical tab increments */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% set tab increments");
			#endif
			#endif
			NEWC;
			switch(c)
				{
				case 0:
				case '0':
					NEWC;
					horizontal_tab_increment(c);
					break;
				case 1:
				case '1':
					NEWC;
					vertical_tab_increment(c);
					break;
				default:
					fprintf(stderr,"Invalid ESC e command\n");
				}
			break;
		case 'f':				/* Print spaces or line feeds */
			NEWC;
			#ifndef PASS1
			if(c < 128)			/* spaces */
				{
				#ifdef DEBUG_COMMENTS
				printf("%% %d spaces\n",c);
				#endif
				xpos+=c * current_char_spacing;
				}
			else
				{
				#ifdef DEBUG_COMMENTS
				printf("%% %d line feeds\n",c-128);
				#endif
				line_feed((c-128) * current_line_spacing * line_feed_direction * line_spacing_multiplier);
				}
			#endif
			break;
		case 'g':				/* select 15 pitch (P6) */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("15 pitch");
			#endif

			current_charmode &= ~MODE_ELITE;
			current_charmode |= MODE_15PITCH;
			select_font();
			#endif
			break;
		case 'h':				/* Select double or quadruple size */

			break;;
		case 'j':				/* reverse paper n/216ths or n/180ths inch */
			NEWC;
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% reverse paper %d units\n",c);
			#endif

			if(emulation & EMULATION_24PIN_UNITS)
				line_feed(c * -2);
			else
				line_feed(c * -1);
			#endif
			break;
		case 'k':				/* Select NLQ font */
			NEWC;				/* (Epson 850, Star NX-1000) */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% select NLQ font: %d\n",c);
			#endif
			nlq_font=c;
			select_font();
			#endif
			break;
		case 'l':				/* Left margin */
			NEWC;
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% left margin: %d\n",c);
			#endif

			left_margin = c * current_char_spacing;
			#endif
			break;
		case 'm':				/* Upper controls */
			NEWC;				/* (Epson LX-80, but not FX-850) */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% upper controls (ESC m): %d\n",c);
			#endif
			#endif
			switch(c)
				{
				case '4':		/* graphics available */
				case 4:
					upper_controls=FALSE;
					break;
				case '0':
				case 0:
					upper_controls=TRUE;
					break;
				default:
					fprintf(stderr,"Invalid ESC m command\n");
				}
			break;
		case 'p':				/* sets or cancels proportional printing (P6) */
			NEWC;
			#ifndef PASS1
			switch(c)
				{
				case '0':
				case 0:
					#ifdef DEBUG_COMMENTS
					puts("proportional spacing off");
					#endif
					current_charmode &= ~MODE_PROPORTIONAL;
					break;
				case '1':
				case 1:
					#ifdef DEBUG_COMMENTS
					puts("proportional spacing on");
					#endif
					current_charmode |= MODE_PROPORTIONAL;
					break;
				default:
					#ifdef DEBUG_COMMENTS
					puts("% invalid ESC p command");
					#endif
					fprintf(stderr,"Invalid ESC p command\n");
					break;
				}
			select_font();
			#endif
			break;
		case 'r':				/* select print colour */
			NEWC;
			#ifdef PASS1
			if(c != COLOUR_BLACK)
				uses_colour=TRUE;
			#else
			#ifdef DEBUG_COMMENTS
			printf("%% select colour %d\n",c);
			#endif
			if(c > 7)
				fprintf(stderr,"Invalid ESC r command\n");
			else
				print_colour=c;
			#endif
			break;
		case 's':				/* print speed */
			NEWC;				/* ignore this command */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% print speed: %d\n",c);
			#endif
			#endif
			break;				/* (Epson LX-80, FX-850 manuals) */
		case 't':				/* Select character set */
			NEWC;				/* "0" and "1" cannot be used */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% select character set: %d\n",c);
			#endif
			switch(c)			/* (Epson FX-850 manual) */
				{
				case 0:			/* Italic */
					charset=CHARSET_ITALIC;
					break;
				case 1:			/* Extended */
					charset=CHARSET_EXTENDED;
					break;
				default:
					fprintf(stderr,"Invalid ESC s code\n");
					break;
				}
			#endif
			break;
		case 'w':				/* turn double high mode on/off */
			NEWC;
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% turn double high mode on/off: %d\n",c);
			#endif
			switch(c)
				{
				case 0:
				case '0':
					current_charmode &= ~MODE_2X_VERTICAL_BASELINE;
					/* line_spacing_multiplier=1; */
					break;
				case 1:
				case '1':
					current_charmode |= MODE_2X_VERTICAL_BASELINE;
					/* line_spacing_multiplier=2; */
					break;
				default:
					fprintf(stderr,"Invalid ESC w code\n");
					break;
				}
			select_font();
			#endif
			break;
		case 'x':				/* NLQ mode on/off */
			NEWC;
			#ifndef PASS1
			switch(c)
				{
				case 0:			/* draft mode */
				case '0':
					#ifdef DEBUG_COMMENTS
					puts("% normal print quality on");
					#endif
					nlq_mode=FALSE;
					break;
				case 1:
				case '1':		/* NLQ mode */
					#ifdef DEBUG_COMMENTS
					puts("% NLQ mode on");
					#endif
					nlq_mode=TRUE;
					break;
				case 3:			/* Genicom high speed mode */
				case '3':
					#ifdef DEBUG_COMMENTS
					puts("% HS mode on");
					#endif
					nlq_mode=FALSE;
					break;
				default:
					#ifdef DEBUG_COMMENTS
					puts("% Invalid ESC x command");
					#endif
					fprintf(stderr,"Invalid ESC x command\n");
					break;
				}
			#endif
			break;
		case 0x5C:				/* backslash, set relative position (Epson) */
			if(emulation & EMULATION_CONFLICTING_IBM)
				{				/* print character from symbol set */

				}
			else
				{
				NEWC;							/* build number from */
				#ifndef PASS1
				length=c;						/* next two bytes */
				#endif
				NEWC;
				#ifndef PASS1
				length += 256 * c;

				if(length > 32768)				/* undo two's complement */
					length=length-65536;

				if( (emulation & EMULATION_P6_INTERPRETATION) && nlq_mode )
				/* if(nlq_mode) */
					{							/* for P6 in NLQ mode, 180ths of an inch */
					length *= (HORIZONTAL_UNITS/180);
					}
				else							/* other cases, 120ths of an inch */
					{
					length *= (HORIZONTAL_UNITS/120);
					}

				#ifdef DEBUG_COMMENTS
				printf("%% horizontal move: %d/%dths inch\n",length,HORIZONTAL_UNITS);
				#endif

				/* If in allowable range, use it. */
				if( (xpos + length) >= left_margin && (xpos + length) <= right_margin )
					xpos += length;
				#endif
				}
			break;
		default:
			#ifndef PASS1
			if(isprint(c))
				{
				#ifdef DEBUG_COMMENTS
				printf("%% Unrecognized escape code: ESC %c\n",c);
				#endif
				fprintf(stderr,"Unrecognized escape code: ESC %c\n",c);
				}
			else
				{
				#ifdef DEBUG_COMMENTS
				printf("%% Unrecognized escape code: ESC %d\n",c);
				#endif
				fprintf(stderr,"Unrecognized escape code: ESC %d\n",c);
				}
			#endif
			break;
		}

	} /* end of escape() */

/*
** NEC FS codes
*/
#ifdef PASS1
void fs_pass1(void)
#else
void fs(void)
#endif
	{
	int c;
	int length;

	#ifdef PASS1		/* P6 commands indicated 24 pin printer */
	uses_24pin_commands=TRUE;
	#endif

	NEWC;				/* get the character after the FS */

	switch(c)			/* act on it */
		{
		case '3':				/* set line spacing to n/360ths of an inch */
			NEWC;				/* (NEC P6 and Panasonic KX-P1124) */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% line spacing: %d/360ths inch\n",c);
			#endif
			current_line_spacing=c;
			#endif
			break;
		case '@':				/* reset printer (total) */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("%% NEC full reset");
			#endif
			#endif
			reset(TRUE);		/* both passes */
			break;
		case 'C':				/* sets optional font ROM */
			NEWC;				/* (ignored) */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% set optional font ROM: %d (ignored)\n",c);
			#endif
			#endif
			break;
		case 'E':				/* sets double or triple width */
			NEWC;
			#ifndef PASS1
			switch(c)
				{
				case 0:
				case '0':		/* cancels enlargment */
					#ifdef DEBUG_COMMENTS
					puts("% cancel double or triple width");
					#endif
					current_charmode &= ~(MODE_EXPANDED
												| MODE_3X_HORIZONTAL
												| MODE_4X_HORIZONTAL);
					break;
				case 1:
				case '1':		/* double horizontal enlargement */
					#ifdef DEBUG_COMMENTS
					puts("% FS E double width");
					#endif
					current_charmode &= ~(MODE_3X_HORIZONTAL
												| MODE_4X_HORIZONTAL);
					current_charmode |= MODE_EXPANDED;
					break;
				case 2:
				case '2':		/* triple horizontal enlargment */
					#ifdef DEBUG_COMMENTS
					puts("% triple width");
					#endif
					current_charmode &= ~(MODE_EXPANDED
												|MODE_4X_HORIZONTAL);
					current_charmode |= MODE_3X_HORIZONTAL;
					break;
				default:
					#ifdef DEBUG_COMMENTS
					puts("% Invalid FS E command");
					#endif
					fprintf(stderr,"Invalid FS E command\n");
					break;
				}

			select_font();
			#endif
			break;
		case 'F':				/* selects forward line feed */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% select forward line feed");
			#endif
			line_feed_direction=1;
			#endif
			break;
		case 'I':				/* Selects IBM or italic character set */
			NEWC;

			switch(c)
				{
				case 0:
				case '0':
					#ifndef PASS1
					#ifdef DEBUG_COMMENTS
					puts("% select italic character set");
					#endif
					#endif
					charset=CHARSET_ITALIC;
					break;
				case 1:
				case '1':
					#ifndef PASS1
					#ifdef DEBUG_COMMENTS
					puts("% select IBM character set");
					#endif
					#endif
					charset=CHARSET_EXTENDED;
					break;
				default:
					#ifndef PASS1
					#ifdef DEBUG_COMMENTS
					puts("% invalid FS I command");
					#endif
					fprintf(stderr,"Invalid FS I command\n");
					#endif
					break;
				}

			break;
		case 'R':				/* selects reverse line feed */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			puts("% select reverse line feed");
			#endif
			line_feed_direction=-1;
			#endif
			break;
		case 'S':				/* selects 12 cpi high-speed mode */
			NEWC;				/* (ignored) */
			#ifndef PASS1
			#ifdef DEBUG_COMMENTS
			printf("%% 12cpi high-speed mode: %d (ignored)\n",c);
			#endif
			#endif
			break;
		case 'V':				/* selects double height printing */
			NEWC;
			#ifndef PASS1
			switch(c)
				{
				case 0:			/* cancel vertical enlargement */
				case '0':
					#ifdef DEBUG_COMMENTS
					puts("% cancel double height printing");
					#endif
					current_charmode &= ~(MODE_2X_VERTICAL | MODE_4X_VERTICAL);
					line_spacing_multiplier=1;
					break;
				case 1:			/* enables vertical enlargement */
				case '1':
					#ifdef DEBUG_COMMENTS
					puts("% double height printing");
					#endif
					current_charmode &= ~MODE_4X_VERTICAL;
					current_charmode |= MODE_2X_VERTICAL;
					line_spacing_multiplier=2;
					break;
				default:
					#ifdef DEBUG_COMMENTS
					puts("% Invalid FS V command");
					#endif
					fprintf(stderr,"Invalid FS V command\n");
					break;
				}
			select_font();
			#endif
			break;
		case 'Z':				/* selects high density dot graphics */
			NEWC;				/* get the number of columns */
			length=c;			/* to expect as a 16 bit number */
			NEWC;
			length+=c*256;

			#ifndef PASS1
			eat_graphic(GRAPHICS_24_360,PINS_8or24,length);
			#else
			#ifdef DEBUG_COMMENTS
			printf("%% graphics mode %d, %d columns\n",GRAPHICS_24_360,length);
			#endif
			graphic(GRAPHICS_24_360,PINS_8or24,length);
			#endif
			break;
		default:
			#ifndef PASS1
			if(isprint(c))
				{
				#ifdef DEBUG_COMMENTS
				printf("%% Unrecognized escape code: FS %c\n",c);
				#endif
				fprintf(stderr,"Unrecognized escape code: FS %c\n",c);
				}
			else
				{
				#ifdef DEBUG_COMMENTS
				printf("%% Unrecognized escape code: FS %d\n",c);
				#endif
				fprintf(stderr,"Unrecognized escape code: FS %d\n",c);
				}
			#endif
			break;
		}

	} /* end of fs() */

/* end of file */
