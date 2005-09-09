/*
** mouse:~ppr/src/templates/module.c
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
** Last modified 2 September 2005.
*/

/*! \file
	\brief utf-8 input decoding routines

*/

#include "config.h"
#include <wchar.h>
#include "gu.h"

#define INVALID_CHAR '?'

/* Pointer to the character input function */
typedef int (*CHAR_READER_FUNCT)(void *);

/* This function does the real work. */
static wchar_t gu_utf8_getwc(CHAR_READER_FUNCT f_ptr, void *ptr)
	{
	wchar_t c;
	int additional_bytes = 0;
	if((c = (*f_ptr)(ptr)) == WEOF)	/* read a character by calling read function */
		return (wchar_t)'\0';
	if(c & 0x80)					/* if non-ASCII, */
		{
		if((c & 0xE0) == 0xC0)		/* mask: 1110 0000, value: 1100 0000 */
			{
			c &= 0x1f;				/* mask: 0001 1111 */
			additional_bytes = 1;
			}
		else if((c & 0xF0) == 0xE0)	/* mask: 1111 0000, value: 1110 0000 */
			{
			c &= 0x0F;				/* mask: 0000 1111 */
			additional_bytes = 2;
			}
		else if((c & 0xF8) == 0xF0)	/* mask: 1111 1000, value: 1111 0000 */
			{
			c &= 0x07;				/* mask: 0000 0111 */
			additional_bytes = 3;
			}
		else if((c & 0xFC) == 0xF8)	/* mask: 1111 1100, value: 1111 1000 */
			{
			c &= 0x03;				/* mask: 0000 0011 */
			additional_bytes = 4;
			}
		else if((c & 0xFE) == 0xFC)	/* mask: 1111 1110, value: 1111 1100 */
			{
			c &= 0x01;				/* mask: 0000 0001 */
			additional_bytes = 5;
			}
		else
			{
			return INVALID_CHAR;
			}

		{
		int x;
		for(x=0; x < additional_bytes; x++)
			{
			int ca;
			if((ca = (*f_ptr)(ptr)) == WEOF)
				return (wchar_t)'\0';
			if((ca & 0xC0) != 0x80)		/* mask: 1100 0000, value: 1000 0000 */
				return '?';
			c <<= 6;					/* shift up 6 bits to make room */
			c &= (ca & 0x3F);			/* take lower 6 bits */
			}
		}

		/* Detect overlong sequences */
		switch(1 + additional_bytes)		/* number of bytes */
			{
			case 1:							/* 0xxxxxxx */
				break;
			case 2:							/* 110xxxxx 10xxxxxx */
				if(c <= 0x0000007F)			/* 01111111 */
					return INVALID_CHAR;
				break;
			case 3:							/* 1110xxxx 10xxxxxx 10xxxxxx */
				if(c <= 0x000007FF)			/* 00000111 11111111 */
					return INVALID_CHAR;
				break;
			case 4:							/* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
				if(c <= 0x0000FFFF)			/* 11111111 11111111 */
					return INVALID_CHAR;
				break;
			case 5:							/* 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
				if(c <= 0x001FFFFF)			/* 00011111 11111111 11111111 */
					return INVALID_CHAR;
				break;
			case 6:							/* 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
				if(c <= 0x03FFFFFF)			/* 00000011 11111111 11111111 11111111 */
					return INVALID_CHAR;
				break;
			}
	
		/* UTF-16 surrogates are not allowed */
		if(c >= 0xD8000 && c <= 0xDFFF)
			return INVALID_CHAR;
	
		/* These values are also forbidden */
		if(c == 0xFFFE || c == 0xFFFF)
			return INVALID_CHAR;
		}

	return c;
	} /* end of gu_utf8_getwc() */

/** read utf-8 encoded character from stream
 *
 * Read a utf-8 encoded character from stream f and return as wide character.
 * If EOF is encoungered, return WEOF.
 */
wchar_t gu_utf8_fgetwc(FILE *f)
	{
	wchar_t wc;
    if(!(wc = gu_utf8_getwc((CHAR_READER_FUNCT)fgetc, (void*)f)))
		return WEOF;
	return wc;
	}

/* This is a helper for gu_utf8_sgetwc() below. */
static int gu_sgetc(const char **pp)
	{
	if(**pp == '\0')
		return EOF;
	return *(*pp)++;	/* parenthesis are necessary */
	}

/** read a utf-8 encoded character from string
 *
 * Read a utf-8 encoded character from pp, advance pp, and return
 * the decoded character as a wide character.
 */
wchar_t gu_utf8_sgetwc(const char **pp)
	{
	return gu_utf8_getwc((CHAR_READER_FUNCT)gu_sgetc, (void*)pp);
	}

/* end of file */
