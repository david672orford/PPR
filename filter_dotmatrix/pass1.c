/*
** mouse:~ppr/src/filter_dotmatrix/pass1.c
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
** Last modified 10 July 1999.
*/

/*
** This module makes the first pass over the Epson code in order to
** determine what fonts will be required.
*/

#include <stdio.h>
#include "filter_dotmatrix.h"

/* Does this file use any colours besides black? */
int uses_colour=FALSE;

/* Does this file use any commands typical of 24 pin printers? */
int uses_24pin_commands=FALSE;

/* Are any graphics emmited? */
int uses_graphics=FALSE;

/* Which fonts are used? */
int uses_normal=FALSE;		/* pretty safe assumption */
int uses_bold=FALSE;
int uses_oblique=FALSE;
int uses_boldoblique=FALSE;

int uses_nonascii_normal=FALSE;
int uses_nonascii_bold=FALSE;
int uses_nonascii_oblique=FALSE;
int uses_nonascii_boldoblique=FALSE;

int uses_proportional1=FALSE;
int uses_proportional2=FALSE;
int uses_proportional3=FALSE;
int uses_proportional4=FALSE;

/*
** The pass 1 loop
*/
void pass1(void)
    {
    int c;

    while((c = input()) != EOF)
    	{
    	switch(c)
    	    {
	    case 7:			/* beap */
	        break;
	    case 8:			/* backspace */
	    	break;
	    case 9:			/* horizontal tab */
	    	break;
	    case 10:			/* line feed */
	    	break;
	    case 11:			/* vertical tab */
	    	break;
	    case 12:			/* form feed */
	    	break;
	    case 13:			/* carriage return */
		break;
	    case 14:			/* one line expanded mode */
	    	break;
	    case 15:			/* condensed mode on */
	    	break;
	    case 17:			/* printer active */
	    	break;
	    case 18:			/* condensed mode off */
		break;
	    case 19:			/* printer inactive */
	    	break;
	    case 20:			/* expanded mode off */
	    	break;
	    case 24:			/* cancel buffer */
	    	break;
	    case 27:			/* start of ESC code */
	    	escape_pass1();
	    	break;
	    case 28:			/* start of NEC FS code */
	    	fs_pass1();
	    	break;
	    case 127:			/* delete last text character in buffer */
	    	break;
	    default:			/* printable character */
		if(current_charmode & (MODE_DOUBLE_STRIKE | MODE_EMPHASIZED) )
		    {
		    if(current_charmode & MODE_ITALIC)
		    	{
		    	uses_boldoblique=TRUE;
		    	if(c < ' ' || c > '~')
		    	    uses_nonascii_boldoblique=TRUE;
			if(current_charmode & MODE_PROPORTIONAL)
		    	    uses_proportional4=TRUE;
		    	}
		    else
		    	{
		    	uses_bold=TRUE;
		    	if(c < ' ' || c > '~')
		    	    uses_nonascii_bold=TRUE;
			if(current_charmode & MODE_PROPORTIONAL)
		    	    uses_proportional2=TRUE;
		    	}
		    }
		else
		    {
		    if(current_charmode & MODE_ITALIC)
		    	{
		    	uses_oblique=TRUE;
		    	if(c < ' ' || c > '~')
		    	    uses_nonascii_oblique=TRUE;
			if(current_charmode & MODE_PROPORTIONAL)
		    	    uses_proportional3=TRUE;
		    	}
		    else
		    	{
		    	uses_normal=TRUE;
		    	if(c < ' ' || c > '~')
		    	    uses_nonascii_normal=TRUE;
			if(current_charmode & MODE_PROPORTIONAL)
		    	    uses_proportional1=TRUE;
		    	}
		    }


		break;
    	    }
    	} /* end of while */
    } /* end of pass1() */

/* end of file */

