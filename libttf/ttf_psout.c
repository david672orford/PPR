/*
** mouse:~ppr/src/libttf/ttf_psout.c
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
** Last modified 13 December 2004.
*/

#include "config.h"
#include "libttf_private.h"

/*
** This generates a PostScript font from a TrueType font
** file.
*/
int ttf_psout(void *p, void (*out_putc)(int c), void (*out_puts)(const char *string), void (*out_printf)(const char *format, ...), int target_type)
	{
	struct TTFONT *font = (struct TTFONT *)p;
	int ret;

	if(font->signiture != TTF_SIGNITURE)
		return -1;

	if((ret = setjmp(font->exception)) != 0)
		{
		font->errno = (TTF_RESULT)ret;
		return -1;
		}

	font->putc = out_putc;
	font->puts = out_puts;
	font->printf = out_printf;

	/* Load the "head" table and extract information from it. */
	ttf_Read_head(font);

	/* Load information from the "name" table. */
	ttf_Read_name(font);

	/* We need to have the PostScript table around. */
	if(!font->post_table) font->post_table = ttf_LoadTable(font, "post");
	font->numGlyphs = getUSHORT(font->post_table + 32);

	/* Write the header for the PostScript font. */
	ttf_PS_header(font, target_type);

	/* Define the encoding. */
	ttf_PS_encoding(font);

	/* Insert FontInfo dictionary. */
	ttf_PS_FontInfo(font);

	/* If we are generating a type 42 font,
	   emmit the sfnts array. */
	if(target_type == 42)
		ttf_PS_sfnts(font);

	/* If we are generating a Type 3 font, we will need to
	   have the 'loca' and 'glyf' tables around while
	   we are generating the CharStrings. */
	if(target_type == 3)
		{
		BYTE *ptr;						/* We need only one value */
		ptr = ttf_LoadTable(font, "hhea");
		font->numberOfHMetrics = getUSHORT(ptr + 34);
		ttf_free(font, ptr);

		if(!font->loca_table) font->loca_table = ttf_LoadTable(font, "loca");
		if(!font->glyf_table) font->glyf_table = ttf_LoadTable(font, "glyf");
		if(!font->hmtx_table) font->hmtx_table = ttf_LoadTable(font, "hmtx");
		}

	/* Emmit the CharStrings array. */
	ttf_PS_CharStrings(font, target_type);

	/* Send the font trailer. */
	ttf_PS_trailer(font, target_type);

	return 0;
	} /* end of ttf_PS() */

/* end of file */

