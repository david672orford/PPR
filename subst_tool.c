/*
** mouse:~ppr/src/subst_tool.c
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

/*
 * This program is a filter which inserts the values of environment variables at marked
 * locations.  It is designed to emulate autoconf.  The patterns are:
 * 
 * @NAME@
 * 		Inserts the value of NAME, if defined, otherwise program aborts.
 * #undef NAME
 * 		Is replaced with
 * 			#define NAME VALUE
 * 		if NAME is defined, otherwise is left unchanged.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
	{
	int linenum = 0;
	char line[256];
	char *p;

	while(fgets(line, sizeof(line), stdin))
		{
		linenum++;

		if(strncmp(line, "#undef ", 7) == 0)
			{
			char *p2, *value;
			p = line + 7;
			if((p2 = strchr(p, '\n')))
				{
				*p2 = '\0';
				if((value = getenv(p)) && *value)
					{
					if(strspn(value, "-.0123456789") == strlen(value))	/* if numberic */
						printf("#define %s %s\n", p, value);
					else
						printf("#define %s \"%s\"\n", p, value);
					continue;
					}
				else
					{
					*p2 = '\n';
					/* fall thru */
					}
				}
			}

		for(p=line; *p; p++)
			{
			if(*p != '@')
				{
				fputc(*p, stdout);
				}
			else
				{
				char *end = strchr(++p, '@');
				const char *value;
				if(!end)
					{
					fprintf(stderr, "%s: unmatched @ in line %d\n", argv[0], linenum);
					return 1;
					}
				*end = '\0';
				if(!(value = getenv(p)))	
					{
					fprintf(stderr, "%s: no %s in environment\n", argv[0], p);
					return 1;
					}
				fputs(value, stdout);
				p = end;
				}
			}
		}

	return 0;
	}
