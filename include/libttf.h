/*
** mouse:~ppr/src/include/libttf.h
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

/* Return type for libttf functions: */
enum TTF_RESULT_TYPE
	{
	TTF_OK,
	TTF_NOTOBJ,
	TTF_NOMEM,
	TTF_BADFREE,
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

