/*
** mouse:~ppr/src/filter_pcl/PSout.java
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
** Last modified 23 January 2004.
*/

import java.io.*;

/*
** This class generates PostScript.
*/
class PSout
	{
	// This is where we send the PostScript.
	private PrintStream out;

	// These members are used to determine if certain PostScript
	// document elements are currently started but not finshed.
	private boolean doc_open;
	private boolean page_open;
	private boolean string_open;

	// This is the physical page size in 1/7200ths of an inch.
	private boolean manualfeed;
	private int page_orientation;
	private int phys_width;
	private int phys_height;
	private String phys_name;
	private String inputslot;
	
	// Everything above duplicated, document default.
	private String default_inputslot;
	private boolean default_manualfeed;
	private int default_phys_width;
	private int default_phys_height;
	private String default_phys_name;
	private int default_page_orientation;

	// This is where the cursor is.  It is relative to the top left
	// corner.	The units are 1/7200th of an inch.
	private int xpos, ypos;
	private int ps_xpos, ps_ypos;

	// Current page number
	private int page;

	// Current font
	PSfont font = null;
	String ps_font_name = "";
	int ps_font_size = 0;

	public PSout(OutputStream outfile) throws AssertionFailed
		{
		out = new PrintStream(outfile);
		inputslot = null;
		manualfeed = false;
		doc_open = page_open = string_open = false;
		page = 0;
		page_reset();
		}

	// This is called at the start of each page to reset any state that
	// is reset at the end of a page.
	private void page_reset()
		{
		xpos = ypos = 0;
		}

	// This is the physical page size in 1/7200ths of an inch.
	public void set_pagesize(int width, int height, String name)
		{
		phys_width = width;
		phys_height = height;
		phys_name = name;
		}

	// Set the PostScript input slot name.
	public void set_inputslot(String name)
		{
		inputslot = name;
		}

	// Set the PostScript manual feed feature on or off.
	public void set_manualfeed(boolean state)
		{
		manualfeed = state;
		}
	
	// Set the font to use.
	public void set_font(PSfont new_font)
		{
		font = new_font;
		}

	// This is the PCL style page orientation.
	public void set_orientation(int orientation) throws AssertionFailed
		{
		if(orientation < 0 || orientation > 3)
			throw new AssertionFailed("requested orientation out of range");
		page_orientation = orientation;
		}

	// Figure out the page width depending on the orientation.
	public int get_page_width() throws AssertionFailed
		{
		switch(page_orientation)
			{
			case 0:
			case 2:
				return phys_width;
			case 1:
			case 3:
				return phys_height;
			default:
				throw new AssertionFailed("Impossible orientation in object state");
			}
		}

	// Figure out the page height depending on the orientation.
	public int get_page_height() throws AssertionFailed
		{
		switch(page_orientation)
			{
			case 0:
			case 2:
				return phys_height;
			case 1:
			case 3:
				return phys_width;
			default:
				throw new AssertionFailed("Impossible orientation in object state");
			}
		}

	// This is where the cursor is.  It is relative to the top left
	// corner.	The units are 1/7200th of an inch.
	public void moveto(int x, int y)
		{
		xpos = x;
		ypos = y;
		}
	public void xmoveto(int x)
		{
		xpos = x;
		}
	public void ymoveto(int y)
		{
		ypos = y;
		}
	public void rmoveto(int x, int y)
		{
		xpos += x;
		ypos += y;
		}
	public void rxmoveto(int x)
		{
		xpos += x;
		}
	public void rymoveto(int y)
		{
		ypos += y;
		}
	public int get_xpos()
		{
		return xpos;
		}
	public int get_ypos()
		{
		return ypos;
		}

	// This is where the PostScript cursor is.	The PostScript cursor
	// might lag behind.
	private void attain_position()
		{
		open_page();

		if(xpos != ps_xpos && ypos != ps_ypos)
			{
			close_string();
			out.print("\n" + xpos + " " + ypos + " XY ");
			}

		else if(ypos != ps_ypos)
			{
			close_string();
			int difference = (ypos - ps_ypos);
			out.print("\n" + difference + " y ");
			}

		else if(xpos != ps_xpos)
			{
			int difference = (xpos - ps_xpos);
			if(string_open && difference > 0 && (difference % get_space_width()) == 0)
				{
				int spaces = (difference / get_space_width());
				if(spaces <= 6)
					{
					while(spaces-- > 0)
						out.print(" ");
					}
				else
					{
					close_string();
					out.print(spaces + " S ");
					}
				}
			else
				{
				close_string();
				out.print(difference + " x ");
				}
			}

		ps_xpos = xpos;
		ps_ypos = ypos;
		}

	// This is where we select one of the fonts.
	private void attain_font()
		{
		String font_name = font.get_postscript_name();
		int font_size = font.get_size();

		if(ps_font_size != font_size || ps_font_name != font_name)
			{
			close_string();
			out.print("/" + font_name + " findfont " + font_size + " scalefont setfont ");

			ps_font_name = font_name;
			ps_font_size = font_size;
			}
		}

	// What is the space width of the selected font?
	public int get_space_width()
		{
		return (int)(font.get_size() * 0.6 + 0.5);
		}

	// Get the width of the indicated character.
	public int get_char_width(int c)
		{
		return (int)(font.get_size() * 0.6 + 0.5);
		}

	private void open_doc()
		{
		if(!doc_open)
			{
			out.print("%!PS-Adobe-3.0\n"
				+ "%%Pages: (atend)\n"
				+ "%%EndComments\n\n");

			out.print("%%BeginProlog\n"
				+ "/XY { neg moveto } def\n"
				+ "/x { 0 rmoveto } def\n"
				+ "/y { neg 0 exch rmoveto } def\n"
				+ "/S { 1 1 3 -1 roll { pop ( ) show } for } def\n"
				+ "/s { show } def\n"
				+ "/bp { [\n"
				+ " { 0 ph translate } % portrait\n"
				+ " { 90 rotate } % landscape\n"
				+ " { pw 0 translate 180 rotate } % reverse portrait\n"
				+ " { pw ph translate -90 rotate } % reverse landscape\n"
				+ " ] orient get exec\n"
				+ " 0.01 0.01 scale\n"
				+ " } def\n"
				+ "/ep { restore showpage } def\n"
				+ "%%EndProlog\n\n");

			out.print("%%BeginSetup\n");
			if(inputslot != null)
				out.print("%%IncludeFeature: *InputSlot " + inputslot + "\n");
			out.print("%%IncludeFeature: *ManualFeed " + (manualfeed ? "True" : "False") + "\n");
			out.print("%%IncludeFeature: *PageSize " + phys_name + "\n");
			out.print("/ph " + phys_height + " 100 div def\n");
			out.print("/pw " + phys_width  + " 100 div def\n");
			out.print("/orient " + page_orientation + " def\n");
			default_inputslot = inputslot;
			default_manualfeed = manualfeed;
			default_phys_name = phys_name;
			default_phys_height = phys_height;
			default_phys_width = phys_width;
			default_page_orientation = page_orientation;
			out.print("%%EndSetup\n\n");

			doc_open = true;
			}
		}

	// This method should be called when the user is done with the object.
	public void finish()
		{
		if(doc_open)
			{
			close_page();
			out.print("%%Trailer\n");
			out.print("%%Pages: "); out.print(page); out.print("\n");
			out.print("%%EOF\n");
			doc_open = false;
			out.close();
			}
		}

	// This is an internal hook for emiting page start code.
	private void open_page()
		{
		if(!page_open)
			{
			open_doc();
			page++;

			out.print("%%Page: " + page + " " + page + "\n");
			if(page_orientation == 1)
				out.print("%%PageOrientation: Landscape\n");
			out.print("%%BeginPageSetup\n");
			out.print("save\n");
			if(inputslot != default_inputslot)
				out.print("%%IncludeFeature: *InputSlot " + inputslot + "\n");
			if(manualfeed != default_manualfeed)
				out.print("%%IncludeFeature: *ManualFeed " + (manualfeed ? "True" : "False") + "\n");
			if(phys_name != default_phys_name)
				out.print("%%IncludeFeature: *PageSize " + phys_name + "\n");
			if(phys_height != default_phys_height)
				out.print("/ph " + phys_height + " 100 div def\n");
			if(phys_width != default_phys_width)
				out.print("/pw " + phys_width  + " 100 div def\n");
			if(page_orientation != default_page_orientation)
				out.print("/orient " + page_orientation + " def\n");
			out.print("bp\n");
			out.print("%%EndPageSetup\n");

			page_open = true;
			ps_xpos = ps_ypos = 0;
			ps_font_name = "";
			ps_font_size = 0;
			}
		}

	// This is called to eject a page.
	public void close_page()
		{
		if(page_open)
			{
			close_string();
			out.print("\nep\n\n");
			page_open = false;
			}
		page_reset();
		}

	// This is an internal hook for emiting printable string start code.
	private void open_string()
		{
		if(!string_open)
			{
			open_page();
			out.print("(");
			string_open = true;
			}
		}

	// This is an internal hook that is called to finish a printable
	// string so that motion commands or the like can be emitted.
	private void close_string()
		{
		if(string_open)
			{
			out.print(")s ");
			string_open = false;
			}
		}

	// This function adds a printable character to the output.
	public void printable(int c)
		{
		attain_position();
		attain_font();
		open_string();
		switch(c)
			{
			case '(':
			case ')':
			case '\\':
				out.print("\\");
				break;
			}
		out.print((char)c);
		int width = get_char_width(c);
		xpos += width;
		ps_xpos += width;
		}

	} // class PSout

// end of PSout.java

