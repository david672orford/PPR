/*
** postscript.c
** Copyright 1997, Trinity College Computing Center.
** Written by David Chappell.
**
** Last modified 13 October 1997.
*/

/*
** This module contains the parent class which actually generates
** the PostScript code.  Among other things, it contains the
** routine "ShipOut()".  (The other important thing is End().)
*/

#include <iostream.h>
#include <stdlib.h>
#include <ctype.h>
#include "typesetter.h"

// If this is defined, all hboxes are outlined.
//#define DEBUG_HBOXES 1

//
// Constructor
//
PostScript::PostScript(void)
	{
	page = 0;

	// TeX uses these defaults.  Why?
	hoffset = IN(1);
	voffset = IN(1);

	// Default is no title:
	DSCTitle = (const char *)NULL;

	} // end of PostScript::PostScript()

//
// This is used to tell this level where the fonts array is.
//
void PostScript::SetFontList(Font **array)
	{
	fonts = array;
	} // end of PostScript::SetFontsList()

//
// Close a started PostScript string.
//
void PostScript::close_string(void)
	{
	if(in_string)
		cout << ")s\n";
	in_string=false;
	} // end of PostScript::close_string()

//
// Move the PostScript `cursor' to position (xpos,ypos).
//
void PostScript::achieve_position(void)
	{
	if(xpos != postscript_xpos && ypos != postscript_ypos)
		{
		close_string();
		cout << xpos << " " << ypos << " xy\n";
		}
	else if(xpos != postscript_xpos)
		{
		close_string();
		cout << xpos << " x\n";
		}
	else if(ypos != postscript_ypos)
		{
		close_string();
		cout << ypos << " y\n";
		}

	postscript_xpos = xpos;
	postscript_ypos = ypos;
	} // end of PostScript::achieve_position()

//
// Make sure we have the correct font.
//
void PostScript::achieve_font( int fontid, ISP size )
	{
	if( postscript_current_font != fontid || postscript_current_size != size )
		{
		close_string();
		if( fonts[fontid]->LoadNow() )
			{
			cout << "%%IncludeResource: font " << fonts[fontid]->GetName() << '\n';
			}
		cout << "/" << fonts[fontid]->GetName() << " " << size.GetValue() << " sf\n";
		}

	postscript_current_font = fontid;
	postscript_current_size = size;
	} // PostScript::achieve_font()

//
// Write PostScript for a character.
//
void PostScript::shipout_char(CBox *cbox)
	{
	// More to the correct position
	achieve_position();

	#ifdef DEBUG_HBOXES
	close_string();
	cout << cbox->GetWidth().GetValue() << " "
		<< cbox->GetHeight().GetValue() << " "
		<< cbox->GetDepth().GetValue() << " box\n";
	#endif

	// Select the correct font and size
	achieve_font( cbox->GetFontID(), cbox->GetSize() );

	// If the character is encoded, use show
	int c=cbox->GetCharCode();
	if( c != -1 )
		{
		if( ! in_string )
			{
			cout << "(";
			in_string=true;
			}

		switch(c)
			{
			case '(':
			case ')':
			case 0x5C:
				cout << "\\" << (char)c;
				break;
			default:
				if(isprint(c))
					cout << (char)c;
				else
					cout << "\\" << oct << c << dec;
				break;
			}
		}

	// If the character is not encoded, use glyphshow
	else
		{
		close_string();
		cout << "/" << cbox->GetCharName() << " glyphshow\n";
		}

	// Printing the character has moved the PostScript
	// current point
	postscript_xpos += cbox->GetWidth().GetValue();
	} // end of PostScript::shipout_char()

//
// Write PostScript code to draw a rule.
//
void PostScript::shipout_rule(Rule *rule)
	{
	close_string();

	// Code missing

	} // end of PostScript::shipout_rule()

//
// Write PostScript for an hbox.
//
void PostScript::shipout_hbox(SuperBox *out_hbox)
	{
	Object *members;
	int length;
	int x;

	members = out_hbox->GetMembers();
	length = out_hbox->GetNumMembers();

	// Code to draw an outline around each hbox:
	#ifdef DEBUG_HBOXES
	close_string();
	achieve_position();
	cout << out_hbox->GetWidth().GetValue() << " "
		<< out_hbox->GetHeight().GetValue() << " "
		<< out_hbox->GetDepth().GetValue() << " box\n";
	#endif

	// Loop thru the members of the hbox:
	for(x = 0; x < length; x++)
		{
		switch(members[x].GetType())
			{
			case (OType)character:
				CBox *this_cbox;
				this_cbox = (CBox*)(members[x].GetPtr());
				shipout_char(this_cbox);
 				xpos += this_cbox->GetWidth().GetValue();
				break;
			case (OType)hbox:				// really is posible!
				SuperBox *this_hbox;
				this_hbox = (SuperBox*)(members[x].GetPtr());
				shipout_hbox(this_hbox);
				break;
			case (OType)vbox:				// rather common
				SuperBox *this_vbox;
				this_vbox = (SuperBox*)(members[x].GetPtr());
				shipout_vbox(this_vbox);
				break;
			case (OType)glue:				// glue just moves the cursor
				Glue *this_hglue;
				this_hglue = (Glue*)(members[x].GetPtr());
				xpos += this_hglue->GetWidth().GetValue();
				break;
			case (OType)vrule:
				Rule *this_vrule;
				this_vrule = (Rule*)(members[x].GetPtr());
				shipout_rule(this_vrule);
				xpos += this_vrule->GetWidth().GetValue();
				break;
			case (OType)kern:				// kerns just move the cursor
				Kern *this_hkern;
				this_hkern = (Kern*)(members[x].GetPtr());
				xpos += this_hkern->GetWidth().GetValue();
				break;
			case (OType)penalty:			// ignore penalties
				break;
			case (OType)disbreak:
				throw("shipout_hbox(): discretionary break not expected in final hboxes");
			default:						// paranoid (compiler should catch)
				throw("shipout_hbox(): illegal item in hbox");
			}
		}

	} // end of Typesetter::shipout_hbox()

