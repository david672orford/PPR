/*
** mouse:~ppr/src/include/libppr_font.h
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
** Last modified 16 March 2005.
*/

struct ENCODING_INFO {
	const char *charset;
	const char *encoding;
	gu_boolean encoding_ascii_compatible;
	char line[256];
	} ;

int charset_to_encoding(const char charset[], struct ENCODING_INFO *encinfo);

struct FONT_INFO
	{
	char *font_family;
	char *font_weight;
	char *font_slant;
	char *font_width;
	char *font_psname;			/* font's PostScript name */
	char *font_encoding;		/* font's default encoding */
	int font_type;				/* font format type FONT_TYPE_* */

	char *ascii_subst_font;		/* use instead for ASCII only */

	char line[256];				/* storage space for strings */
	};

int encoding_to_font(const char encoding[], const char fontfamily[], const char fontweight[], const char fontslant[], const char fontwidth[], struct FONT_INFO *fontinfo);
struct FONT_INFO *font_info_new(void);
void font_info_delete(struct FONT_INFO *p);

/* end of file */

