/*
** mouse:~ppr/src/libttf/ps_sfnts.c
** Copyright 1995, 1996, 1997, 1998, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 10 November 1998.
*/

#include "libttf_before_system.h"
#include <string.h>
#include "libttf_private.h"

/*-------------------------------------------------------------------
** sfnts routines
** These routines generate the PostScript "sfnts" array which
** contains one or more strings which contain a reduced version
** of the TrueType font.
**
** A number of functions are required to accomplish this rather
** complicated task.
-------------------------------------------------------------------*/
static int string_len;
static int line_len_sofar;
static int in_string;

/*
** This is called once at the start.
*/
static void sfnts_start(struct TTFONT *font)
    {
    (*font->puts)("/sfnts[<");
    in_string = TRUE;
    string_len = 0;
    line_len_sofar = 8;
    } /* end of sfnts_start() */

/*
** Write a BYTE as a hexadecimal value as part of the sfnts array.
*/
static void sfnts_pputBYTE(struct TTFONT *font, BYTE n)
    {
    static const char hexdigits[] = "0123456789ABCDEF";

    if(!in_string)
    	{
	(*font->putc)('<');
    	string_len = 0;
    	line_len_sofar++;
    	in_string = TRUE;
    	}

    (*font->putc)( hexdigits[ n / 16 ] );
    (*font->putc)( hexdigits[ n % 16 ] );
    string_len++;
    line_len_sofar+=2;

    if(line_len_sofar > 70)
   	{
   	(*font->putc)('\n');
   	line_len_sofar = 0;
   	}

    } /* end of sfnts_pputBYTE() */

/*
** Write a USHORT as a hexadecimal value as part of the sfnts array.
*/
static void sfnts_pputUSHORT(struct TTFONT *font, USHORT n)
    {
    sfnts_pputBYTE(font, n / 256);
    sfnts_pputBYTE(font, n % 256);
    } /* end of sfnts_pputUSHORT() */

/*
** Write a ULONG as part of the sfnts array.
*/
static void sfnts_pputULONG(struct TTFONT *font, ULONG n)
    {
    int x1,x2,x3;

    x1 = n % 256;
    n /= 256;
    x2 = n % 256;
    n /= 256;
    x3 = n % 256;
    n /= 256;

    sfnts_pputBYTE(font, n);
    sfnts_pputBYTE(font, x3);
    sfnts_pputBYTE(font, x2);
    sfnts_pputBYTE(font, x1);
    } /* end of sfnts_pputULONG() */

/*
** This is called whenever it is
** necessary to end a string in the sfnts array.
**
** (The array must be broken into strings which are
** no longer than 64K characters.)
*/
static void sfnts_end_string(struct TTFONT *font)
    {
    if(in_string)
    	{
	string_len = 0;		/* fool sfnts_pputBYTE() */

	#ifdef DEBUG_INLINE
	(*font->puts)("\n% dummy byte:\n");
	#endif

	sfnts_pputBYTE(font, 0);	/* extra byte for pre-2013 compatibility */
	(*font->putc)('>');
	line_len_sofar++;
    	}
    in_string = FALSE;
    } /* end of sfnts_end_string() */

/*
** This is called at the start of each new table.
** The argement is the length in bytes of the table
** which will follow.  If the new table will not fit
** in the current string, a new one is started.
*/
static void sfnts_new_table(struct TTFONT *font, ULONG length)
    {
    if( (string_len + length) > 65528 )
	sfnts_end_string(font);
    } /* end of sfnts_new_table() */

/*
** We may have to break up the 'glyf' table.  That is the reason
** why we provide this special routine to copy it into the sfnts
** array.
*/
static void sfnts_glyf_table(struct TTFONT *font, ULONG oldoffset, ULONG correct_total_length)
    {
    int x;
    ULONG off;
    ULONG length;
    int c;
    ULONG total=0;		/* running total of bytes written to table */

    DODEBUG(("sfnts_glyf_table(font,%d)", (int)correct_total_length));

    if(!font->loca_table) font->loca_table = ttf_LoadTable(font, "loca");

    /* Seek to proper position in the file. */
    fseek(font->file, oldoffset, SEEK_SET);

    /* Copy the glyphs one by one */
    for(x=0; x < font->numGlyphs; x++)
	{
	/* Read the glyph offset from the index-to-location table. */
	if(font->indexToLocFormat == 0)
	    {
	    off = getUSHORT( font->loca_table + (x * 2) );
	    off *= 2;
	    length = getUSHORT( font->loca_table + ((x+1) * 2) );
	    length *= 2;
	    length -= off;
	    }
	else
	    {
	    off = getULONG( font->loca_table + (x * 4) );
	    length = getULONG( font->loca_table + ((x+1) * 4) );
	    length -= off;
	    }

	DODEBUG(("glyph length=%d", (int)length));

	/* Start new string if necessary. */
	sfnts_new_table(font, (int)length);

	/*
	** Make sure the glyph is padded out to a
	** two byte boundary.
	*/
	if(length % 2)
	    longjmp(font->exception, (int)TTF_GLYF_BADPAD);

	/* Copy the bytes of the glyph. */
	while(length--)
	    {
	    if((c = fgetc(font->file)) == EOF)
	    	longjmp(font->exception, (int)TTF_GLYF_CANTREAD);

	    sfnts_pputBYTE(font, c);
	    total++;		/* add to running total */
	    }

	}

    /* Pad out to full length from table directory */
    while(total < correct_total_length)
    	{
    	sfnts_pputBYTE(font, 0);
    	total++;
    	}

    /* Look for unexplainable descrepancies between sizes */
    if(total != correct_total_length)
	longjmp(font->exception, (int)TTF_GLYF_SIZEINC);

    } /* end of sfnts_glyf_table() */

