/*
** fonts.h
** Copyright 1997, Trinity College Computing Center.
** Written by David Chappell.
**
** Last modified 3 October 1997.
*/

/*
** This file describes the classes which describe fonts
** and their metrics.
*/

// The size of the character code hash table.
// The character code hash table is used to
// translate the character codes of the current
// encoding into PostScript character names.
const int char_table_size = 100;

// The size of the hash table in each font entry.
// This hash table is used to look up character names
// and translate them into character codes in the
// current font encoding.
const int font_table_size=100;

//
// One of these structures is used to represent
// each character in the current character set.
//
class CharName
	{
	int code;				// code in selected character set
	const char *name;		// PostScript name
	CharName *next;			// next record in string

	public:

	CharName(int icode = 0, const char *iname = (char*)NULL)
		{ code = icode; name = iname; next = (CharName*)NULL; }
	void SetNext(CharName *nnext) { next = nnext; }
	int GetCode(void) const { return code; }
	const char *GetName(void) const { return name; }
	CharName *GetNext(void) { return next; }
	} ;

//
// One of these structures is used to represent a
// character set.  A character set is used to translate
// incoming character codes into PostScript character
// names.
//
class CharNameList
	{
	CharName *list[char_table_size];
		void AddMember(int code, char *name);

	public:

	CharNameList()
		{
		int x;
		for(x=0; x < char_table_size; x++)
			list[x] = (CharName*)NULL;
		}
	void LoadFile(const char *file);
	const char *GetName(int code);
	} ;

//
// One of these structures is used to represent each
// character in each font.
//
class FontChar
	{
	const char *name;		// name from AFM file
	int code;				// code in default encoding
	double width;
	double height;
	double depth;
	double correction;
	FontChar *next;			// pointer to next record for this hash

	public:

	FontChar(const char *iname, int icode, double iw, double ih, double id, double ic)
		{
		name = iname; code = icode; width = iw; height = ih; depth = id;
		correction = ic; next = (FontChar*)NULL;
		}
	const char *GetName(void) const { return name; }
	int GetCode(void) const { return code; }
	void SetNext(FontChar *n) { next = n; }
	double GetWidth(void) const { return width; }
	double GetHeight(void) const { return height; }
	double GetDepth(void) const { return depth; }
	double GetCorrection(void) const { return correction; }
	FontChar *GetNext(void) { return next; }
	} ;

//
// One of these structures is used to represent each font.
//
enum FontType {postscript, truetype, metafont};
class Font
	{
	FontType type;
	const char *family;			// "Times"
	const char *name;				// "Times-Bold"
	const char *fullname;			// "Times Bold"
	const char *weight;			// "Bold"
	const char *filename;			// "/usr/lib/psfonts/Times-Roman"
	FontChar *characters[font_table_size];
	int used;
	int used_this_page;

	public:

	Font(FontType type, const char *metrics_file);
	void SetFamily(const char *s) { family = s; }
	void SetName(char *s) { name = s; }
	void SetFullname(char *s) { fullname = s; }
	void SetWeight(char *s) { weight = s; }
	void SetFilename(char *s) { filename = s; }
	const char *GetFamily(void) const { return family; }
	const char *GetName(void) const { return name; }
	const char *GetFullName(void) const { return fullname; }
	const char *GetWeight(void) const { return weight; }
	const char *GetFilename(void) const { return filename; }
	void AddChar(const char *name, int code, double w, double h, double d, double c);
	const FontChar *FindChar(const char *name) const;
	int LoadNow(void);
	void ClearPageFlags(void);
	} ;

// end of file

/*
** fonts.cc
** Copyright 1997, Trinity College Computing Center.
** Written by David Chappell.
**
** Last modified 24 October 1997.
*/

/*
** This module is concerned with low level operations upon fonts
** such as loading them, retrieving their metrics, etc.
**
** It is also concerned with loading and using character sets.
** Character sets are defined as translation tables which
** convert incoming character codes into PostScript character names.
*/

#include <iostream.h>
#include <stdio.h>
#include <fstream.h>
#include <ctype.h>
#include <string.h>
#include <string>
#include "typesetter.h"
#include "util.h"

//
// This is called from CharNameList::LoadFile().
//
// Add a character code and its PostScript name to the
// hash table.  It is expected that the character
// name is static and that is it sufficient for us
// to keep the pointer.
//
void CharNameList::AddMember(int code, char *name)
	{
	int hash = code % char_table_size;

	CharName *p = list[hash];

	if(p == (const CharName*)NULL)
		{
		list[hash] = new CharName(code,name);
		}
	else
		{
		while(p->GetNext() != (const CharName*)NULL)
			{
			p = p->GetNext();
			}
		p -> SetNext( new CharName(code, name) );
		}
	} // end of CharNameList::AddMember()

