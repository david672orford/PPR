/*
** mouse:~ppr/src/libttf/get_loadname.c
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
** Last modified 6 November 1998.
*/

#include "libttf_before_system.h"
#include <string.h>
#include "libttf_private.h"

/*
** Get the PostScript name for this font from the 'name' table.
*/
char *ttf_loadname(struct TTFONT *font, int get_platform, int get_nameid)
    {
    BYTE *table_ptr, *ptr2;
    int numrecords;			/* Number of strings in this table */
    BYTE *strings;			/* pointer to start of string storage */
    int x;
    int platform, encoding;		/* Current platform id, encoding id, */
    int language, nameid;		/* language id, name id, */
    int offset, length;			/* offset and length of string. */
    char *name = (char*)NULL;

    if(!font->name_table) font->name_table = ttf_LoadTable(font, "name");

    table_ptr = font->name_table;

    numrecords = getUSHORT(table_ptr + 2);
    strings = table_ptr + getUSHORT(table_ptr + 4);

    ptr2 = table_ptr + 6;
    for(x=0; x < numrecords; x++,ptr2+=12)
    	{
	platform = getUSHORT(ptr2);
	encoding = getUSHORT(ptr2+2);
	language = getUSHORT(ptr2+4);
	nameid = getUSHORT(ptr2+6);
	length = getUSHORT(ptr2+8);
	offset = getUSHORT(ptr2+10);

	/* PostScript name */
	if(platform == get_platform && nameid == get_nameid)
	    {
	    name = gu_strndup((char*)(strings+offset), length);
	    break;
	    }
    	}

    return name;
    } /* end of get_postscript_name() */

/* end of file */

