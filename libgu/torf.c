/*
** mouse:~ppr/src/libgu/gu_torf.c
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
** Last modified 14 December 2000.
*/

#include "before_system.h"
#include "gu.h"

/*
** Read a True or False answer and return ANSWER_TRUE, ANSWER_FALSE,
** or ANSWER_UNKNOWN.
*/
int gu_torf(const char *s)
	{
	while( *s == ' ' || *s == '\t' )	/* eat up */
		s++;							/* leading white space */

	switch(*s)
		{
		case 'y':
		case 'Y':
		case 't':
		case 'T':
			return ANSWER_TRUE;
		case 'n':
		case 'N':
		case 'f':
		case 'F':
			return ANSWER_FALSE;
		default:
			return ANSWER_UNKNOWN;
		}
	} /* end of gu_torf() */

/*
** Set a gu_boolean to the answer.  If the answer is not
** true or false, don't change the gu_boolean and return -1.
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
