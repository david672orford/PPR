/*
** mouse:~ppr/src/ppr/ppr_infile.c
** Copyright 1995--2006, Trinity College Computing Center.
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
** Last modified 7 April 2006.
*/

/*
** This module is for input file routines.  It examines the input file,
** converts it to PostScript if necessary, and sets up buffering routines.
**
** If you wish to add a new input file type with filter, this
** is where you do it.
*/

#include "config.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "ppr.h"
#include "ppr_conffile.h"
#include "ppr_infile.h"
#include "ppr_exits.h"
#include "respond.h"
#include "ppr_gab.h"

/* We use this so we don't have to load the math library. */
#define FABS(x) ((x) >= 0 ? (x) : -(x))

/* We will set these if we create a *-infile or *-barbar.
   We need to know this so that the file(s) can be deleted
   by calling keepinfile_file_cleanup() if this program
   exits with a fatal error.
   */
static gu_boolean keepinfile_file_created = FALSE;
static gu_boolean barbar_file_created = FALSE;

#define MAX_LAUNCHED_FILTERS 10
static struct {
	pid_t pid;
	char *name;
	} launched_filters[MAX_LAUNCHED_FILTERS];
static int launched_filters_count = 0;

/*
** structure used to analyze the input
*/
struct ANALYZE {
	int cr;						/* count of carriage returns */
	int lf;						/* count of line feeds */
	int ff;						/* count of form feeds */
	int esc;					/* count of escape characters */
	int hp;						/* count of HP PCL escapes */
	int epson;					/* count of epson escapes */
	int simple_epson;			/* count of single character epson controls */
	int left_brace;				/* count of "{"'s */
	int right_brace;			/* count of "}"'s */
	int ps_def;					/* count of PostScript definitions */
	int ps_quoted;				/* approximate number of characters in PostScript strings */
	int left_parenthesis;		/* count of PostScript left quotes */
	int right_parenthesis;		/* count of PostScript right quotes */
	int non_ascii;				/* count of characters 128-255 */
	int troff_dot_commands;		/* count of dot commands */
	int tex_commands;			/* count of TeX/LaTeX commands */
	int NULLs;					/* count of zero-value bytes */
	} ;

/*
** Input file types.  Add to this list if you want to add a new input file
** type with filter.  You must also add it to the table below.  If you want
** auto-detect to work for your type, you must add code to analyze_input().
*/

/* Input type unknown, must do auto-detect: */
#define IN_TYPE_AUTODETECT -1

/* PostScript */
#define IN_TYPE_POSTSCRIPT 0			/* ordinary clean PostScript */
#define IN_TYPE_POSTSCRIPT_D 1			/* ^D, then PostScript */
#define IN_TYPE_POSTSCRIPT_PJL 2		/* PS with PJL wrapper */
#define IN_TYPE_POSTSCRIPT_TBCP 3		/* tagged binary communications protocol PostScript */
#define IN_TYPE_BADPS 4					/* tail of severed PostScript */

/* Basically plain text */
#define IN_TYPE_LP 100					/* line printer format */
#define IN_TYPE_LP_AUTOLF 101			/* line printer, CR line endings */
#define IN_TYPE_PR 102					/* plain file, pass thru pr */
#define IN_TYPE_FORTRAN 103				/* Fortran carriage control */
#define IN_TYPE_EMAIL 104				/* RFC-822 Email messages */
#define IN_TYPE_TEXT 105				/* text/plain */

/* Metafile formats */
#define IN_TYPE_CAT4 200				/* CAT/4 phototypesetter */
#define IN_TYPE_DITROFF 201				/* ditroff output */
#define IN_TYPE_DVI 202					/* TeX dvi format */
#define IN_TYPE_WMF 203					/* MS-Windows Metafile */
#define IN_TYPE_EMF 204					/* MS-Windows Enhanced Metafile */
#define IN_TYPE_MGC 205					/* Mentor Graphics Plot Metafile */

/* Printer languages */
#define IN_TYPE_PCL 300					/* HP PCL format */
#define IN_TYPE_DOTMATRIX 301			/* Generic dot matrix printer */
#define IN_TYPE_HPGL2 302				/* HP-GL/2 */
#define IN_TYPE_PCLXL 303				/* HP PCL-XL format */
#define IN_TYPE_PCL3GUI 304				/* HP PCL3GUI format */

/* Document languages */
#define IN_TYPE_TROFF 500				/* TRoff source */
#define IN_TYPE_TEX 501					/* TeX/LaTeX source */
#define IN_TYPE_WP 502					/* WordPerfect document */
#define IN_TYPE_RTF 503					/* Rich Text Format */
#define IN_TYPE_TEXINFO 504				/* GNU TeXInfo format */
#define IN_TYPE_HTML 505				/* Hyper Text Markup Language */
#define IN_TYPE_SGML 506				/* Other SGML */
#define IN_TYPE_PDF 507					/* Adobe Portable Document Format */
#define IN_TYPE_ENRICHED 508			/* text/enriched */

/* Graphics formats */
#define IN_TYPE_JPEG 600				/* JPEG compressed picture */
#define IN_TYPE_GIF 601					/* GIF compressed picture */
#define IN_TYPE_SUNRAS 602				/* Sun Raster format */
#define IN_TYPE_PLOT 603				/* Berkeley plot library output */
#define IN_TYPE_CIF 604					/* CIF format */
#define IN_TYPE_TIFF 605				/* TIFF format */
#define IN_TYPE_BMP 606					/* MS-Windows & OS/2 format */
#define IN_TYPE_XBM 607					/* X-Windows bitmap */
#define IN_TYPE_XPM 608					/* X-Windows pixel map */
#define IN_TYPE_XWD 609					/* X-Windows window dump */
#define IN_TYPE_PNM 610					/* Portable aNy Map */
#define IN_TYPE_PNG 611					/* Portable Network Graphics format */
#define IN_TYPE_FIG 612					/* FIG format */

#define IN_TYPE_UNRECOGNIZED 1000		/* unidentifiable file type */
#define IN_TYPE_BINARY 1001				/* unidentified binary file */

/*
** Structure of an entry in the table of filters for
** converting files to PostScript.
*/
struct FILTER
	{
	int in_type;				/* IN_TYPE_* */
	const char *mime;			/* MIME type */
	gu_boolean markup;			/* Is it a markup language? */
	const char *name;			/* keyword for -T switch */
	const char *help;			/* help description */
	const char *excuse;			/* what we can't print */
	} ;

/*
** The table of filters.  The first entry, "postscript", is not really
** a filter at all, there is special case code in get_input_file()
** that takes care of it.
**
** New input types must be added here.  It is also best to add auto-detect
** code to analyze_input().
*/
struct FILTER filters[]=
{
/* IN_TYPE_*			MIME type						Markup? -T name					-T description											can't print message */

/* PostScript types: */
{IN_TYPE_POSTSCRIPT,	"application/postscript",		FALSE,	"postscript",			N_("input file is PostScript"),							""},
{IN_TYPE_POSTSCRIPT_D,	NULL,							FALSE,	"postscript-d-ps",		NULL,													""},
{IN_TYPE_POSTSCRIPT_PJL,NULL,							FALSE,	"postscript-pjl",		NULL,													""},
{IN_TYPE_POSTSCRIPT_TBCP,NULL,							FALSE,	"postscript-tbcp",		NULL,													""},

/* Basicaly plain text types: */
{IN_TYPE_LP,			NULL,							FALSE,	"lp",					N_("pass file thru line printer emulator"),				N_("line printer files")},
{IN_TYPE_LP_AUTOLF,		NULL,							FALSE,	"lp_autolf",			N_("file type is Macintosh text"),						N_("Macintosh text")},
{IN_TYPE_TEXT,			"text/plain",					FALSE,	"text",					N_("file is plain text"),								N_("Plain text")},
{IN_TYPE_PR,			NULL,							FALSE,	"pr",					N_("pass file thru pr"),								N_("using pr")},
{IN_TYPE_FORTRAN,		NULL,							FALSE,	"fortran",				N_("file employs Fortran carriage control"),			N_("Fortran files")},
{IN_TYPE_EMAIL,			NULL,							FALSE,	"email",				N_("file is an RFC-822 email message"),					N_("email messages")},

/* Metafile types: */
{IN_TYPE_CAT4,			NULL,							FALSE,	"cat4",					N_("file is old troff output"),							N_("CAT/4 (old Troff) files")},
{IN_TYPE_DITROFF,		NULL,							FALSE,	"ditroff",				N_("file type is Ditroff (modern Troff) output"),		N_("Ditroff output")},
{IN_TYPE_DVI,			NULL,							FALSE,	"dvi",					N_("file type is TeX DVI"),								N_("TeX DVI files")},
{IN_TYPE_WMF,			NULL,							FALSE,	"wmf",					N_("file is an MS-Windows Metafile"),					N_("MS-Windows Metafiles")},
{IN_TYPE_EMF,			NULL,							FALSE,	"emf",					N_("file is an MS-Windows Enhanced Metafile"),			N_("MS-Windows Enhanced Metafiles")},
{IN_TYPE_MGC,			NULL,							FALSE,	"mgc",					N_("file is a Mentor Graphics Plot Metafile"),			N_("Mentor Graphics Plot Metafiles")},

/* Printer languages: */
{IN_TYPE_PCL,			NULL,							FALSE,	"pcl",					N_("file type is HP PCL"),								N_("HP PCL files")},
{IN_TYPE_HPGL2,			NULL,							FALSE,	"hpgl2",				N_("file type is HP-GL/2"),								N_("HP-GL/2 files")},
{IN_TYPE_DOTMATRIX,		NULL,							FALSE,	"dotmatrix",			N_("file type is generic dot matrix"),					N_("dot matrix printer files")},
{IN_TYPE_PCLXL,			NULL,							FALSE,	"pclxl",				N_("file type is HP PCL-XL"),							N_("HP PCL-XL files")},
	/* HP DeskJet 882C (and possibly others) won't recognize PCL3GUI unless it is in upper case! */
{IN_TYPE_PCL3GUI,		NULL,							FALSE,	"PCL3GUI",				N_("file type is HP PCL3GUI"),							N_("HP PCL3GUI files")},

/* Document languages: */
{IN_TYPE_TROFF,			NULL,							TRUE,	"troff",				N_("file is Troff source"),								N_("Troff source files")},
{IN_TYPE_TEX,			NULL,							TRUE,	"tex",					N_("file is TeX or LaTeX source"),						N_("TeX and LaTeX source")},
{IN_TYPE_WP,			NULL,							FALSE,	"wp",					N_("file is a WordPerfect document"),					N_("WordPerfect documents")},
{IN_TYPE_RTF,			"application/rtf",				TRUE,	"rtf",					N_("file is in Rich Text Format"),						N_("Rich Text Format documents")},
{IN_TYPE_TEXINFO,		NULL,							TRUE,	"texinfo",				N_("file is in TeXInfo format"),						N_("TeXInfo documents")},
{IN_TYPE_HTML,			"text/html",					TRUE,	"html",					N_("file is in HTML format"),							N_("HTML documents")},
{IN_TYPE_SGML,			NULL,							TRUE,	"sgml",					N_("file is in SGML format (other than HTML)"),			N_("SGML documents")},
{IN_TYPE_PDF,			NULL,							FALSE,	"pdf",					N_("file is in Adobe Portable Document Format"),		N_("the Adobe Portable Document Format")},

/* Graphics formats: */
{IN_TYPE_JPEG,			"image/jpeg",					FALSE,	"jpeg",					N_("file is a JPEG compressed picture"),				N_("JPEG pictures")},
{IN_TYPE_GIF,			"image/gif",					FALSE,	"gif",					N_("file is a GIF compressed picture"),					N_("GIF pictures")},
{IN_TYPE_SUNRAS,		NULL,							FALSE,	"sunras",				N_("file is in Sun raster format"),						N_("Sun raster files")},
{IN_TYPE_PLOT,			NULL,							FALSE,	"plot",					N_("file is in Berkeley plot library format"),			N_("Berkeley Plot files")},
{IN_TYPE_CIF,			NULL,							FALSE,	"cif",					N_("file is in CIF graphics language"),					N_("CIF files")},
{IN_TYPE_TIFF,			"image/tiff",					FALSE,	"tiff",					N_("file is in TIFF graphics format"),					N_("TIFF pictures")},
{IN_TYPE_BMP,			NULL,							FALSE,	"bmp",					N_("file is an MS-Windows or OS/2 BMP"),				N_("MS-Windows Bitmaps")},
{IN_TYPE_XBM,			"image/xbm",					FALSE,	"xbm",					N_("file is an X-Windows bit map"),						N_("X-Windows bitmaps")},
{IN_TYPE_XPM,			"image/xpm",					FALSE,	"xpm",					N_("file is an X-Windows pixel map"),					N_("X-Windows pixel maps")},
{IN_TYPE_XWD,			NULL,							FALSE,	"xwd",					N_("file is an X-Windows window dump"),					N_("X-Windows window dumps")},
{IN_TYPE_PNM,			NULL,							FALSE,	"pnm",					N_("file is a Portable aNy Map"),						N_("Portable aNy Maps")},
{IN_TYPE_PNG,			"image/png",					FALSE,	"png",					N_("file is in Portable Network Graphics format"),		N_("Portable Network Graphics files")},
{IN_TYPE_FIG,			NULL,							FALSE,	"fig",					N_("file is in FIG format"),							N_("FIG files")},

/* Wierd ones unprintable types: */
{IN_TYPE_BADPS,			NULL,							FALSE,	"internal-bad-ps",		NULL,													N_("fragmentary PostScript files")},
{IN_TYPE_UNRECOGNIZED,	NULL,							FALSE,	"internal-unrecognized",		NULL,													N_("files of unrecognized format")},
{IN_TYPE_BINARY,		NULL,							FALSE,	"internal-binary",		NULL,													N_("unrecognized binary files")}
} ;