/*
** Here is the routine which ties it all together.
**
** Create the array called "sfnts" which
** holds the actual TrueType data.
*/
void ttf_PS_sfnts(struct TTFONT *font)
    {
    static const char *table_names[]=	/* The names of all tables */
    	{				/* which it is worth while */
    	"cvt ",				/* to include in a Type 42 */
    	"fpgm",				/* PostScript font. */
    	"glyf",
    	"head",
    	"hhea",
    	"hmtx",
    	"loca",
    	"maxp",
    	"prep"
    	} ;

    struct {			/* The location of each of */
    	ULONG oldoffset;	/* the above tables. */
    	ULONG newoffset;
    	ULONG length;
    	ULONG checksum;
    	} tables[9];

    BYTE *ptr;			/* a pointer into the origional table directory */
    unsigned int x, y;		/* general use loop countes */
    int c;			/* input character */
    int diff;
    ULONG nextoffset;
    int count;			/* How many `important' tables did we find? */

    ptr = font->offset_table + 12;
    nextoffset = 0;
    count = 0;

    /*
    ** Find the tables we want and store there vital
    ** statistics in tables[].
    */
    for(x=0; x < 9; x++)
    	{
    	do  {
    	    diff = strncmp((char*)ptr, table_names[x], 4);

	    if(diff > 0)		/* If we are past it. */
	    	{
		tables[x].length = 0;
		diff = 0;
	    	}
	    else if( diff < 0 )		/* If we haven't hit it yet. */
	        {
	        ptr += 16;
	        }
	    else if( diff == 0 )	/* Here it is! */
	    	{
		tables[x].newoffset = nextoffset;
		tables[x].checksum = getULONG( ptr + 4 );
		tables[x].oldoffset = getULONG( ptr + 8 );
		tables[x].length = getULONG( ptr + 12 );
		nextoffset += ( ((tables[x].length + 3) / 4) * 4 );
		count++;
		ptr += 16;
	    	}
    	    } while(diff != 0);

    	} /* end of for loop which passes over the table directory */

    /* Begin the sfnts array. */
    sfnts_start(font);

    /* Generate the offset table header.  Start by copying
       the TrueType version number. */
    ptr = font->offset_table;
    for(x=0; x < 4; x++)
	{
   	sfnts_pputBYTE(font, *(ptr++) );
   	}

    /* Now, generate those silly numTables numbers. */
    sfnts_pputUSHORT(font, count);	/* number of tables */
    if(count == 9)
    	{
    	sfnts_pputUSHORT(font, 7);	/* searchRange */
    	sfnts_pputUSHORT(font, 3);	/* entrySelector */
    	sfnts_pputUSHORT(font, 81);	/* rangeShift */
    	}
    #ifdef DEBUG
    else
    	{
	debug("only %d tables selected",count);
    	}
    #endif

    /* Now, emmit the table directory. */
    for(x=0; x < 9; x++)
    	{
	if(tables[x].length == 0)	/* Skip missing tables */
	    continue;

	/* Name */
	sfnts_pputBYTE(font, table_names[x][0] );
	sfnts_pputBYTE(font, table_names[x][1] );
	sfnts_pputBYTE(font, table_names[x][2] );
	sfnts_pputBYTE(font, table_names[x][3] );

	/* Checksum */
	sfnts_pputULONG(font, tables[x].checksum );

	/* Offset */
	sfnts_pputULONG(font, tables[x].newoffset + 12 + (count * 16) );

	/* Length */
	sfnts_pputULONG(font, tables[x].length );
    	}

    /* Now, send the tables */
    for(x=0; x < 9; x++)
    	{
    	if(tables[x].length == 0)	/* skip tables that aren't there */
    	    continue;

	DODEBUG(("emmiting table '%s'", table_names[x]));

	/* 'glyf' table gets special treatment */
	if(strcmp(table_names[x], "glyf") == 0)
	    {
	    sfnts_glyf_table(font, tables[x].oldoffset, tables[x].length);
	    }
	else			/* Other tables may not exceed */
	    {			/* 65535 bytes in length. */
	    if(tables[x].length > 65535)
	    	{
	    	DODEBUG(("Table '%s' is too long at %d bytes", table_names[x], tables[x].length));
                longjmp(font->exception, (int)TTF_TBL_TOOBIG);
                }

	    /* Start new string if necessary. */
	    sfnts_new_table(font, tables[x].length);

	    /* Seek to proper position in the file. */
	    fseek(font->file, tables[x].oldoffset, SEEK_SET);

	    /* Copy the bytes of the table. */
	    for(y=0; y < tables[x].length; y++)
	        {
	        if((c = fgetc(font->file)) == EOF)
	    	    {
	    	    DODEBUG(("read error in '%s' table", table_names[x]));
	    	    longjmp(font->exception, (int)TTF_TBL_CANTREAD);
	    	    }

	        sfnts_pputBYTE(font, c);
	        }
	    }

	/* Padd it out to a four byte boundary. */
	y = tables[x].length;
	while( (y % 4) != 0 )
	    {
	    sfnts_pputBYTE(font, 0);
	    y++;
	    #ifdef DEBUG_INLINE
	    (*font->puts)("\n% pad byte:\n");
	    #endif
	    }

    	} /* End of loop for all tables */

    /* Close the array. */
    sfnts_end_string(font);
    (*font->puts)("]def\n");
    } /* end of ttf_PS_sfnts() */

/* end of file */
