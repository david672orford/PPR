/*
** mouse:~ppr/src/include/libppr_font.h
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
** Last modified 19 July 1999.
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
    char *font_psname;		/* font's PostScript name */
    char *font_encoding;	/* font's default encoding */
    char *font_type;		/* font format type name */

    char *ascii_subst_font;	/* use instead for ASCII only */

    char line[256];		/* storage space for strings */
    };

int encoding_to_font(const char encoding[], const char fontfamily[], const char fontweight[], const char fontslant[], const char fontwidth[], struct FONT_INFO *fontinfo);
struct FONT_INFO *font_info_new(void);
void font_info_delete(struct FONT_INFO *p);

/* end of file */

