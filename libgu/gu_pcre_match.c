/*
** mouse:~ppr/src/libgu/gu_pcre_match.c
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

This module implements regular expression matching with the results
returned as a PCA.
  
*/

#include "config.h"
#include <string.h>
#include "gu.h"
#include "pcre.h"

/** attempt a regular expression match against a string
 */
void *gu_pcre_match(const char pattern[], const char string[])
	{
	const char function[] = "gu_pcre_match";
	pcre *compiled_pattern;
	void *submatch_list = NULL;
	int ovector[33];
	int match_count;

	pcre_malloc = gu_malloc;
	pcre_free = gu_free;

	{
	const char *errptr;
	int erroffset;
	if(!(compiled_pattern = pcre_compile(pattern, 0, &errptr, &erroffset, NULL)))
		gu_Throw("%s(): pcre_compile() failed, error \"%s\" at offset %d", function, errptr, erroffset);
	}

	gu_Try
		{
		if((match_count = pcre_exec(compiled_pattern, NULL, string, strlen(string), 0, 0, ovector, sizeof(ovector)/sizeof(ovector[0]))) > 0)
			{
			int iii;
			submatch_list = gu_pca_new(10, 0);
			for(iii=1; iii < match_count; iii++)
				{
				if(ovector[iii*2] == -1)
					{
					gu_pca_push(submatch_list, NULL);
					}
				else
					{
					gu_pca_push(
						submatch_list,
						gu_strndup(&string[ovector[iii*2]], ovector[iii*2+1]-ovector[iii*2])
						);
					}
				}
			}
		else if(match_count == 0)
			{
			gu_Throw("%s(): too many extracted substrings", function);
			}
		else if(match_count != PCRE_ERROR_NOMATCH)
			{
			gu_Throw("%s(): pcre_execute() failed, error %d", function, match_count);
			}
		}
	gu_Final
		{
		gu_free(compiled_pattern);
		}
	gu_Catch
		{
		gu_ReThrow();
		}

	return submatch_list;
	}

/* gcc -Wall -I../include -DTEST -o gu_pcre_match gu_pcre_match.c ../libgu.a */
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
	void *matches;
	
	matches = gu_pcre_match("([A-Z]+)", "now is the TIME for all good");
	print(matches);

	matches = gu_pcre_match("^(\\S+) (\\S+) (\\S+) ", "now is the TIME for all good");
	print(matches);

	matches = gu_pcre_match("^(\\S+) (\\S+) (\\S+) ", "nowgood");
	print(matches);

	return 0;
	}
#endif

/* end of file */
