/*
** mouse:~ppr/src/filter_dotmatrix/linebuf.c
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
** Last modified 15 April 2004.
*/

/*
** This code has nothing to do with reading lines from the input file. It
** implements an Epson line buffer which holds the current line of output
** until it is complete.  Characters which enter the line buffer may not
** actually get printed if the line buffer is canceled or if backspace is
** used to erase them.
*/

#include "filter_dotmatrix.h"

/* Tolerable horizontal spacing error. */
#define TOLERR 1.8

/* Get the absolute value. */
#define FABS(x) ((x) > 0 ? (x) : 0 - (x))

/* Truncate line longer than this. */
#define BUFFER_SIZE 300

/* The character description structure */
struct CHAR {
		int style;
		double hscale;
		double vscale;
		int movement;			/* difference between xpos and previous xpos */
		int xpos;				/* xpos of start in HORIZONTAL_UNITS */
		int fixed_spacing;
		int width;				/* current_char_spacing in HORIZONTAL_UNITS */
		int extra_space;		/* extra space after */
		int c;
		int colour;				/* desired colour */
		} ;
struct CHAR buffer[BUFFER_SIZE];
int buffer_count = 0;

/* What we have instructed the PostScript engine to do. */
static int postscript_out_style;
static double postscript_out_hscale;
static double postscript_out_vscale;
static int postscript_blbias;			/* Baseline bias */
int postscript_print_colour;			/* not static! */
static int postscript_extra_space;
static double postscript_space_width;

/* Previous xpos for getting space width data. */
int predicted_xpos;

/*
** Called at the start of each page to reset the output routines.
** The idea is to leave the output routines in a state which
** is consistent with the state of the program running on the
** PostScript interpreter which will have reset itself at the
** top of the page.
*/
void buffer_top_of_page_reset(void)
	{
	postscript_out_style = 0;			/* set to mode */
	postscript_out_hscale = 1.0;		/* defined in */
	postscript_out_vscale = 1.0;		/* document setup section */
	postscript_blbias = 0;
	postscript_print_colour = COLOUR_BLACK;
	postscript_extra_space = 0;
	postscript_space_width = HORIZONTAL_UNITS/10;
	} /* end of buffer_top_of_page_reset() */

/*
** Delete characters from the buffer, 0 means all, 1 means one.
*/
void buffer_delete(int howmuch)
	{
	if(howmuch==0)
		{
		buffer_count=0;
		}
	if(howmuch==1 && buffer_count)
		{
		buffer_count--;
		}
	} /* end of buffer_delete() */

/*
** Add a character to the line output buffer.
*/
void buffer_add(int c)
	{
	if(buffer_count==0)
		predicted_xpos=0;

	if(buffer_count < BUFFER_SIZE)
		{
		struct CHAR *p = &buffer[buffer_count++];

		p->style = out_style;
		p->hscale = out_hscale;
		p->vscale = out_vscale;
		p->movement = xpos-predicted_xpos;
		p->xpos = xpos;
		p->extra_space = (extra_dot_spacing * (HORIZONTAL_UNITS/120));
		p->c = c;
		p->colour = print_colour;
		p->fixed_spacing = current_char_spacing;

		/*
		** If proportional spacing is currently in effect,
		** the width comes from the proportional spacing table.
		*/
		if(out_style & OSTYLE_PROPORTIONAL)
			p->width = (int)(((double)width(c,out_style & OSTYLE_OBLIQUE)*out_hscale)+0.5);
		else
			p->width = current_char_spacing;

		#ifdef DEBUG_COMMENTS
		printf("%% buffer_add(): c='%c' (%d), out_style=%d, xpos=%d, ypos=%d, width=%d, extra_space=%d\n",
				c, c, out_style, xpos, ypos, p->width, p->extra_space);
		#endif

		/* Advance to next position. */
		xpos += p->width;
		xpos += p->extra_space;

		/* Save this xpos for next time. */
		predicted_xpos = xpos;
		}
	} /* end of buffer_add() */

