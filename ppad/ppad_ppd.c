/*
** mouse:~ppr/src/ppad/ppad_ppd.c
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
** Last modified 23 October 2003.
*/

#include "before_system.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "util_exits.h"
#include "ppad.h"

/*
** Passing this stuff piece-by-piece to various functions was a drag.  Here 
** it is wrapped up in a nice tidy package.
*/
struct THE_FACTS
	{
	char *hrDeviceDescr;
	char *product;
	char *version;
	int revision;
	char *pjl_info_id;
	};
	
/*
** List all PPD files which match the indicated criteria.
*/
static int ppd_choices(const char printer[], struct THE_FACTS *facts)
	{
	FILE *f;
	const char filename[] = VAR_SPOOL_PPR"/ppdindex.db";
	char *line = NULL;
	int line_len = 80;
	char *p, *f_filename, *f_vendor, *f_description, *f_product;
	int count = 0;

	if(!(f = fopen(filename, "r")))
		gu_Throw("Can't open \"%s\", errno=%d (%s)", filename, errno, gu_strerror(errno));

	while((line = gu_getline(line, &line_len, f)))
		{
		if(line[0] == '#')
			continue;

		p = line;
		if(!(f_filename = gu_strsep(&p,":"))
				|| !(f_vendor = gu_strsep(&p,":"))
				|| !(f_description = gu_strsep(&p,":"))
				|| !(f_product = gu_strsep(&p,":"))
				)
			{
			if(!machine_readable)
				fprintf(stderr, "Bad line in \"%s\":\n%s\n", filename, line);
			continue;
			}

		/*printf("Product: %s\n", f_product);*/
		if(    (facts->product && strcmp(f_product, facts->product) == 0)
			|| (facts->hrDeviceDescr && strcmp(f_product, facts->hrDeviceDescr) == 0)
			)
			{
			p = (p = lmatchp(f_filename, PPDDIR"/")) ? p : f_filename;
			if(machine_readable)
				{
				/* This is for the web front end which needs extra fields. */
				printf("%s:%s:%s\n", p, f_vendor, f_description);
				}
			else
				{
				/* We wait to print this until we know that we have at least one match. */
				if(count++ < 1)
					printf("Run one of these commands to select the cooresponding PPD file:\n");

				printf("    ppad ppd %s \"%s\"\n", printer, p);
				}
			}
		}

	fclose(f);

	return count;
	} /* end of ppd_choices() */

/*
** Interface program probe query
**
** This function invokes the printer interface program with the --probe 
** option.  Hopefully it will be able to perform some interface-specific
** probing in order to obtain information about the printer.
*/
static int ppd_query_interface_probe(const char printer[], struct QUERY *q, struct THE_FACTS *facts)
	{
	int retval = 0;

	gu_Try
		{
		if(!machine_readable)
			printf("Connecting for interface program probe...\n");
		query_connect(q, TRUE);

		gu_Try
			{
			char *line, *p;
			gu_boolean is_stderr;
			int timeout = 10;

			if(!machine_readable)
				printf("Reading response...\n");
			while((line = query_getline(q, &is_stderr, timeout)))
				{
				if(!is_stderr && (p = lmatchp(line, "PROBE:")))
					{
					char *f1, *f2;

					if((f1 = gu_strsep(&p, "=")) && (f2 = gu_strsep(&p, "=")))
						{
						if(!machine_readable)
							printf("    %s: \"%s\"\n", f1, f2);

						if(strcmp(f1, "hrDeviceDescr") == 0)
							{
							if(!facts->hrDeviceDescr)
								{
								facts->hrDeviceDescr = gu_strdup(f2);
								retval = 1;
								}
							}
						else if(strcmp(f1, "Product") == 0)
							{
							if(!facts->product)
								{
								facts->product = gu_strdup(f2);
								retval = 1;
								}
							}
						else if(strcmp(f1, "Version") == 0)
							{
							if(!facts->version)
								{
								facts->version = gu_strdup(f2);
								/* retval = 1; */
								}
							}
						else if(strcmp(f1, "Revision") == 0)
							{
							if(!facts->revision)
								{
								facts->revision = atoi(f2);
								/* retval = 1; */
								}
							}
						}

					timeout = 60;
					continue;
					}

				printf("    %s\n", line);
				}
			}
		gu_Final
			{
			if(!machine_readable)
				printf("Disconnecting...\n");
			query_disconnect(q);
			}
		gu_Catch
			{
			gu_ReThrow();
			}
		}
	gu_Catch
		{
		fprintf(stderr, "Query failed: %s\n", gu_exception);
		return 0;
		}

	return retval;
	} /* end of ppd_query_interface_probe() */

/*
** PJL query
*/
static int ppd_query_pjl(const char printer[], struct QUERY *q, struct THE_FACTS *facts)
	{
	int retval = 0;

	gu_Try
		{
		if(!machine_readable)
			printf("Connecting for PJL query...\n");
		query_connect(q, FALSE);

		gu_Try
			{
			char *p;
			gu_boolean is_stderr;
			int timeout = 10;

			if(!machine_readable)
				printf("Sending PJL query...\n");
			query_puts(q,	"\033%-12345X"
					"@PJL PJL Query to determine printer type\r\n"
					"@PJL INFO ID\r\n");

			if(!machine_readable)
				printf("Reading response...\n");
			while((p = query_getline(q, &is_stderr, timeout)))
				{
				timeout = 60;

				if(strcmp(p, "\f") == 0)
					break;

				/*printf("%s%s\n", is_stderr ? "stderr: " : "", p);*/

				if(p[0] == '"' && !facts->pjl_info_id)
					{
					p++;
					facts->pjl_info_id = gu_strndup(p, strcspn(p, "\""));

					if(!machine_readable)
						printf("    INFO ID: \"%s\"\n", facts->pjl_info_id);

					retval = 1;
					}
				}
			}
		gu_Final
			{
			if(!machine_readable)
				printf("Disconnecting...\n");

			/* This is PCL Universal Exit Langauge. */
			query_puts(q,	"\033%-12345X");

			query_disconnect(q);
			}
		gu_Catch
			{
			gu_ReThrow();
			}
		}
	gu_Catch
		{
		fprintf(stderr, "Query failed: %s\n", gu_exception);
		return 0;
		}

	return retval;
	} /* end of ppd_query_pjl() */

