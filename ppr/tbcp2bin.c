/*
** mouse:~ppr/src/ppr/tbcp2bin.c
** Copyright 1995--2005, Trinity College Computing Center.
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
** Last modified 24 August 2005.
*/

/*
** This file reads a Tagged Binary Communictions Protocol file on stdin
** and decodes it, writing the binary data on stdout.
**
** As soon as the sequence ^AM is seen, TBCP is enabled.  From then on,
** a ^A followed by any character is interpreted as the second character
** exclusive ored with 0x40.  All occurences of ^AM are deleted from the
** input stream.  All occurances of ^D are deleted as well since any
** ^D which gets to PPR ought to be data.  The ending ^[%-12345X is
** not deleted since PPR knows how to deal with it.
*/

#include <stdio.h>

int main(int argc, char *argv[])
	{
	int c;

	while((c = fgetc(stdin)) != EOF)
		{
		if(c == 4)				/* control-D's have no real meaning */
			continue;

		if(c == 1)
			{
			c = fgetc(stdin);

			if( c == 'M' )		/* This simply activates TBCP mode */
				continue;

			c = c ^ 0x40;		/* decode byte */
			}

		fputc(c, stdout);
		}

	return 0;
	} /* end of main() */

/* end of file */
