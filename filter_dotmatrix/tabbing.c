/*
** mouse:~ppr/src/filter_dotmatrix/tabbing.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 18 June 1999.
*/

#include <stdio.h>
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
    	    xpos=tabs_horizontal[x];
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
    	    ypos=tabs_vertical[vertical_tab_channel][x];
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

    for( ; x < 32; x++)			/* clear remaining tab stops */
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

    for( ; x < 16; x++)			/* clear remaining tab stops */
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
