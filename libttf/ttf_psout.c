/*
** mouse:~ppr/src/libttf/ttf_psout.c
** Copyright 1995, 1996, 1997, 1998, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 10 November 1998.
*/

#include "libttf_before_system.h"
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