static const int filters_count = ( sizeof(filters) / sizeof(filters[0]) );

/* Type of input current file: */
static int in_type = IN_TYPE_AUTODETECT;

/*
** Find a file type by name.
*/
static const struct FILTER *filter_by_name(const char name[])
	{
	int x;
	for(x=0; x < filters_count; x++)
		{
		if(filters[x].name && gu_strcasecmp(name, filters[x].name) == 0)
			return &filters[x];
		}
	return NULL;
	}

/*
** Find a file type by MIME name.
*/
static const struct FILTER *filter_by_mime(const char mime[])
	{
	int x;
	for(x=0; x < filters_count; x++)
		{
		if(filters[x].mime && gu_strcasecmp(mime, filters[x].mime) == 0)
			return &filters[x];
		}
	return NULL;
	}

/*
** Find a file type by internal code number.
*/
static const struct FILTER *filter_by_code(int code)
	{
	int x;
	for(x=0; x < filters_count; x++)
		{
		if(filters[x].in_type == code)
			return &filters[x];
		}
	return NULL;
	}

/*
** Add to a space separated list.
*/
static char *append_to_list(char *oldpart, const char *newpart)
	{
	if(!oldpart)
		{
		oldpart = gu_strdup(newpart);
		}
	else
		{
		int len = strlen(oldpart) + strlen(newpart) + 2;
		oldpart = (char*)gu_realloc(oldpart, len, sizeof(char));
		strcat(oldpart, " ");
		strcat(oldpart, newpart);
		}

	return oldpart;
	}

/*=====================================================================
** Input buffering code
=====================================================================*/

/* This static will be used until I can find a better way. */
static gu_boolean input_is_file;

/* The input file buffer */
static int in_handle = -1;						/* input file handle */
static gu_boolean in_lastbuf = TRUE;					/* TRUE if physical end of file yet */
static gu_boolean logical_eof = TRUE;					/* inited to TRUE so we must call in_reset_buffering() */
static unsigned char *in_buffer_rock_bottom = NULL;		/* extra space used for in_ungetc() */
static unsigned char *in_buffer;						/* normal start of input buffer */
static int in_bsize;							/* size in bytes of normal part of input buffer */
static unsigned char *in_ptr;					/* pointer to next character in buffer */
static int in_left = 0;							/* characters remaining in in buffer */

/* These variables are used by in_getline() to handle binary
   tokens correctly.  They are reset by in_reset_buffering(). */
static int bintoken_grace;						/* bytes left in next PostScript binary token */
static int string_level;						/* parenthesis nesting level */
static gu_boolean in_ascii85;							/* Are we in <~ ... ~> ? */
static gu_boolean in_comment;							/* Are we between % and EOL? */

/*
** Reset the buffering code.  We do this after pushing
** a filter onto the input stream.
*/
static void in_reset_buffering(void)
	{
	in_lastbuf = FALSE;
	logical_eof = FALSE;
	bintoken_grace = 0;					/* in case we were in the middle of a binary token */
	qentry.attr.postscript_bytes = 0;	/* If we read anything before, it wasn't PostScript. */
	string_level = 0;
	in_ascii85 = FALSE;
	in_comment = FALSE;
	}

/*
** Load the next block into the input file buffer.
**
** The bytes are read into in_buffer.  Up to in_bsize bytes are
** read.  The in_ptr is reset to the start of the buffer.
**
** The number of bytes read is added to the count of PostScript
** input bytes.  If it later turns out that the stuff we were
** reading wasn't PostScript we will reset this variable to zero.
*/
static void in_load_buffer(void)
	{
	/* Under DEC OSF/1 3.2 we get mysterious interuptions of
	   system calls.  That is why we have to be fancy here. */
	while((in_left = read(in_handle, in_buffer, in_bsize)) < 0)
		{
		if(errno != EINTR)
			fatal(PPREXIT_OTHERERR, "read() failed on input file, errno=%d (%s)", errno, gu_strerror(errno));
		}

	if(in_left == 0)					/* If didn't get any bytes, */
		in_lastbuf = TRUE;				/* it was end of file. */

	in_ptr = in_buffer;

	qentry.attr.postscript_bytes += in_left;
	} /* end of in_load_buffer() */

/*
** Make sure no characters can be read from the buffering system
** in any way.
*/
static void in_scotch(void)
	{
	logical_eof = TRUE;
	in_left = 0;
	in_lastbuf = TRUE;
	} /* end of in_scotch() */

/*
** Return TRUE if the end of file has been encountered.
** This will be set the the call to in_getline() _after_
** the one on which it returns the last line.
*/
gu_boolean in_eof(void)
	{
	return logical_eof;
	}

/*
** Get a single character from the input file.
** Return EOF if nothing is left.
**
** Notice that this function can return characters
** even after in_eof() returns TRUE.
*/
int in_getc(void)
	{
	while(in_left == 0)			/* we use while and not if because */
		{						/* the file size may be an exact */
		if(in_lastbuf)			/* multiple of the buffer size */
			return EOF;
		in_load_buffer();
		}

	in_left--;					/* there will be one less */
	return *(in_ptr++);			/* return the character, INCing pointer */
	} /* end of in_getc() */

/*
** "unget" an input character.
**
** If we think that the caller is attempting to
** put back an end of file indicator, just ignore it.
**
** This routine guaratees that we can put back at
** least 11 bytes.
*/
void in_ungetc(int c)
	{
	const char function[] = "in_ungetc";
	if(c != EOF)
		{
		if(in_ptr > in_buffer_rock_bottom)
			{
			in_ptr--;
			in_left++;
			*in_ptr = c;
			logical_eof = FALSE;
			}
		else
			{
			fatal(PPREXIT_OTHERERR, "%s(): in_ungetc(): too far!", function);
			}
		}
	} /* end of in_ungetc() */

