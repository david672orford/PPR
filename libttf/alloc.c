/*
** mouse:~ppr/src/libttf/alloc.c
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
#include <stdlib.h>
#include <string.h>
#include "libttf_private.h"

void *ttf_alloc(struct TTFONT *font, size_t number, size_t size)
    {
    void *rval;

    if((rval = malloc(size*number)) == (void*)NULL)
	longjmp(font->exception, (int)TTF_NOMEM);

    return rval;
    }

char *ttf_strdup(struct TTFONT *font, const char *string)
    {
    char *rval;

    if((rval = (char*)malloc(strlen(string)+1)) == (char*)NULL)
	longjmp(font->exception, (int)TTF_NOMEM);

    strcpy(rval, string);

    return rval;
    }

char *ttf_strndup(struct TTFONT *font, const char *string, size_t len)
    {
    char *rval;

    if((rval = (char*)malloc(len+1)) == (char*)NULL)
	longjmp(font->exception, (int)TTF_NOMEM);

    strncpy(rval, string, len);
    rval[len] = '\0';

    return rval;
    }

void *ttf_realloc(struct TTFONT *font, void *ptr, size_t number, size_t size)
    {
    void *rval;

    if((rval = realloc(ptr, number*size)) == (void*)NULL)
	longjmp(font->exception, (int)TTF_NOMEM);

    return rval;
    }

void ttf_free(struct TTFONT *font, void *ptr)
    {
    if(!ptr)
    	longjmp(font->exception, TTF_BADFREE);

    free(ptr);
    }

/* end of file */

