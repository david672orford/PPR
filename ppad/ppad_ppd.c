/*
** mouse:~ppr/src/ppad/ppad_ppd.c
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
** Last modified 31 August 2005.
*/

#include "config.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "util_exits.h"
#include "ppad.h"

#if 0
#define DEBUG 1
#endif

/*
** Passing this stuff piece-by-piece to various functions was a drag.  Here 
** it is wrapped up in a nice tidy package.
*/
struct THE_FACTS
	{
	char *product;					/* PostScript product string */
	float version;					/* PostScript interpreter version */
	int revision;					/* PostScript interpreter revision */
	char *deviceid_manufacturer;	/* IEEE 1284 */
	char *deviceid_model;			/* IEEE 1284 */
	char *pjl_id;					/* Response to "@PJL INFO ID" */
	char *SNMP_sysDescr;			/* SNMP node description */
	char *SNMP_hrDeviceDescr;		/* SNMP HP printer model */
	};

struct MATCH
	{
	char *manufacturer;
	char *description;
	gu_boolean fuzzy;
	};

/*
 * This function looks for a case-insenstive match in some initial segment of
 * two strings.  If the match is long enough and it represents a sufficient
 * percentage of the longer string, then it is a match.
 */
static gu_boolean very_fuzzy(char *a, char *b)
	{
	#ifdef DEBUG
	printf("very_fuzzy(\"%s\", \"%s\")\n", a, b);
	#endif
	{
	int alen = strlen(a);
	int blen = strlen(b);
	int shortest = alen < blen ? alen : blen;
	int longest = alen > blen ? alen : blen; 
	if(shortest < 8)
		return FALSE;
	if((longest / shortest) > 2)
		return FALSE;
	return (strncasecmp(a,b,shortest) == 0);
	}
	} /* end of very_fuzzy */

