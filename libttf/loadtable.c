/*
** mouse:~ppr/src/libttf/loadtable.c
** Copyright 1995, 1996, 1998, 1998, Trinity College Computing Center.
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
#include <unistd.h>
#include <string.h>
#include "libttf_private.h"

/*-----------------------------------------------------------------------
** Load a TrueType font table into memory and return a pointer to it.
** The font's "file" and "offset_table" fields must be set before this
** routine is called.
**
** This first argument is a TrueType font structure, the second
** argument is the name of the table to retrieve.  A table name
** is always 4 characters, though the last characters may be
** padding spaces.
-----------------------------------------------------------------------*/
BYTE *ttf_LoadTable(struct TTFONT *font, const char name[])
    {
    BYTE *ptr;
    unsigned int x;

    DODEBUG(("ttf_LoadTable(font=%p, name=\"%s\", tp=%p)", font, name, tp));

    /* We must search the table directory. */
    ptr = font->offset_table + 12;
    x = 0;
    while(TRUE)
    	{
	if(strncmp((const char*)ptr, name, 4) == 0)
	    {
	    ULONG offset,length;
	    BYTE *table;

	    offset = getULONG(ptr + 8);
	    length = getULONG(ptr + 12);
	    table = (BYTE*)ttf_alloc(font, sizeof(BYTE), length );

	    DODEBUG(("Loading table \"%s\" from offset %d, %d bytes",name,offset,length));

	    if(fseek(font->file, (long)offset, SEEK_SET ) )
	    	longjmp(font->exception, (int)TTF_TBL_CANTSEEK);

	    if(fread(table,sizeof(BYTE),length,font->file) != (sizeof(BYTE) * length))
		longjmp(font->exception, (int)TTF_TBL_CANTREAD);

	    return table;
	    }

    	x++;
    	ptr += 16;
    	if(x == font->numTables)
	    longjmp(font->exception, (int)TTF_TBL_NOTFOUND);
    	}

    } /* end of ttf_LoadTable() */

/* end of file */

