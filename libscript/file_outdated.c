/*
** mouse:~ppr/src/libscript/file_outdated.c
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
** Last modified 26 May 2005.
*/

/*
** This small program is used by cron jobs to determine if they need to
** rebuild certain PPR indexes.  They use it to determine if the index file
** is older than the thing indexed.
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

const char myname[] = "file_outdated";

int main(int argc, char *argv[])
	{
	struct stat statbuf_depending, statbuf_source;
	int x;

	/* The depending name file is mandatory. */
	if(argc < 2)
		{
		fprintf(stderr, "%s: Usage: file_outdated <derived_file> <master_file> ...\n", myname);
		return 1;
		}

	if(stat(argv[1], &statbuf_depending) == -1)
		{
		fprintf(stdout, "%s: Master file \"%s\" does not exist.\n", myname, argv[1]);
		return 0;
		}

	for(x=2; x < argc; x++)
		{
		if(stat(argv[x], &statbuf_source) == -1)
			{
			fprintf(stderr, "%s: Master file \"%s\" does not exist.\n", myname, argv[x]);
			return 0;
			}

		if(statbuf_source.st_mtime >= statbuf_depending.st_mtime)
			{
			/* fprintf(stdout, "%s: Master file \"%s\" is newer than dependent file \"%s\".\n", myname, argv[x], argv[1]); */
			return 0;
			}
		}

	return 1;
	} /* end of main() */

/* end of file */
