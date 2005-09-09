/*
** mouse:~ppr/src/libttf/charstrings.c
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
** Last modified 9 September 2005.
*/

#include "config.h"
#include <string.h>
#include "libttf_private.h"

/*
** Create the CharStrings dictionary which will translate
** PostScript character names to TrueType font character
** indexes.
**
** If we are creating a type 3 instead of a type 42 font,
** this array will instead convert PostScript character names
** to executable procedures.
*/

static char *Apple_CharStrings[]={
".notdef",".null","nonmarkingreturn","space","exclam","quotedbl","numbersign",
"dollar","percent","ampersand","quotesingle","parenleft","parenright",
"asterisk","plus", "comma","hyphen","period","slash","zero","one","two",
"three","four","five","six","seven","eight","nine","colon","semicolon",
"less","equal","greater","question","at","A","B","C","D","E","F","G","H","I",
"J","K", "L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z",
"bracketleft","backslash","bracketright","asciicircum","underscore","grave",
"a","b","c","d","e","f","g","h","i","j","k", "l","m","n","o","p","q","r","s",
"t","u","v","w","x","y","z","braceleft","bar","braceright","asciitilde",
"Adieresis","Aring","Ccedilla","Eacute","Ntilde","Odieresis","Udieresis",
"aacute","agrave","acircumflex","adieresis","atilde","aring","ccedilla",
"eacute","egrave","ecircumflex","edieresis","iacute","igrave","icircumflex",
"idieresis","ntilde","oacute","ograve","ocircumflex","odieresis","otilde",
"uacute","ugrave","ucircumflex","udieresis","dagger","degree","cent",
"sterling","section","bullet","paragraph","germandbls","registered",
"copyright","trademark","acute","dieresis","notequal","AE","Oslash",
"infinity","plusminus","lessequal","greaterequal","yen","mu","partialdiff",
"summation","product","pi","integral","ordfeminine","ordmasculine","Omega",
"ae","oslash","questiondown","exclamdown","logicalnot","radical","florin",
"approxequal","Delta","guillemotleft","guillemotright","ellipsis",
"nobreakspace","Agrave","Atilde","Otilde","OE","oe","endash","emdash",
"quotedblleft","quotedblright","quoteleft","quoteright","divide","lozenge",
"ydieresis","Ydieresis","fraction","currency","guilsinglleft","guilsinglright",
"fi","fl","daggerdbl","periodcentered","quotesinglbase","quotedblbase",
"perthousand","Acircumflex","Ecircumflex","Aacute","Edieresis","Egrave",
"Iacute","Icircumflex","Idieresis","Igrave","Oacute","Ocircumflex","apple",
"Ograve","Uacute","Ucircumflex","Ugrave","dotlessi","circumflex","tilde",
"macron","breve","dotaccent","ring","cedilla","hungarumlaut","ogonek","caron",
"Lslash","lslash","Scaron","scaron","Zcaron","zcaron","brokenbar","Eth","eth",
"Yacute","yacute","Thorn","thorn","minus","multiply","onesuperior",
"twosuperior","threesuperior","onehalf","onequarter","threequarters","franc",
"Gbreve","gbreve","Idot","Scedilla","scedilla","Cacute","cacute","Ccaron",
"ccaron","dmacron","markingspace","capslock","shift","propeller","enter",
"markingtabrtol","markingtabltor","control","markingdeleteltor",
"markingdeletertol","option","escape","parbreakltor","parbreakrtol",
"newpage","checkmark","linebreakltor","linebreakrtol","markingnobreakspace",
"diamond","appleoutline"};

/*
** This routine is called by the one below.
** It is also called from pprdrv_tt2.c
*/
const char *ttf_charindex2name(struct TTFONT *font, int charindex)
	{
	unsigned int GlyphIndex;
	static char temp[80];
	BYTE *ptr;
	size_t len;

	GlyphIndex = (unsigned int)getUSHORT( font->post_table + 34 + (charindex * 2) );

	if(GlyphIndex <= 257)				/* If a standard Apple name, */
		{
		return Apple_CharStrings[GlyphIndex];
		}
	else								/* Otherwise, use one */
		{								/* of the pascal strings. */
		GlyphIndex -= 258;

		/* Set pointer to start of Pascal strings. */
		ptr = ( font->post_table + 34 + (font->numGlyphs * 2) );

		len = (size_t)*(ptr++);			/* Step thru the strings */
		while(GlyphIndex--)				/* until we get to the one */
			{							/* that we want. */
			ptr += len;
			len = (size_t)*(ptr++);		/* cast BTYE to unsigned int */
			}

		if(len >= sizeof(temp))
			longjmp(font->exception, (int)TTF_LONGPSNAME);

		strncpy(temp, (char*)ptr, len); /* Copy the pascal string into */
		temp[len] = '\0';				/* a buffer and make it ASCIIz. */

		return temp;
		}
	} /* end of ttfont_charindex2name() */

/*
** This is the central routine of this section.
*/
void ttf_PS_CharStrings(struct TTFONT *font, int target_type)
	{
	Fixed post_format;
	int x;

	/* The 'post' table format number. */
	post_format = getFixed( font->post_table );

	if(post_format.whole != 2 || post_format.fraction != 0)
		longjmp(font->exception, (int)TTF_UNSUP_POST);

	/* Emmit the start of the PostScript code to define the dictionary. */
	(*font->printf)("/CharStrings %d dict dup begin\n", font->numGlyphs);

	/* Emmit one key-value pair for each glyph. */
	for(x=0; x < font->numGlyphs; x++)
		{
		if(target_type == 42)			/* type 42 */
			{
			(*font->printf)("/%s %d def\n", ttf_charindex2name(font,x), x);
			}
		else							/* type 3 */
			{
			(*font->printf)("/%s{", ttf_charindex2name(font, x));

			ttf_PS_type3_charproc(font,x);

			(*font->puts)("}_d\n");		/* "} bind def" */
			}
		}

	(*font->puts)("end readonly def\n");
	} /* end of ttf_PS_CharStrings() */

/* end of file */

