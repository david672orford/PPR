/*
** mouse:~ppr/src/libgu/gu_pcre_split.c
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
** Last modified 28 February 2005.
*/

/*! \file 

This module implements regular expression string splitting with the results
returned as a PCA.
  
*/

#include "config.h"
#include <string.h>
#include "gu.h"
#include "pcre.h"

/** Split a string into an array using pattern[] as the separator
 */
void *gu_pcre_split(const char pattern[], const char string[])
	{
	const char function[] = "gu_pcre_split";
	pcre *compiled_pattern;
	pcre_extra *compiled_pattern_study;
	void *list = NULL;
	int iii = 0;
	int ovector[3];
	int match_count;

	pcre_malloc = gu_malloc;
	pcre_free = gu_free;

	{
	const char *errptr;
	int erroffset;
	if(!(compiled_pattern = pcre_compile(pattern, 0, &errptr, &erroffset, NULL)))
		gu_Throw("%s(): pcre_compile() failed, error \"%s\" at offset %d", function, errptr, erroffset);
	#if 1
	compiled_pattern_study = pcre_study(compiled_pattern, 0, &errptr);
	if(errptr)
		gu_Throw("%s(): pcre_study() failed, error \"%s\"", function, errptr);
	#else
	compiled_pattern_study = NULL;
	#endif
	}

	list = gu_pca_new(16, 32);

	gu_Try
		{
		while((match_count = pcre_exec(compiled_pattern, compiled_pattern_study, string, strlen(string), iii, 0, ovector, sizeof(ovector)/sizeof(ovector[0]))) == 1)
			{
			gu_pca_push(list, gu_strndup(&string[iii], ovector[0] - iii));
			iii = ovector[1];
			}
		if(match_count != PCRE_ERROR_NOMATCH)
			{
			if(match_count == 0)
				{
				gu_Throw("%s(): attempt to extract substrings", function);
				}
			else
				{
				gu_Throw("%s(): pcre_execute() failed, error %d", function, match_count);
				}
			}
		gu_pca_push(list, gu_strdup(&string[iii]));
		}
	gu_Final
		{
		gu_free(compiled_pattern);
		if(compiled_pattern_study)
			gu_free(compiled_pattern_study);
		}
	gu_Catch
		{
		char *p;
		while((p = gu_pca_shift(list)))
			gu_free(p);
		gu_pca_free(list);
		gu_ReThrow();
		}
	
	return list;
	}

/* gcc -Wall -I../include -DTEST -o gu_pcre_split gu_pcre_split.c ../libgu.a */
#ifdef TEST
#include <stdio.h>
void print(void *array)
	{
	char *p;
	int count = 0;
	if(!array)
		{
		printf("No matches\n");
		return;
		}
	while((p = gu_pca_shift(array)))
		{
		if(count++)
			printf(", ");
		printf("\"%s\"", p);
		gu_free(p);
		}
	printf("\n");
	gu_pca_free(array);
	}
int main(int argc, char *argv[])
	{
	void *list;
	int iii;
	
	list = gu_pcre_split("\\s+", "Now is the\tTIME  for all good");
	print(list);

	list = gu_pcre_split("\\t", "Now is the\t\tTIME for all good");
	print(list);

	for(iii=0; iii < 10000; iii++)
		{
		list = gu_pcre_split("\\s", "Now is the TIME for all good men to come to the aid of the party.");
		print(list);
		}

	return 0;
	}
#endif

/* end of file */
