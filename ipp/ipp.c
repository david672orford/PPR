/*
** mouse:~ppr/src/ipp/ipp.c
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
** Last modified 15 April 2003.
*/

#include "before_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "ipp_constants.h"
#include "ipp_except.h"
#include "ipp_utils.h"

struct exception_context the_exception_context[1];

static void do_print(struct IPP *ipp)
	{
	ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_CHARSET, "attributes-charset", "us-ascii");
	ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE, "natural-language", "en");
	ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_TEXT, "status-message", "successful-ok");
	
	ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_INTEGER, "job-id", 140);
	ipp_add_string(ipp, IPP_TAG_JOB, IPP_TAG_URI, "job-uri", "http://localhost:15010/cgi-bin/ipp/x/147");
	ipp_add_string(ipp, IPP_TAG_JOB, IPP_TAG_NAME, "job-state", "pending");
	
	
	}

int main(int argc, char *argv[])
	{
	const char *e;
	struct IPP *ipp;

	Try {
		ipp = ipp_new();
		ipp_parse_request(ipp);

		debug("dispatching");
		switch(ipp->operation_id)
			{
			case IPP_PRINT_JOB:
				do_print(ipp);
				break;
				
			default:
				Throw("unsupported operation");
				break;
			}
		
		ipp_send_reply(ipp);
		ipp_delete(ipp);
		}

	Catch(e)
		{
		printf("Content-Type: text/plain\n");
		printf("Status: 500\n");
		printf("\n");
		printf("ipp: exception caught: %s\n", e);
		fprintf(stderr, "ipp: exception caught: %s\n", e);
		return 1;
		}

	return 0;
	}

/* end of file */
