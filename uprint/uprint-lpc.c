/*
** mouse:~ppr/src/uprint/uprint-lpc.c
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
** Last modified 22 September 2004.
*/

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "uprint.h"

/*
** This program is a wrapper that emulates lpc from BSD Unix.
**
** It needs work!
*/
static const char myname[] = "uprint-lpc";

void uprint_error_callback(const char *format, ...)
	{
	va_list va;
	fprintf(stderr, "%s: ", myname);
	va_start(va, format);
	vfprintf(stderr, format, va);
	fputc('\n', stderr);
	va_end(va);
	} /* end of uprint_error_callback() */

/*

This function implements "lpc status".  Here is an output sample
from True64 Unix.

hpjet:
	printer is on remote host localhost with name hpjet
	queuing is enabled
	printing is enabled
	12 entries in spool area
	Tue Sep  7 14:45:02 2004: waiting for queue to be enabled on localhost
ecsprn:
	printer is on remote host printers.trincoll.edu with name ecsprn
	queuing is enabled
	printing is enabled
	no entries
	no daemon present

*/
static int lpc_status(char *argv[])
	{
	{
	FILE *f;
	if((f = fopen(UPRINTREMOTECONF, "r")) != (FILE*)NULL)
		{
		char *line = NULL;
		int line_available = 80;
		while((line = gu_getline(line, &line_available, f)))
			{
			if(line[0] == '[')
				{
				line[strcspn(line, "]")] = '\0';
				if(strchr(line+1, '?') || strchr(line+1, '*'))
					continue;
				printf("%s:\n", line + 1);
				printf("\tqueuing is enabled\n");
				}
			}
		fclose(f);
		}
	}

	/*
	** Chug through the PPR printers, printing dummy acceptance messages as
	** (again with fake times) we go.
	*/
	{
	DIR *dir;
	struct dirent *direntp;
	int len;
	if((dir = opendir(PRCONF)))
		{
		while((direntp = readdir(dir)))
			{
			/* Skip . and .. and hidden files. */
			if(direntp->d_name[0] == '.')
				continue;

			/* Skip Emacs style backup files. */
			len = strlen(direntp->d_name);
			if( len > 0 && direntp->d_name[len-1]=='~' )
				continue;

			printf("%s:\n", direntp->d_name);
			printf("\tqueuing is enabled\n");
			}
		}
	}

	return 0;
	}

int main(int argc, char *argv[])
	{
	int c;

	/* Initialize international messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	/* Trap loops: */
	if(uprint_loop_check(myname) == -1)
		return 1;

	/*
	** Parse the switches.  Note that we don't actually _do_ anything yet.
	** We want to make sure we understand the whole line.  If we don't
	** bail out or (if we have real-lpstat), exec() real-lpstat.
	*/
	while((c = getopt(argc, argv, "a")) != -1)
		{
		switch(c)
			{
			case 'a':
				break;
			}
		}

	if(optind < argc)
		{
		if(strcmp(argv[optind], "status") == 0)
			{
			return lpc_status(&argv[optind + 1]);
			}
		else
			{
			fprintf(stderr, "Subcommand %s not implemented.\n", argv[optind]);
			return 1;
			}
		}
	
	return 0;
	} /* end of main() */

/* end of file */