/*
** Empty the print buffer.  By "empty" we mean to emmit PostScript
** code which represents the contents and then to clear it, not
** to simply throw the contents away.
**
** This is called at the end of the line and
** at certain other times.
*/
void empty_buffer(void)
	{
	int x;
	int c;						/* current character code */
	struct CHAR *p;				/* pointer to current character structure */
	int string_open=FALSE;
	int blbias;					/* baseline modification */
	int ul_start=-1;			/* Starting point of underline */
	int saved_xpos;				/* We will save xpos here while we backtrack */

	int total;
	int count;
	int movement;
	double average;

	double acumulated_error=0;	/* later, we keep track of how much this fails */

	if(buffer_count==0)			/* If nothing to do, */
		return;					/* don't issue any code. */

	/* Make a first pass, determining the average space width. */
	total=0;
	count=0;
	for(x=0; x < buffer_count; x++)
		{
		movement=buffer[x].movement;
		if(movement > 0 && movement <= ((double)buffer[x].fixed_spacing*1.5))
			{	/* ignore zero and negative and large */
			#ifdef DEBUG_COMMENTS
			printf("%% movement=%d\n",movement);
			#endif
			total+=movement;
			count++;
			}
		}

	/* compute the average. */
	if(count)			/* if something to average */
		{
		average = ( (double)total / (double) count);
		#ifdef DEBUG_COMMENTS
		printf("%% total=%d, count=%d, average=%f\n",total,count,average);
		#endif
		}
	else				/* select something arbitrary */
		{
		average=HORIZONTAL_UNITS/10;
		}

	/* Save xpos and set xpos to postscript position. */
	saved_xpos = xpos;
	xpos = (int)postscript_xpos;

	/*
	** Underline commands can't stretch across lines
	** so, turn underline off now.
	*/
	postscript_out_style &= ~OSTYLE_UNDERLINE;

	/* The loop which handles the characters. */
	for(x=0; x < buffer_count; x++)
		{
		p=&buffer[x];					/* p will point to the character structure */
		c=p->c;							/* c will hold the character code */

		/* See if underlining changes here. */
		if( (p->style & OSTYLE_UNDERLINE) != (postscript_out_style & OSTYLE_UNDERLINE) )
			{
			if(p->style & OSTYLE_UNDERLINE)		/* Underline on */
				{
				ul_start=p->xpos;				/* Mark the spot */
				postscript_out_style |= OSTYLE_UNDERLINE;
				}
			else								/* Underline off */
				{
				if(string_open)					/* If we are in a string, */
					{							/* close it. */
					fputs(")p\n",stdout);
					string_open=FALSE;
					}

				xpos=p->xpos;
				achieve_position();
				gu_psprintf("%d ul\n",ul_start);
				ul_start=-1;

				postscript_out_style &= ~OSTYLE_UNDERLINE;
				}
			}

		/* Don't actually print spaces. */
		if(c==' ')
			{
			xpos = p->xpos;				/* Actually keeping */
			xpos += p->width;			/* xpos up to date */
			xpos += p->extra_space;		/* is necessary for underlining. */
			continue;
			}

		/*
		** See if we must select a new font.  (But the
		** current font doesn't matter if character is
		** space, so in that case we won't worry about
		** it.)
		*/
		if( (c != ' ') && (p->style != postscript_out_style
				|| p->hscale != postscript_out_hscale
				|| p->vscale != postscript_out_vscale) )
			{
			if(string_open)				/* If we are in a string, */
				{						/* close it. */
				puts(")p");
				string_open=FALSE;
				}

			/* We must set blias before selecting font. */
			blbias=0;
			if(p->style & OSTYLE_HALFRAISE)
				blbias=4;
			if(p->style & OSTYLE_FULLDROP)
				blbias=-8;

			/* We we have changed baseline bias, say so now. */
			if(blbias != postscript_blbias)
				gu_psprintf("%d bb\n",blbias);

			/* Select proportional if appropriate. */
			if(p->style & OSTYLE_PROPORTIONAL)
				fputc('p', stdout);

			/* Select the correct style. */
			fputc('f', stdout);					/* font */
			if(p->style & OSTYLE_BOLD)			/* add bold? */
				fputc('b', stdout);
			if(p->style & OSTYLE_OBLIQUE)		/* add italic? */
				fputc('o', stdout);

			if(p->vscale == 1.0)				/* scale */
				gu_psprintf(" %f sf\n", p->hscale);
			else
				gu_psprintf(" %f %f sfh\n",p->hscale,p->vscale);

			postscript_out_style = p->style;
			postscript_out_hscale = p->hscale;
			postscript_out_vscale = p->vscale;
			postscript_blbias = blbias;

			postscript_space_width = (double)(HORIZONTAL_UNITS/10) * p->hscale;
			}

		/* See if we must select a new colour. */
		if(p->colour != postscript_print_colour)
			{
			if(string_open)				/* If we are in a string, */
				{						/* close it. */
				puts(")p");
				string_open=FALSE;
				}

			gu_psprintf("%d colour\n", p->colour);

			postscript_print_colour = p->colour;
			}

		/* See if we must select a new extra spacing. */
		if(p->extra_space != postscript_extra_space)
			{
			if(string_open)				/* If we are in a string, */
				{						/* close it. */
				puts(")s");
				string_open=FALSE;
				}

			gu_psprintf("%d e\n", p->extra_space);

			postscript_extra_space=p->extra_space;
			}

		/* Move current position to proper one for this character. */
		xpos=p->xpos;

		/* If we are in a string, are we at the right spot? */
		if(string_open && xpos != postscript_xpos)
			{
			double rel = xpos - postscript_xpos;

			if( (rel > 0) && (rel < HORIZONTAL_UNITS) )
				{						/* if moderate forward movement */
				rel+=acumulated_error;
				while(rel > (postscript_space_width-TOLERR))
					{
					fputc(' ',stdout);
					postscript_xpos += postscript_space_width;
					rel -= postscript_space_width;
					}
				acumulated_error=rel;

				if(FABS(acumulated_error) > TOLERR)
					{
					fputs(")p\n",stdout);
					#ifdef DEBUG_COMMENTS
					printf("%% acumulated error too high = %f\n",acumulated_error);
					#endif
					string_open=FALSE;
					acumulated_error=0;
					}
				}
			else								/* If too long or backwards, */
				{								/* break off the string, */
				fputs(")p\n",stdout);
				string_open=FALSE;
				}
			}

		/*
		** If we have not yet started a PostScript string,
		** set the PostScript position and start a string.
		*/
		if( ! string_open )
			{
			achieve_position();
			if(postscript_space_width != average)
				{
				gu_psprintf("%f s ",average);
				postscript_space_width=average;
				}
			fputc('(',stdout);
			string_open=TRUE;
			}

		/*
		** Send the string character in a manner which conforms to
		** Clean7Bit encoding.  This means that non-ASCII characters
		** and control codes must be encoding in octal.  Additionally,
		** some characters must be escaped because they have special
		** meaning in PostScript strings.
		*/
		if(c < ' ' || c > '~')
			{
			gu_psprintf("\\%o", c);		/* octal */
			}
		else
			{
			switch(c)
				{
				case '(':				/* Escape these */
				case ')':				/* characters which have */
				case 0x5C:				/* special significance within */
					gu_psprintf("\\%c", c);	/* PostScript strings. */
					break;
				default:
					fputc(c, stdout);
					break;
				}
			}

		/*
		** We have moved the cursor and the PostScript
		** cursor moved with it.  We must update the
		** variable postscript_xpos so as to avoid
		** confusing postscript.c:achieve_position().
		*/
		xpos += (p->width + p->extra_space);
		postscript_xpos = xpos;
		}

	if(string_open)						/* Close last PostScript string */
		puts(")p");

	if(ul_start != -1)					/* End underline */
		{
		achieve_position();
		gu_psprintf("%d ul\n", ul_start);
		}

	buffer_count=0;						/* buffer is empty now */

	xpos = saved_xpos;					/* restore xpos */
	} /* end of empty_buffer() */

/* end of file */
