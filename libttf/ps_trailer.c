/*
** mouse:~ppr/src/libttf/ps_trailer.c
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
** Emmit the code to finish up the dictionary and turn
** it into a font.
*/
void ttf_PS_trailer(struct TTFONT *font, int target_type)
	{
	/* If we are generating a type 3 font, we need to provide
	   a BuildGlyph and BuildChar procedures. */
	if(target_type == 3)
		{
		(*font->putc)('\n');

		(*font->puts)(	"/BuildGlyph\n"
						" {exch begin\n"
						" CharStrings exch\n"
						" 2 copy known not{pop /.notdef}if\n"
						" true 3 1 roll get exec\n"
						" end}_d\n\n");

		/* This proceedure is for compatiblity with
		   level 1 interpreters. */
		(*font->puts)(	"/BuildChar {\n"
						" 1 index /Encoding get exch get\n"
						" 1 index /BuildGlyph get exec\n"
						"}_d\n\n");
		}

	/*
	** If we are generating a type 42 font, we need to check to see
	** if this PostScript interpreter understands type 42 fonts.  If
	** it doesn't, we will hope that the Apple TrueType rasterizer
	** has been loaded and we will adjust the font accordingly.
	** I found out how to do this by examining a TrueType font
	** generated by a Macintosh.  That is where the TrueType interpreter
	** setup instructions and part of BuildGlyph came from.
	*/
	else if(target_type == 42)
		{
		(*font->putc)('\n');

		/* If we have no "resourcestatus" command, or FontType 42
		   is unknown, leave "true" on the stack. */
		(*font->puts)(	"systemdict/resourcestatus known\n"
						" {42 /FontType resourcestatus\n"
						"    {pop pop false}{true}ifelse}\n"
						" {true}ifelse\n");

		/* If true, execute code to produce an error message if
		   we can't find Apple's TrueDict in VM. */
		(*font->puts)("{/TrueDict where{pop}{(%%[ Error: no TrueType rasterizer ]%%)= flush}ifelse\n");

		/* Since we are expected to use Apple's TrueDict TrueType
		   reasterizer, change the font type to 3. */
		(*font->puts)("/FontType 3 def\n");

		/* Define a string to hold the state of the Apple
		   TrueType interpreter. */
		(*font->puts)(" /TrueState 271 string def\n");

		/* It looks like we get information about the resolution
		   of the printer and store it in the TrueState string. */
		(*font->puts)(	" TrueDict begin sfnts save\n"
						" 72 0 matrix defaultmatrix dtransform dup\n"
						" mul exch dup mul add sqrt cvi 0 72 matrix\n"
						" defaultmatrix dtransform dup mul exch dup\n"
						" mul add sqrt cvi 3 -1 roll restore\n"
						" TrueState initer end\n");

		/*
		** This BuildGlyph procedure will look the name up in the
		** CharStrings array, and then check to see if what it gets
		** is a procedure.  If it is, it executes it, otherwise, it
		** lets the TrueType rasterizer loose on it.
		**
		** When this proceedure is executed the stack contains
		** the font dictionary and the character name.  We
		** exchange arguments and move the dictionary to the
		** dictionary stack.
		*/
		(*font->puts)(" /BuildGlyph{exch begin\n");
				/* stack: charname */

		/* Put two copies of CharStrings on the stack and consume
		   one testing to see if the charname is defined in it,
		   leave the answer on the stack. */
		(*font->puts)("  CharStrings dup 2 index known\n");
				/* stack: charname CharStrings bool */

		/* Exchange the CharStrings dictionary and the charname,
		   but if the answer was false, replace the character name
		   with ".notdef". */
		(*font->puts)("    {exch}{exch pop /.notdef}ifelse\n");
				/* stack: CharStrings charname */

		/* Get the value from the CharStrings dictionary and see
		   if it is executable. */
		(*font->puts)("  get dup xcheck\n");			/* stack: CharStrings_entry */

		/* If is a procedure, Execute according to RBIIp 277-278. */
		(*font->puts)("    {currentdict systemdict begin begin exec end end}\n");

		/* Is a TrueType character index, let the rasterizer at it. */
		(*font->puts)("    {TrueDict begin /bander load cvlit exch TrueState render end}\n");

		(*font->puts)("    ifelse\n");

		/* Pop the font's dictionary off the stack. */
		(*font->puts)(" end}bind def\n");

		/* This is the level 1 compatibility BuildChar
		   procedure.  See RBIIp 281. */
		(*font->puts)(	" /BuildChar{\n"
						"  1 index /Encoding get exch get\n"
						"  1 index /BuildGlyph get exec\n"
						" }bind def\n");

		/* Here we close the condition which is true
		   if the printer has no built-in TrueType
		   rasterizer. */
		(*font->puts)("}if\n\n");
		} /* end of if Type 42 not understood. */

	(*font->puts)("FontName currentdict end definefont pop\n");
	(*font->puts)("%%EOF\n");
	} /* end of ttf_PS_trailer() */

/* end of file */