/*
** PostScript query
*/
static int ppd_query_postscript(const char printer[], struct QUERY *q, struct THE_FACTS *facts)
	{
	const char *result_labels[] = {"Revision", "Version", "Product"};
	char *results[] = {NULL, NULL, NULL};
	char *p;

	gu_Try
		{
		if(!machine_readable)
			printf("Connecting for PostScript query...\n");
		query_connect(q, FALSE);

		gu_Try
			{
			int i;
			gu_boolean is_stderr;

			if(!machine_readable)
				printf("Sending PostScript query...\n");
			query_sendquery(q,
				"Printer",			/* DSC defined name (NULL if not defined in DSC) */
				NULL,				/* Ad-hoc name if above is NULL */
				"spooler",			/* default response */
				"statusdict begin revision == version == product == flush end\n"
				);

			if(!machine_readable)
				printf("Reading response...\n");
			for(i=0; i < 3 && (p = query_getline(q, &is_stderr, 30)); )
				{
				/*printf("%s%s\n", is_stderr ? "stderr: " : "", p);*/

				if(is_stderr)
					continue;

				if(strcmp(p, "spooler") == 0)
					gu_Throw("supposed printer is actually a spooler");

				results[i] = gu_strdup(p);

				i++;
				}

			if(i != 3)
				gu_Throw("not enough response lines");

			if(!machine_readable)
				{
				for(i=0; i < 3; i++)
					printf("    %s: %s\n", result_labels[i], results[i]);
				}

			p = results[2];
			if(p[0] == '(')
				{
				p++;
				p[strcspn(p, ")")] = '\0';
				}
			facts->product = gu_strdup(p);
			gu_free(results[2]);

			facts->version = results[1];
			facts->revision = atoi(results[0]);
			}
		gu_Final
			{
			if(!machine_readable)
				printf("Disconnecting...\n");
			query_disconnect(q);
			}
		gu_Catch
			{
			int i;						/* clean this up right way so as to avoid red herrings */
			for(i=0; i < 3; i++)
				{
				if(results[i])
					gu_free(results[i]);
				}
			gu_ReThrow();
			}
		}
	gu_Catch
		{
		fprintf(stderr, "Query failed: %s\n", gu_exception);
		return 0;
		}

	return 1;
	} /* end of ppd_query_postscript() */

/*
** This is called from ppad_printer.c:printer_ppdq() and ppd_query().
*/
int ppd_query_core(const char printer[], struct QUERY *q)
    {
	struct THE_FACTS facts;
	int total_answers = 0;
	int matches = 0;

	facts.hrDeviceDescr = NULL;
	facts.product = NULL;
	facts.version = NULL;
	facts.revision = 0;
	facts.pjl_info_id = NULL;

	total_answers += ppd_query_interface_probe(printer, q, &facts);
	if(!machine_readable)
		PUTS("\n");

	total_answers += ppd_query_postscript(printer, q, &facts);
	if(!machine_readable)
		PUTS("\n");

	if(total_answers < 1)
		{
		total_answers += ppd_query_pjl(printer, q, &facts);
		if(!machine_readable)
			PUTS("\n");
		}

	if(total_answers > 0)
		{
		matches = ppd_choices(printer, &facts);
		}

	if(facts.hrDeviceDescr)
		gu_free(facts.hrDeviceDescr);
	if(facts.product)
		gu_free(facts.product);
	if(facts.version)
		gu_free(facts.version);
	if(facts.pjl_info_id)
		gu_free(facts.pjl_info_id);

	if(matches < 1)
		{
		if(!machine_readable)
			printf("No PPD files matched.\n");
		return EXIT_NOTFOUND;
		}

	return EXIT_OK;
	} /* end of ppd_query_core() */

/*
** ppad query
**
** Send a query to a printer using a specified interface and address and
** produce a list of suitable PPD files.
*/
int ppd_query(const char *argv[])
	{
	const char *interface, *address, *options;
	struct QUERY *q = NULL;
	int ret = EXIT_OK;

	if(!(interface = argv[0]) || !(address = argv[1]) || ((options = argv[2]) && argv[3]))
		{
		fputs(
			_("You must supply the name of an interface and an address.  If necessary, a\n"
			  "quoted list of options may follow the address.\n"),
			errors
			);
		return EXIT_SYNTAX;
		}

	gu_Try
		{
		/* Create an object from the printer's configuration. */
		q = query_new_byaddress(interface, address, options);

		/* Now call the function that does the real work. */
		ret = ppd_query_core("<printer>", q);

		query_delete(q);
		}
	gu_Catch
		{
		fprintf(stderr, _("Query failed: %s\n"), gu_exception);
		return EXIT_INTERNAL;
		}

	return ret;
	} /* end of ppd_query */

/* end of file */
