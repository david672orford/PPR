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

