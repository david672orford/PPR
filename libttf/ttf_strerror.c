/*
** mouse:~ppr/src/libttf/ttf_strerror.c
** Copyright 1998, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 10 November 1998.
*/

#include "libttf_before_system.h"
#include "libttf_private.h"

const char *ttf_strerror(TTF_RESULT result_code)
    {
    switch(result_code)
	{
	case TTF_OK:
	    return "No error";
	case TTF_NOTOBJ:
	    return "Not a TTF object";
	case TTF_NOMEM:
	    return "Can't allocate memory for object";
	case TTF_BADFREE:
	    return "Can't free memory";
	case TTF_CANTOPEN:
	    return "Can't open font file";
	case TTF_TBL_NOTFOUND:
	    return "Table not found";
	case TTF_TBL_CANTSEEK:
	    return "File error seeking to table start";
	case TTF_TBL_CANTREAD:
	    return "File error reading table";
	case TTF_TBL_TOOBIG:
	    return "A table is too big";
	case TTF_REQNAME:
	    return "Required name missing";
	case TTF_UNSUP_LOCA:
	    return "Unsupported 'loca' table format";
	case TTF_UNSUP_GLYF:
	    return "Unsupported 'glyf' table format";
	case TTF_UNSUP_POST:
	    return "Unsupported 'post' table format";
	case TTF_GLYF_BADPAD:
	    return "Glyph with bad padding";
	case TTF_GLYF_CANTREAD:
	    return "File error reading a glyph";
	case TTF_GLYF_SIZEINC:
	    return "Size inconsistency in 'glyf' table";
	case TTF_GLYF_BADFLAGS:
	    return "Invalid flags in a glyph";
	case TTF_LONGPSNAME:
	    return "A glyph's PostScript name is too long for the buffer";

	default:
	    return "Invalid error code";
	}
    } /* end of ttf_strerror() */

/* end of file */

