/*
** mouse:~ppr/src/ipp/ipp_except.h
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 21 November 2000.
*/

#include "cexcept.h"
define_exception_type(const char *);
extern struct exception_context the_exception_context[1];

/* end of file */
