/*
** mouse:~ppr/src/ttfutils/ppr-ttf2ps.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 4 February 2000.
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
	fprintf(stderr, "Usage: ttf2ps [--type3] [--type42] <filename>\n");
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

