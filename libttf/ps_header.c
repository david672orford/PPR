/*
** mouse:~ppr/src/libttf/ps_header.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 9 September 2000.
*/

#include "libttf_before_system.h"
#include "libttf_private.h"

/*
** Write a string to the PostScript output with non-printables
** and quotes and backslashes escaped.  Notice that we are
** careful to treat the characters as unsigned.
*/
static void ttf_PS_string(struct TTFONT *font, const char *string)
    {
    unsigned char *p = (unsigned char *)string;
    int c;

    while((c = *p++))
    	{
	switch(c)
	    {
	    case '(':
	    case ')':
	    case 0x5C:
	        (*font->putc)(0x5C);
	        (*font->putc)(c);
	        break;
	    default:
	        if(c >= 32 && c < 127)
	            (*font->putc)(c);
	        else
		    (*font->printf)("\\%o", c);
	    	break;
	    }
    	}
    } /* ttf_PS_string() */

/*
** Write the header for a PostScript font.
*/
void ttf_PS_header(struct TTFONT *font, int target_type)
    {
    int VMMin;
    int VMMax;

    /*
    ** To show that it is a TrueType font in PostScript format,
    ** we will begin the file with a specific string.
    ** This string also indicates the version of the TrueType
    ** specification on which the font is based and the
    ** font manufacturer's revision number for the font.
    */
    if(target_type == 42)
    	{
    	(*font->printf)("%%!PS-TrueTypeFont-%d.%d-%d.%d\n",
    		font->TTVersion.whole, font->TTVersion.fraction,
    		font->MfrRevision.whole, font->MfrRevision.fraction);
    	}

    /* If it is not a Type 42 font, we will use a different format. */
    else
    	{
    	(*font->puts)("%!PS-Adobe-3.0 Resource-Font\n");
    	}	/* See RBIIp 641 */

    /* We will make the title the name of the font. */
    (*font->printf)("%%%%Title: %s\n", font->FullName);

    /* If there is a Copyright notice, put it here too. */
    if(font->Copyright)
	(*font->printf)("%%%%Copyright: %s\n",font->Copyright);

    /* We created this file. */
    if(target_type == 42)
	(*font->puts)("%%Creator: Converted from TrueType to type 42 by PPR's libttf\n");
    else
	(*font->puts)("%%Creator: Converted from TrueType by PPR's libttf\n");

    /* If VM usage information is available, print it. */
    if(target_type == 42)
    	{
	VMMin = (int)getULONG( font->post_table + 16 );
	VMMax = (int)getULONG( font->post_table + 20 );
	if(VMMin > 0 && VMMax > 0)
	    (*font->printf)("%%%%VMUsage: %d %d\n",VMMin,VMMax);
    	}

    /* Start the dictionary which will eventually
       become the font. */
    if(target_type != 3)
	{
	(*font->puts)("15 dict begin\n");
	}
    else
	{
	(*font->puts)("25 dict begin\n");

    	/* Type 3 fonts will need some subroutines here. */
	(*font->puts)(	"/_d{bind def}bind def\n"
			"/_m{moveto}_d\n"
			"/_l{lineto}_d\n"
			"/_cl{closepath eofill}_d\n"
			"/_c{curveto}_d\n"
			"/_sc{7 -1 roll{setcachedevice}{pop pop pop pop pop pop}ifelse}_d\n"
			"/_e{exec}_d\n");
	}

    (*font->printf)("/FontName /%s def\n",font->PostName);
    (*font->puts)("/PaintType 0 def\n");

    if(target_type == 42)
	(*font->puts)("/FontMatrix[1 0 0 1 0 0]def\n");
    else
	(*font->puts)("/FontMatrix[.001 0 0 .001 0 0]def\n");

    (*font->printf)("/FontBBox[%d %d %d %d]def\n", font->llx,font->lly,font->urx,font->ury);
    (*font->printf)("/FontType %d def\n", target_type);
    } /* end of ttf_PS_header() */


/*-------------------------------------------------------------
** Define the encoding array for this font.
** It seems best to just use "Standard".
-------------------------------------------------------------*/
void ttf_PS_encoding(struct TTFONT *font)
    {
    (*font->puts)("/Encoding StandardEncoding def\n");
    } /* end of ttf_PS_encoding() */

/*-----------------------------------------------------------
** Create the optional "FontInfo" sub-dictionary.
-----------------------------------------------------------*/
void ttf_PS_FontInfo(struct TTFONT *font)
    {
    Fixed ItalicAngle;

    /* We create a sub dictionary named "FontInfo" where we
       store information which though it is not used by the
       interpreter, is useful to some programs which will
       be printing with the font. */
    (*font->puts)("/FontInfo 10 dict dup begin\n");

    /* These names come from the TrueType font's "name" table. */
    (*font->printf)("/FamilyName (%s) def\n", font->FamilyName);
    (*font->printf)("/FullName (%s) def\n", font->FullName);

    if(font->Copyright || font->Trademark)
    	{
    	(*font->puts)("/Notice (");
    	if(font->Copyright)
    	    ttf_PS_string(font, font->Copyright);
	if(font->Trademark)
	    {
	    if(font->Copyright) (*font->putc)(' ');
	    ttf_PS_string(font, font->Trademark);
	    }
	(*font->puts)(") def\n");
    	}

    /* This information is not quite correct. */
    (*font->printf)("/Weight (%s) def\n", font->Style);

    /* Some fonts have this as "version". */
    (*font->printf)("/Version (%s) def\n",font->Version);

    /* Some information from the "post" table. */
    ItalicAngle = getFixed( font->post_table + 4 );
    (*font->printf)("/ItalicAngle %d.%d def\n",ItalicAngle.whole,ItalicAngle.fraction);
    (*font->printf)("/isFixedPitch %s def\n", getULONG( font->post_table + 12 ) ? "true" : "false" );
    (*font->printf)("/UnderlinePosition %d def\n", (int)getFWord( font->post_table + 8 ) );
    (*font->printf)("/UnderlineThickness %d def\n", (int)getFWord( font->post_table + 10 ) );
    (*font->puts)("end readonly def\n\n");
    } /* end of ttf_PS_FontInfo() */

/* end of file */
