/*
** mouse:~ppr/src/libttf/endian.c
** Copyright 1995, 1996, 1997, 1998, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 5 November 1998.
*/

#include "libttf_before_system.h"
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

