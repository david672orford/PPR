/*
** mouse:~ppr/src/unixuser/ppr-hexdump.c
** Copyright 1995--2002, Trinity College Computing Center.
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
** Last modified 27 November 2002.
*/

/*
** I wrote this ultra simple hexdump program after suffering years of 
** frustration with the fact that Unix's hexdump prints in a wacko 
** format.	I have never been able to get it to output bytes in the 
** style of the old MS-DOS hex editors.	 I have been making do with
** Xv's hexdump feature, which is fine for X-Windows.  But sometimes one 
** simply wants a hexdump one can easily print out and study.  I finally
** broke down and wrote this one.  It is basically a hacked down
** version of ../filters_misc/filter_hexdump.c.	 The output format 
** has been changed to match that of Xv.
*/

#include <stdio.h>
#include <ctype.h>

#define BYTES_PER_LINE 16 

static void hexline(int offset, unsigned char *segment, int len)
	{
	int x;
	int c;

	/* Print the offset into the file. */
	printf("0x%08X: ", offset);

	/* Print the bytes in hexadecimal. */
	for(x=0; x < BYTES_PER_LINE; x++)
		{
		if(x && x % 8 == 0)
			printf("- ");
		if(x < len)
			printf("%2.2X ", segment[x]);
		else
			printf("   ");
		}

	printf(" ");

	/* Print the bytes in ASCII. */
	for(x=0; x < len; x++)
		{
		c = segment[x];

		if(!isprint(c))			/* if unprintable, use dot */
			fputc('.', stdout);
		else					/* if printable, print */
			fputc(c,stdout);
		}

	printf("\n");
	}

int main(int argc, char *argv[])
	{
	int offset = 0;
	char buffer[BYTES_PER_LINE];
	int len;
		
	while((len = fread(buffer, sizeof(unsigned char), sizeof(buffer), stdin)))
		{
		hexline(offset, buffer, len);
		offset += len;
		}

	return 0;
	}

/* end of file */