/*
** Read a line from the input file, into line[].
**
** In doing so, the line ending will be removed.
** The length of the line (exclusive of the line
** ending) is placed in line_len.
**
** This function is called from outside this module
** only by getline_simplify() and copy_data().
**
** This function shows special consideration for binary
** token encoding (see RBII pp. 106-119).  It is not
** guaranteed to respect binary tokens which follow
** string syntax errors.
**
** This function may set logical_eof.  Once it is set TRUE,
** this function will return zero length lines.
*/
void in_getline(void)
	{
	int c, c2, c3, c4;			/* space for character and 3 bytes look-ahead */
	unsigned int x = 0;			/* length of line retrieved so far */
	int prevc = '\0';			/* no previous character on this line */

	line_overflow = FALSE;		/* we don't yet know it will overflow */

	while(!logical_eof)							/* Don't try to read past */
		{										/* logical end of file. */
		if((c = in_getc()) == EOF)				/* Physical end of file */
			{									/* terminates the line. */
			if(x == 0)							/* If there is no line it is */
				logical_eof = TRUE;				/* logical end of file. */
			break;
			}

		/* Don't execute this block if we are within a binary token
		   because no character code has any special meaning within
		   a binary token. */
		if(!bintoken_grace)
			{
			if(c == '\n')							/* if line feed, */
				{
				if((c = in_getc()) != '\r')			/* Eat a carriage return */
					in_ungetc(c);					/* if it is there */
				in_comment = FALSE;
				break;								/* accept as end of line */
				}
			if(c == '\r')							/* if CR, */
				{
				if((c = in_getc()) != '\n')			/* eat a line feed */
					in_ungetc(c);					/* if it is there */
				in_comment = FALSE;
				break;								/* accept as end of line */
				}

			/* Here we set flags which tell the code below when
			   we are in a comment or a string so that it won't
			   look for binary tokens.  We must also know when
			   we are in an ASCII85 string and not consider "%"
			   to be a comment marker if it is in a string or
			   ASCII85 string.
			   */
			switch(c)
				{
				case '%':
					if(!in_ascii85 && !string_level)
						in_comment = TRUE;
					break;
				case '(':
					if(!in_comment && !in_ascii85)
						string_level++;
					break;
				case ')':
					if(prevc != '\\' && string_level)
						string_level--;
					break;
				case '~':
					if(prevc == '<' && !string_level && !in_comment)
						in_ascii85 = TRUE;
					break;
				case '>':
					if(prevc == '~' && in_ascii85)
						in_ascii85 = FALSE;
					break;
				}
			}

		/* It passed those tests!  Put the character into the buffer. */
		line[x++] = prevc = c;

		/*
		** This block maintains the value of bintoken_grace which
		** tell how many characters remain in the current binary
		** token.  If it is 0 then we are not in a binary token!
		*/
		if(bintoken_grace)
			{
			bintoken_grace--;
			}
		/* Binary tokens cannot begin in comments or strings.  In fact,
		   comments and strings may contain things that look like binary
		   tokens! */
		else if(!in_comment && !string_level)
			{
			switch(c)
				{
				case 128:				/* binary object sequences */
				case 129:				/* (not implemented) */
				case 130:
				case 131:
					break;

				case 132:				/* 32-bit integer, high-order byte first */
				case 133:				/* 32-bit integer, low-order byte first */
				case 138:				/* 32-bit IEEE standard real, high-order byte first */
				case 139:				/* 32-bit IEEE standard real, low-order byte first */
				case 140:				/* 32-bit native real */
					bintoken_grace = 4;
					break;
				case 134:				/* 16-bit integer, high-order byte first */
				case 135:				/* 16-bit integer, low-order byte first */
					bintoken_grace = 2;
					break;
				case 136:				/* 8-bit integer, signed */
				case 141:				/* Boolean */
				case 145:				/* Literal name from the system name table */
				case 146:				/* Executable name from the system name table */
				case 147:				/* Literal name from the user name table */
				case 148:				/* Executable name from the user name table */
					bintoken_grace = 1;
					break;
				case 137:				/* 16 or 32 bit fixed point !!! */
					if((c2 = in_getc()) <= 31 || (c2 >= 128 && c2 <= 159) )
						bintoken_grace = 2;
					else
						bintoken_grace = 5;
					in_ungetc(c2);
					break;
				case 142:				/* String of length c2 */
					c2 = in_getc();
					bintoken_grace = c2 + 1;
					in_ungetc(c2);
					break;
				case 143:				/* String of length c2,c3, high-order byte first */
					c2 = in_getc();
					c3 = in_getc();
					in_ungetc(c3);
					in_ungetc(c2);
					bintoken_grace = (c2 * 256 + c3) + 2;
					break;
				case 144:				/* String of length c2,c3, low-order byte first */
					c2 = in_getc();
					c3 = in_getc();
					in_ungetc(c3);
					in_ungetc(c2);
					bintoken_grace = c3 * 256 + c2;
					break;
				case 149:				/* Homogenous number array */
					c2 = in_getc();		/* <-- type or numbers */
					c3 = in_getc();		/* <-- length byte 1 */
					c4 = in_getc();		/* <-- length byte 2 */
					in_ungetc(c4);
					in_ungetc(c3);
					in_ungetc(c2);
					if( c2 <= 31 || c2 == 48 || c2 == 49 )		/* 32-bit fixed point or real, */
						bintoken_grace = (c3 * 256 + c4) * 4;	/* high-order byte first */
					else if( c2 <= 47 )							/* 16-bit fixed point, */
						bintoken_grace = (c3 * 256 + c4) * 2;	/* high-order byte first */
					else if( c2 < 128 )							/* Undefined */
						bintoken_grace = 0;
					else if( (c2-=128) <= 31 || c2 == 48 || c2 == 49 )	/* same, low-order byte first */
						bintoken_grace = (c4 * 256 + c3) * 4;
					else if( c2 <= 47 )
						bintoken_grace = (c4 * 256 + c3) * 2;
					else
						bintoken_grace = 0;
					break;
				}
			}

		/*
		** Handle line overflows.  Normally, we just note the
		** fact, if we set line_overflow to TRUE, the line will
		** be re-assembled properly.  If it is a hexadecimal
		** data line we just split it up.
		*/
		if(x >= MAX_LINE)
			{
			line[x] = '\0';				/* terminate what we have, */
			line_overflow = TRUE;
			line_len = x;
			return;
			}

		} /* while( !logical_eof ) */

	line[x] = '\0';						/* Terminate line, */
	line_len = x;						/* and pass length back in global variable. */

	/*
	** Accept a lone control-D as EOF, but don't be nice about it.
	**
	** Of course, this might cause problems if we are receiving
	** binary data.  As a quick hack, we won't accept ^D as EOF
	** if TBCP is being used.
	*/
	if(line[0] == 4 && line[1] == '\0' && in_type != IN_TYPE_POSTSCRIPT_TBCP)
		{
		warning(WARNING_PEEVE, _("^D as EOF is spurious in a spooled environment"));
		line[0] = '\0';
		line_len = 0;
		logical_eof = TRUE;
		}

	/*
	** If this file had HP Printer Job Language commands at
	** the begining, then accept control-D followed by
	** "Universal Exit Language" as EOF.
	**
	** (Actually, no control-D will proceed UEL when in_type is
	** IN_TYPE_POSTSCRIPT_TBCP because tbcp2bin will have deleted
	** it if it was present.)
	*/
	if( (in_type == IN_TYPE_POSTSCRIPT_PJL || in_type == IN_TYPE_POSTSCRIPT_TBCP)
			&& ( ((line[0]==27) && (strncmp(line,"\x1b%-12345X",9)==0))
				|| ((line[0]==4) && (strncmp(line,"\x04\x1b%-12345X",10)==0)) ) )
		{
		line[0] = '\0';
		line_len = 0;
		logical_eof = TRUE;
		}

	} /* end of in_getline() */

/*=====================================================================
** Code to read informational headers.
=====================================================================*/

/*
** If a PJL header is found, strip it off after setting the language
** type to that which corresponds to the "ENTER LANGUAGE" command.
**
** If the "ENTER LANGUAGE" command specifies either an unknown
** language or one for which a filter is required, but none is
** available, the job will rejected.  If the langauge is unknown,
** it will sort of happen in this function, if the filter is missing,
** the problem will not be discoved until we try to execute it.  That
** happens well after this function is done.
*/
static void check_for_PJL(void)
	{
	int consume;

	/* The MS-Windows 98 driver for the HP Deskjet 882C sends 600 zero bytes at 
	 * the start of the job.  Could this be a hack to clear out a buffer?
	 */
	for(consume = 0; consume < in_left && in_ptr[consume] == 0; consume++)
		{ /* do nothing */ }

	/* The driver mentioned above also sends this reset sequence. */
	if((in_left - consume) >= 2 && strncmp((char*)in_ptr + consume, "\x1b""E", 2) == 0)
		consume += 2;

	/* This is make-it-or-break-it.  If we see UEL, ok. */
	if((in_left - consume) >= 9 && strncmp((char*)in_ptr + consume, "\x1b%-12345X", 9) == 0)
		consume += 9;
	/* Otherwise, no PJL here, bail out. */
	else
		return;

	in_ptr += consume;
	in_left -= consume;

	{
	char *si, *di;
	int space_credit;
	char *pjl_buffer = NULL;
	int pjl_size = 0;

	while(!in_eof())
		{
		in_getline();							/* Read a PJL line */

		if(gu_strncasecmp(line, "@PJL", 4))		/* if not PJL line */
			return;								/* then go back to auto-detect */

		/* Convert all whitespace sequences to
		   a single space and remove trailing
		   whitespace. */
		for(di=si=line, space_credit=0; *si; )
			{
			switch(*si)
				{
				case ' ':
				case '\t':
				case '\r':
				case '\n':
					space_credit = 1;
					si++;
					break;
				case '=':
				case ':':
					*di++ = *si++;
					while(isspace(*si))
						si++;
					space_credit = 0;
					break;

				default:
					if(space_credit)
						{
						*di++ = ' ';
						space_credit = 0;
						}
					*di++ = *si++;
					break;
				}
			}
		*di = '\0';

		/* If language requested, use as input type
		   and consider the request to be the end of
		   the PJL header. */
		if(gu_strncasecmp(line, "@PJL ENTER LANGUAGE=", 20) == 0)
			{
			char *ptr = line + 20;

			if(option_gab_mask & GAB_INFILE_AUTOTYPE)
				printf("@PJL ENTER LANGUAGE = %s\n", ptr);

			/* Accept the specified input type, ignore any error return. */
			if(infile_force_type(ptr) == -1)
				warning(WARNING_SEVERE, _("Language \"%s\" in PJL header not recognized"), ptr);

			/* If PJL code selected PostScript, */
			if(in_type == IN_TYPE_POSTSCRIPT)
				{
				int c;

				/* Set to special kind of PS */
				in_type = IN_TYPE_POSTSCRIPT_PJL;

				/*
				** Eat up control-d which some drivers insert
				** and blank line inserted by Adobe MS-Windows driver 4.
				*/
				while( (c=in_getc()) == 4 || c == '\r' || c == '\n' ) ;
				in_ungetc(c);
				}

			break;
			}

		/*
		** Here we weed out PJL things that PPR considers its own turf.
		** This includes commands to turn on unsolicitied status reports.
		** These are removed because pprdrv generates these commands
		** itself and doesn't want commands from the job interfering.
		**
		** The list of things weeded out should probably be expanded.
		*/
		if(gu_strncasecmp(line, "@PJL USTATUS", 12) == 0)
			continue;

		/* Grow the buffer to fit this command. */
		pjl_buffer = gu_realloc(pjl_buffer, (pjl_size + strlen(line) + 2), sizeof(char));

		/* Save this PJL command in the queue file. */
		strcpy(pjl_buffer + pjl_size, line);
		pjl_size += strlen(line);
		strcpy(pjl_buffer + pjl_size, "\n");
		pjl_size++;
		} /* this while loop only ends if we hit EOF */

	/* If PJL lines were gathered, save them.  (If none have been, then
	 * pjl_buffer will still be NULL.) */
	qentry.PJL = pjl_buffer;
	}
	} /* end of check_for_PJL() */

/*=====================================================================
** Automatic type detection code
=====================================================================*/

/*
** This routine counts things in the first block
** of the input file.  It is a state machine.
**
** This routine is only called after an attempt to identify the
** file by magic number has failed.  The statistics gathered by
** this routine are used to make educated guesses as to the
** type of the file's contents.
*/
#define INITIAL 0		/* after each carriage return */
#define PSYMBOL 1		/* after "/" in column 1 */
#define ESCAPE	2		/* escape seen */
#define ESCAPE_AND 3	/* second character of HP escape found */
#define MIDLINE 4		/* after 1st character */
#define DOT		5		/* line began with dot */
#define DOT1	6		/* first character of dot command seen */
#define DOT2	7		/* second character of dot command seen */
#define BSLASH	8		/* line begin with backslash */

