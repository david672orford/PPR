/*
** mouse:~ppr/src/libgu/gu_strtrim.c
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
** Last modified 12 October 2005.
*/

#include "config.h"
#include <ctype.h>
#include <string.h>
#include "gu.h"

char *gu_strtrim(char *string)
	{
	char *p;
	int len;
	for(p=string; *p && isspace(*p); p++)	/* skip to first non-space */
		{
		}
	len = strlen(p);						/* length excluding leading space */
	if(p > string)							/* If there was leading space, */
		{									/* move the string down to 'cover' it. */
		memmove(string, p, len);
		}
	while(--len >= 0)						/* Move back from the end */
		{									/* replacing trailing space with nulls. */
		if(isspace(string[len]))
			string[len] = '\0';
		}
	return string;
	}

#ifdef TEST
#include <stdio.h>
int main(int argc, char *argv[])
	{
	char test[40];

	strlcpy(test, "  my test string", sizeof(test));
	gu_strtrim(test);
	printf("\"%s\"\n", test);

	strlcpy(test, "my test string  ", sizeof(test));
	gu_strtrim(test);
	printf("\"%s\"\n", test);

	strlcpy(test, "  my test string  ", sizeof(test));
	gu_strtrim(test);
	printf("\"%s\"\n", test);

	return 0;
	}
#endif

/* end of file */