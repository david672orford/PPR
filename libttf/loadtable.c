/*
** mouse:~ppr/src/libttf/loadtable.c
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

