/*
** mouse:~ppr/src/ipp/ipp.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 18 April 2006.
*/

/*
 * This is PPR's IPP (Internet Printer Protocol) server.
 * This is the main module.
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
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
	fflush(stderr);
	} /* end of debug() */


/* Serve up the PPD file for a specified printer.  The URL for printer
 * "smith" will be "ipp://hostname/printers/smith.ppd".
 */
static void send_ppd(const char prnname[])
	{
	void *qip;
	const void *ppd;

	if((qip = queueinfo_new_load_config(QUEUEINFO_PRINTER, prnname))
			&& (ppd = queueinfo_ppdFile(qip))
		)
		{
		void *ppdobj;
		char *line;
		printf("Content-Type: text/plain\n\n");
		ppdobj = ppdobj_new(ppd);
		while((line = ppdobj_readline(ppdobj)))
			printf("%s\n", line);
		ppdobj_free(ppdobj);
		}
	else
		{
		printf("Status: 404 Not Found\n");
		printf("Content-Type: text/plain\n\n");
		printf("\n");
		}

	if(qip)
		queueinfo_free(qip);
	} /* end of send_ppd() */

static const char *setlang(const char language[], const char charset[])
	{
	#ifdef INTERNATIONAL
	char *lang, *ret;

	gu_asprintf(&lang, "%s.%s", language, charset);

	/* Convert "ru-ru" to "ru_RU".  This may be a Linux hack. */
	if(strlen(lang) >= 5 && lang[2] == '-')
		{
		lang[2] = '_';
		lang[3] = toupper(lang[3]);
		lang[4] = toupper(lang[4]);
		}

	ret = setlocale(LC_ALL, lang);

	gu_free(lang);

	return ret;

	/* Dummy implentation */
	#else
	if(strcmp(language, "en-us") == 0 && strcmp(charset, "utf-8") == 0)
		return "OK";
	else
		return NULL;
	#endif
	}

int main(int argc, char *argv[])
	{
	void *our_pool;
	struct IPP *ipp = NULL;
	char *root = NULL;
	
	#ifdef INTERNATIONAL
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
				/* leave off ".ppd" */
				char *prnname = gu_strndup(p, strlen(p) - 4);
				send_ppd(prnname);
				gu_free(prnname);
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
	
		/* For "ipp://localhost/printers/dummy" it script will be "".  For 
		 * "ipp://host/cgi-bin/ipp/printers/dummy" it will be "cgi-bin/ipp".
		 * We want to produce "ipp://localhost" for the former and 
		 * "ipp://localhost/cgi-bin/ipp" for the latter.
		 */
		if(strcmp(port, "631") == 0)
			gu_asprintf(&root, "ipp://%s%s%s", server, strlen(script) > 0 ? "/" : "", script);
		else
			gu_asprintf(&root, "http://%s:%s%s%s", server, port, strlen(script) > 0 ? "/" : "", script);
		}
	
		/* Wrap all of this information up in an IPP object. */
		ipp = ipp_new(root, path_info, content_length, 0, 1);
		#ifdef DEBUG
		ipp_set_debug_level(ipp, 1);
		#endif

		if((p = getenv("REMOTE_USER")) && *p)	/* defined and not empty */
			ipp_set_remote_user(ipp, p);
		if((p = getenv("REMOTE_ADDR")))
			ipp_set_remote_addr(ipp, p);

		ipp_parse_request(ipp);

		/* Simple exception handling block */
		do	{
			ipp_attribute_t *attr1, *attr2;
			
			if(ipp->version_major != 1)
				{
				ipp->response_code = IPP_VERSION_NOT_SUPPORTED;
				break;
				}
			
			/* Charset must be first attribute */
			attr1 = ipp->request_attrs;
			if(!attr1 || attr1->group_tag != IPP_TAG_OPERATION
					|| attr1->value_tag != IPP_TAG_CHARSET 
					|| attr1->num_values != 1
					|| strcmp(attr1->name, "attributes-charset") != 0
				)
				{
				debug("first attribute is not attributes-charset");
				ipp->response_code = IPP_BAD_REQUEST;
				break;
				}

			/* Natural language must be the second */	
			attr2 = attr1->next;
			if(!attr2 || attr2->group_tag != IPP_TAG_OPERATION
					|| attr2->value_tag != IPP_TAG_LANGUAGE
					|| attr2->num_values != 1
					|| strcmp(attr2->name, "attributes-natural-language") != 0
				)
				{
				debug("second attribute is not attributes-charset");
				ipp->response_code = IPP_BAD_REQUEST;
				break;
				}

			/* Do the best we can to accommodate the client's language
			 * and character set requests.
			 */
			{
			const char *language = attr2->values[0].string.text;
			const char *charset = attr1->values[0].string.text;

			if(!setlang(language, charset))
				{
				language = "en-us";
				if(!setlang(language, charset))
					{
					ipp->response_code = IPP_CHARSET;
					ipp->suppress_unsupported = TRUE;
					}
				}

			ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_CHARSET,
				"attributes-charset", charset);
			ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE,
				"attributes-natural-language", language);
			}

			switch(ipp->operation_id)
				{
				case IPP_PRINT_JOB:
					p_handler = ipp_print_job;
					break;
				case IPP_GET_PRINTER_ATTRIBUTES:
					p_handler = ipp_get_printer_attributes;
					break;
				case IPP_GET_JOBS:
					p_handler = ipp_get_jobs;
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
				DEBUG(("invoking handler..."));
				ipp->response_code = IPP_OK;	/* default */
				(*p_handler)(ipp);
				}
			else
				{
				ipp->response_code = IPP_OPERATION_NOT_SUPPORTED;
				}
			} while(FALSE);

		switch(ipp->response_code)
			{
			case IPP_OK:
				ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_TEXT,
					"status-message", "successful-ok");
				break;
			case IPP_OPERATION_NOT_SUPPORTED:
				ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_TEXT,
					"status-message", _("IPP server does not support this IPP operation"));
				break;
			case IPP_CHARSET:
				ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_TEXT,
					"status-message", _("IPP server does not support your chosen character set"));
				break;
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
