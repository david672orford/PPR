/*
** mouse:~ppr/src/libppr/padded.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 30 December 2000.
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