//
// This is invoked during typesetter initialization.
//
// Load a character set definition file
// and call AddMember repeatedly to add
// all the character definitions in it.
//
void CharNameList::LoadFile(const char *filename)
	{
	ifstream in(filename, ios::in);

	if( ! in )
		{
		string *s = new string("Can't open character set definition file \"");
		*s += filename;
		*s += "\"";
		throw(s);
		}

	int linenum = 0;
	char line[80];
	int code;
	int x, n;
	char *ptr;
	while( in.getline(line, sizeof(line)) )
		{
		linenum++;

		// Lines which don't begin with a hexadecimal digit
		// are to be considered comments.
		if( ! isxdigit( line[0] ) )
			continue;

		// Interpret the hexadecimal number.
		code = 0;
		for(x = 0; ; x++)
			{
			n = line[x];
			if( n >= '0' && n <= '9' )
				n -= '0';
			else if( n >= 'A' && n <= 'F' )
				n -= ( 'A' - 10 );
			else if( n >= 'a' && n <= 'f' )
				n -= ( 'a' - 10 );
			else
				break;

			code *= 16;
			code += n;
			}

		// Eat up the white space between
		// the code number and the name.
		while( isspace( line[x] ) )
			x++;

		// Get a fresh copy of the name into ptr.
		ptr = &line[x];
		ptr[strcspn(ptr," \t")] = (char)NULL;
		if( *ptr == (char)NULL )
			{
			cerr << "Syntax error \"" << filename << "\" line " << linenum << ":\n";
			cerr << line << '\n';
			exit(1);
			}

		// Add it to the character set.
		AddMember( code, mystrdup(ptr) );

		} // until end of file

	} // end of CharNameList::LoadFile()

//
// Translate a character code in the current character set
// into a PostScript character name.
//
// If the character is not known, return ".notdef".
//
const char *CharNameList::GetName(int code)
	{
	int hash = code % char_table_size;

	CharName *p = list[hash];

	while(p != (CharName*)NULL)
		{
		if( p->GetCode() == code )
			return p->GetName();
		else
			p = p->GetNext();
		}

	return ".notdef";
	} // end of CharNameList::GetName()

//
// Load font information.
// For PostScript fonts, this information comes from
// an Adobe Font Metrics file.
//
extern FILE *yyin;	// the lexer's input
int yylex(void);	// the lexer
Font *thisfont;		// global for yylex()
Font::Font(FontType type, const char *metrics_file)
	{
	int x;

	// Clear the font's character name hash table
	for(x=0; x < font_table_size; x++)
		characters[x] = (FontChar*)NULL;

	// Save the metrics file name
	filename = metrics_file;

	// Clear the records of use
	used = false;
	used_this_page = false;

	// For PostScript fonts we read a .AFM file
	if(type == (FontType)postscript)
		{
	    string fname(filename);
	    fname += ".afm";

		// Open the metrics file for lex
		if((yyin=fopen(fname.c_str(), "r")) == (FILE*)NULL)
			{
		    string *s = new string("Font::Font(): failed to open font metrics file \"");
   		    *s += fname;
		    *s += "\"";
		    throw(s);
			}

		// Set a pointer which yylex() can use
		// set set things in the font.
		thisfont = this;

		// Let lex get to work
		yylex();

		// Close the metrics file
		fclose(yyin);
		}

	// Support for other metrics file formats is not here yet
	else
		{
		throw("Font::Font(): only PostScript fonts are supported");
		}

	} // Font::Font()

//
// Add a character to a font.
// This is called from the lexer.
//
void Font::AddChar(const char *name, int code, double w, double h, double d, double c)
	{
	// cerr << "Font::AddChar( name=\"" << name << "\", code=" << code
	// << ", w=" << w
	// << ", h=" << h << ", d=" << d << ", c=" << c << " )\n";

	// Hash the name and reduce the hash value to something
	// which will fit into the character table.
	unsigned int hashval = hash(name) % font_table_size;

	FontChar *p = characters[hashval];

	if(p == (FontChar*)NULL)	// if hash bucket empty,
		{						// just assign it
		characters[hashval] = new FontChar(name, code, w, h, d, c);
		}
	else						// otherwise, we must tour
		{						// to the end and add it there
		while(p->GetNext() != (FontChar*)NULL)
			p = p->GetNext();
		p -> SetNext( new FontChar(name, code, w, h, d, c) );
		}
	} // end of Font::AddChar()

//
// Find a character in a font.
// We search by PostScript character name,
// If it is found, we return a pointer to its FontChar structure.
// If we can't find it, we return a NULL pointer.
//
const FontChar *Font::FindChar(const char *name) const
	{
	unsigned int hashval = hash(name) % font_table_size;

	FontChar *p = characters[hashval];

	while(p != (FontChar*)NULL)
		{
		if(strcmp(p->GetName(), name) == 0)
			{
			return p;
			}
		else
			{
			p = p -> GetNext();
			}
		}

	return (FontChar*)NULL;
	} // Font::FindChar()

//
// This is used when generating the PostScript output.
// Return true if we need to download the font now.
//
int Font::LoadNow(void)
	{
	if(used_this_page)
		{
		return false;
		}
	else
		{
		used_this_page = true;
		return true;
		}
	} // end of Font::LoadNow()

//
// This is used when generating PostScript output.
// Clear the used one this page flag for a font.
//
void Font::ClearPageFlags(void)
	{
	used_this_page = false;
	} // end of Font::LoadNow()

// end of file
