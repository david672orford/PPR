/*
** mouse:~ppr/src/include/libttf.h
** Copyright 1995--2005, Trinity College Computing Center.
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
** Last modified 9 September 2005.
*/

/* Return type for libttf functions: */
enum TTF_RESULT_TYPE
	{
	TTF_OK,
	TTF_NOTOBJ,
	TTF_CANTOPEN,
	TTF_TBL_NOTFOUND,
	TTF_TBL_CANTSEEK,
	TTF_TBL_CANTREAD,
	TTF_TBL_TOOBIG,
	TTF_REQNAME,
	TTF_UNSUP_LOCA,
	TTF_UNSUP_GLYF,
	TTF_UNSUP_POST,
	TTF_GLYF_BADPAD,
	TTF_GLYF_CANTREAD,
	TTF_GLYF_SIZEINC,
	TTF_GLYF_BADFLAGS,
	TTF_LONGPSNAME

	};
typedef enum TTF_RESULT_TYPE TTF_RESULT;

/* Exported funnctions: */
TTF_RESULT ttf_new(void **pp, const char filename[]);
int ttf_delete(void *p);
TTF_RESULT ttf_errno(void *p);
const char *ttf_strerror(TTF_RESULT result_code);
int ttf_psout(void *p, void (*out_putc)(int c), void (*out_puts)(const char *string), void (*out_printf)(const char *format, ...), int target_type);
char *ttf_get_psname(void *p);

/* end of file */

