/*
** mouse:~ppr/src/libscript/file_outdated.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 30 June 2000.
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
	fprintf(stderr, "%s: Usage: file_outdated <file> <master> ...\n", myname);
	return 1;
	}

    if(stat(argv[1], &statbuf_depending) == -1)
    	{
	fprintf(stdout, "%s: Target file \"%s\" does not exist.\n", myname, argv[1]);
    	return 0;
    	}

    for(x=2; x < argc; x++)
	{
	if(stat(argv[x], &statbuf_source) == -1)
	    {
	    fprintf(stderr, "%s: Source file \"%s\" does not exist.\n", myname, argv[x]);
	    return 0;
	    }

        if(statbuf_source.st_mtime >= statbuf_depending.st_mtime)
	    {
	    fprintf(stdout, "%s: Source file \"%s\" is newer than \"%s\".\n", myname, argv[x], argv[1]);
            return 0;
            }
	}

    return 1;
    } /* end of main() */

/* end of file */
