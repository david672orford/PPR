/*
** mouse:~ppr/src/filter_dotmatrix/graphics.c
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
** Last modified 24 August 2005.
*/

/*
** This file is part of PPR's dot matrix printer emulator. This module is
** responsible for printing dotmatrix graphics.
*/

#include <limits.h>
#include "filter_dotmatrix.h"

/* #define DEBUG_COMPRESSION 1 */

/* Maximum bytes of graphics data. */
#define MAX_GBYTES 8640

/* Bytes of hexadecimal data per line. */
#define HEX_BPL 39

/* Bytes of ASCII85 data per line, must be multiple of 4. */
#define A85_BPL 60

/*
** Eat up a graphic.  This is used on the first pass.
*/
void eat_graphic(int mode, int pins, int length)
	{
	int countdown = 0;

	/* Set flag to include graphics routines in PostScript prolog. */
	uses_graphics = TRUE;						

	if(mode > 7)								/* advanced graphics modes */
		uses_24pin_commands = TRUE;				/* suggests target was a 24 pin printer */

	if(pins == PINS_8or24 && mode < 32)			/* 8 pin */
		countdown=length;
	else if(pins == PINS_8or24)					/* 24 pin */
		countdown=length*3;		
	else if(pins == PINS_9)						/* 9 pin */
		countdown=length*2;
	
	while(countdown-- && input()!=EOF)
		{ /* no code */ }
	} /* end of eat_graphic() */

