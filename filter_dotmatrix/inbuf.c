/*
** mouse:~ppr/src/filter_dotmatrix/inbuf.c
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
** Last modified 23 March 2005.
*/

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

		if((bytes_left = read(0,inbuf,sizeof(inbuf))) == -1)
			gu_Throw(_("%s(): read() failed, errno=%d (%s)"), "input", errno, gu_strerror(errno));

		if(bytes_left < (int)sizeof(inbuf))
			eof = TRUE;

		ptr = inbuf;
		}

	bytes_left--;
	c =* (ptr++);

	return ( (c & clear8th) | set8th );
	} /* end of input() */

/*
** Return to the begining of the input file.
*/
void rewind_input(void)
	{
	bytes_left=0;
	eof=FALSE;
	if(lseek(0,(off_t)0,SEEK_SET) == -1)
		gu_Throw(_("%s(): lseek() failed, errno=%d (%s)"), "rewind_input", errno, gu_strerror(errno));
	} /* end of rewind_input() */

/* end of file */
