/*
** mouse:~ppr/src/include/before_system.h
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
** Last modified 3 February 2004.
*/

/*
** This file should be included before any system include
** files.  It sets options which may be used to decide
** which system include files to use or instructs the
** system include files to include certain parts of the API.
*/

/* Detect duplicate inclusion. */
#ifdef _INC_BEFORE_SYSTEM
#error Double inclusion of "before_system.h"
#endif
#define _INC_BEFORE_SYSTEM 1

/* GLIBC 2.x must be told how much of its API to declare. */
#define _GNU_SOURCE 1

/* Dietlibc leaves out u_short if we don't set this. */
#ifdef __dietlibc__
#define _BSD_SOURCE 1
#endif

/* Read that part of the system dependent settings which
   tell use which include files to include. */
#define PASS1
#include "sysdep.h"
#undef PASS1

/* end of file */