//
// Write PostScript for a vbox.
//
void PostScript::shipout_vbox(SuperBox *out_vbox)
	{
	Object *members;
	int length;
	int x;
	int xstart;

	members = out_vbox->GetMembers();
	length = out_vbox->GetNumMembers();

	xstart = xpos;			// save left margin of this vbox

	for(x = 0; x < length; x++)
		{
		xpos = xstart;		// return to level margin of this vbox

		switch(members[x].GetType())
			{
			case (OType)hbox:
				SuperBox *this_hbox;
				this_hbox = (SuperBox*)(members[x].GetPtr());
				ypos += this_hbox->GetHeight().GetValue();
				shipout_hbox( this_hbox );
				ypos += this_hbox->GetDepth().GetValue();
				break;
			case (OType)vbox:
				SuperBox *this_vbox;
				this_vbox = (SuperBox*)(members[x].GetPtr());
				ypos += this_vbox->GetHeight().GetValue();
				shipout_vbox( this_vbox );
				ypos += this_vbox->GetDepth().GetValue();
				break;
			case (OType)glue:
				Glue *this_vglue;
				this_vglue = (Glue*)(members[x].GetPtr());
				ypos += this_vglue->GetWidth().GetValue();
				break;
			case (OType)hrule:
				Rule *this_hrule;
				this_hrule = (Rule*)(members[x].GetPtr());
				ypos += this_hrule->GetHeight().GetValue();
				shipout_rule(this_hrule);
				ypos += this_hrule->GetHeight().GetValue();
				break;
			case (OType)penalty:		// ignore penalties
				break;
			default:
				throw("PostScript::shipout_vbox(): illegal item in vbox\n");
			}
		} // end of for loop

	} // end of Typesetter::shipout_vbox()

//
// Take a box and convert it to PostScript.
//
void PostScript::ShipOut(Object *obj)
	{
	cerr << "PostScript::ShipOut()\n";

	// the type of object we have on our hands
	OType type = obj->GetType();

	if(type != (OType)hbox && type != (OType)vbox)
		throw("PostScript::ShipOut() must have a box as its argument");

	// Even if we have already printed a page, our PostScript
	// font selection is now invalid since we executed a
	// restore at the end of the last page.
	postscript_current_font = -1;
	postscript_current_size = -1;

	// We are, of course, not in a PostScript string.
	in_string = false;

	// Send the PS header if we haven't already:
	if(page == 0)
		send_header();

	// Start a new page:
	page++;
	cout << "%%Page: " << page << " " << page << "\n";
	cout << "%%BeginPageSetup\n";
	cout << "bp\n";
	cout << "%%EndPageSetup\n";

	xpos = hoffset.GetValue();
	ypos = voffset.GetValue();
	postscript_xpos = postscript_ypos = -1;

	if(type == (OType)hbox)
		shipout_hbox( (SuperBox*)(obj->GetPtr()) );
	else
		shipout_vbox( (SuperBox*)(obj->GetPtr()) );

	close_string();
	cout << "%%PageTrailer\n";
	cout << "ep\n\n";
	} // end of PostScript::ShipOut()

void PostScript::send_header(void) const
	{
	cout << "%!PS-Adobe-3.0\n";
	cout << "%%Creator: David Chappell's Typeset library of "__DATE__"\n";
	if(DSCTitle) { cout << "%%Title: " << DSCTitle << "\n"; }
	cout << "%%Pages: (atend)\n";
	cout << "%%EndComments\n\n";

	cout << "%%BeginProlog\n";

	cout << "/bp {save} def\n";
	cout << "/ep {restore showpage} def\n";

	cout << "/x {65782 div currentpoint exch pop moveto} def\n";
	cout << "/y {65782 div 792 exch sub currentpoint pop exch moveto} def\n";
	cout << "/xy {65782 div 792 exch sub exch 65782 div exch moveto} def\n";
	cout << "/s {show} def\n";
	cout << "/sf {65782 div selectfont} def\n";
	cout << "/box {gsave /d exch 65782 div def\n"
				<< "/h exch 65782 div def /w exch 65782 div def\n"
				<< ".125 setlinewidth\n"
				<< "0 h rlineto w 0 rlineto\n"
				<< "0 h d add neg rlineto w neg 0 rlineto\n"
				<< "closepath stroke\n"
				<< "grestore}def\n";

	cout << "%%EndProlog\n\n";

	cout << "%%BeginSetup\n";
	cout << "%%EndSetup\n\n";
	}

//
// Write out any lingering data.
//
void PostScript::End(void)
	{
	if(page > 0)
		{
		cout << "%%Trailer\n";
		cout << "%%Pages: " << page << "\n";
		cout << "%%EOF\n";
		}
	} // end of PostScript::End()

// end of file
