/*
** fontselect.cc
** Copyright 1997, Trinity College Computing Center.
** Written by David Chappell.
**
** Last modified 13 October 1997.
**/

#include <iostream.h>
#include <stdlib.h>
#include "typesetter.h"

//
// Load the specified font and return the assigned font id.
// This routine creates a new font structure and saves a
// pointer to it in the fonts[] array.  The routine "Font::Font()"
// in fonts.cc loads a font metrics file into a font structure.
//
int Typesetter::LoadFont(char *name)
	{
	// See if we have room in the font table.
	if(fonts_index == MAX_FONTS)
		{
		cerr << "Typesetter::LoadFont(): font table overflow\n";
		exit(1);
		}

	// Load the font information.
	fonts[fonts_index] = new Font( (FontType)postscript, name );

	// Move pointer to be ready for the next new font.
	return fonts_index++;
	} // end of Typesetter::LoadFont()

//
// Select a font by font id.
// This one is very basic.  It selects a font by
// the font id which was assigned by Typesetter::LoadFont().
//
// This is considered a low level routine because higher level
// routines select the font on the basis of family name and
// attributes.
//
void Typesetter::LowLevelSelectFont(int fid)
	{
	if( fid >= fonts_index || fid < 0 )
		{
		cerr << "SelectFont(" << fid << "), non-existent fid\n";
		exit(1);
		}

	current_font=fid;
	} // end of Typesetter::LowLevelSelectFont()

//
// Select a font by family and attributes.
//
void Typesetter::SelectFont(char *family, char *attributes)
	{


	} // end of Typesetter::SelectFont()

//
// Amend the attributes list given in the SelectFont() call.
//
void Typesetter::ChangeFontAttributes(char *attributes)
	{


	} // end of Typesetter::ChangeFontAttributes()

//
// Add a character to the horizontal list.
// This is in fontselect.cc because this may require substitute fonts.
//
void Typesetter::AddCharacter(wchar_t c)
	{
	if(current_font == -1)
		throw("Typesetter::AddCharacter(): no font selected");

	// Look up the character code in the current character set
	// in order to get a PostScript name.  If the character code
	// is not defined in this character set, return ".notdef".
	const char *name = charset.GetName(c);

	// Look up the PostScript name in the current font
	// to find out what we can about it.
	const FontChar* fc = fonts[current_font] -> FindChar(name);

	// What do we do if we don't have it in this font?
	if(fc == (const FontChar*)NULL)
		return;

	// Figure the width, height, and depth
	SP width = (int)((double)(current_size.GetValue()) * fc->GetWidth() / 1000.0 + 0.5);
	SP height = (int)((double)(current_size.GetValue()) * fc->GetHeight() / 1000.0 + 0.5);
	SP depth = (int)((double)(current_size.GetValue()) * fc->GetDepth() / 1000.0 + 0.5);

	// Add the character to the current horizontal list.
	CBox *cboxptr = new CBox( current_font, current_size, name,
		fc->GetCode(), width, height, depth );
	contribute_horizontal( (OType)character, (void*)cboxptr );
	} // end of Typesetter::AddCharacter()

//
// Select a new font size.
//
void Typesetter::SelectSize(ISP newsize)
	{
	current_size = newsize;
	} // end of Typesetter::SelectSize()

// end of file
