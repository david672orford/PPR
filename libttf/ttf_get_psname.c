/*
** mouse:~ppr/src/libttf/ttf_get_psname.c
** Copyright 1998, Trinity College Computing Center.
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
#include "libttf_private.h"

char *ttf_get_psname(void *p)
    {
    struct TTFONT *font = (struct TTFONT *)p;
    int ret;

    if(font->signiture != TTF_SIGNITURE)
    	return NULL;

    if((ret = setjmp(font->exception)) != 0)
	{
	font->errno = (TTF_RESULT)ret;
    	return NULL;
    	}

    if(!font->PostName) font->PostName = ttf_loadname(font, 1, 6);

    return font->PostName;
    }

/* end of file */
