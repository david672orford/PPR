/*
** mouse:~ppr/src/libttf/head.c
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
#include "libttf_private.h"

int ttf_Read_head(struct TTFONT *font)
    {
    BYTE *ptr;

    DODEBUG(("ttf_Read_head()"));

    ptr = ttf_LoadTable(font, "head");

    font->MfrRevision = getFixed( ptr + 4 );		/* font revision number */
    font->unitsPerEm = getUSHORT( ptr + 18 );
    font->HUPM = font->unitsPerEm / 2;
    DODEBUG(("unitsPerEm=%d", (int)font->unitsPerEm));
    font->llx = topost( getFWord( ptr + 36 ) );		/* bounding box info */
    font->lly = topost( getFWord( ptr + 38 ) );
    font->urx = topost( getFWord( ptr + 40 ) );
    font->ury = topost( getFWord( ptr + 42 ) );
    font->indexToLocFormat = getSHORT( ptr + 50 );	/* size of 'loca' data */
    if(font->indexToLocFormat != 0 && font->indexToLocFormat != 1)
    	longjmp(font->exception, (int)TTF_UNSUP_LOCA);
    if(getSHORT(ptr+52) != 0)
    	longjmp(font->exception, (int)TTF_UNSUP_GLYF);

    ttf_free(font, ptr);

    return 0;
    } /* end of ttf_Read_head() */

/* end of file */