static void count_things(struct ANALYZE *ccount)
	{
	int x;
	int state = INITIAL;
	int p_quote_level = 0;				/* 1 if we are in PostScript string */

	ccount->cr=							/* zero all the counts */
		ccount->lf=
		ccount->ff=
		ccount->esc=
		ccount->hp=
		ccount->epson=
		ccount->simple_epson=
		ccount->left_brace=
		ccount->right_brace=
		ccount->ps_def=
		ccount->left_parenthesis=
		ccount->right_parenthesis=
		ccount->non_ascii=
		ccount->troff_dot_commands=
		ccount->tex_commands=
		ccount->ps_quoted=
		ccount->NULLs=0;

	for(x=0; x<in_left; x++)		/* scan the whole buffer */
		{
		switch(in_ptr[x])
			{
			case 0:					/* NULL character */
				ccount->NULLs++;
				break;
			case 13:				/* carriage returns */
				ccount->cr++;		/* are counted */
				state=INITIAL;		/* and reset to initial state */
				p_quote_level=0;	/* PostScript strings do not cross line bounderies */
				break;
			case 10:				/* line feeds */
				ccount->lf++;		/* are counted */
				state=INITIAL;		/* and reset the state to the initial */
				p_quote_level=0;	/* PostScript strings do not cross line bounderies */
				break;
			case 12:				/* form feeds */
				ccount->ff++;		/* are counted */
				state=INITIAL;		/* and reset the state to the initial */
				break;
			case 27:				/* escapes */
				ccount->esc++;		/* are counted */
				state=ESCAPE;		/* and set the state to a new level */
				break;
			case '{':					/* left braces */
				ccount->left_brace++;	/* are counted */
				if(state==PSYMBOL)		/* and if PostScript symbol state, */
					ccount->ps_def++;	  /* are counted as a definition */
				state=MIDLINE; /* <-- formerly was else... */
				break;
			case '}':					/* right braces */
				ccount->right_brace++;	/* are counted */
				state=MIDLINE;
				break;
			case '*':					/* 2nd HP escape character */
				if(state==ESCAPE)		/* or maybe epson graphics, */
					ccount->epson++;	/* fall thru */
			case '&':					/* 2nd char of HP escape */
				if(state==ESCAPE)		/* if the state is escape */
					{
					state=ESCAPE_AND;	/* go to next state */
					break;
					}
				goto _default;
			case '@':					/* epson reset */
			case 'K':					/* graphics */
			case 'L':					/* graphics */
			case 'Y':					/* graphics */
			case 'Z':					/* graphics */
			case '^':					/* graphics */
				if(state==ESCAPE)		/* (Epson graphics could otherwise */
					{					/* cause false binary detection.) */
					ccount->epson++;
					state=INITIAL;
					break;
					}
				goto _default;
			case 14:					/* simple Epson codes */
			case 15:
			case 18:
			case 20:
				ccount->simple_epson++;
				state=MIDLINE;
				break;
			case 'l':					/* third character of HP escape */
			case 'p':
				if(state==ESCAPE_AND)
					{
					ccount->hp++;
					state=MIDLINE;
					break;
					}
				goto _default;
			case '/':					/* slash in initial state */
				if(state==INITIAL)		/* may be a PostScript symbol */
					state=PSYMBOL;
				else
					state=MIDLINE;
				break;
			case '(':					/* left parenthesis */
				if(state==ESCAPE)		/* If the state is ESCAPE, it is */
					{					/* second character of HP escape, */
					state=ESCAPE_AND;	/* go to next state. */
					}
				else					/* Otherwise, may be start of */
					{					/* a PostScript string. */
					ccount->left_parenthesis++;
					state=MIDLINE;
					p_quote_level++;
					}
				break;
			case ')':					/* right parenthesis */
				ccount->right_parenthesis++;
				state=MIDLINE;
				if(p_quote_level)		/* PostScript closing quote */
					p_quote_level--;
				break;
			case '.':
				if(state==INITIAL)
					state=DOT;
				break;
			case 0x5C:
				if(state==INITIAL)
					state=BSLASH;
				break;
			_default:
			default:					/* other characters change state */
				if(in_ptr[x] > 127)		/* count out of ASCII range byte codes */
					ccount->non_ascii++;

				if(state==DOT && isalpha(in_ptr[x]))
					{
					state=DOT1;
					break;				/* break prevents change to MIDLINE state */
					}

				if(state==DOT1 && isalpha(in_ptr[x]))
					{
					state=DOT2;
					break;				/* break prevents change to MIDLINE state */
					}

				if( (state==DOT1 || state==DOT2) && isspace(in_ptr[x]) )
					ccount->troff_dot_commands++;

				if(state==BSLASH && isalpha(in_ptr[x])) /* backslash command */
					ccount->tex_commands++;

				if(state!=PSYMBOL)		/* except in PostScript symbol */
					state=MIDLINE;

				if(p_quote_level==1)	/* if in PostScript quotes */
					ccount->ps_quoted++;

				break;
			} /* end of switch */
		} /* end of for loop */
	} /* end of count_things() */

