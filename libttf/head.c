/*
** mouse:~ppr/src/libttf/head.c
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
#include "libttf_private.h"

int ttf_Read_head(struct TTFONT *font)
	{
	BYTE *ptr;

	DODEBUG(("ttf_Read_head()"));

	ptr = ttf_LoadTable(font, "head");

	font->MfrRevision = getFixed( ptr + 4 );			/* font revision number */
	font->unitsPerEm = getUSHORT( ptr + 18 );
	font->HUPM = font->unitsPerEm / 2;
	DODEBUG(("unitsPerEm=%d", (int)font->unitsPerEm));
	font->llx = topost( getFWord( ptr + 36 ) );			/* bounding box info */
	font->lly = topost( getFWord( ptr + 38 ) );
	font->urx = topost( getFWord( ptr + 40 ) );
	font->ury = topost( getFWord( ptr + 42 ) );
	font->indexToLocFormat = getSHORT( ptr + 50 );		/* size of 'loca' data */
	if(font->indexToLocFormat != 0 && font->indexToLocFormat != 1)
		longjmp(font->exception, (int)TTF_UNSUP_LOCA);
	if(getSHORT(ptr+52) != 0)
		longjmp(font->exception, (int)TTF_UNSUP_GLYF);

	ttf_free(font, ptr);

	return 0;
	} /* end of ttf_Read_head() */

/* end of file */

