/*
** mouse:~ppr/src/ipp/ipp.c
** Copyright 1995--2006, Trinity College Computing Center.
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
** Last modified 12 April 2006.
*/

/*
 * This is PPR's IPP (Internet Printer Protocol) server.
 * This is the main module.
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "ipp_constants.h"
#include "ipp_utils.h"
#include "queueinfo.h"
#include "ipp.h"

#ifdef DEBUG
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#endif

/** Send a debug message to the HTTP server's error log

This function sends a message to stderr.  Messages sent to stderr end up in
the HTTP server's error log.  The function takes a printf() style format
string and argument list.  The marker "ipp: " is prepended to the message.

This function is defined in ipp_utils.h.  It is a callback function 
from the IPP library.

*/
void debug(const char message[], ...)
	{
	va_list va;
	va_start(va, message);
	fputs("ipp: ", stderr);
	vfprintf(stderr, message, va);
	fputc('\n', stderr);
	va_end(va);
	} /* end of debug() */

/** validate a request, set an error if it is bad
*/
static gu_boolean ipp_validate_request(struct IPP *ipp)
	{
	/* ipp_attribute_t *attr; */

	/* For now, English is all we are capable of. */
	ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_CHARSET,
		"attributes-charset", "utf-8");
	ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE,
		"attributes-natural-language", "en-us");

/*	if(!(attr = ipp_find_attribute(ipp, IPP_TAG_OPERATION, IPP_TAG_CHARSET, "attributes-charset")))
*/		

	return TRUE;
	}

static void send_ppd(const char name[])
	{
	char *queue = gu_strndup(name, strlen(name) - 4);	/* leave off ".ppd" */
	void *qip = queueinfo_new_load_config(QUEUEINFO_SEARCH, queue);
	const void *ppd = queueinfo_ppdFile(qip);
	if(ppd)
		{
		void *ppdobj;
		char *line;
		printf("Content-Type: text/plain\n\n");
		ppdobj = ppdobj_new(ppd);
		while((line = ppdobj_readline(ppdobj)))
			{
			printf("%s\n", line);
			}
		ppdobj_free(ppdobj);
		}
	else
		{
		printf("Status: 404 Not Found\n");
		printf("Content-Type: text/plain\n\n");
		printf("\n");
		}
	queueinfo_free(qip);
	gu_free_if(queue);
	} /* end of send_ppd() */

int main(int argc, char *argv[])
	{
	void *our_pool;
	struct IPP *ipp = NULL;
	char *root = NULL;
	
	/* Initialize international messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	{
	const char *p;
	if((p = getenv("REQUEST_METHOD")) && strcmp(p, "GET") == 0)
		{
		const char *path_info;
		if((path_info = getenv("PATH_INFO")))
		   	{
			/* This is for web browsers. */
			if(strcmp(path_info, "/") == 0)
				{
				fputs("Location: /index.html\n"
				      "Content-Length: 0\n"
				      "\n", stdout);
				return 0;
				}
			/* This is for CUPS PPD downloading. */
			if((p = lmatchp(path_info, "/printers/")) && gu_rmatch(p, ".ppd"))
				{
				send_ppd(p);
				return 0;
				}
			}
		}
	}

	/* Start of IPP handling */	
	gu_pool_push((our_pool = gu_pool_new()));
	gu_Try {
		char *p, *path_info;
		int content_length;
		void (*p_handler)(struct IPP *ipp);

		/* Do basic input validation */
		if(!(p = getenv("REQUEST_METHOD")) || strcmp(p, "POST") != 0)
			gu_Throw("REQUEST_METHOD is not POST");
		if(!(p = getenv("CONTENT_TYPE")) || strcmp(p, "application/ipp") != 0)
			gu_Throw("CONTENT_TYPE is not application/ipp");
		if(!(path_info = getenv("PATH_INFO")) || strlen(path_info) < 1)
			gu_Throw("PATH_INFO is missing");
		if(!(p = getenv("CONTENT_LENGTH")) || (content_length = atoi(p)) < 0)
			gu_Throw("CONTENT_LENGTH is missing or invalid");

		/* These CGI variables comprise the full URL of this "script".
		 * We reassemble the URL.
		 */ 
		{
		char *server, *port, *script;

		if(!(server = getenv("SERVER_NAME")))
			gu_Throw("SERVER_NAME is not defined");
		if(!(port = getenv("SERVER_PORT")))
			gu_Throw("SERVER_PORT is not defined");
		if(!(script = getenv("SCRIPT_NAME")))
			gu_Throw("SCRIPT_NAME is not defined");

		debug("server: %s, port: %s, script: %s", server, port, script);
		
		if(strcmp(script, "/") == 0)
			script = "";

		if(strcmp(port, "631") == 0)
			gu_asprintf(&root, "ipp://%s/%s", server, script);
		else
			gu_asprintf(&root, "http://%s:%s/%s", server, port, script);
		}
	
		/* Wrap all of this information up in an IPP object. */
		ipp = ipp_new(root, path_info, content_length, 0, 1);

		if((p = getenv("REMOTE_USER")) && *p)	/* defined and not empty */
			ipp_set_remote_user(ipp, p);
		if((p = getenv("REMOTE_ADDR")))
			ipp_set_remote_addr(ipp, p);

		ipp_parse_request_header(ipp);
		ipp_parse_request_body(ipp);

		DEBUG(("dispatching operation 0x%.4x (%s)", ipp->operation_id, ipp_operation_id_to_str(ipp->operation_id)));
		switch(ipp->operation_id)
			{
			case IPP_PRINT_JOB:
				p_handler = ipp_print_job;
				break;
			case CUPS_GET_DEFAULT:
				p_handler = cups_get_default;
				break;
			case CUPS_GET_DEVICES:
				p_handler = cups_get_devices;
				break;
			case CUPS_GET_PPDS:
				p_handler = cups_get_ppds;
				break;
			case CUPS_ADD_PRINTER:
				p_handler = cups_add_printer;
				break;
			case IPP_GET_PRINTER_ATTRIBUTES:
				p_handler = ipp_get_printer_attributes;
				break;
			case CUPS_GET_CLASSES:
				p_handler = cups_get_classes;
				break;
			case CUPS_GET_PRINTERS:
				p_handler = cups_get_printers;
				break;
			default:
				p_handler = NULL;
				break;
			}

		if(p_handler)		/* if we found a handler function, */
			{
			DEBUG(("handler found"));

			DEBUG(("validating request"));
			if(!ipp_validate_request(ipp))
				{
				DEBUG(("request is invalid"));
				}
			else
				{
				DEBUG(("dispatching"));
				kill(getpid(), SIGSTOP);
				(*p_handler)(ipp);
				if(ipp->response_code == IPP_OK)
					{
					ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_TEXT,
						"status-message", "successful-ok");
					}
				}
			}
		else
			{
			ipp->response_code = IPP_OPERATION_NOT_SUPPORTED;
			ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_TEXT,
				"status-message", "Server does not support this IPP operation.");
			}

		ipp_send_reply(ipp, TRUE);
		}

	gu_Final {
		if(ipp)
			ipp_delete(ipp);
		gu_pool_free(gu_pool_pop(our_pool));
		}

	gu_Catch
		{
		printf("Status: 500\n");
		printf("Content-Type: text/plain\n");
		printf("\n");
		printf("ipp: exception caught: %s\n", gu_exception);
		fprintf(stderr, "ipp: exception caught: %s\n", gu_exception);
		return 1;
		}

	return 0;
	}

/* end of file */
