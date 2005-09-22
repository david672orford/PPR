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
** Last modified 1 March 2005.
*/

/*! \file */

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include "gu.h"

/** parse a URI and return its components in a structure
 *
 * This function parses simple URL's into their component parts and fills in a 
 * struct URI.  It will not parse URL's with usernames or passwords.
 */
struct URI *gu_uri_new(const char uri_string[])
	{
	struct URI *uri;
	void *uri_matches;
	char *p;

	uri_matches = gu_pcre_match(
		"^([a-zA-Z]+)://([a-zA-Z0-9\\.-]*)(?::(\\d+))?((?:/[^/]+)*?(?:/([^/\\?]*)))?(?:\\?(.*))?$",
		/* ^method       ^node                 ^port      ^path         ^basename */
		uri_string
		);

	if(!uri_matches)
		return NULL;

	uri = gu_alloc(1, sizeof(struct URI));

	uri->method = gu_pca_shift(uri_matches);
	gu_ascii_strlower(uri->method);

	uri->node = gu_pca_shift(uri_matches);

	if((p = gu_pca_shift(uri_matches)))
		{
		uri->port = atoi(p);
		gu_free(p);
		}
	else if(strcmp(uri->method, "http") == 0)
		uri->port = 80;
	else if(strcmp(uri->method, "ipp") == 0)
		uri->port = 631;
	else
		uri->port = 0;

	uri->path = gu_pca_shift(uri_matches);		/* posibly NULL */
	
	uri->basename = gu_pca_shift(uri_matches);	/* posibly NULL */

	uri->query = gu_pca_shift(uri_matches);		/* posibly NULL */

	gu_pca_free(uri_matches);

	return uri;
	}

/** deallocate a URI object
 */
void gu_uri_free(struct URI *uri)
	{
	gu_free(uri->method);
	gu_free(uri->node);
	gu_free_if(uri->path);
	gu_free_if(uri->basename);
	gu_free_if(uri->query);
	gu_free(uri);
	}

/* gcc -Wall -I../include -DTEST -o gu_uri gu_uri.c */
#ifdef TEST
#include <stdio.h>
void test(const char uri_string[])
	{
	struct URI *uri;
	printf("\"%s\"\n", uri_string);
	if((uri = gu_uri_new(uri_string)))
		{
		printf("URI = {\n");
		printf("    method->\"%s\",\n", uri->method);
		printf("    node->\"%s\",\n", uri->node);
		printf("    port->%d,\n", uri->port);
		printf("    path->\"%s\",\n", uri->path);
		printf("    basename->\"%s\",\n", uri->basename);
		printf("    query->\"%s\",\n", uri->query);
		printf("    }\n");
		gu_uri_free(uri);
		}
	printf("%d blocks\n", gu_alloc_checkpoint());
	printf("\n");
	}
int main(int argc, char *argv[])
	{
	test("http://mouse");
	test("http://mouse:72");
	test("http://mouse:72/x");
	test("http://mouse/x");
	test("file:///bin/ls");
	test("file:/bin/ls");		/* not valid */
	test("ipp://smith.trincoll.edu/cgi-bin/ipp/printers/adshp4m");
	test("http://smith.trincoll.edu/cgi-bin/prn_show.cgi?name=johnprn");
	return 0;
	}
#endif

/* end of file */
