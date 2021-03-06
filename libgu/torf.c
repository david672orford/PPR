/*
** mouse:~ppr/src/libgu/gu_torf.c
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
** Last modified 1 March 2005.
*/

/*! \file
    \brief Parse boolean values
*/

#include "config.h"
#include "gu.h"

/** Parse a boolean value
 *
 * Set a gu_boolean to the answer.  If the answer is not
 * true or false, don't change the gu_boolean and return -1.
 */
int gu_torf_setBOOL(gu_boolean *b, const char *s)
	{
	while( *s == ' ' || *s == '\t' )	/* eat up */
		s++;							/* leading white space */

	switch(*s)
		{
		case 'y':
		case 'Y':
		case 't':
		case 'T':
			*b = TRUE;
			return 0;
		case 'n':
		case 'N':
		case 'f':
		case 'F':
			*b = FALSE;
			return 0;
		default:
			return -1;
		}
	} /* end of gu_torf_setBOOL() */

/* end of file */
