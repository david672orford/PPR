/*
** mouse:~ppr/src/ipp/ipp.c
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
** Last modified 21 November 2000.
*/

#include "before_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "gu.h"
#include "global_defines.h"

#include "ipp_utils.h"
#include "ipp_constants.h"
#include "ipp_except.h"

struct exception_context the_exception_context[1];

int main(int argc, char *argv[])
    {
    const char *e;

    Try {
	char *p;			/* for work */
	int content_length;
	char *path_info;
	char *buffer;

	/* Do basic input validation */
	if(!(p = getenv("REQUEST_METHOD")) || strcmp(p, "POST") != 0)
	    Throw("REQUEST_METHOD is not POST");
	if(!(p = getenv("CONTENT_TYPE")) || strcmp(p, "application/ipp") != 0)
	    Throw("CONTENT_TYPE is not application/ipp");
	if(!(path_info = getenv("PATH_INFO")) || strlen(path_info) < 1)
	    Throw("PATH_INFO is missing");
	if(!(p = getenv("CONTENT_LENGTH")) || (content_length = atoi(p)) < 0)
	    Throw("CENTENT_LENGTH is missing or invalid");
	if(content_length < 9)
	    Throw("request is too short to be an IPP request");

	debug("request for %s, %d bytes", path_info, content_length);

	/* Allocate a buffer and read the IPP request into it. */
	buffer = gu_alloc(content_length, sizeof(char));
	{
	int len;
	if((len = read(0, buffer, content_length)) == -1)
	    {
	    Throw("read() failed");
	    }
	else if(len != content_length)
	    {
	    Throw("Short read");
	    }
	}

	debug("version-number: %d.%d, operation-id: 0x%.4X, request-id: %d",
		ipp_gsb(buffer), ipp_gsb(buffer + 1),
		ipp_gss(buffer + 2),
		ipp_gsi(buffer + 4)
		);

	switch(ipp_gsi(buffer + 2))
	    {


	    }
	}

    Catch(e)
    	{
	fprintf(stderr, "ipp: exception caught: %s\n", e);
	return 1;
    	}

    return 0;
    }

/* end of file */
