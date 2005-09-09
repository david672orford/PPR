/*
** mouse:~ppr/src/libttf/name.c
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
			font->Copyright = gu_strndup((char*)strings+offset, length);
			DODEBUG(("font->Copyright=\"%s\"",font->Copyright));
			continue;
			}

		/* Font Family name */
		if(platform == 1 && nameid == 1)
			{
			font->FamilyName = gu_strndup((char*)strings+offset, length);
			DODEBUG(("font->FamilyName=\"%s\"",font->FamilyName));
			continue;
			}

		/* Font Family name */
		if( platform == 1 && nameid == 2 )
			{
			font->Style = gu_strndup((char*)strings+offset, length);
			DODEBUG(("font->Style=\"%s\"",font->Style));
			continue;
			}

		/* Full Font name */
		if(platform == 1 && nameid == 4)
			{
			font->FullName = gu_strndup((char*)(strings+offset), length);
			DODEBUG(("font->FullName=\"%s\"",font->FullName));
			continue;
			}

		/* Version string */
		if(platform == 1 && nameid == 5)
			{
			font->Version = gu_strndup((char*)(strings+offset), length);
			DODEBUG(("font->Version=\"%s\"",font->Version));
			continue;
			}

		/* PostScript name */
		if(platform == 1 && nameid == 6)
			{
			font->PostName = gu_strndup((char*)(strings+offset), length);
			DODEBUG(("font->PostName=\"%s\"",font->PostName));
			continue;
			}

		/* Trademark string */
		if(platform == 1 && nameid == 7)
			{
			font->Trademark = gu_strndup((char*)(strings+offset), length);
			DODEBUG(("font->Trademark=\"%s\"",font->Trademark));
			continue;
			}

		}

	if(!font->PostName || !font->FullName || !font->FamilyName || !font->Version)
		longjmp(font->exception, TTF_REQNAME);

	return 0;
	} /* end of ttf_Read_name() */

/* end of file */