/*
** List all PPD files which match the indicated criteria.
*/
static int ppd_choices(struct THE_FACTS *facts, struct MATCH **matches, int matches_count)
	{
	FILE *f;
	char *line = NULL;
	int line_len = 80;
	int linenum = 0;
	char *p;
	char *f_description,				/* probably ModelName, never empty */
		*f_filename,
		*f_manufacturer,						/* Manufacturer, possibly empty */
		*f_modelname,					/* ModelName, possibly empty */
		*f_nickname,					/* NickName, possibly empty */
		*f_shortnickname,				/* ShortNickName, possibly empty */
		*f_product,						/* Product, possibly empty */
		*f_psversion,					/* PSVersion, possibly empty */
		*f_deviceid_manufacturer,
		*f_deviceid_model,
		*f_pjl_id,
		*f_SNMP_sysDescr,
		*f_SNMP_hrDeviceDescr;
	struct {
		char *name;
		char **ptr;
		char *beyond_manufacturer;
		} names[] =
		{
		{"ModelName", &f_modelname, NULL},
		{"NickName", &f_nickname, NULL},
		{"ShortNickName", &f_shortnickname, NULL},
		{"Product", &f_product, NULL},
		{NULL, (char**)NULL}
		};
	int count = 0;

	if(!(f = fopen(PPD_INDEX, "r")))
		gu_Throw("Can't open \"%s\", errno=%d (%s)", PPD_INDEX, errno, gu_strerror(errno));

	while((line = gu_getline(line, &line_len, f)))
		{
		linenum++;

		if(line[0] == '#')
			continue;

		p = line;
		if(!(f_description = gu_strsep(&p,":"))
				|| !(f_filename = gu_strsep(&p,":"))
				|| !(f_manufacturer = gu_strsep(&p,":"))
				|| !(f_modelname = gu_strsep(&p,":"))
				|| !(f_nickname = gu_strsep(&p,":"))
				|| !(f_shortnickname = gu_strsep(&p,":"))
				|| !(f_product = gu_strsep(&p,":"))
				|| !(f_psversion = gu_strsep(&p,":"))
				|| !(f_deviceid_manufacturer = gu_strsep(&p,":"))
				|| !(f_deviceid_model = gu_strsep(&p,":"))
				|| !(f_pjl_id = gu_strsep(&p,":"))
				|| !(f_SNMP_sysDescr = gu_strsep(&p,":"))
				|| !(f_SNMP_hrDeviceDescr = gu_strsep(&p,":"))
				)
			{
			if(!machine_readable)
				fprintf(stderr, "Too few fields at line %d in \"%s\".\n", linenum, PPD_INDEX);
			continue;
			}
		
		{
		int i;
		for(i=0; names[i].name; i++)
			{
			char *name = *(names[i].ptr);
			names[i].beyond_manufacturer = NULL;
			if(*f_manufacturer && strncasecmp(name, f_manufacturer, strlen(f_manufacturer)) == 0
					&& isspace(name[strlen(f_manufacturer)]))
				{
				names[i].beyond_manufacturer = name + strlen(f_manufacturer);
				names[i].beyond_manufacturer += strspn(names[i].beyond_manufacturer, " \t");
				}
			}
		}

		{
		#define defined_and_match(a,b) (a && b && strcmp(a,b)==0)
		#ifdef DEBUG
		#define debug(a) printf("Match:\n\tDescription: \"%s\"\n\tMatched Field: %s\n\n", f_description, a);
		#define debug2(a,b) printf("Match:\n\tDescription: \"%s\"\n\tMatching Condition: %s =~ %s\n\n", f_description, a, b);
		#define debug3(a,b) printf("Match:\n\tDescription: \"%s\"\n\tMatching Condition: %s =~ %s (initial substring)\n\n", f_description, a, b);
		#else
		#define debug(a)
		#define debug2(a,b)
		#define debug3(a,b)
		#endif
		int matched = 0, fuzzy = 0;

		/* Straight match: PostScript Product string */
		if(defined_and_match(facts->product, f_product))
			{
			debug("Product")
			matched++;
			}

		/* Staight match: IEEE 1284 ID strings */
		if(defined_and_match(facts->deviceid_manufacturer, f_deviceid_manufacturer)
			&& defined_and_match(facts->deviceid_model, f_deviceid_model)
			)
			{
			debug("DeviceID MANUFACTURER and DeviceID MODEL")
			matched++;
			}

		/* Straight match: PJL ID string */
		if(defined_and_match(facts->pjl_id, f_pjl_id))
			{
			debug("PJL ID")
			matched++;
			}

		/* Straight match: SNMP sysDescr */
		if(defined_and_match(facts->SNMP_sysDescr, f_SNMP_sysDescr))
			{
			debug("SNMP sysDescr")
			matched++;
			}

		/* Straight match: SNMP hrDeviceDescr */
		if(defined_and_match(facts->SNMP_hrDeviceDescr, f_SNMP_hrDeviceDescr))
			{
			debug("SNMP hrDeviceDescr")
			matched++;
			}
				
		/* If we have the printer's SNMP_hrDeviceDescr, but this PPD file
		 * doesn't say what it should be, try to guess what it should be
		 * using other information in the PPD file.
		 */
		if(facts->SNMP_hrDeviceDescr && !*f_SNMP_hrDeviceDescr)
			{
			int i;
			char *name;
			for(i=0; names[i].name; i++)
				{
				if(!*(name = *(names[i].ptr)))		/* if empty string */
					continue;
				if(strcasecmp(name, facts->SNMP_hrDeviceDescr) == 0)
					{
					debug2("SNMP hrDeviceDescr", names[i].name)
					matched++;
					continue;
					}
				if(very_fuzzy(name, facts->SNMP_hrDeviceDescr))
					{
					debug3("SNMP hrDeviceDescr", names[i].name)
					matched++; fuzzy++;
					continue;
					}
				if((name = names[i].beyond_manufacturer))
					{
					if(strcasecmp(name, facts->SNMP_hrDeviceDescr) == 0)
						{
						debug2("PPD Vendor + SNMP hrDeviceDescr", names[i].name)
						matched++;
						continue;
						}
					if(very_fuzzy(name, facts->SNMP_hrDeviceDescr))
						{
						debug3("PPD Vendor + SNMP hrDeviceDescr", names[i].name)
						matched++; fuzzy++;
						continue;
						}
					}
				}
			} /* SNMP hrDeviceDescr fuzzy */

		/* If we have the printer's PCL ID, but this PPD file doesn't say 
		 * what it should be, try to guess.
		 */ 
		if(facts->pjl_id && !*f_pjl_id)
			{
			int i;
			char *name;
			for(i=0; names[i].name; i++)
				{
				if(!*(name = *(names[i].ptr)))
					continue;
				if(strcasecmp(name, facts->pjl_id) == 0)
					{
					debug2("PJL ID", names[i].name)
					matched++;
					continue;
					}
				if(very_fuzzy(name, facts->pjl_id ))
					{
					debug3("PJL ID", names[i].name)
					matched++; fuzzy++;
					continue;
					}
				if((name = names[i].beyond_manufacturer))
					{
					if(strcasecmp(name, facts->pjl_id) == 0)
						{
						debug2("PPD Vendor + PJL ID", names[i].name)
						matched++;
						continue;
						}
					if(very_fuzzy(name, facts->pjl_id))
						{
						debug3("PPD Vendor + PJL ID", names[i].name)
						matched++; fuzzy++;
						continue;
						}
					}
				}
			} /* PJL ID fuzzy */

		/* If we have the IEEE 1284 device ID but the PPD file doesn't say
		 * what it should be, go guess.
		 */
		if(facts->deviceid_manufacturer && facts->deviceid_model &&
				(!*f_deviceid_manufacturer || !*f_deviceid_model))
			{
			int i;
			char *name;
			for(i=0; names[i].name; i++)
				{
				if(!(name = names[i].beyond_manufacturer))
					continue;
				if(strcasecmp(facts->deviceid_manufacturer, f_manufacturer) == 0)
					{
					if(strcasecmp(name, facts->deviceid_model) == 0)
						{
						debug2("DeviceID MANUFACTURER + DeviceID MODEL", names[i].name);
						matched++;
						continue;
						}
					if(very_fuzzy(name, facts->deviceid_model))
						{
						debug3("DeviceID MANUFACTURER + DeviceID MODEL", names[i].name);
						matched++; fuzzy++;
						continue;
						}
					}
				}
			} /* DeviceID fuzzy */
		
		if(matched > 0)
			{
			*matches = gu_realloc(*matches, matches_count + count + 1, sizeof(struct MATCH));
			(*matches)[matches_count + count].fuzzy = (matched==fuzzy);
			(*matches)[matches_count + count].manufacturer = gu_strdup(f_manufacturer);
			(*matches)[matches_count + count].description = gu_strdup(f_description);
			count++;
			}
		}
		} /* while */

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

					if((f1 = gu_strsep(&p, "=")) && (f2 = gu_strsep(&p, "")))
						{
						if(!machine_readable)
							printf("    %s=\"%s\"", f1, f2);

						if(strcmp(f1, "SNMP sysDescr") == 0)
							{
							if(!facts->SNMP_sysDescr)
								{
								facts->SNMP_sysDescr = gu_strdup(f2);
								/* retval = 1; */
								}
							}
						else if(strcmp(f1, "SNMP hrDeviceDescr") == 0)
							{
							if(!facts->SNMP_hrDeviceDescr)
								{
								facts->SNMP_hrDeviceDescr = gu_strdup(f2);
								retval = 1;
								}
							}
						else if(strcmp(f1, "PostScript Product") == 0)
							{
							if(!facts->product)
								{
								facts->product = gu_strdup(f2);
								retval = 1;
								}
							}
						else if(strcmp(f1, "PostScript Version") == 0)
							{
							if(!facts->version)
								{
								gu_sscanf(f2, "%f", &facts->version);
								/* retval = 1; */
								}
							}
						else if(strcmp(f1, "PostScript Revision") == 0)
							{
							if(!facts->revision)
								{
								facts->revision = atoi(f2);
								/* retval = 1; */
								}
							}
						else if(strcmp(f1, "1284DeviceID MANUFACTURER") == 0
								|| strcmp(f1, "1284DeviceID MFG") == 0)
							{
							if(!facts->deviceid_manufacturer)
								{
								facts->deviceid_manufacturer = gu_strdup(f2);
								retval = 1;
								}
							}
						else if(strcmp(f1, "1284DeviceID MODEL") == 0
								|| strcmp(f1, "1284DeviceID MDL") == 0)
							{
							if(!facts->deviceid_model)
								{
								facts->deviceid_model = gu_strdup(f2);
								retval = 1;
								}
							}
						else if(strcmp(f1, "1284DeviceID CLASS") == 0
								|| strcmp(f1, "1284DeviceID CLS") == 0)
							{
							}
						else if(strcmp(f1, "1284DeviceID COMMAND SET") == 0
								|| strcmp(f1, "1284DeviceID CMD") == 0)
							{
							}
						else if(strcmp(f1, "1284DeviceID DESCRIPTION") == 0
								|| strcmp(f1, "1284DeviceID DES") == 0)
							{
							}
						else
							{
							if(!machine_readable)
								printf(" (not recognized)");
							}

						if(!machine_readable)
							printf("\n");
						}

					timeout = 60;
					continue;
					}

				/* Not a MODEL: line, but probably of interest. */
				if(!machine_readable)
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
		fprintf(stderr, "%s: query failed: %s\n", myname, gu_exception);
		retval = 0;
		}

	if(!machine_readable)
		gu_puts("\n"); 

	return retval;
	} /* end of ppd_query_interface_probe() */

