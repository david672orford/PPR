/*
** ~ppr/src/filter_dotmatrix/inbuf.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 18 June 1999.
*/

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "filter_dotmatrix.h"

int clear8th = 0xFF;
int set8th = 0;

static int bytes_left = 0;
static int eof = FALSE;

/*
** Get a character from the input buffer.
*/
int input(void)
    {
    static unsigned char inbuf[INPUT_BUFFER_SIZE];
    static unsigned char *ptr;
    int c;

    while(bytes_left==0)
    	{
    	if(eof)
    	    return EOF;

	if( (bytes_left=read(0,inbuf,sizeof(inbuf))) == -1 )
	    {
	    fprintf(stderr,"input(): read() failed, errno=%d",errno);
	    exit(10);
	    }

	if(bytes_left < (int)sizeof(inbuf))
	    eof=TRUE;

	ptr=inbuf;
    	}

    bytes_left--;
    c=*(ptr++);

    return ( (c & clear8th) | set8th );
    } /* end of input() */

/*
** Return to the begining of the input file.
*/
void rewind_input(void)
    {
    bytes_left=0;
    eof=FALSE;

    lseek(0,(off_t)0,SEEK_SET);
    } /* end of rewind_input() */

/* end of file */