/*
** Determine what type of input we must deal with.
**
** First, we try to identify the file by magic number.  If that
** doesn't work, we call count_things() and try to make an
** educated guess based uppon the quantities of certain characters
** and constructs present.
**
** If we determine that there are some bytes which must be
** skipt before the file is used, then we communicate that
** fact thru skip_size.
*/
static int analyze_input(int *skip_size)
	{
	struct ANALYZE ccount;

	*skip_size = 0;

	/*
	** Anything begining with "%!" is PostScript.
	*/
	if(in_ptr[0] == '%'	 && in_ptr[1] == '!')
		return IN_TYPE_POSTSCRIPT;

	/*
	** Anything begining with control-D is probably PostScript
	** generated by a stupid code generator which does not know
	** it is talking to a spooler.
	*/
	if(*in_ptr == 4)
		{
		*skip_size = 1;
		return IN_TYPE_POSTSCRIPT_D;
		}

	/*
	** Check for TBCP.  Since TBCP is supposed to _only_
	** work when a PostScript interpreter is active, we
	** take the presence of TBCP as an indication that
	** the file is a PostScript file.
	**
	** Note that if PJL commands are used as well, they
	** will appear before the switch to TBCP mode.	(At
	** least if the writers of the generating software
	** followed HP's recomendations.)
	*/
	{
	unsigned char *p = in_ptr;
	int temp_left = in_left;

	while(temp_left && (*p == '\n' || *p == '\n'))
		{
		p++;
		temp_left--;
		}

	if(temp_left >= 2 && p[0] == 1 && p[1] == 'M')
		{
		*skip_size = in_left - temp_left;
		return IN_TYPE_POSTSCRIPT_TBCP;
		}
	}

	/*
	** If it begins with "#!" it is code for some sort of script language
	** such as sh, perl, or tcl.
	*/
	if(in_ptr[0] == '#' && in_ptr[1] == '!')
		return IN_TYPE_LP;

	/*
	** PCL often begins with "\033E\033" (reset and the
	** start of another command) or "\033*&".
	*/
	if(in_left > 3 && strncmp((char*)in_ptr, "\033E\033", 3) == 0)
		return IN_TYPE_PCL;
	if(in_left > 3 && strncmp((char*)in_ptr, "\033*&", 3) == 0)
		return IN_TYPE_PCL;

	/*
	** HP-GL/2
	*/
	if(in_left > 4 && strncmp((char*)in_ptr, "\033%BB", 4) == 0)
		return IN_TYPE_HPGL2;

	/*
	** If the 1st line is the start of a ditroff style output device
	** type line, assume it is ditroff output.
	*/
	if(strncmp((char*)in_ptr, "x T ", 4) == 0)
		return IN_TYPE_DITROFF;

	/*
	** If the 1st two bytes what I think is the signiture for TeX
	** DVI files, assume this is one.  (This relies on the fact
	** that the input buffer is unsigned chars.)
	*/
	if(in_ptr[0] == 0xF7 && in_ptr[1] == 0x02)
		return IN_TYPE_DVI;

	/*
	** MS-Windows placable metafiles have the signiture 0x9AC6CDD7.
	*/
	if(in_ptr[0] == 0xD7 && in_ptr[1] == 0xCD && in_ptr[2] == 0xC6 && in_ptr[3] == 0x9A)
		return IN_TYPE_WMF;

	/*
	** Mentor Graphics Plot Metafiles have a signiture too.
	*/
	if(in_left > 6 && strncmp((char*)in_ptr, "ID81\n", 5) == 0)
		return IN_TYPE_MGC;

	/*
	** Test for GIF files.  The signature is
	** "GIF87a" or "GIF89a".
	*/
	if( in_left > 6 && (strncmp((char*)in_ptr, "GIF87a", 6) == 0 || strncmp((char*)in_ptr, "GIF89a", 6) == 0) )
		return IN_TYPE_GIF;

	/*
	** Test for JFIF (JPEG) files.
	*/
	if(in_left > 10 && in_ptr[0] == 0xFF && in_ptr[1] == 0xD8 
		&& (strncmp((char*)&in_ptr[6], "JFIF", 4) == 0
			|| strncmp((char*)&in_ptr[6], "Exif", 4) == 0
			)
		)
		return IN_TYPE_JPEG;

	/*
	** Test for TIFF files, both big and little endian.
	*/
	if(in_left > 2 && ( (in_ptr[0]=='\115' && in_ptr[1]=='\115')
					  || (in_ptr[0]=='\111' && in_ptr[1]=='\111') ) )
		return IN_TYPE_TIFF;

	/*
	** Test for MS-Windows and OS/2 BMP files.
	*/
	if(in_left > 2 && strncmp((char*)in_ptr, "BM", 2) == 0)
		return IN_TYPE_BMP;

	/*
	** Test for portable bit/gray/pixel maps.
	*/
	if(in_left > 2 && in_ptr[0] == 'P' && in_ptr[1] >= '0' && in_ptr[1] <= '6')
		return IN_TYPE_PNM;

	/*
	** Test for X-Windows bit maps by looking for the XBM signiture
	** Since the signiture is a C comment we must use special quoting to
	** get it thru cpp.
	*/
	if(in_left > 10 && strncmp((char*)in_ptr, "/""* XBM *""/", 9) == 0)
		return IN_TYPE_XBM;

	/*
	** Most X-Windows bit maps do not have a signiture.  Since they are
	** valid C code we must be careful not to misidentify ordinary C code
	** as an X bitmap.
	*/
	if(in_left > 60 && strncmp((char*)in_ptr, "#define", 7) == 0 && isspace(((char*)in_ptr)[7]))
		{
		char copy[200];			/* Space for ASCIIZ copy of header */
		char *p, *p2, *name;
		int namelen;

		/* Make a NULL terminated copy of the supposed XBM header. */
		strncpy(copy, (const char*)(in_ptr + 8), 200);
		copy[199] = '\0';

		/* Locate the bitmap name. */
		name = copy;
		name += strspn(name, " \t");
		if( (p = strstr(name, "_width")) != (char*)NULL && (p2 = strchr(name, '\n')) != (char*)NULL && p < p2 )
			{
			namelen = p - name;

			if(strncmp(name + namelen, "_width", 6) == 0)
				{
				p += strcspn(p, "\n");			/* move to next line */
				p += strspn(p, "\n");

				if(strncmp(p, "#define ", 8) == 0)
					{
					p = p + 8;
					p += strspn(p, " \t");
					if(strncmp(p, name, namelen) == 0 && strncmp(p + namelen, "_height", 7) == 0)
						{
						return IN_TYPE_XBM;
						}
					}
				}
			}
		}

	/*
	** Test for X-Windows pixel maps.  Again, we must use quoting
	** tricks so that the signiture is not recognized as a comment when
	** we compile this.
	*/
	if( in_left > 10 && strncmp((char*)in_ptr,"/""* XPM *""/",9)==0 )
		return IN_TYPE_XPM;

	/*
	** Test for X-Windows window dump.
	*/
	if( in_left > 8 && in_ptr[4]==0 && in_ptr[5]==0
				&& in_ptr[6]==0 && in_ptr[7]==7 )
		return IN_TYPE_XWD;

	/*
	** Portable Network Graphics format.
	** This format has an 8 byte magic number.
	*/
	if( in_left > 8 && memcmp(in_ptr,"\x89\x50\x4E\x47\x0D\x0A\x1A\x0a",8)==0 )
		return IN_TYPE_PNG;

	/*
	** Test for WordPerfect files.
	** At present, we can't distinguish between
	** the different types of WordPerfect files.
	*/
	if(in_left > 4 && in_ptr[0] == 0xFF && strncmp((char*)&in_ptr[1], "WPC", 3) == 0 )
		return IN_TYPE_WP;

	/*
	** Test for TeXInfo files.
	** We identify them by the fact that they begin
	** with an instruction to TeX to load the texinfo macros.
	*/
	if(in_left > 14 && strncmp((char*)in_ptr, "\\input texinfo", 14) == 0)
		return IN_TYPE_TEXINFO;

	/*
	** Test for HTML files which are tagged as such,
	** then test for other types of SGML.
	*/
	{
	char *p = (char*)in_ptr;
	int left = in_left;

	while(left > 0 && isspace(*p))
		{
		p++;
		left--;
		}

	if(left > 6 && gu_strncasecmp(p, "<HTML>", 6) == 0)
		return IN_TYPE_HTML;

	if(left > 30 && gu_strncasecmp(p, "<!DOCTYPE html PUBLIC", 21) == 0)
		return IN_TYPE_HTML;

	if(left > 30 && gu_strncasecmp(p, "<!DOCTYPE ", 10) == 0)
		return IN_TYPE_SGML;
	}

	/*
	** Test for Adobe Portable Document Format.
	*/
	if( in_left > 5 && strncmp((char*)in_ptr, "%PDF-", 5) == 0 )
		return IN_TYPE_PDF;

	/*
	** Test for plot format files.  This test will only work if
	** the plot size is square.  I don't know, this may always
	** be the case.
	*/
	if( in_left > 9 && in_ptr[0]=='s'
		&& in_ptr[1]==(unsigned char)0 && in_ptr[2]==(unsigned char)0
		&& in_ptr[3]==(unsigned char)0 && in_ptr[4]==(unsigned char)0
		&& in_ptr[5]==in_ptr[7] && in_ptr[6]==in_ptr[8] )
		return IN_TYPE_PLOT;

	/*
	** The FIG figure drawing format.  It begins with a line like
	** #FIG 2.1
	*/
	if(in_left > 10 && strncmp((char*)in_ptr, "#FIG ", 5) == 0
				&& strspn((char*)&in_ptr[5], "0123456789.") == strcspn((char*)&in_ptr[5], "\n"))
		return IN_TYPE_FIG;

	/*
	** Things are getting difficult, time to roll out the big guns.
	** Get statistics for the buffer.
	*/
	count_things(&ccount);

	/* Print out debugging information */
	if(option_gab_mask & GAB_INFILE_AUTOTYPE)
		{
		printf("Statistics for %d bytes:\n\t{\n", (int)in_left);
		printf("\tcr = %d\n", ccount.cr);
		printf("\tlf = %d\n", ccount.lf);
		printf("\tff = %d\n", ccount.ff);
		printf("\tesc = %d\n", ccount.esc);
		printf("\tleft_brace = %d\n", ccount.left_brace);
		printf("\tright_brace = %d\n", ccount.right_brace);
		printf("\tps_def = %d\n", ccount.ps_def);
		printf("\tps_quoted = %d\n", ccount.ps_quoted);
		printf("\thp = %d\n", ccount.hp);
		printf("\tepson = %d\n", ccount.epson);
		printf("\tsimple_epson = %d\n", ccount.simple_epson);
		printf("\tleft_parenthesis = %d\n", ccount.left_parenthesis);
		printf("\tright_parenthesis = %d\n", ccount.right_parenthesis);
		printf("\ttroff_dot_commands = %d\n", ccount.troff_dot_commands);
		printf("\ttex_commands = %d\n", ccount.tex_commands);
		printf("\tnon_ascii = %d\n", ccount.non_ascii);
		printf("\tNULLs = %d\n", ccount.NULLs);
		printf("\t}\n");
		}

	/*
	** If there is at least one escape code that is certainly not
	** an Epson escape code, conclude that the file is in PCL.
	*/
	if( (ccount.hp - ccount.epson) >= 1 )
		return IN_TYPE_PCL;

	/*
	** First try at identifying Epson stuff.  This one will work only
	** if the input file contains at least one Epson escape code.
	** Not all files which could benefit from being passed thru
	** the dotmatrix filter will meet this condition since Epson
	** printers support some single character control codes.
	*/
	if(ccount.epson > 0)
		return IN_TYPE_DOTMATRIX;

	/*
	** If we have gotten this far and there is an awful lot
	** of non-ASCII stuff, assume it is binary.  Had the file
	** contained a dot matrix printer graphic, it would have
	** been caught by the clause above.
	**
	** The rule is more than 20% high bit characters with
	** no NULLs.  The bit about the NULLs is so as not to
	** trigger on international text.  few binary data
	** files or executables will contain no NULLs.
	*/
	if(ccount.non_ascii > (in_left/5+1) && ccount.NULLs > 0)
		return IN_TYPE_BINARY;

	/*
	** If there is at least one escape code, identify file as
	** dot matrix printer format.  Or, if at least two
	** (presumably a pair of) single byte dot matrix
	** printer codes.
	*/
	if(ccount.esc > 0 || ccount.simple_epson > 1)
		return IN_TYPE_DOTMATRIX;

	/*
	** Last ditch effort to identify PostScript.
	** If the file is more than 0.1% left braces and more than
	** 0.1% right braces and the braces are less than 10%
	** mismatched and there is at least one object which looks
	** like a PostScript definition, then decide it is PostScript.
	*/
	if( (ccount.left_brace > (in_left/1000))
		&& (ccount.right_brace > (in_left/1000))
		&& ( FABS( (double)(ccount.left_brace-ccount.right_brace)
				/
			(double)(ccount.left_brace+ccount.right_brace) ) < 0.05 )
		&& (ccount.ps_def > 0) )								/* ^ yes, this is 10% ^ */
		return IN_TYPE_POSTSCRIPT;

	/*
	** Test if this is the tail of a fragmented PostScript job.
	** Microsoft WorkGroups for DOS has been known to break
	** a job into fragments.
	**
	** The qualification is if there are lots of parenthesis which
	** are mostly matched and there are few curly brackets.
	** To this we will add that at least 50% of the characters
	** must be enclosed in PostScript strings.
	*/
	if( (ccount.left_parenthesis + ccount.right_parenthesis)
						> (in_left/20)			/* more than 5% */
				&& (ccount.left_brace + ccount.right_brace)
						<= (in_left/1000)		/* less than 0.1% */
				&& FABS( (double)(ccount.left_parenthesis-ccount.right_parenthesis)
								/
					(double)(ccount.left_parenthesis+ccount.right_parenthesis) )
						< 0.05	/* less than 10% mismatch */
				&& ccount.ps_quoted > (in_left/2)
		)						/* at least 50% of characters quoted */
		return IN_TYPE_BADPS;

	/* If it has dot commands, it is Troff. */
	if(ccount.troff_dot_commands > 5)
		return IN_TYPE_TROFF;

	/* If it has backslash commands, it is TeX. */
	if(ccount.tex_commands > 2)
		return IN_TYPE_TEX;

	/*
	** If this has carriage returns but no line feeds and
	** we didn't identify it as PostScript in the step above,
	** assume it is Macintosh style ASCII text.
	*/
	if(ccount.cr > 0 && ccount.lf == 0)
		return IN_TYPE_LP_AUTOLF;

	/*
	** Anything unclaimed so far we will assume
	** to be ASCII text or line printer format.
	*/
	return IN_TYPE_LP;
	} /* end of analyze_input() */

/*=====================================================================
** This section contains routines for attaching filters to the
** input stream.
=====================================================================*/

/*
** Rewind in_handle, even if this means copying it to a
** temporary file.  When this is called the first buffer
** of data is always still in memory.
**
** The variable "skip" specifies the number of bytes at the
** start of the buffer which should be skipt.  If "skip" is
** non-zero, then we _must_ copy it to a file since the
** filters are allowed to assume that the file be fully seekable.
*/
static void stubborn_rewind(void)
	{
	const char *function = "stubborn_rewind";
	static int tmpnum = 1;
	off_t skip;
	struct stat statbuf;

	/* Compute the size of header to skip.  In effect, skip = in_ptr - in_buffer. */
	skip = (qentry.attr.postscript_bytes - in_left);

	/* Get information about the file. */
	if(fstat(in_handle, &statbuf) < 0)
		fatal(PPREXIT_OTHERERR, _("%s(): %s() failed, errno=%d (%s)"), function, "fstat", errno, gu_strerror(errno) );

	/*
	** If input is a file and we don't have to skip a header,
	** then we can simply rewind it.
	*/
	if(input_is_file && skip == 0)
		{
		lseek(in_handle, 0, SEEK_SET);
		qentry.attr.input_bytes = statbuf.st_size;
		}

	/*
	** Either it was not a file or we must skip a header.
	** We must copy the input data to a temporary file.
	*/
	else
		{
		char fname[MAX_PPR_PATH];					/* Name of temporary file. */
		int t_handle;							/* Temporary file handle. */
		int retval;

		/*
		** Set the size (so far) of the unfiltered input file to skip.  We set
		** it to skip instead of 0 because the code below adds the length of
		** the part it copies which does not include the header whose size is
		** indicated by skip.
		*/
		qentry.attr.input_bytes = skip;

		/* Build a temporary file name and open the file. */
		ppr_fnamef(fname, "%s/ppr-%d-%d-XXXXXX", TEMPDIR, (int)getpid(), tmpnum++);
		if((t_handle = mkstemp(fname)) == -1)
			fatal(PPREXIT_OTHERERR, _("%s(): %s(\"%s\") failed, errno=%d (%s)"), function, "mkstemp", fname, errno, gu_strerror(errno) );

		/* Delete the temporary file's name so it will
		   magically disappear when it is closed.  This
		   is important because we might be killed or we
		   might exit with a fatal error such as if the
		   /tmp disk fills up. */
		unlink(fname);

		/* If, by some odd chance we have emptied the buffer,
		   and not started a new one yet, here we reload it. */
		if(in_left == 0)
			in_load_buffer();

		/* Copy the input to the temporary file. */
		while(in_left > 0)
			{
			qentry.attr.input_bytes += in_left;
			if((retval = write(t_handle, in_ptr, in_left)) == -1)
				fatal(PPREXIT_OTHERERR, _("%s(): %s() failed, errno=%d (%s)"), function, "write", errno, gu_strerror(errno) );
			else if(retval != in_left)
				fatal(PPREXIT_OTHERERR, "%s(): disk full while writing temporary file", function);
			in_load_buffer();
			}

		/* Close file we just read from. */
		close(in_handle);

		/* Rewind the temporary file and make it the input file. */
		if(lseek(t_handle, (off_t)0, SEEK_SET) == -1)
			fatal(PPREXIT_OTHERERR, "%s(): can't rewind temporary file, errno=%d (%s)", function, errno, gu_strerror(errno));
		in_handle = t_handle;
		}
	} /* end of stubborn_rewind() */

