/*
** mouse:~ppr/src/templates/module.c
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
** Last modified 16 October 2003.
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
** List all PPD files which match the indicated criteria.
*/
static void ppd_choices(const char printer[], const char product[], const char version[], int revision, const char pjl_info_id[])
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

		/*printf("X: %s vs. %s\n", f_product, product);*/
		if(strcmp(f_product, product) == 0)
			{
			p = (p = lmatchp(f_filename, PPDDIR"/")) ? p : f_filename;
			if(machine_readable)
				{
				printf("\"%s\"\n", p);
				}
			else
				{
				if(count++ < 1)
					printf("Run one of these commands to select the cooresponding PPD file:\n");
				printf("    ppad %s \"%s\"\n", printer, p);
				}
			}
		}

	fclose(f);
	}

/*
** Interface program probe query
*/
static int ppd_ppdq_interface_probe(const char printer[], struct QUERY *q)
	{
	gu_Try
		{
		if(!machine_readable)
			printf("Connecting for interface program probe...\n");
		query_connect(q, TRUE);

		gu_Try
			{
			char *p;
			gu_boolean is_stderr;
			int timeout = 10;

			if(!machine_readable)
				printf("Reading response...\n");
			while((p = query_getline(q, &is_stderr, timeout)))
				{
				printf("%s%s\n", is_stderr ? "stderr: " : "", p);
				timeout = 60;
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

	return 1;
	}
/*
** PJL query
*/
static int ppd_ppdq_pjl(const char printer[], struct QUERY *q, char **pjl_info_id)
	{
	gu_Try
		{
		if(!machine_readable)
			printf("Connecting for PJL query...\n");
		query_connect(q, FALSE);

		gu_Try
			{
			char *p;
			gu_boolean is_stderr;

			if(!machine_readable)
				printf("Sending PJL query...\n");
			query_puts(q,	"\033%-12345X"
					"@PJL PJL Query to determine printer type\r\n"
					"@PJL INFO ID\r\n");

			if(!machine_readable)
				printf("Reading response...\n");
			while((p = query_getline(q, &is_stderr, 60)))
				{
				if(strcmp(p, "\f") == 0)
					break;

				/*printf("%s%s\n", is_stderr ? "stderr: " : "", p);*/

				if(p[0] == '"' && !*pjl_info_id)
					{
					p++;
					*pjl_info_id = gu_strndup(p, strcspn(p, "\""));

					if(!machine_readable)
						printf("    Printer Model: %s\n", *pjl_info_id);
					}
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

	return 1;
	}

/*
** PostScript query
*/
static int ppd_ppdq_postscript(const char printer[], struct QUERY *q, char **product, char **version, int *revision)
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
			for(i=0; i < 3 && (p = query_getline(q, &is_stderr, 10)); )
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
			*product = gu_strdup(p);
			gu_free(results[2]);

			*version = results[1];
			*revision = atoi(results[0]);
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
	} /* end of ppd_ppdq_postscript() */

int ppd_ppdq(const char printer[], struct QUERY *q)
    {
	char *product = NULL;
	char *version = NULL;
	int revision = 0;
	char *pjl_info_id = NULL;
	int total_answers = 0;

	total_answers += ppd_ppdq_interface_probe(printer, q);
	if(!machine_readable)
		PUTS("\n");

	total_answers += ppd_ppdq_pjl(printer, q, &pjl_info_id);
	if(!machine_readable)
		PUTS("\n");

	total_answers += ppd_ppdq_postscript(printer, q, &product, &version, &revision);
	if(!machine_readable)
		PUTS("\n");

	if(total_answers > 0)
		ppd_choices(printer, product, version, revision, pjl_info_id);

	if(product)
		gu_free(product);
	if(version)
		gu_free(version);
	if(pjl_info_id)
		gu_free(pjl_info_id);

	if(total_answers < 1)
		{
		if(!machine_readable)
			printf("No PPD files matched.\n");	
		return EXIT_NOTFOUND;
		}

	return EXIT_OK;
	} /* end of ppd_ppdq() */

/*
** Send a query to a printer using a specified interface and address and 
** produce a list of suitable PPD files.
*/
int ppd_query(const char *argv[])
	{
	const char *interface, *address;
	struct QUERY *q = NULL;
	int ret = EXIT_OK;

	if(!(interface = argv[0]) || !(address = argv[1]) || argv[2])
		{
		fputs(_("You must supply the name of an interface and an address.\n"), errors);
		return EXIT_SYNTAX;
		}

	gu_Try
		{
		/* Create an object from the printer's configuration. */
		q = query_new_byaddress(interface, address);

		/* Now call the function that does the real work. */
		ret = ppd_ppdq("<printer>", q);

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
