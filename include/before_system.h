/*
** mouse:~ppr/src/include/before_system.h
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 5 July 2000.
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

/* Read that part of the system dependent settings which
   tell use which include files to include. */
#define PASS1
#include "sysdep.h"
#undef PASS1

/* end of file */

