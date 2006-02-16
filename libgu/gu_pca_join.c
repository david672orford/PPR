/*
** mouse:~ppr/src/libgu/gu_pcs_join.c
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

  This module joins a PCA into a PCS.
  
*/

#include "config.h"
#include <string.h>
#include "gu.h"
#include "pcre.h"

/** Join array members into a string
 */
char *gu_pca_join(const char separator[], void *array)
	{
	void *string = gu_pcs_new();
	int iii;
	for(iii=0; iii < gu_pca_size(array); iii++)
		{
		if(iii > 0)
			gu_pcs_append_cstr(&string, separator);
		gu_pcs_append_cstr(&string, gu_pca_index(array,iii));
		}
	return gu_pcs_free_keep_cstr(&string);
	}

/* gcc -Wall -I../include -DTEST -o gu_pca_join gu_pca_join.c ../libgu.a */
#ifdef TEST
#include <stdio.h>
int main(int argc, char *argv[])
	{
	void *list;
	
	list = gu_pcre_split("\\s", "Now is the TIME for all good men to come to the aid of the party.");
	printf("\"%s\"\n", gu_pca_join(",", list));

	return 0;
	}
#endif

/* end of file */
