/*
** mouse:~ppr/src/fontutils/ppr-ttf2ps.c
** Copyright 1995--2003, Trinity College Computing Center.
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
** Last modified 6 March 2003.
*/

/*
** This is a demonstration program for the libttf library.  It does
** not actually depend on any other part of PPR such as libppr or
** global_defines.h.  Neither is it internationalized.  This was all
** done deliberately, to make it easier to understand.
*/

#include <stdio.h>
#include <stdarg.h>
#include "libttf.h"

static void out_putc(int c)
    {
    fputc(c, stdout);
    }

static void out_puts(const char *s)
    {
    fputs(s, stdout);
    }

static void out_printf(const char *format, ...)
    {
    va_list va;
    va_start(va, format);
    vfprintf(stdout, format, va);
    va_end(va);
    }

int main(int argc, char *argv[])
    {
    void *font;
    int fonttype = 42;
    int x;

    for(x=1; argv[x]; x++)
	{
	if(argv[x][0] != '-')
	    break;
	if(strcmp(argv[x], "--type42") == 0)
	    fonttype = 42;
	else if(strcmp(argv[x], "--type3") == 0)
	    fonttype = 3;
	else
	    break;
	}

    if((argc - x) != 1)
    	{
	fprintf(stderr, "Usage: ppr-ttf2ps --type3] [--type42] <filename>\n");
	return 1;
    	}

    {
    TTF_RESULT ttf_result;
    if((ttf_result = ttf_new(&font, argv[x])) != TTF_OK)
    	{
    	printf("ttf_new() failed, %s\n", ttf_strerror(ttf_result));
    	return 2;
    	}
    }

    if(ttf_psout(font, out_putc, out_puts, out_printf, fonttype) == -1)
    	{
    	printf("ttf_psout() failed, %s\n", ttf_strerror(ttf_errno(font)));
    	return 3;
    	}

    if(ttf_delete(font) == -1)
    	{
    	printf("ttf_delete() failed, %s\n", ttf_strerror(ttf_errno(font)));
    	return 4;
    	}

    return 0;
    } /* end of main */

/* end of file */