/*
** Read the graphics bytes and emmit PostScript.  This is called
** during the second pass.
*/
void graphic(int mode, int pins, int length)
	{
	unsigned char gbuffer[MAX_GBYTES];	/* Graphics buffer. */
	int linelen;
	int c;
	unsigned int x;
	int eof_noted=FALSE;
	int density;
	unsigned int countdown;
	int true_pins;				/* 8, 9, or 24 */
	int someblack = FALSE;

	/*
	** Convert the graphics mode code into lines-per-inch.
	*/
	switch(mode)
		{
		case GRAPHICS_60:
		case GRAPHICS_24_60:
			density=60;
			break;
		case GRAPHICS_120a:
		case GRAPHICS_120b:
		case GRAPHICS_24_120:
			density=120;
			break;
		case GRAPHICS_240:
			density=240;
			break;
		case GRAPHICS_80:
			density=80;
			break;
		case GRAPHICS_72:
			density=72;
			break;
		case GRAPHICS_90:
		case GRAPHICS_24_90:
			density=90;
			break;	  
		case GRAPHICS_24_180:
			density=180;
			break;
		case GRAPHICS_24_360:
			density=360;
			break;
		default:
			fprintf(stderr,"invalid graphics mode in graphic()\n");
			exit(1);	
		}

	/* Issue the proper graphics command for the mode. */
	if(pins == PINS_8or24 && mode < 32)			/* 8 pin */
		{
		countdown=length;
		true_pins=8;
		}
	else if(pins == PINS_8or24)					/* 24 pin */
		{
		countdown=length*3;		
		true_pins=24;
		}
	else if(pins == PINS_9)						/* 9 pin */
		{
		countdown=length*2;
		true_pins=9;
		}
	else
		{
		fprintf(stderr, _("%s: graphic(): unknown graphic mode\n"), myname);
		exit(10);
		}
		
	/* See if the data is too big for our buffer. */
	if(countdown > sizeof(gbuffer))
		{
		fprintf(stderr, _("%s: graphic too big\n"), myname);
		exit(1);
		}

	/* Read the data into the buffer. */
	for(x=0; x < countdown; x++)
		{
		if((c=input()) == EOF)
			{
			if(!eof_noted)
				{
				fprintf(stderr, _("%s: end of file in graphic\n"), myname);
				eof_noted = TRUE;
				}
			c = 0;
			}

		gbuffer[x] = c;

		if(c != 0)
			someblack = TRUE;
		}

	/* Only emmit the line if it makes marks. */
	if(someblack)
		{
		/*
		** We might not even have started the PostScrpt page yet, especially if
		** we are printing a screen dump.
		*/
		if(!in_page)
			{
			top_of_page();
			in_page = TRUE;
			}

		/* Write any characters which are lingering in the line output buffer. */
		empty_buffer();

		/* Move to proper position for graphic. */
		achieve_position();

		/* Make sure we have the correct colour set. */
		if(print_colour != postscript_print_colour)
			{
			gu_psprintf(" %d colour\n", print_colour);
			postscript_print_colour = print_colour;
			}

		/* If level 2, compress the data. */
		if(level2)
			{
			/* unsigned char gbuffer2[MAX_GBYTES + ((MAX_GBYTES+127)/128)]; */
			unsigned char gbuffer2[20000];

			int rlength;
			int litlength;
			int lastc,lastlastc;
			unsigned int di;
			int unclaimed;
			#if UINT_MAX < 0xFFFF
			#error code assumes that unsigned int is at least 32 bits
			#endif
			unsigned int tuple;		/* at least 32 bits required */

			/* 
			** This loop runs once for each character of graphics data and
			** then once more.
			*/
			unclaimed = litlength = rlength = di = 0;
			lastlastc = lastc = -1;
			countdown++;
			for(x=0; countdown--; x++,lastlastc=lastc,lastc=c)
				{
				c=gbuffer[x];
				
				#ifdef DEBUG_COMPRESSION
				printf("%% x=%d, c=%2.2X, lastc=%2.2X, rlength=%d, litlength=%d\n",
						x,c,lastc,rlength,litlength);
				#endif
				
				if(countdown && lastc==-1)		/* If no lastc exists, */
					{
					unclaimed++;				/* don't let anyone claim this one yet, */
					continue;					/* go back for another. */
					}

				if( rlength						/* If end of run or run full, */
						 && (c!=lastc || rlength==128) )
					{							/* empty it. */
					#ifdef DEBUG_COMPRESSION
					printf("%% dumping run, %d 0x%2.2X\n", rlength, lastc);
					#endif
					gbuffer2[di++] = (257 - rlength);
					gbuffer2[di++] = lastc;
					rlength=0;
					}

				/* If literal in progress, but we see a run starting, */
				if(litlength >= 2 && c == lastc && c == lastlastc)
					{
					litlength -= 2;				/* lastc should have been part of the run. */
					unclaimed += 2;				/* Leave for rlength to claim. */
					}  

				if(litlength					/* If run starting or literal full, */
						&& ((c==lastc && c==lastlastc) || litlength==128) )
					{							/* empty literal. */
					#ifdef DEBUG_COMPRESSION
					printf("%% dumping literals (%d bytes)\n", litlength);
					#endif

					gbuffer2[di++] = litlength-1;
					while(litlength--)
						{
						#ifdef DEBUG_COMPRESSION
						printf("%% 0x%02X\n", gbuffer[x-litlength-unclaimed-1]);
						#endif
						gbuffer2[di++] = gbuffer[x-litlength-unclaimed-1];
						}
					litlength=0;
					}

				/* Check if this character is part of a run. */
				if(c==lastc && c==lastlastc)
					{
					#ifdef DEBUG_COMPRESSION
					printf("%% run member\n");
					#endif
					rlength++;
					rlength += unclaimed;
					unclaimed = 0;
					}

				/* If not, it must be part of a literal list. */
				else
					{
					#ifdef DEBUG_COMPRESSION
					printf("%% literal list member\n");
					#endif
					litlength++;
					litlength += unclaimed;
					unclaimed = 0;
					}
				}

			/* Dump remaining runs and literals. */
			if(rlength)
				{
				#ifdef DEBUG_COMPRESSION
				printf("%% dumping final run, %d %2.2X\n",rlength,lastc);
				#endif
				gbuffer2[di++] = (257 - rlength);
				gbuffer2[di++] = lastc;
				}
			else if(litlength)
				{
				#ifdef DEBUG_COMPRESSION
				printf("%% dumping final literals\n");
				#endif

				gbuffer2[di++]=litlength-1;
				while(litlength--)
					{
					#ifdef DEBUG_COMPRESSION
					printf("%% %2.2X\n", gbuffer[x-litlength-unclaimed-1]);
					#endif
					gbuffer2[di++] = gbuffer[x-litlength-unclaimed-1];
					}
				}

			/* Compression done. */
			countdown = di;						/* new byte count is number stored */
			while(di%4)							/* Complete last */
				gbuffer2[di++]=0;				/* 4-tuple. */

			/* start the graphic */
			gu_psprintf("%%%%BeginData: %d ASCII Lines\n",((countdown+A85_BPL-1)/A85_BPL)+1);
			gu_psprintf("%d %d cgraphic%d\n", density, length, true_pins);

			/* Emmit in ASCII85 */
			linelen=0;
			for(x=0; countdown; )
				{
				linelen+=4;
				if(linelen > A85_BPL)			/* If current line is full, */
					{
					fputc('\n', stdout);		/* start a new one. */
					linelen=4;
					}

				/* Take four bytes to bet encoded as 5 symbols */
				tuple =  (unsigned)gbuffer2[x++] << 24;
				tuple |= (unsigned)gbuffer2[x++] << 16;
				tuple |= (unsigned)gbuffer2[x++] << 8;
				tuple |= (unsigned)gbuffer2[x++];
				#ifdef DEBUG_COMPRESSION
				printf("%% tuple=0x%08X\n", tuple);
				#endif


				fputc((tuple/(85*85*85*85)) + 33, stdout);
				tuple %= (85*85*85*85); /* keep remainder */

				fputc((tuple/(85*85*85)) + 33, stdout);
				tuple %= (85*85*85);	/* always at least two characters */
				countdown--;
					
				if(countdown)			/* if more than one input byte */
					{
					fputc((tuple/(85*85)) + 33, stdout);
					tuple %= (85*85);
					countdown--;
						
					if(countdown)
						{
						fputc((tuple/85) + 33, stdout);
						tuple %= 85;
						countdown--;
							
						if(countdown)
							{		
							fputc(tuple + 33, stdout);
							countdown--;
							}
						}
					}
				}								/* end of data loop */

			fputs("~>\n",stdout);				/* Terminate output */
			}									/* end of if(level2) */

		/*
		** PostScript level 1, emmit uncompressed hexadecimal.
		*/
		else									
			{									
			/* start the graphic */
			gu_psprintf("%%%%BeginData: %d Hex Lines\n",((countdown+HEX_BPL-1)/HEX_BPL)+1);
			gu_psprintf("%d %d graphic%d\n", density, length, true_pins);

			/* Emmit the data in hexadecimal. */
			linelen=0;
			for(x=0; countdown--; x++)
				{
				if(++linelen > HEX_BPL)			/* If current line is full, */
					{
					fputc('\n',stdout);			/* start a new one. */
					linelen=1;
					}

				printf("%2.2X", gbuffer[x]);
				}

			fputc('\n', stdout);				/* terminate final line */
			}

		/* End of graphics block */
		fputs("%%EndData\n",stdout);
		}								/* end of if black marks exist */
	
	/*
	** Adjust xpos.  (Because on a real dot matrix printer, a graphics
	** command would move the cursor.)
	*/
	xpos += (int)(((double)HORIZONTAL_UNITS/(double)density)*(double)length);
	} /* end of graphic() */
	
/* end of file */
