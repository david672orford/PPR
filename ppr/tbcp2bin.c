/*
** mouse.trincoll.edu:~ppr/src/ppr/tbcp2bin.c
** Copyright 1996, 1998, Trinity College Computing Center.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 4 March 1998.
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
