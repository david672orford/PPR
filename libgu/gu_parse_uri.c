/*
** mouse:~ppr/src/libgu/gu_parse_uri.c
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
** Last modified 24 February 2005.
*/

#include "config.h"
#include "gu.h"
#include "vector.h"
#include "pool.h"
#include "pre.h"

int gu_parse_uri(pool callers_pool, struct URI *uri, char uri_string[])
	{
	pcre *uri_pattern;
	vector uri_matches;
	char *p;

	uri_pattern = precomp(callers_pool,
		"^([a-zA-Z]+)://([a-zA-Z0-9\\.-]+)((?:/[^/]+)*?(?:/([^/]*))?)$",
		0);

	if(!(uri_matches = prematch(callers_pool, uri_string, uri_pattern, 0)))
		return -1;

	vector_pop_front(uri_matches, p);	/* discard */
	vector_pop_front(uri_matches, uri->method);
	vector_pop_front(uri_matches, uri->node);
	vector_pop_front(uri_matches, uri->path);
	if(vector_size(uri_matches) > 0)
		vector_pop_front(uri_matches, uri->basename);
	else
		uri->basename[0] = '\0';
	
	return 0;
	}

/* end of file */
