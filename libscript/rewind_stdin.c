/*
** mouse:~ppr/src/filters_misc/rewind_stdin.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 2 March 1999.
*/

/*
** This beauty is used by the Groff-based Troff input filter.  After Grog
** has read the input to decide what to do with it, we use this to rewind
** it so that Groff can process it.
*/

#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[])
    {
    if(lseek(0, 0, SEEK_SET) < 0)
    	write(2, "rewind_stdin: failed\n", 21);
    return 0;
    }

/* end of file */

