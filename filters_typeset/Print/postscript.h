/*
** postscript.h
** Copyright 1997, Trinity College Computing Center.
** Written by David Chappell.
**
** Last modified 13 October 1997.
*/

class PostScript
	{
	Font **fonts;					// array of font data structures

	const char *DSCTitle;			// Text for "%%Title:"

	ISP hoffset;					// top left corner
	ISP voffset;					// of printable area

	int xpos;						// horizontal position from left
	int ypos;						// vertical position from top

	int page;						// current page number, starts at 1
	int in_string;					// are we in the middle of a PS string?
	int postscript_xpos;			// current position set in PostScript
	int postscript_ypos;			// y coordinate of same
	int postscript_current_font;
	ISP postscript_current_size;

	void send_header(void) const;
	void close_string(void);
	void achieve_position(void);
	void achieve_font(int fontid, ISP size);
	void shipout_char(CBox *cbox);
	void shipout_rule(Rule *rule);
	void shipout_hbox(SuperBox *hbox);
	void shipout_vbox(SuperBox *vbox);

	public:

	PostScript(void);				// constructor
	void SetFontList(Font **fonts);
	void Set_hoffset(ISP new_hoffset) { hoffset = new_hoffset; }
	void Set_voffset(ISP new_voffset) { voffset = new_voffset; }
	ISP Get_hoffset(void) const { return hoffset; }
	ISP Get_voffset(void) const { return voffset; }
	void SetDSCTitle(const char *title) { DSCTitle = title; }
	void ShipOut(Object *obj);		// send object to output
	void End(void);					// finish incomplete page and document
	} ;

// end of file
