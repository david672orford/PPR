/*
** mouse:~ppr/src/filter_pcl/PSfont.java
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

/*
** This class selects PostScript fonts and returns information about them.
*/
class PSfont
	{
	public PSfont()
		{
		family = "courier";
		bold = false;
		italic = false;
		size = 1200;
		postscript_name = "";
		}

	private String family;
	private boolean italic;
	private boolean bold;
	private int size;					// hundredths of a PostScript point
	private String postscript_name;		// "" when we don't know it yet

	public void set_family(String new_family)
		{
		family = new_family;
		postscript_name = "";
		}

	public void set_bold(boolean new_bold)
		{
		bold = new_bold;
		postscript_name = "";
		}

	public void set_italic(boolean new_italic)
		{
		italic = new_italic;
		postscript_name = "";
		}

	public void set_size(int new_size)
		{
		size = new_size;
		}

	public int get_size()
		{
		return size;
		}

	public String get_postscript_name()
		{
		if(postscript_name == "")
			{
			postscript_name = "Courier";
			if(bold)
				postscript_name += "-Bold";
			if(italic)
				{
				if(!bold)
					postscript_name += "-";
				postscript_name += "Oblique";
				}
			}
		return postscript_name;
		}

	}

// end of file
