/*
** mouse.trincoll.edu:~ppr/src/dotmatrix/graphics.c
** Copyright 1995, 1996, 1997, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 9 July 1996.
*/

/*
** This file is part of PPR's dot matrix printer emulator.
** This module is responsible for printing dotmatrix graphics.
*/

#include <stdio.h>
#include <stdlib.h>
#include "filter_dotmatrix.h"

/* Maximum bytes of graphics data. */
#define MAX_GBYTES 8640

/* Bytes of hexadecimal data per line. */
#define HEX_BPL 39

/* Bytes of ASCII85 data per line, must be multiple of 4. */
#define A85_BPL 60

/* Graphics buffer. */
unsigned char gbuffer[MAX_GBYTES];

/* Buffer for the compressed graphic.  It is bigger because it is
** theoretically possible to contrive data which will be bigger
** when run length compressed. */
/* unsigned char gbuffer2[MAX_GBYTES + ((MAX_GBYTES+127)/128)]; */
unsigned char gbuffer2[20000];

void graphic(int mode, int pins, int length)
	{
	int linelen;
	int c;
	unsigned int x;
	int eof_noted=FALSE;
	int density;
	unsigned int countdown;
	int true_pins;				/* 8, 9, or 24 */
	int someblack=FALSE;

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

	/*
	** We might not even be in a page yet, especially if we.
	** are printing a screen dump.
	*/
	if(!in_page)
		{
		top_of_page();
		in_page=TRUE;
		}

	/* Write any characters which are lingering in the line output buffer. */
	empty_buffer();

	/* Move to proper position for graphic. */
	achieve_position();

	/* Make sure we have the correct colour. */
	if(print_colour != postscript_print_colour)
		{
		printf(" %d colour\n",print_colour);
		postscript_print_colour=print_colour;
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
		fprintf(stderr,"filter_dotmatrix: graphic(): unknown graphic mode\n");
		exit(10);
		}
		
	/* See if the data is too big for our buffer. */
	if(countdown > sizeof(gbuffer))
		{
		fprintf(stderr,"filter_dotmatrix: graphic too big\n");
		exit(1);
		}

	/* Read the data into the buffer. */
	for(x=0; x < countdown; x++)
		{
		if( (c=input()) == EOF)
			{
			if(!eof_noted)
				{
				fprintf(stderr,"End of file in graphic\n");
				eof_noted=TRUE;
				}
			c=0;
			}

		gbuffer[x]=c;
		someblack=TRUE;
		}

	/* Only emmit the line if it makes marks. */
	if(someblack)
		{
		/* If level 2, compress the data. */
		if(level2)
			{
			int rlength;
			int litlength;
			int lastc,lastlastc;
			unsigned int di;
			int unclaimed;
			unsigned long int tuple;

			/* 
			** This loop runs once for each character and
			** then once more.
			*/
			unclaimed=litlength=rlength=di=0; lastlastc=lastc=-1;
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
					unclaimed++;				/* don't let anyone claim this on yet, */
					continue;					/* go back for another. */
					}

				if( rlength						/* If end of run or run full, */
						 && (c!=lastc || rlength==128) )
					{							/* empty it. */
					#ifdef DEBUG_COMPRESSION
					printf("%% dumping run, %d %2.2X\n",rlength,lastc);
					#endif
					gbuffer2[di++]=257-rlength;
					gbuffer2[di++]=lastc;
					rlength=0;
					}

				/* If literal in progress, but */
				/* we see a run starting, */
				if(litlength>=2 && c==lastc && c==lastlastc)
					{
					litlength-=2;				/* lastc should have been part of the run. */
					unclaimed+=2;				/* Leave for rlength to claim. */
					}  

				if( litlength					/* If run starting or literal full, */
						&& ((c==lastc && c==lastlastc) || litlength==128) )
					{							/* empty literal. */
					#ifdef DEBUG_COMPRESSION
					printf("%% dumping literals\n");
					#endif

					gbuffer2[di++]=litlength-1;
					while(litlength--)
						{
						#ifdef DEBUG_COMPRESSION
						printf("%% %2.2X\n",gbuffer[x-litlength-unclaimed-1]);
						#endif
						gbuffer2[di++]=gbuffer[x-litlength-unclaimed-1];
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
					rlength+=unclaimed;
					unclaimed=0;
					}

				/* If not, it must be part of a literal list. */
				else
					{
					#ifdef DEBUG_COMPRESSION
					printf("%% literal list member\n");
					#endif
					litlength++;
					litlength+=unclaimed;
					unclaimed=0;
					}
				}

			/* Dump remaining runs and literals. */
			if(rlength)
				{
				#ifdef DEBUG_COMPRESSION
				printf("%% dumping final run, %d %2.2X\n",rlength,lastc);
				#endif
				gbuffer2[di++]=257-rlength;
				gbuffer2[di++]=lastc;
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
					printf("%% %2.2X\n",gbuffer[x-litlength-unclaimed-1]);
					#endif
					gbuffer2[di++]=gbuffer[x-litlength-unclaimed-1];
					}
				}

			/* Compression done. */
			countdown = di;						/* new byte count is number stored */
			while(di%4)							/* Complete last */
				gbuffer2[di++]=0;				/* 4-tuple. */

			/* start the graphic */
			printf("%%%%BeginData: %d ASCII Lines\n",((countdown+A85_BPL-1)/A85_BPL)+1);
			printf("%d %d cgraphic%d\n", density, length, true_pins);

			/* Emmit in ASCII85 */
			linelen=0;
			for(x=0; countdown; )
				{
				linelen+=4;
				if(linelen > A85_BPL)			/* If current line is full, */
					{
					fputc('\n',stdout);			/* start a new one. */
					linelen=4;
					}

				tuple= gbuffer2[x++] * (256*256*256);
				tuple+=gbuffer2[x++] * (256*256);
				tuple+=gbuffer2[x++] * 256;
				tuple+=gbuffer2[x++];

				fputc( (tuple/(85*85*85*85)) + 33, stdout);
				tuple %= (85*85*85*85); /* keep remainder */

				fputc( (char)(tuple/(85*85*85)) + 33, stdout);
				tuple %= (85*85*85);	/* always at least two characters */
				countdown--;
					
				if(countdown)
					{
					fputc( (char)(tuple/(85*85)) + 33, stdout);
					tuple %= (85*85);
					countdown--;
						
					if(countdown)
						{
						fputc( (char)(tuple/85) + 33, stdout);
						tuple %= 85;
						countdown--;
							
						if(countdown)
							{		
							fputc( (char)tuple + 33, stdout);
							countdown--;
							}
						}
					}
				}								/* end of data loop */

			fputs("~>\n",stdout);				/* Terminate output */
			}									/* end of if(level2) */

		else									/* level one graphic */
			{									/* emmit uncompressed hexadecimal */
			/* start the graphic */
			printf("%%%%BeginData: %d Hex Lines\n",((countdown+HEX_BPL-1)/HEX_BPL)+1);
			printf("%d %d graphic%d\n", density, length, true_pins);

			/* Emmit the data in hexadecimal. */
			linelen=0;
			for(x=0; countdown--; x++)
				{
				if(++linelen > HEX_BPL)			/* If current line is full, */
					{
					fputc('\n',stdout);			/* start a new one. */
					linelen=1;
					}

				printf("%2.2X",gbuffer[x]);
				}

			fputc('\n',stdout);			/* terminate final line */
			}

		/* End of graphics block */
		fputs("%%EndData\n",stdout);
		}								/* end of if black marks exist */
	
	/* Adjust xpos.	 (On a real dot matrix printer, a graphics */
	/* command would move the cursor. */
	xpos += (int)(((double)HORIZONTAL_UNITS/(double)density)*(double)length);
	} /* end of graphic() */
	
/*
** Eat up a graphic.
*/
void eat_graphic(int mode, int pins, int length)
	{
	int countdown=0;

	uses_graphics=TRUE;							/* set flag to include graphics routines */

	if(mode > 7)								/* advanced graphics modes */
		uses_24pin_commands=TRUE;				/* suggest 24 pin printer */

	if(pins == PINS_8or24 && mode < 32)			/* 8 pin */
		countdown=length;
	else if(pins == PINS_8or24)					/* 24 pin */
		countdown=length*3;		
	else if(pins == PINS_9)						/* 9 pin */
		countdown=length*2;
	
	while(countdown-- && input()!=EOF)
		{ /* no code */ }
	} /* end of eat_graphic() */

/* end of file */
