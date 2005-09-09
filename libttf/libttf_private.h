/*
** mouse:~ppr/src/libttf/libttf_private.h
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

/*
** This file is included by all of the modules in the library
** "libttf".  This file defines things the modules must know
** but that an external caller needn't.
*/

/* For the time being, we will rely on some parts of PPR's
   libgu, so we must include its include file: */
#include "gu.h"

/* We should know everything an external caller will: */
#include "libttf.h"

/* We use longjmp for internal exception handing */
#include <setjmp.h>

/* Turns on debugging */
#if 0
#define DEBUG 1
#define DEBUG_INLINE 1
#define DODEBUG(a) debug a
#else
#undef DEBUG
#undef DEBUG_INLINE
#define DODEBUG(a)
#endif

/* Types used in TrueType font files. */
#define BYTE unsigned char
#define USHORT unsigned short int
#define SHORT short signed int
#define ULONG unsigned int
#define FIXED long signed int
#define FWord short signed int
#define uFWord short unsigned int

/* This structure stores a 16.16 bit fixed
   point number. */
typedef struct
	{
	short int whole;
	unsigned short int fraction;
	} Fixed;

/* We use this magic value to be sure that the
   pointer passed to a libttf routine really points
   to a struct TTFONT. */
#define TTF_SIGNITURE 1442

/*
** This structure tells what we have found out about
** the current font.
*/
struct TTFONT
	{
	int signiture;

	TTF_RESULT errno;					/* last error code */

	jmp_buf exception;					/* exception handling job */

	const char *filename;				/* Name of TT file */
	FILE *file;							/* the open TT file */

	unsigned int numTables;				/* number of tables present */
	char *PostName;						/* Font's PostScript name */
	char *FullName;						/* Font's full name */
	char *FamilyName;					/* Font's family name */
	char *Style;						/* Font's style string */
	char *Copyright;					/* Font's copyright string */
	char *Trademark;					/* Font's trademark string */
	char *Version;						/* Font's version string */
	int llx,lly,urx,ury;				/* bounding box */

	Fixed TTVersion;					/* Truetype version number from offset table */
	Fixed MfrRevision;					/* Revision number of this font */

	BYTE *offset_table;					/* Offset is loaded right away. */

	/* These table are loaded when they are first needed. */
	BYTE *post_table;
	BYTE *loca_table;
	BYTE *glyf_table;
	BYTE *hmtx_table;
	BYTE *name_table;

	USHORT numberOfHMetrics;
	int unitsPerEm;						/* unitsPerEm converted to int */
	int HUPM;							/* half of above */

	int numGlyphs;						/* from 'post' table */

	int indexToLocFormat;				/* short or long offsets */

	void (*putc)(int c);
	void (*puts)(const char *string);
	void (*printf)(const char *format, ...);
	} ;

/*===================================================================
** Prototype for endian conversion routines
===================================================================*/

ULONG getULONG(BYTE *p);
USHORT getUSHORT(BYTE *p);
Fixed getFixed(BYTE *p);

/*
** Get an funits word.
** since it is 16 bits long, we can
** use getUSHORT() to do the real work.
*/
#define getFWord(x) (FWord)getUSHORT(x)
#define getuFWord(x) (uFWord)getUSHORT(x)

/*
** We can get a SHORT by making USHORT signed.
*/
#define getSHORT(x) (SHORT)getUSHORT(x)

/*=====================================================================
** Other library routines
=====================================================================*/

int ttf_Open(struct TTFONT *font, const char filename[]);
int ttf_Close(struct TTFONT *font);
int ttf_Read_head(struct TTFONT *font);
int ttf_Read_name(struct TTFONT *font);
const char *ttf_charindex2name(struct TTFONT *font, int charindex);
BYTE *ttf_LoadTable(struct TTFONT *font, const char name[]);
char *ttf_loadname(struct TTFONT *font, int get_platform, int get_nameid);

void ttf_PS_header(struct TTFONT *font, int target_type);
void ttf_PS_encoding(struct TTFONT *font);
void ttf_PS_FontInfo(struct TTFONT *font);
void ttf_PS_trailer(struct TTFONT *font, int target_type);
void ttf_PS_CharStrings(struct TTFONT *font, int target_type);
void ttf_PS_type3_charproc(struct TTFONT *font, int charindex);
void ttf_PS_sfnts(struct TTFONT *font);

/* This routine converts a number in the font's character coordinate
   system to a number in a 1000 unit character system. */
#define topost(x) (int)( ((int)(x) * 1000 + font->HUPM) / font->unitsPerEm )

/* Composite glyph values. */
#define ARG_1_AND_2_ARE_WORDS 1
#define ARGS_ARE_XY_VALUES 2
#define ROUND_XY_TO_GRID 4
#define WE_HAVE_A_SCALE 8
/* RESERVED 16 */
#define MORE_COMPONENTS 32
#define WE_HAVE_AN_X_AND_Y_SCALE 64
#define WE_HAVE_A_TWO_BY_TWO 128
#define WE_HAVE_INSTRUCTIONS 256
#define USE_MY_METRICS 512

/* end of file */

