/*
** mouse:~ppr/src/libgu/gu_except.c
** Copyright 1995--2003, Trinity College Computing Center.
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
** Last modified 31 August 2003.
*/

/*! \file
	\brief Exception Handling

*/

#include "before_system.h"
#include <stdlib.h>
#include "gu.h"

#define GU_EXCEPTION_MAX_TRY_DEPTH 25

char gu_exception[100];
static jmp_buf *_gu_exception_jmp_bufs[GU_EXCEPTION_MAX_TRY_DEPTH];
int _gu_exception_try_depth;
int _gu_exception_setjmp_retcode;

/** Get Ready to Try

This function is called by the gu_Try macro.

*/
void _gu_Try(jmp_buf *p_jmp_buf)
	{
	if(_gu_exception_try_depth >= GU_EXCEPTION_MAX_TRY_DEPTH)
		gu_Throw(255, "Try nested too deep");
	_gu_exception_jmp_bufs[_gu_exception_try_depth++] = p_jmp_buf;
	}

/** Throw an Exception

When throwing an exception, one specifies a code number (which is passed to exit() if
the exception is not caught) and an error message.  The error message is a printf()-style
format and arguments.

*/
void gu_Throw(const char message[], ...)
	{
	va_list va;
	va_start(va, message);
	gu_vsnprintf(gu_exception, sizeof(gu_exception), message, va);
	va_end(va);
	gu_ReThrow();
	}
	
/** Re-throw Exception

This is intended to be called from within a gu_Catch block in order to pass the
exception higher up the call stack. 

*/
void gu_ReThrow(void)
	{
	if(_gu_exception_try_depth < 1)
		{
		fprintf(stderr, "Fatal: %s\n", gu_exception);
		exit(255);
		}
	else
		{
		longjmp(*(_gu_exception_jmp_bufs[_gu_exception_try_depth - 1]), 1);
		}
	}

/* end of file */
