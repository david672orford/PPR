/*
** mouse:~ppr/src/libttf/name.c
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

#include "config.h"
#include <string.h>
#include "libttf_private.h"

/*--------------------------------------------------------------------
** Load the 'name' table, get information from it,
** and store that information in the font structure.
**
** The 'name' table contains information such as the name of
** the font, and it's PostScript name.
--------------------------------------------------------------------*/
int ttf_Read_name(struct TTFONT *font)
	{
	BYTE *table_ptr,*ptr2;
	int numrecords;						/* Number of strings in this table */
	BYTE *strings;						/* pointer to start of string storage */
	int x;
	int platform,encoding;				/* Current platform id, encoding id, */
	int language,nameid;				/* language id, name id, */
	int offset,length;					/* offset and length of string. */

	DODEBUG(("Read_name()"));

	if(!font->name_table) font->name_table = ttf_LoadTable(font, "name");
	table_ptr = font->name_table;
	numrecords = getUSHORT(table_ptr + 2);				/* number of names */
	strings = table_ptr + getUSHORT(table_ptr + 4);		/* start of string storage */

	ptr2 = table_ptr + 6;
	for(x=0; x < numrecords; x++,ptr2+=12)
		{
		platform = getUSHORT(ptr2);
		encoding = getUSHORT(ptr2+2);
		language = getUSHORT(ptr2+4);
		nameid = getUSHORT(ptr2+6);
		length = getUSHORT(ptr2+8);
		offset = getUSHORT(ptr2+10);

		DODEBUG(("platform %d, encoding %d, language 0x%x, name %d, offset %d, length %d",
				platform,encoding,language,nameid,offset,length));

		/* Copyright notice */
		if(platform == 1 && nameid == 0)
			{
			font->Copyright = ttf_strndup(font, (char*)strings+offset, length);
			DODEBUG(("font->Copyright=\"%s\"",font->Copyright));
			continue;
			}

		/* Font Family name */
		if(platform == 1 && nameid == 1)
			{
			font->FamilyName = ttf_strndup(font, (char*)strings+offset, length);
			DODEBUG(("font->FamilyName=\"%s\"",font->FamilyName));
			continue;
			}

		/* Font Family name */
		if( platform == 1 && nameid == 2 )
			{
			font->Style = ttf_strndup(font, (char*)strings+offset, length);
			DODEBUG(("font->Style=\"%s\"",font->Style));
			continue;
			}

		/* Full Font name */
		if(platform == 1 && nameid == 4)
			{
			font->FullName = ttf_strndup(font, (char*)(strings+offset), length);
			DODEBUG(("font->FullName=\"%s\"",font->FullName));
			continue;
			}

		/* Version string */
		if(platform == 1 && nameid == 5)
			{
			font->Version = ttf_strndup(font, (char*)(strings+offset), length);
			DODEBUG(("font->Version=\"%s\"",font->Version));
			continue;
			}

		/* PostScript name */
		if(platform == 1 && nameid == 6)
			{
			font->PostName = ttf_strndup(font, (char*)(strings+offset), length);
			DODEBUG(("font->PostName=\"%s\"",font->PostName));
			continue;
			}

		/* Trademark string */
		if(platform == 1 && nameid == 7)
			{
			font->Trademark = ttf_strndup(font, (char*)(strings+offset), length);
			DODEBUG(("font->Trademark=\"%s\"",font->Trademark));
			continue;
			}

		}

	if(!font->PostName || !font->FullName || !font->FamilyName || !font->Version)
		longjmp(font->exception, TTF_REQNAME);

	return 0;
	} /* end of ttf_Read_name() */

/* end of file */