/*
** Execute a filter on the input file.  Filters may be chained
** together by calling this function repeatedly.
**
** It attaches the current in_handle to the stdin of the filter
** and attach the stdout of the filter to in_handle.
*/
static void exec_filter_argv(const char *filter_path, const char *arg_list[])
	{
	const char function[] = "exec_filter_argv";
	int pipefds[2];								/* pipe from filter */
	pid_t pid;									/* process id of filter */
	struct stat statbuf;

	/* Possibly gab about what we are doing. */
	if(option_gab_mask & GAB_INFILE_FILTER)
		{
		int x;

		printf("Filter command: %s, argv[] = {", filter_path);

		for(x = 0; arg_list[x] != (const char *)NULL; x++)
			printf("%s\"%s\"", x > 0 ? ", " : "", arg_list[x]);

		printf("}\n");
		}

	/*
	** Make sure the filter exists and get mode information so
	** we know if it has the setuid bit or setgid bit set.
	*/
	if(stat(filter_path, &statbuf) < 0)
		fatal(PPREXIT_OTHERERR, _("%s(): stat(\"%s\", ?) failed, errno=%d (%s)"), function, filter_path, errno, gu_strerror(errno));

	/*
	** Rewind file or, if we can't, copy it to a temporary file
	** and open that instead.
	*/
	stubborn_rewind();

	if(launched_filters_count >= MAX_LAUNCHED_FILTERS)
		fatal(PPREXIT_OTHERERR, "%s(): launched_filters[] overflow", function);

	/*
	** Open a pipe which will be used to convey the filter's
	** output back to this process.
	*/
	if(pipe(pipefds))
		fatal(PPREXIT_OTHERERR, "%s(): can't make pipe, errno=%d (%s)", function, errno, gu_strerror(errno) );

	if((pid = fork()) < 0)
		fatal(PPREXIT_OTHERERR, _("%s(): %s() failed, errno=%d (%s)"), function, "fork", errno, gu_strerror(errno) );

	if(pid)								/* parent */
		{
		close(pipefds[1]);				/* we won't use write end */
		close(in_handle);				/* we won't read input file directly */
		in_handle = pipefds[0];			/* henceforth we will read from pipe */

		/* Record information for reapchild() in case the filter dies. */
		launched_filters[launched_filters_count].name = gu_strdup(filter_path);
		launched_filters[launched_filters_count].pid = pid;
		launched_filters_count++;
		}
	else							/* child */
		{
		close(pipefds[0]);			/* don't need read end */
		dup2(in_handle,0);			/* attach input file to stdin */
		close(in_handle);			/* we may now close input file handle */
		dup2(pipefds[1],1);			/* copy write end of pipe to stdout */
		close(pipefds[1]);			/* we don't need origional handle */
									/* stderr goes to parent's */

		/*
		** Run under user's UID and GID.
		**
		** Setting the real IDs would not seem to be necessary, but on Linux it
		** results in the saved IDs being set too, which setting the effective
		** IDs alone does not accomplish.  However Linux does set the saved
		** IDs during execv().  This was verified by examining /proc/self/status.
		** So even though setting both isn't necessary on Linux, it does seem
		** to be stronger medicine and so may get results on some other systems
		** which might not implement saved IDs some sanely.
		*/
		if(setreuid(user_uid, user_uid) == -1)
			{
			fprintf(stderr, _("%s(): %s(%ld, %ld) failed, errno=%d (%s)\n"), function, "setreuid", (long)user_uid, (long)user_uid, errno, gu_strerror(errno));
			exit(241);
			}
		if(setregid(user_gid, user_gid) == -1)
			{
			fprintf(stderr, _("%s(): %s(%ld, %ld) failed, errno=%d (%s)\n"), function, "setregid", (long)user_gid, (long)user_gid, errno, gu_strerror(errno));
			exit(241);
			}

		/* Protect privacy of temporary files. */
		umask(PPR_FILTER_UMASK);

		/* Execute the filter. */
		execv(filter_path, (char *const *)arg_list);

		_exit(242);						/* give reapchild() a hint */
		}

	in_reset_buffering();
	in_load_buffer();					/* We must do load buffer again. */

	input_is_file = FALSE;				/* Even if it was, it is no longer. */
	} /* end of exec_filter_argv() */

/*
** A simple wrapper function for exec_filter_argv().
*/
static void exec_filter(const char *filter_path, ...)
	{
	va_list argp;
	const char *argv[11];
	int x;

	va_start(argp, filter_path);

	for(x=0; x < 10; x++)
		{
		if((argv[x] = va_arg(argp, const char *)) == (const char *)NULL)
			break;
		}

	va_end(argp);
	argv[x] = (char*)NULL;

	exec_filter_argv(filter_path, argv);
	} /* end of exec_filter() */

/*
** This function executes a filter to convert the input file to
** PostScript.  It is a complex front end for exec_filter_argv().
**
** The arguments to this function are the name of the executable file,
** the name to be passed in argv[0], and the name to be passed as the
** job title.  This routine gets the other filter arguments for itself.
**
** This function uses the global option_filter_options which is set
** by the command line parser.
*/
static void exec_tops_filter(const char filter_path[], const char filter_name[], const char title[])
	{
	const char function[] = "exec_tops_filter";
	void *clean_options;

	/* We will assemble the final option string in this Perl Compatible String. */
	clean_options = gu_pcs_new();

	/* Search the -F table and generate the implied pagesize= option for each
	   -F *PageSize or -F *PageRegion.
	   */
	{
	int i;
	const char *name, *value;
	for(i=0; i<features_count; i++)
		{
		name = features[i];
		if((value = lmatchp(name, "*PageSize ")) || (value = lmatchp(name, "*PageRegion ")))
			{
			if(gu_pcs_length(&clean_options) > 0)
				gu_pcs_append_cstr(&clean_options, " ");
			gu_pcs_append_cstr(&clean_options, "pagesize=");
			gu_pcs_append_cstr(&clean_options, value);
			}
		}
	}

	/* If we have any kind of a duplex setting, add a duplex= option. */
	if(current_duplex_enforce)
		{
		const char *value;

		switch(current_duplex)
			{
			case DUPLEX_NONE:
				value = "none";
				break;
			case DUPLEX_DUPLEX_NOTUMBLE:
				value = "notumble";
				break;
			case DUPLEX_DUPLEX_TUMBLE:
				value = "tumble";
				break;
			case DUPLEX_SIMPLEX_TUMBLE:
				value = "simplextumble";
				break;
			default:
				fatal(PPREXIT_OTHERERR, "%s(): assertion failed at %s line %d", function, __FILE__, __LINE__);
				break;
			}

		if(gu_pcs_length(&clean_options) > 0)
			gu_pcs_append_cstr(&clean_options, " ");
		gu_pcs_append_cstr(&clean_options, "duplex=");
		gu_pcs_append_cstr(&clean_options, value);
		}

	#ifdef INTERNATIONAL
	{
	const char *locale, *p;
	if((locale = setlocale(LC_MESSAGES, NULL)))
		{
		/*printf("locale=%s\n", locale);*/
		if((p = strchr(locale, '.')))
			{
			p++;
			/*printf("charset=%s\n", p);*/
			if(gu_pcs_length(&clean_options) > 0)
				gu_pcs_append_cstr(&clean_options, " ");
			gu_pcs_append_cstr(&clean_options, "charset=");
			gu_pcs_append_cstr(&clean_options, p);
			}
		}
	}
	#endif

	/* If the -G switch requires it, describe what we are doing. */
	if(option_gab_mask & GAB_INFILE_FILTER)
		{
		const char *p;
		printf("Implied filter options: \"%s\"\n", (p = gu_pcs_get_cstr(&clean_options)) ? p : "");
		printf("Raw queue default filter options: \"%s\"\n", (p = extract_deffiltopts()) ? p : "");
		printf("Raw explicit filter options: \"%s\"\n", option_filter_options ? option_filter_options : "");
		}

	/*
	** In this block we combine the various option lists while selecting only
	** those options which apply to the present filter.
	*/
	{
	const char *si_list[3];
	int i;
	const char *si;
	const char *filter_basename;
	int filter_basename_len;
	int item_len, key_len, prefix_len;

	/* We will use the pointer to the basename and the length of the basename
	   to test prefixes.
	   */
	filter_basename = &filter_name[7];
	filter_basename_len = strlen(filter_basename);

	/* Get the default options from the printer or group config file. */
	si_list[0] = extract_deffiltopts();

	/* And the options from the command line. */
	si_list[1] = option_filter_options;

	si_list[2] = NULL;

	/*
	** Merge the default filter options with those the user has
	** provided.  Remove filter options which don't apply to this
	** filter.  For those which contain this filter's prefix, chop
	** off the prefix.  Convert keywords to lower case but not
	** their arguments.
	*/
	for(i=0; (si = si_list[i]); i++)
	  {
	  while(*si)
		{
		item_len = strcspn(si," \t");	/* Length of next non-space segment */
		key_len = strcspn(si,"=");		/* length of keyword */
		prefix_len = strcspn(si,"-");	/* Find distance to next hyphen */
		if(prefix_len < key_len)		/* If this item contains a prefix, */
			{
			if(prefix_len == filter_basename_len && gu_strncasecmp(si, filter_basename, filter_basename_len) == 0)
				{						/* If the correct filter prefix, */
				si += prefix_len;		/* skip the prefix, */
				si += 1;				/* skip the hyphen. */
				}
			else						/* If not for us, */
				{						/* eat it up. */
				si += item_len;
				si += strspn(si, " \t");
				continue;
				}
			}

		/* if not first option, add a space */
		if(gu_pcs_length(&clean_options) > 0)
			gu_pcs_append_char(&clean_options, ' ');

		/* Copy the keyword while converting it to lower case. */
		while(*si && *si != '=')
			gu_pcs_append_char(&clean_options, tolower(*si++));

		if(*si && *si == '=')			/* Copy the equals sign. */
			gu_pcs_append_char(&clean_options, *si++);

		/* Copy the (possibly quoted) value. */
		{
		int c, lastc = '\0';
		gu_boolean qmode = FALSE;
		while((c=*si) && (!isspace(c) || qmode))
			{
			if(c == '"' && lastc != '\\')		/* unbackslashed quote */
				qmode = ~qmode;					/* toggles quote mode */
			gu_pcs_append_char(&clean_options, c);
			lastc = c;
			si++;
			}
		}

		/* Eat any spaces which follow. */
		si += strspn(si, " \t");
		}
	  }
	}

	/* If the -G switch requires it, describe what we have accomplished. */
	if(option_gab_mask & GAB_INFILE_FILTER)
		printf("Final filter options: \"%s\"\n", gu_pcs_get_cstr(&clean_options));

	exec_filter(filter_path, filter_name, gu_pcs_get_cstr(&clean_options), qentry.jobname.destname, title, (char*)NULL);

	gu_pcs_free(&clean_options);
	} /* end of exec_tops_filter() */

