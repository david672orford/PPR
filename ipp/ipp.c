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
** Last modified 7 April 2003.
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

int main(int argc, char *argv[])
	{
	const char *e;

	Try {
		char *p;
		int content_length;
		char *path_info;
		struct IPP *ipp;
		int version_major, version_minor;
		int operation_id, request_id;
		int tag, delimiter_tag = 0, value_tag, name_length, value_length;
		char *name;

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

		ipp = ipp_new(content_length);

		version_major = ipp_get_sb(ipp);
		version_minor = ipp_get_sb(ipp);
		operation_id = ipp_get_ss(ipp);
		request_id = ipp_get_si(ipp);

		debug("version-number: %d.%d, operation-id: 0x%.4X, request-id: %d",
				version_major, version_minor, operation_id, request_id
				);

		while((tag = ipp_get_byte(ipp)) != IPP_TAG_END)
			{
			if(tag >= 0x00 && tag <= 0x0f)
				{
				delimiter_tag = tag;
				}
			else if(tag >= 0x10 && tag <= 0xff)
				{
				value_tag = tag;

				name_length = ipp_get_ss(ipp);
				name = ipp_get_bytes(ipp, name_length);
				debug("0x%.2x 0x%.2x name[%d]=\"%s\"", delimiter_tag, value_tag, name_length, name);

				value_length = ipp_get_ss(ipp);
				switch(value_tag)
					{
					case IPP_TAG_INTEGER:
						debug("    integer[%d]=%d", value_length, ipp_get_si(ipp));
						break;
					case IPP_TAG_NAME:
						debug("    nameWithoutLanguage[%d]=\"%s\"", value_length, ipp_get_bytes(ipp, value_length));
						break;
					case IPP_TAG_URI:
						debug("    uri[%d]=\"%s\"", value_length, ipp_get_bytes(ipp, value_length));
						break;
					case IPP_TAG_KEYWORD:
						debug("    keyword[%d]=\"%s\"", value_length, ipp_get_bytes(ipp, value_length));
						break;
					case IPP_TAG_CHARSET:
						debug("    charset[%d]=\"%s\"", value_length, ipp_get_bytes(ipp, value_length));
						break;
					case IPP_TAG_LANGUAGE:
						debug("    naturalLanguage[%d]=\"%s\"", value_length, ipp_get_bytes(ipp, value_length));
						break;

					default:
						ipp_get_bytes(ipp, value_length);
						debug("    ?????????[%d]", value_length);
						break;
					}
				}
			else
				{
				Throw("invalid tag value");
				}
			}




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
