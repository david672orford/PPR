/*
** mouse:~ppr/src/libttf/fixed.c
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
** Get a 32 bit fixed point (16.16) number.
** A special structure is used to return the value.
*/
Fixed getFixed(BYTE *s)
	{
	Fixed val={0,0};

	val.whole = ((s[0] * 256) + s[1]);
	val.fraction = ((s[2] * 256) + s[3]);

	return val;
	} /* end of getFixed() */

/* end of file */

