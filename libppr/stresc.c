/*
** mouse:~ppr/src/libppr/stresc.c
** Copyright 1995--2003, Trinity College Computing Center.
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
** Last modified 28 March 2003.
*/

#include "before_system.h"
#include "gu.h"
#include "global_defines.h"

/*! \file
*/

/** This function is unused for now.

This function takes a string with ANSI C string escape
sequences in it and replaces the escape sequences with
the values they represent.  The length of the resulting
string is returned.

*/
int compile_string_escapes(char *s)
	{
	int len = 0;
	int c;
	char *si, *di;

	si = di = s;

	while((c = *si++))
		{
		if(c == '\\' && (c = *si))
			{
			si++;
			switch(c)
				{
				case 'a': c = '\a'; break;		/* bell */
				case 'b': c = '\b'; break;		/* backspace */
				case 'f': c = '\f'; break;		/* form feed */
				case 'n': c = '\n'; break;		/* newline */
				case 'r': c = '\r'; break;		/* carriage return */
				case 't': c = '\t'; break;		/* horizontal tab */
				case 'v': c = '\v'; break;		/* vertical tab */

				case '\\':		/* backslash */
				case '\'':		/* single quote */
				case '\"':		/* double quote */
				case '?':		/* question mark */
					break;

				case '0':		/* octal */
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
					{
					int digits = 2;		/* plus the one we already have */
					c -= '0';
					while(digits-- && *si && *si >= '0' && *si <= '9')
						{
						c = ( (c << 3) | (*(si++) - '0') );
						}
					}
					break;

				case 'x':		/* hexadecimal */
					{
					int c2;
					int digits = 2;
					c = 0;
					while(digits-- && (c2 = *si)
							&&	(
								((c2 -= '0') >= 0 && c2 <= 9)
								|| ((c2 -= ('A' - '0' - 10)) >= 10 && c2 <= 15)
								|| ((c2 -= ('a' - 'A')) >= 10 && c2 <= 15)
								)
						)
						{
						si++;
						c = ((c << 4) | c2);
						}
					}
					break;
				}
			}

		*(di++) = c;
		len++;
		}

	*di = '\0';

	return len;
	}

#ifdef TEST
#include <stdio.h>
#include <string.h>
int main(int argc, char *argv[])
	{
	char test[256];
	int count;
	int x;

	strcpy(test, "Hello\\t\\tworld!\\n");
	printf("test[] = \"%s\"\n", test);
	count = compile_string_escapes(test);
	printf("count = %d, test[] = \"%s\"\n", count, test);
	printf("%d\n", strcmp(test, "Hello\t\tworld!\n"));

	strcpy(test, "\\a\\b\\f\\n\\r\\t\\v\\\\\\'\\\"\\?");
	printf("test[] = \"%s\"\n", test);
	count = compile_string_escapes(test);
	printf("count = %d, test[] = \"%s\"\n", count, test);
	printf("%d\n", strcmp(test, "\a\b\f\n\r\t\v\\\'\"\?"));

	strcpy(test, "\\001\\01\\1\\011\\11\\111\\2222");
	printf("test[] = \"%s\"\n", test);
	count = compile_string_escapes(test);
	printf("count = %d, test[] = \"%s\"\n", count, test);
	printf("%d\n", strcmp(test, "\001\01\1\011\11\111\2222"));

	strcpy(test, "\\x01\\x1\\x11");
	printf("test[] = \"%s\"\n", test);
	count = compile_string_escapes(test);
	printf("count = %d, test[] = \"%s\"\n", count, test);
	printf("%d\n", strcmp(test, "\x01\x1\x11"));

	/* eroneous hexadecimal */
	strcpy(test, "\\0x41A\\0x42B\\0x43C");
	printf("test[] = \"%s\"\n", test);
	count = compile_string_escapes(test);
	printf("count = %d, test[] = \"%s\"\n", count, test);
	printf("%d\n", strcmp(test, "\0x41A\0x42B\0x43C"));
	for(x=0; x < count; x++) if(test[x] == '\0') test[x] = '?';
	printf("test[] = \"%s\"\n", test);

	strcpy(test, "\\x41\\x42\\x43\\x44\\x45\\x46\\x47\\x48\\x49\\x4a\\x4A\\x4b\\x4B\\x4c\\x4C\\x4d\\x4D\\x4e\\x4E\\x4f\\x4F");
	printf("test[] = \"%s\"\n", test);
	count = compile_string_escapes(test);
	printf("count = %d, test[] = \"%s\"\n", count, test);
	printf("%d\n", strcmp(test, "\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4a\x4A\x4b\x4B\x4c\x4C\x4d\x4D\x4e\x4E\x4f\x4F"));

	/* This may fail on some compilers */
	strcpy(test, "\\x41A\\x42B\\x43C");
	printf("test[] = \"%s\"\n", test);
	count = compile_string_escapes(test);
	printf("count = %d, test[] = \"%s\"\n", count, test);
	printf("%d\n", strcmp(test, "\x41A\x42B\x43C"));

	return 0;
	}
#endif

/* end of file */

