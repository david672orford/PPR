/*
** mouse:~ppr/src/libttf/alloc.c
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

