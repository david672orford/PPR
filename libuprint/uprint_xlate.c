/*
** mouse.trincoll.edu:~ppr/src/libuprint/uprint_xlate.c
** Copyright 1997, 1998, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 24 March 1998.
*/

#include "before_system.h"
#include "gu.h"
#include "global_defines.h"

#include "uprint.h"
#include "uprint_private.h"

struct LP_LPR_TYPE_XLATE lp_lpr_type_xlate[] =
	{
	{ "simple", 'f' },
	{ (const char *)NULL, 'l' },
	{ "pr", 'p' },
	{ "postscript", 'o' },
	{ "fortran", 'r' },
	{ "cif", 'c' },
	{ "plot", 'g' },
	{ "sunras", 'v' },
	{ "troff", 'n' },
	{ "dvi", 'd' },
	{ "otroff", 't' },
	{ "-r", 'x' },		
	{ (const char *)NULL, '\0' }	
	} ;

/* end of file */