/*
** This routine is used by compressed() and run_appropriate_filter()
** when the required filter is not available.
**
** If the "-e hexdump" switch was not used try to use the responder to
** tell the user that we are rejecting the job.  If the "-e hexdump"
** switch WAS used or the responder fails, run the file thru the hexdump
** filter, put a message in the job's log file, and try to force a banner
** page.
*/
static void no_filter(const char *file_type_str)
	{
	const char *xlated_file_type_str = gettext(file_type_str);

	/*
	** If we are printing with the option "-H transparent" then
	** filtering the input file wasn't of the utmost importance
	** anyway (since the filter output would only be used to get
	** the page count and other interesting information), so we
	** just call in_scotch() to produce, in effect, a zero length
	** PostScript file.
	*/
	if(qentry.opts.hacks & HACK_TRANSPARENT)
		{
		in_scotch();
		}

	/*
	** I don't like this code.
	**
	** If the -e hexdump switch has not been used, and we are putting
	** the message on stderr or on the responder and a responder is
	** available, then ppr_abort() can handle it.
	*/
	else if(!option_nofilter_hexdump
				&&
				(
					(ppr_respond_by & PPR_RESPOND_BY_STDERR)
					||
					((ppr_respond_by & PPR_RESPOND_BY_RESPONDER)
						&& strcmp(qentry.responder.name, "none") != 0)
				)
			)
		{
		ppr_abort(PPREXIT_NOFILTER, file_type_str);
		}

	/*
	** If we have been explicity asked to use the hexdump filter (such
	** a request is made by using the -e hexdump switch), or the responder
	** didn't work (possibly because the -m none switch was used),
	** then use the hexdump filter to produce a one page hex dump of the
	** first few hundred bytes of the input file.
	*/
	else
		{
		char lfname[MAX_PPR_PATH];		/* log file name */
		FILE *lfile;					/* log file object */

		/* Select the hex dump filter. */
		exec_tops_filter(FILTDIR"/filter_hexdump", "filter_hexdump", xlated_file_type_str);

		/* Try to force banner page option on. */
		qentry.do_banner = BANNER_YESPLEASE;

		/* Try to add a message to the log file. */
		ppr_fnamef(lfname, "%s/%s-%d.0-log", DATADIR, qentry.jobname.destname, qentry.jobname.id);
		if((lfile = fopen(lfname, "a")))
			{
			fprintf(lfile, _("Can't print %s.\n"), xlated_file_type_str);
			fclose(lfile);
			}
		}
	} /* end of no_filter() */

/*
** If the block in the buffer is the first block of a
** compressed or gziped file, execute a filter to uncompress
** it and return TRUE.  This is called from the function
** get_input_file(), below.
*/
static const char *compressed(void)
	{
	if(in_left >= 3)
		{
		/* Check for files compressed with gzip. */
		if(in_ptr[0] == (unsigned char)'\37' && in_ptr[1] == (unsigned char)'\213')
			{
			#ifdef GUNZIP_PATH
			exec_filter(GUNZIP_PATH, "gunzip", "-c", (char*)NULL);
			#else
			no_filter("gzipped files");
			#endif
			return "gunzip";
			}

		/* Check for files compressed with Unix compress. */
		if(in_ptr[0] == (unsigned char)0x1f && in_ptr[1] == (unsigned char)0x9d
						&& in_ptr[2] == (unsigned char)0x90)
			{
			#ifdef GUNZIP_PATH
			exec_filter(GUNZIP_PATH, "gunzip", "-c", (char*)NULL);
			#elif defined(UNCOMPRESS_PATH)
			exec_filter(UNCOMPRESS_PATH, "uncompress", "-c", (char*)NULL);
			#else
			no_filter("compressed files");
			#endif
			return "uncompress";
			}
		/* Check for files compressed with Bzip2. */
		if(in_ptr[0] == (unsigned char)'B' && in_ptr[1] == (unsigned char)'Z'
						&& in_ptr[2] == (unsigned char)'h')
			{
			#ifdef BUNZIP2
			exec_filter(BUNZIP2, "bunzip2", "-c", (char*)NULL);
			#else
			no_filter("bzip2ed files");
			#endif
			return "bunzip2";
			}
		}

	return NULL;
	} /* end of compressed() */

/*
** Determine which filter should be run for files of the type
** indicated by the code number in in_type and use the routine
** exec_tops_filter() to run it.
*/
static void run_appropriate_filter(const struct FILTER *f, const char *infile_name)
	{
	const char *function = "run_appropriate_filter";
	const char *filter_title;
	char fpath[MAX_PPR_PATH];

	/*
	** Settle on a title string for the filter.  Since
	** no DSC comments have been read yet, this will not
	** be from a "%%Title:" comment, but it may have come
	** from a -C (--title) switch.  Otherwise, it is
	** probably the name of the input file.
	*/
	if(qentry.Title)					/* Try Title from command line. */
		filter_title = qentry.Title;
	else if(qentry.lpqFileName)			/* Try file name according to lprsrv. */
		filter_title = qentry.lpqFileName;
	else if(infile_name)				/* Try actuall file name. */
		filter_title = infile_name;
	else								/* Use nothing. */
		filter_title = "";

	/* Construct filter filename.  Note the silly hack for NULL for such
	   things as IN_TYPE_BINARY. */
	ppr_fnamef(fpath, "%s/filter_%s", FILTDIR, f->name ? f->name : "?");

	if(option_gab_mask & GAB_INFILE_FILTER)
		printf("Filter for this file type: %s\n", fpath);

	/*
	** Sometimes we will be instructed to print markup language
	** text such as HTML or Troff using the lp or pr filters
	** rather than formatting it.  This if is true if the input
	** file is not in a markup language or we have been
	** instructed to insist on formatting markup languages.
	** (By formatting markup languages we mean converting them
	** to typeset PostScript.)
	*/
	if(! f->markup || option_markup == MARKUP_FORMAT)
		{
		/*
		** Some filters may not be present.  Check and see
		** if this filter is present.  If it isn't, print
		** the excuse string.
		*/
		if(access(fpath, X_OK) == -1)
			{
			if(option_gab_mask & GAB_INFILE_FILTER)
				printf("Filter doesn't exist.  File will not be printed.\n");
			no_filter(f->excuse);
			}
		/* Filter is present, use it. */
		else
			{
			exec_tops_filter(fpath, fpath, filter_title);
			}
		}

	/*
	** We reach this else if the input file contains marked up text
	** and the ppr --markup option was something other than
	** --markup=format.	 (The option --markup=format means, "format
	** this or die!")
	**
	** Notice that this block of code blindly assumes that the lp and
	** pr filters are always present.
	*/
	else
		{
		const char *unformatted_filter;
		const char *formatted_filter = (const char *)NULL;
		const char *filter;

		switch(option_markup)
			{
			case MARKUP_FALLBACK_LP:
				formatted_filter = fpath;
			case MARKUP_LP:
				unformatted_filter = FILTDIR"/filter_lp";
				break;

			case MARKUP_FALLBACK_PR:
				formatted_filter = fpath;
			case MARKUP_PR:
				unformatted_filter = FILTDIR"/filter_pr";
				break;

			default:
				fatal(PPREXIT_OTHERERR, "%s(): missing case line %d", function, __LINE__);
			}

		if(option_gab_mask & GAB_INFILE_FILTER)
			{
			printf("formatted_filter = \"%s\"\n", formatted_filter ? formatted_filter : "");
			printf("unformatted_filter = \"%s\"\n", unformatted_filter);
			}

		if(formatted_filter == (char *)NULL || access(formatted_filter, X_OK) == -1)
			filter = unformatted_filter;
		else
			filter = formatted_filter;

		if(option_gab_mask & GAB_INFILE_FILTER)
			printf("selected filter: %s\n", filter);

		exec_tops_filter(filter, filter, filter_title);
		} /* markup langauge */

	} /* end of run_appropriate_filter() */

