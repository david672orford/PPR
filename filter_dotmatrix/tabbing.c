/*
** mouse:~ppr/src/filter_dotmatrix/tabbing.c
** Copyright 1995--2003, Trinity College Computing Center.
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
** Last modified 12 September 2003.
*/

#include "filter_dotmatrix.h"

/* Clear all tabs. */
void reset_tabs(void)
	{
	int x,y;

	/* Clear vertical tabs */
	for(y=0; y < 8; y++)
		for(x=0; x < 16; x++)
			tabs_vertical[y][x]=0;

	/* Reset horizontal tabs to 8 character intervals */
	/* (Is this right?) */
	for(x=0; x < 32; x++)
		tabs_horizontal[x]=x * 8 * current_char_spacing;

	} /* end of reset_tabs() */

/* Horizontal Tab */
void horizontal_tab(void)
	{
	int x;

	for(x=0; x < 32; x++)
		{
		if( tabs_horizontal[x] > xpos )
			{
			xpos = tabs_horizontal[x];
			break;
			}
		}

	} /* end of horizontal_tab() */

/* Vertical Tab */
void vertical_tab(void)
	{
	int x;

	for(x=0; x < 16; x++)
		{
		if(tabs_vertical[vertical_tab_channel][x] > ypos )
			{
			ypos = tabs_vertical[vertical_tab_channel][x];
			break;
			}
		}
	} /* end of vertical_tab() */

/*
** Set up to 32 horizontal tabs.
*/
void horizontal_tabs_set(void)
	{
	int x;
	int c,lastc;

	lastc=0;
	for(x=0; x < 32; x++)
		{
		if( (c=input()) == EOF )
			{
			fprintf(stderr,"Unexpected end of file in ESC D command\n");
			return;
			}

		if(c < lastc || c==0)
			break;

		tabs_horizontal[x]=c * current_char_spacing;

		lastc=c;
		}

	for( ; x < 32; x++)					/* clear remaining tab stops */
		tabs_horizontal[x]=0;

	} /* end of horizontal_tabs_set() */

void vertical_tabs_set(int channel)
	{
	int x;
	int c,lastc;

	lastc=0;
	for(x=0; x < 16; x++)
		{
		if( (c=input()) == EOF )
			{
			fprintf(stderr,"Unexpected end of file in ESC B or ESC b command\n");
			return;
			}

		if(c < lastc || c==0)
			break;

		tabs_vertical[channel][x]=c * current_line_spacing;

		lastc=c;
		}

	for( ; x < 16; x++)					/* clear remaining tab stops */
		tabs_vertical[channel][x]=0;

	} /* end of vertical_tabs_set() */

void horizontal_tab_increment(int inc)
	{
	int x;

	for(x=0; x < 32; x++)
		tabs_horizontal[x] = x * inc * current_char_spacing;
	}

void vertical_tab_increment(int inc)
	{
	/* I don't understand this command */
	}

/* end of file */
