/*
** mouse:~ppr/src/libppr/padded.c
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
** Last modified 24 February 2005.
*/

#include "config.h"
#include <ctype.h>
#include "gu.h"
#include "global_defines.h"

/*
** Copy an ASCIIZ string into a blank padded string
** and vice versa.
*/
void ASCIIZ_to_padded(char *padded, const char *asciiz, int len)
	{
	while( *asciiz && len-- )
		*(padded++)=*(asciiz++);

	while( len-- > 0 )					/* the "> 0" is important! */
		*(padded++)=' ';
	}

void padded_to_ASCIIZ(char *asciiz, const char *padded, int len)
	{
	while(len!=-1 && padded[--len]==' ');		/* clip trailing spaces */
	len++;										/* we overshot */
	while(len--)								/* copy what is left */
		*(asciiz++)=*(padded++);
	*asciiz = '\0';
	}

/*
** Return TRUE if the padded strings are
** identical.
*/
gu_boolean padded_cmp(const char *padded1, const char *padded2, int len)
	{
	while(len--)
		{
		if(*(padded1++) != *(padded2++))		/* on 1st mismatch, */
			return FALSE;						/* we know they differ */
		}

	return TRUE;
	}

/*
** Return TRUE if the padded strings are
** identical in all respects except case.
*/
gu_boolean padded_icmp(const char *padded1, const char *padded2, int len)
	{
	while(len--)
		{
		if(tolower(*(padded1++)) != tolower(*(padded2++)))
			return FALSE;
		}

	return TRUE;
	}

/* end of file */
