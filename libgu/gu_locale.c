/*
** mouse:~ppr/src/libgu/gu_locale.c
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
** Last modified 28 September 2005.
*/

#include "config.h"
#include <string.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#include <iconv.h>
#include <langinfo.h>
#endif
#include "gu.h"

/*! \file
	\brief Locale and character set support functions

*/

/** perform locale initialization
*/
void gu_locale_init(int argc, char *argv[], const char *domainname, const char *localedir)
	{
	#ifdef INTERNATIONAL
	if(!setlocale(LC_ALL, ""))
		fprintf(stderr, "%s: can't set requested locale\n", argv[0] ? argv[0] : "?");

	bindtextdomain(domainname, localedir);

	if(!bind_textdomain_codeset(domainname, "utf-8"))
		fprintf(stderr, "%s: bind_textdomain_codeset() failed\n", argv[0] ? argv [0] : "?");
	
	textdomain(domainname);
	#endif
	}

/** copy a string while converting to UTF-8
 */
static void gu_utf8_locale_copy(char *dst, size_t dst_size, const char *src)
	{
	#if 0
	iconv_t state = iconv_open("utf-8", nl_langinfo(CODESET));
	char *inbuf = (char*)src;		/* iconv() won't modify it */
	size_t inbytesleft = strlen(src);
	char *outbuf = dst;
	size_t outbytesleft = dst_size - 1;
	iconv(state, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
	*outbuf = '\0';
	iconv_close(state);
	#else
	gu_strlcpy(dst, src, dst_size);
	#endif
	}

size_t gu_utf8_strftime(char *s, size_t max, const char *format, const struct tm *tm)
	{
	char *buffer = gu_alloc(max, sizeof(char));
	int retval = strftime(buffer, max, format, tm);
	if(retval != 0)
		gu_utf8_locale_copy(s, max, buffer);
	gu_free(buffer);
	return retval;
	}

const char *gu_utf8_getenv(const char name[])
	{
	return getenv(name);
	}

/* end of file */