/*
** PostScript query
*/
static int ppd_query_postscript(const char printer[], struct QUERY *q, struct THE_FACTS *facts)
	{
	const char *result_labels[] = {"Revision", "Version", "Product"};
	char *results[] = {NULL, NULL, NULL};
	char *p;
	int retval = 0;

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

			gu_sscanf(results[1], "%f", &facts->version);
			facts->revision = atoi(results[0]);

			retval = 1;
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
		fprintf(stderr, "%s: query failed: %s\n", myname, gu_exception);
		retval = 0;
		}

	if(!machine_readable)
		gu_puts("\n");
		
	return retval;
	} /* end of ppd_query_postscript() */

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

				if(p[0] == '"' && !facts->pjl_id)
					{
					p++;
					facts->pjl_id = gu_strndup(p, strcspn(p, "\""));

					if(!machine_readable)
						printf("    PJL ID: \"%s\"\n", facts->pjl_id);

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
		fprintf(stderr, "%s: query failed: %s\n", myname, gu_exception);
		retval = 0;
		}

	if(!machine_readable)
		gu_puts("\n");
		
	return retval;
	} /* end of ppd_query_pjl() */

/*
** This is called from ppad_printer.c:printer_ppdq() and ppd_query().
*/
int ppd_query_core(const char printer[], struct QUERY *q)
	{
	struct THE_FACTS facts;
	struct MATCH *matches = NULL;
	int matches_count = 0;
	gu_boolean testmode = FALSE;		/* for debugging, will try all probe methods */

	facts.product = NULL;
	facts.version = 0.0;
	facts.revision = 0;
	facts.deviceid_manufacturer = NULL;
	facts.deviceid_model = NULL;
	facts.pjl_id = NULL;
	facts.SNMP_sysDescr = NULL;
	facts.SNMP_hrDeviceDescr = NULL;

	/* First we ask the interface program to do the job. */
	if(ppd_query_interface_probe(printer, q, &facts) > 0)
		matches_count += ppd_choices(&facts, &matches, matches_count);

	/* Now we connect and try to send a PostScript query. */
	if(matches_count < 1 || testmode)
		if(ppd_query_postscript(printer, q, &facts) > 0)
			matches_count += ppd_choices(&facts, &matches, matches_count);

	/* Now we connect and try to send a PJL query. */
	if(matches_count < 1 || testmode)
		if(ppd_query_pjl(printer, q, &facts) > 0)
			matches_count += ppd_choices(&facts, &matches, matches_count);

	/* If there are any results, print them. */
	if(matches_count > 0)
		{
		gu_boolean include_fuzzy = TRUE;
		int i;

		if(!machine_readable)
			printf("Run one of these commands to select the corresponding PPD file:\n");

		/* First pass.  Decide if we should print the fuzzy matches. */
		if(!testmode)
			{
			for(i=0; i < matches_count; i++)
				{
				if(!matches[i].fuzzy)
					{
					include_fuzzy = FALSE;
					break;
					}
				}
			}

		for(i=0; i < matches_count; i++)
			{
			if(!include_fuzzy && matches[i].fuzzy)
				continue;

			if(machine_readable)	/* for the web front end */
				{
				printf("%s:%s:%s\n", matches[i].manufacturer, matches[i].description, matches[i].fuzzy ? "1" : "0");
				}
			else					/* for human beings */
				{
				printf("    # %s\n", matches[i].fuzzy ? "fuzzy match" : "exact match");
				printf("    ppad ppd %s \"%s\"\n", printer, matches[i].description);
				}

			gu_free(matches[i].manufacturer);
			gu_free(matches[i].description);
			}

		gu_free(matches);
		}

	/* Deallocate the storage of the facts about the printer which we collected above. */
	if(facts.SNMP_hrDeviceDescr)
		gu_free(facts.SNMP_hrDeviceDescr);
	if(facts.product)
		gu_free(facts.product);
	if(facts.pjl_id)
		gu_free(facts.pjl_id);
	if(facts.deviceid_manufacturer)
		gu_free(facts.deviceid_manufacturer);
	if(facts.deviceid_model)
		gu_free(facts.deviceid_model);

	if(matches_count < 1)
		{
		if(!machine_readable)
			printf("No matching PPD files found.\n");
		return EXIT_NOTFOUND;
		}

	return EXIT_OK;
	} /* end of ppd_query_core() */

