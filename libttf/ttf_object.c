/*
** mouse:~ppr/src/libttf/ttf_object.c
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
#include <stdlib.h>
#include "libttf_private.h"

TTF_RESULT ttf_new(void **pp, const char filename[])
	{
	int setjmp_retval;
	struct TTFONT *font;

	/* Allocate space for the object. */
	if((font = (struct TTFONT *)calloc(1, sizeof(struct TTFONT))) == NULL)
		return TTF_NOMEM;

	/* Set signiture in structure for later proof. */
	font->signiture = TTF_SIGNITURE;

	/* Clear a lot of pointers so we will know that there is no
	   allocated memory attached to them. */
	font->offset_table = font->post_table = font->loca_table =
		font->glyf_table = font->hmtx_table = font->name_table = NULL;
	font->PostName = font->FullName = font->FamilyName = font->Style =
		font->Copyright = font->Trademark = font->Version = (char *)NULL;
	font->file = NULL;

	/* Install an exception handler. */
	if((setjmp_retval = setjmp(font->exception)) != 0)
		{
		if(font->file) fclose(font->file);
		if(font->offset_table) ttf_free(font, font->offset_table);
		free(font);
		return (TTF_RESULT)setjmp_retval;
		}

	/* Open the font file */
	font->filename = filename;
	if((font->file = fopen(filename, "r")) == (FILE*)NULL)
		longjmp(font->exception, (int)TTF_CANTOPEN);

	/* Allocate space for the unvarying part of the offset table. */
	font->offset_table = (BYTE*)ttf_alloc(font, 12, sizeof(BYTE));

	/* Read the first part of the offset table. */
	if(fread(font->offset_table, sizeof(BYTE), 12, font->file) != 12)
		longjmp(font->exception, (int)TTF_TBL_CANTREAD);

	/* Determine how many directory entries there are. */
	font->numTables = getUSHORT(font->offset_table + 4);
	DODEBUG(("numTables=%d", (int)font->numTables));

	/* Expand the memory block to hold the whole thing. */
	font->offset_table = (BYTE*)ttf_realloc(font, font->offset_table, (12 + font->numTables * 16), sizeof(BYTE) );

	/* Read the rest of the table directory. */
	if(fread(font->offset_table + 12, sizeof(BYTE), (font->numTables*16), font->file ) != (font->numTables*16) )
		longjmp(font->exception, (int)TTF_TBL_CANTREAD);

	/* Extract information from the "Offset" table. */
	font->TTVersion = getFixed(font->offset_table);

	*pp = (void*)font;
	return TTF_OK;
	} /* end of ttf_new() */

int ttf_delete(void *p)
	{
	struct TTFONT *font = (struct TTFONT *)p;

	if(font->signiture != TTF_SIGNITURE)
		return -1;

	fclose(font->file);

	ttf_free(font, font->offset_table);

	if(font->post_table) ttf_free(font, font->post_table);
	if(font->loca_table) ttf_free(font, font->loca_table);
	if(font->glyf_table) ttf_free(font, font->glyf_table);
	if(font->hmtx_table) ttf_free(font, font->hmtx_table);
	if(font->name_table) ttf_free(font, font->name_table);

	if(font->PostName) ttf_free(font, font->PostName);
	if(font->FullName) ttf_free(font, font->FullName);
	if(font->FamilyName) ttf_free(font, font->FamilyName);
	if(font->Style) ttf_free(font, font->Style);
	if(font->Copyright) ttf_free(font, font->Copyright);
	if(font->Trademark) ttf_free(font, font->Trademark);
	if(font->Version) ttf_free(font, font->Version);

	font->signiture = 0;
	free(p);	/* not ttf_free() */

	return 0;
	} /* end of ttf_delete() */

TTF_RESULT ttf_errno(void *p)
	{
	struct TTFONT *font = (struct TTFONT *)p;

	if(font->signiture != TTF_SIGNITURE)
		return TTF_NOTOBJ;

	return font->errno;
	} /* end of ttf_errno() */

/* end of file */

