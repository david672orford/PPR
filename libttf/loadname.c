/*
** mouse:~ppr/src/libttf/get_loadname.c
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
** Last modified 13 December 2004.
*/

#include "config.h"
#include <string.h>
#include "libttf_private.h"

/*
** Get the PostScript name for this font from the 'name' table.
*/
char *ttf_loadname(struct TTFONT *font, int get_platform, int get_nameid)
	{
	BYTE *table_ptr, *ptr2;
	int numrecords;						/* Number of strings in this table */
	BYTE *strings;						/* pointer to start of string storage */
	int x;
	int platform, encoding;				/* Current platform id, encoding id, */
	int language, nameid;				/* language id, name id, */
	int offset, length;					/* offset and length of string. */
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