/*==========================================================================
** ppad ppdlib query
**
** Send a query to a printer using a specified interface and address and
** produce a list of suitable PPD files.
==========================================================================*/
int ppdlib_query(const char *argv[])
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

		query_free(q);
		}
	gu_Catch
		{
		fprintf(stderr, _("%s: query failed: %s\n"), myname, gu_exception);
		return EXIT_INTERNAL;
		}

	return ret;
	} /* end of ppd_query */

/*=========================================================================
 * ppad ppdlib search
 *
 * List PPD files matching a pattern.
=========================================================================*/
int ppdlib_search(const char *argv[])
	{
	const char *pattern;
	gu_boolean wildcards;
	char *pattern_lowered, *f_description_lowered;
	FILE *f;
	char *line = NULL;
	int line_len = 80;
	char *p;
	char *f_description, *f_filename;

	if(!(pattern = argv[0]) || argv[1])
		{
		fputs(
			_("You must supply a search pattern.\n"),
			errors
			);
		return EXIT_SYNTAX;
		}

	if(!(f = fopen(PPD_INDEX, "r")))
		{
		fprintf(errors, _("Can't open \"%s\", errno=%d (%s)\n"), PPD_INDEX, errno, gu_strerror(errno));
		return EXIT_INTERNAL;
		}

	pattern_lowered = gu_strdup(pattern);
	gu_strlower(pattern_lowered);
	wildcards = strchr(pattern, '*') || strchr(pattern, '?');

	while((line = gu_getline(line, &line_len, f)))
		{
		if(line[0] == '#')
			continue;

		p = line;
		if(!(f_description = gu_strsep(&p,":"))
				|| !(f_filename = gu_strsep(&p,":"))
			)
			{
			continue;
			}

		f_description_lowered = gu_strdup(f_description);
		gu_strlower(f_description_lowered);
		
		if((wildcards && gu_wildmat(f_description_lowered, pattern_lowered))
				||
			(!wildcards && strstr(f_description_lowered, pattern_lowered))
			)
			{
			printf("\"%s\"\n", f_description);
			}

		gu_free(f_description_lowered);
		}	

	gu_free(pattern_lowered);

	return EXIT_OK;
	} /* end of ppdlib_search() */

/*=========================================================================
** Display the text of the requested PPD file.
=========================================================================*/
int ppdlib_get(const char *argv[])
	{
	const char *name;
	void *ppd = NULL;;

	if(!(name = argv[0]) || argv[1])
		{
		fputs(
			_("You must supply the name of a PPD file.\n"),
			errors
			);
		return EXIT_SYNTAX;
		}

	gu_Try {
		ppd = ppdobj_new(name);
		}
	gu_Catch
		{
		fprintf(errors, "%s: %s", myname, gu_exception);
		return EXIT_NOTFOUND;	/* a guess */
		}

	gu_Try {
		const char *p;
		while((p = ppdobj_readline(ppd)))
			{
			printf("%s\n", p);
			}
		}
	gu_Final {
		ppdobj_free(ppd);
		}
	gu_Catch {
		fprintf(errors, "%s: %s\n", myname, gu_exception);
		return EXIT_INTERNAL;
		}
	
	return EXIT_OK;
	} /* end of ppdlib_get() */

/* end of file */