/*=====================================================================
** This code is used by the hack "keepinfile".  It is called after
** in_load_buffer() is called, so it must start by writing the data
** in the input buffer.  This routine is called after read_dot_header()
** but before check_for_PJL().
=====================================================================*/
static void save_infile(void)
	{
	const char *function = "save_infile";
	char fname[MAX_PPR_PATH];
	int out_handle;
	int bytes_written;

	/*
	** Unless someone changes main(), this if will always be
	** true.  We must assign the id now because we will be
	** using it in the ppr_fnamef() below.  The code in main()
	** will see that we have assingned the id and will
	** not do it again.
	*/
	if(qentry.jobname.id == 0)
		get_next_id(&qentry);

	/*
	** The input file will be copied into a file in the jobs directory.
	*/
	ppr_fnamef(fname, DATADIR"/%s-%d.%d-infile", qentry.jobname.destname, qentry.jobname.id, qentry.jobname.subid);

	if((out_handle = open(fname, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) == -1)
		fatal(PPREXIT_OTHERERR, "%s(): can't create \"%s\", errno=%d (%s)", function, fname, errno, gu_strerror(errno));

	keepinfile_file_created = TRUE;

	while(in_left > 0 || (in_left = read(in_handle, in_buffer, in_bsize)) > 0)
		{
		if((bytes_written = write(out_handle, in_ptr, in_left)) < 0)
			fatal(PPREXIT_OTHERERR, "%s(): write() failed, errno=%d (%s)", function, errno, gu_strerror(errno));

		if(bytes_written != in_left)
			fatal(PPREXIT_OTHERERR, "%s(): disk full", function);

		in_left = 0;
		in_ptr = in_buffer;
		}

	if(in_left < 0)
		fatal(PPREXIT_OTHERERR, _("input file read error, errno=%d (%s)"), errno, gu_strerror(errno));

	close(out_handle);

	close(in_handle);
	if((in_handle = open(fname, O_RDONLY)) < 0)
		fatal(PPREXIT_OTHERERR, "%s(): can't re-open \"%s\", errno=%d (%s)", function, fname, errno, gu_strerror(errno));

	in_reset_buffering();
	in_load_buffer();
	input_is_file = TRUE;				/* now it is! */
	} /* end of save_infile() */

/*=====================================================================
** Code for the PassThruPDL (BarBar) feature:
=====================================================================*/

/*
** Return TRUE if the indicated in type is a language
** which should be passed thru using the barbar feature.
**
** If we return TRUE, then the do_passthru() function below
** will be called.
*/
static gu_boolean should_passthru(const char type_name[])
	{
	gu_boolean ret = FALSE;

	/*
	** There is no point to preparing a -barbar file
	** in transparent mode.
	*/
	if(qentry.opts.hacks & HACK_TRANSPARENT)
		return FALSE;

	/* If there is a list of file types to pass
	   directly to the printer,
	*/
	if(extract_passthru())
		{
		char *mycopy, *ptr;
		char *substr;

		ptr = mycopy = gu_strdup(extract_passthru());

		/* Search that list. */
		while((substr = strtok(ptr, " ")))
			{
			ptr = NULL;

			if(gu_strcasecmp(type_name, substr) == 0)	/* if match, */
				{
				ret = TRUE;
				break;
				}
			}

		/* Discard our copy of the list. */
		gu_free(mycopy);
		}

	return ret;
	} /* end of should_passthru() */

/*
** This is called if the input type is one specified with
** the "ppad passthru" or "ppad group passthru" command.
** Usually these types will be PCL or HP-GL/2.
*/
static void do_passthru(const struct FILTER *f)
	{
	const char *function = "do_passthru";
	char fname[MAX_PPR_PATH];
	int out_handle;
	int bytes_written;

	if(qentry.jobname.id == 0)
		get_next_id(&qentry);

	/* The input file will be copied into a "-barbar" file in the
	   jobs directory.  Create the "-barbar" file.  We will use this
	   fname[] value again later. */
	ppr_fnamef(fname, DATADIR"/%s-%d.%d-barbar", qentry.jobname.destname, qentry.jobname.id, qentry.jobname.subid);
	if((out_handle = open(fname, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) < 0)
		fatal(PPREXIT_OTHERERR, "%s(): can't create \"%s\", errno=%d (%s)", function, fname, errno, gu_strerror(errno));
	barbar_file_created = TRUE;

	/* Start the reckoning of the size of the raw input file with the amount that was
	   read as supposed PostScript and already consumed (which is probably PJL). */
	qentry.attr.input_bytes = (qentry.attr.postscript_bytes - in_left);

	/* Copy what is already in the buffer to the -barbar file, then keep
	   reading blocks and copying them too to the -barbar file. */
	while(in_left > 0 || (in_left = read(in_handle, in_buffer, in_bsize)) > 0)
		{
		qentry.attr.input_bytes += in_left;

		if((bytes_written = write(out_handle, in_ptr, in_left)) < 0)
			fatal(PPREXIT_OTHERERR, _("%s(): %s() failed, errno=%d (%s)"), function, "write", errno, gu_strerror(errno));

		if(bytes_written != in_left)
			fatal(PPREXIT_OTHERERR, _("%s(): disk full"), function);

		in_left = 0;
		in_ptr = in_buffer;
		}

	/* Did the last read() fail for some reason other than end of file? */
	if(in_left < 0)
		fatal(PPREXIT_OTHERERR, _("input file read error, errno=%d (%s)"), errno, gu_strerror(errno));

	close(out_handle);
	close(in_handle);
	in_handle = -1;

	/* Re-open the barbar file as the input file.  Here is where we use
	   the name previously constructed in fname[]. */
	if((in_handle = open(fname, O_RDONLY)) < 0)
		fatal(PPREXIT_OTHERERR, "%s(): can't re-open \"%s\", errno=%d (%s)", function, fname, errno, gu_strerror(errno));
	input_is_file = TRUE;				/* now it is! */
	in_reset_buffering();

	/* This is the passthru page description language. */
	qentry.PassThruPDL = f->name;

	/* Construct name of filter to extrace pseudo PostScript. */
	ppr_fnamef(fname, "%s/passthru_%s", FILTDIR, qentry.PassThruPDL);

	/* If the filter exists, use it, otherwise
	   arrange for zero length PostScript. */
	if(access(fname, X_OK) != -1)
		{
		exec_filter(fname, fname, NULL);
		}
	else
		{
		in_scotch();
		}

	} /* end of do_passthru() */

/*=====================================================================
** This function is called from main().  It opens the input file.  If
** the input file is not PostScript, it attaches a filter to it and
** prepares the file reading routines to read from the filter output.
**
** Return non-zero if the input file is of zero length.
=====================================================================*/
int infile_open(const char filename[])
	{
	const char function[] = "infile_open";
	int skip_size = 0;
	const struct FILTER *f;
	char *temp_Filters = NULL;

	/*
	** Allocate a buffer of size IN_BSIZE plus space for in_ungetc()
	** and for '\0' terminating the buffer to make it a valid string.
	*/
	in_bsize = IN_BSIZE;
	in_buffer_rock_bottom = (unsigned char *)gu_alloc(sizeof(unsigned char), in_bsize + IN_UNGETC_SIZE + 1);
	in_buffer = in_buffer_rock_bottom + IN_UNGETC_SIZE;

	/* If no file name, use stdin. */
	if(! filename || strcmp(filename, "-") == 0)
		{
		in_handle = dup(0);		/* in_handle = 0 can cause confusion */

		input_is_file = FALSE;

		/* If we knew call to detect when stdin is a regular file,
		   we could set input_is_file = TRUE here. */
		}

	/* Specific file. */
	else
		{
		become_user();
		if((in_handle = open(filename, O_RDONLY)) < 0)
			fatal(PPREXIT_OTHERERR, _("can't open input file \"%s\", %s"), filename, gu_strerror(errno));
		unbecome_user();
		input_is_file = TRUE;
		}

	/* Load the data buffer's worth of the file into memory: */
	in_reset_buffering();
	in_load_buffer();

	/* If the "keepinfile" or "transparent" hack is being used
	   then it is necessary to keep a copy of the input file.
	   The routine save_infile() copies the contents of the
	   input buffer and anything else it can read from the
	   input file to a queue file ending in "-infile".  It then
	   makes the new file the input file and calls in_load_buffer(). */
	if(qentry.opts.hacks & (HACK_KEEPINFILE | HACK_TRANSPARENT))
		save_infile();

	/* We jump back here after pushing certain filters
	   onto the filter pipeline.  (At present, only if
	   we push uncompress or gunzip onto the pipeline.) */
	again:

	/* If input file is of zero length, there is not
	   much to do, just tell main() so it can abort. */
	if(in_left == 0)
		{
		infile_file_cleanup();
		return -1;
		}

	/* If file is compressed, push a filter onto the
	   the pipeline and try again. */
	{
	const char *p;
	if((p = compressed()))
		{
		temp_Filters = append_to_list(temp_Filters, p);
		goto again;
		}
	}

	/*
	** Look for and process PJL.  This routine will set in_type if the
	** PJL header contains a "enter language =" command.
	*/
	check_for_PJL();

	/*
	** If the user has not manually specified the input file type,
	** use analyze_input() to learn file type.
	*/
	if(in_type == IN_TYPE_AUTODETECT)
		in_type = analyze_input(&skip_size);

	/* Find the filter table entry so we will know the name. */
	if((f = filter_by_code(in_type)) == NULL)
		fatal(PPREXIT_OTHERERR, "%s(): invalid in_type: %d", function, in_type);

	/* Possibly gab about what we have decided. */
	if(option_gab_mask & GAB_INFILE_AUTOTYPE)
		printf("in_type = %d (%s)\n", in_type, f->name);

	/* Discard the number of bytes that analyze_input()
	   told us to discard. */
	while(skip_size--)
		in_getc();


	/* Input file is PostScript (including goary mutiliations), */
	if(lmatch(f->name, "postscript"))
		{
		switch(in_type)
			{
			case IN_TYPE_POSTSCRIPT:			/* PostScript is already ok */
			case IN_TYPE_POSTSCRIPT_PJL:		/* PostScript with stript PJL header */
				break;
			case IN_TYPE_POSTSCRIPT_TBCP:		/* PostScript encoded with TBCP */
				exec_filter(TBCP2BIN_PATH, "tbcp2bin", (char*)NULL);
				break;
			case IN_TYPE_POSTSCRIPT_D:			/* Control-D is messy PostScript */
				warning(WARNING_PEEVE, _("spurious leading ^D"));
				break;
			}

		if(should_passthru("postscript"))
			do_passthru(f);

		if(qentry.opts.hacks & HACK_EDITPS)
			{
			const char **arg_list;

			/* editps_identify wants a zero byte terminated string.
			   We have already allocated extra room for this. */
			in_ptr[in_left] = '\0';

			/* If the editps system wants to edit the file,
			   execute the filter it specifies. */
			if((arg_list = editps_identify(in_ptr, in_left)) != (const char **)NULL)
				{
				exec_filter_argv(arg_list[0], arg_list);
				}
			}
		}

	/* Input file is not PostScript, */
	else
		{
		temp_Filters = append_to_list(temp_Filters, f->name);
		if(should_passthru(f->name))
			do_passthru(f);
		else
			run_appropriate_filter(f, filename);
		}

	qentry.Filters = temp_Filters;

	return 0;
	} /* end of infile_open() */

/*=====================================================================
** Other externally called routines:
=====================================================================*/

/*
** This function exists so that in_handle does not
** have to be externaly visible.
*/
void infile_close(void)
	{
	in_scotch();

	if(in_handle != -1)
		{
		close(in_handle);
		in_handle = -1;
		}

	if(in_buffer_rock_bottom)
		{
		gu_free(in_buffer_rock_bottom);
		in_buffer_rock_bottom = NULL;
		}
	} /* end of infile_close() */

/*
** Called from main() for the -T switch, this function forces to
** input file type to a certain type, disabling auto detect.  It
** is also called for "@PJL ENTER LANGAUGE = XXX" from
** check_for_pjl() above.
**
** Return -1 if the type is unrecognized.
*/
int infile_force_type(const char type[])
	{
	const struct FILTER *f;

	if(! strchr(type, '/'))
		f = filter_by_name(type);
	else
		f = filter_by_mime(type);

	if(f)
		{
		in_type = f->in_type;
		return 0;
		}

	/* failure */
	in_type = IN_TYPE_UNRECOGNIZED;
	return -1;
	} /* end of infile_force_type() */

/*
** This is called from the code in ppr_main.c which implements
** ppr --help.
**
** It prints the part of the ppr --help output which relates to filters.
** This consists of the -T option which selects the filter and
** a brief description of the file type.  File types which can't
** be selected, such as IN_TYPE_UNRECOGNIZED are indicated by a NULL
** pointer in the .name member.  These are skipt.
*/
void minus_tee_help(FILE *outfile)
	{
	int x;
	for(x=0; x < filters_count; x++)
		{
		if(filters[x].help)		/* some types are hidden */
			fprintf(outfile,"\t-T %-23s %s\n", filters[x].name, gettext(filters[x].help));
		}
	} /* end of minus_tee_help() */

/*
** This is called when aborting in order to remove any files
** which this module has created.
*/
void infile_file_cleanup(void)
	{
	char fname[MAX_PPR_PATH];

	if(keepinfile_file_created)
		{
		ppr_fnamef(fname, "%s/%s-%d.0-infile", DATADIR, qentry.jobname.destname, qentry.jobname.id);
		unlink(fname);
		keepinfile_file_created = FALSE;
		}

	if(barbar_file_created)
		{
		ppr_fnamef(fname, "%s/%s-%d.0-barbar", DATADIR, qentry.jobname.destname, qentry.jobname.id);
		unlink(fname);
		barbar_file_created = FALSE;
		}

	} /* end of infile_file_cleanup() */

/*
 * Given a PID, tell what filter it is.
 */
const char *infile_filter_name_by_pid(pid_t pid)
	{
	int i;
	for(i = 0; i < launched_filters_count; i++)
		{
		if(launched_filters[i].pid == pid)
			return launched_filters[i].name;
		}
	return "?";
	}

/* end of file */

