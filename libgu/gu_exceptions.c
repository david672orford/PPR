/*
** mouse:~ppr/src/libgu/gu_exceptions.c
** Copyright 1995--2007, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 31 May 2007.
*/

/*! \file
 *	\brief Exception Handling
 *
 * Here is the pattern of use:
 *
 *	char *p = gu_strdup("hello");
 *	gu_Try
 *		{
 *		gu_Throw("error 5");
 *		}
 *	gu_Final
 *		{
 *		gu_free(p);
 *		}
 *	gu_Catch
 *		{
 *		fprintf(stderr, "I caught error \"%s\", ouch!\n", gu_exception);
 *		}
*/

#include "config.h"
#include <stdlib.h>
#include "gu.h"

#define GU_EXCEPTION_MAX_TRY_DEPTH 25

/* our globals */
int gu_exception_code;
char gu_exception[256];				/* text of last exception message */
int gu_exception_try_depth;			/* number of gu_Try statements active */
int gu_exception_temp;
int gu_exception_debug = 0;

/* our list of active gu_Try contexts */
static jmp_buf *gu_exception_jmp_bufs[GU_EXCEPTION_MAX_TRY_DEPTH];

/** Start a block that catches exceptions

This function is called by the gu_Try() macro.  

*/
void gu_Try_funct(jmp_buf *p_jmp_buf)
	{
	if(gu_exception_try_depth >= GU_EXCEPTION_MAX_TRY_DEPTH)
		gu_Throw("gu_Try nested too deep");
	gu_exception_jmp_bufs[gu_exception_try_depth++] = p_jmp_buf;
	}

/** Throw an exception as a printf()-style formated string

When throwing an exception, one specifies a code number (which is passed to exit() if
the exception is not caught) and an error message.  The error message is a printf()-style
format and arguments.

This function calls longjmp() if there is a gu_Try() context, otherwise
it prints the message on stderr and calls exit(255).

*/
void gu_Throw(const char message[], ...)
	{
	va_list va;
	char temp[256];
	gu_exception_code = -1;
	va_start(va, message);
	gu_vsnprintf(temp, sizeof(temp), message, va);
	va_end(va);
	gu_strlcpy(gu_exception, temp, sizeof(gu_exception));
	gu_ReThrow();
	}

void gu_CodeThrow(int code, const char message[], ...)
	{
	va_list va;
	char temp[100];
	gu_exception_code = code;
	va_start(va, message);
	gu_vsnprintf(temp, sizeof(temp), message, va);
	va_end(va);
	gu_strlcpy(gu_exception, temp, sizeof(gu_exception));
	gu_ReThrow();
	}
	
/** Re-throw The last exception
 *
 * This function throws a new exception using the exception message string
 * stored by the last call to gu_Throw().
 *
 * This is intended to be called from within a gu_Catch block in order to pass
 * the exception higher up the call stack.  It also is used to do the actually
 * throwing for gu_Throw().
 */
void gu_ReThrow(void)
	{
	if(gu_exception_debug > 0)
		fprintf(stderr, "Throwing exception at try depth %d: %s\n", gu_exception_try_depth, gu_exception);		

	if(gu_exception_try_depth < 1)		/* if not inside a gu_Try(), */
		{
		fprintf(stderr, "Fatal: %s\n", gu_exception);
		exit(255);
		}
	else								/* jump back to context saved by last gu_Try() */
		{
		longjmp(*(gu_exception_jmp_bufs[gu_exception_try_depth - 1]), 1);
		}
	}

/* end of file */
