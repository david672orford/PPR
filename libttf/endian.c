/*
** mouse:~ppr/src/libttf/endian.c
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
#include "libttf_private.h"

/*
** Endian conversion routines
**
** These routines take a BYTE pointer and return a value formed by reading
** bytes starting at that point.  These routines read the big-endian values
** which are used in TrueType font files.
*/

/*
** Get an Unsigned 32 bit number.
*/
ULONG getULONG(BYTE *p)
	{
	int x;
	ULONG val=0;

	for(x=0; x<4; x++)
		{
		val *= 0x100;
		val += p[x];
		}

	return val;
	} /* end of getULONG() */

/*
** Get an unsigned 16 bit number.
*/
USHORT getUSHORT(BYTE *p)
	{
	int x;
	USHORT val=0;

	for(x=0; x<2; x++)
		{
		val *= 0x100;
		val += p[x];
		}

	return val;
	} /* end of getUSHORT() */

/* end of file */

