/*
** mouse:~ppr/src/libttf/ttf_strerror.c
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
** Last modified 13 December 2004.
*/

#include "config.h"
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

