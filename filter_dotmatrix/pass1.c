/*
** mouse:~ppr/src/filter_dotmatrix/pass1.c
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

/*
** This module makes the first pass over the Epson code in order to
** determine what fonts will be required.
*/

#include "filter_dotmatrix.h"

/* Does this file use any colours besides black? */
int uses_colour=FALSE;

/* Does this file use any commands typical of 24 pin printers? */
int uses_24pin_commands=FALSE;

/* Are any graphics emmited? */
int uses_graphics=FALSE;

/* Which fonts are used? */
int uses_normal=FALSE;			/* pretty safe assumption */
int uses_bold=FALSE;
int uses_oblique=FALSE;
int uses_boldoblique=FALSE;

gu_boolean uses_nonascii_normal = FALSE;
gu_boolean uses_nonascii_bold = FALSE;
gu_boolean uses_nonascii_oblique = FALSE;
gu_boolean uses_nonascii_boldoblique = FALSE;

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
			case 7:						/* beap */
				break;
			case 8:						/* backspace */
				break;
			case 9:						/* horizontal tab */
				break;
			case 10:					/* line feed */
				break;
			case 11:					/* vertical tab */
				break;
			case 12:					/* form feed */
				break;
			case 13:					/* carriage return */
				break;
			case 14:					/* one line expanded mode */
				break;
			case 15:					/* condensed mode on */
				break;
			case 17:					/* printer active */
				break;
			case 18:					/* condensed mode off */
				break;
			case 19:					/* printer inactive */
				break;
			case 20:					/* expanded mode off */
				break;
			case 24:					/* cancel buffer */
				break;
			case 27:					/* start of ESC code */
				escape_pass1();
				break;
			case 28:					/* start of NEC FS code */
				fs_pass1();
				break;
			case 127:					/* delete last text character in buffer */
				break;
			default:					/* printable character */
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

