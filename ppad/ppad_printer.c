/*
** mouse:~ppr/src/ppad/ppad_printer.c
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
** Last modified 5 November 2003.
*/

/*==============================================================
** This module is part of the printer administrator's utility.
** It contains the code to implement those sub-commands which
** manipulate printers.
==============================================================*/

#include "before_system.h"
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "util_exits.h"
#include "interface.h"
#include "ppad.h"

/*
** Send the spooler a command to re-read a printer definition.
*/
static void reread_printer(const char *printer)
	{
	write_fifo("NP %s\n", printer);
	} /* end of rearead_printer() */

/*
** Update the "DefFiltOpts:" lines of any groups which have
** this printer as a member.
**
** We must call this if we change the PPD file or change any
** of the "PPDOpts:" lines.
*/
static int update_groups_deffiltopts(const char *printer)
	{
	const char *function = "update_groups_deffiltopts";
	DIR *dir;
	struct dirent *direntp;
	int len;
	int is_a_member;

	if(debug_level > 0)
		printf("Updating \"DefFiltOpts:\" for groups of which \"%s\" is a member.\n", printer);

	if((dir = opendir(GRCONF)) == (DIR*)NULL)
		{
		fprintf(errors, "%s(): opendir() failed\n", function);
		return EXIT_INTERNAL;
		}

	while((direntp = readdir(dir)))
		{
		if(direntp->d_name[0] == '.')			/* skip . and .. */
			continue;							/* and temporary files */

		len=strlen(direntp->d_name);			/* Emacs style backup files */
		if( len > 0 && direntp->d_name[len-1]=='~' )
			continue;							/* should be skipt. */

		is_a_member = FALSE;					/* start by assuming it is not */

		if( grpopen(direntp->d_name, FALSE) )	/* open the group file for read */
			{
			fprintf(errors, "%s(): grpopen(\"%s\", FALSE) failed", function, direntp->d_name);
			closedir(dir);
			return EXIT_INTERNAL;
			}

		while(confread())						/* read until end of file */
			{
			if(lmatch(confline, "Printer:"))
				{
				char *p = &confline[8];
				p += strspn(p, " \t");
				if(strcmp(p, printer) == 0)
					{
					is_a_member = TRUE;
					break;
					}
				}
			}

		confclose();							/* close the group file */

		/*
		** If membership was detected, then call the routine in
		** ppad_group.c which group_deffiltopts() calls.
		*/
		if(is_a_member)
			{
			if(debug_level > 0)
				printf("  Updating \"DefFiltOpts:\" for group \"%s\".\n", direntp->d_name);
			_group_deffiltopts(direntp->d_name);
			}
		}

	closedir(dir);

	return EXIT_OK;
	} /* end of update_groups_deffiltopts() */

/*
** Read the word provided and return BANNER_*
*/
static int flag_code(const char *option)
	{
	if(gu_strcasecmp(option, "never") == 0)
		return BANNER_FORBIDDEN;
	if(gu_strcasecmp(option, "no") == 0)
		return BANNER_DISCOURAGED;
	if(gu_strcasecmp(option, "yes") == 0)
		return BANNER_ENCOURAGED;
	if(gu_strcasecmp(option, "always") == 0)
		return BANNER_REQUIRED;
	return BANNER_INVALID;				/* no match */
	} /* end of flag_code() */

/*
** Set the default Alert: line.
*/
int printer_new_alerts(const char *argv[])
	{
	int frequency;
	const char *method;
	const char *address;
	FILE *newprn;

	if( ! am_administrator() )
		return EXIT_DENIED;

	if(! argv[0]
				|| strspn(argv[0],"-0123456789") != strlen(argv[0])
				|| ! argv[1] || ! argv[2])
		{
		fputs(_("You must supply an alert frequency, such as \"7\", an alert method,\n"
				"such as \"mail\", and an alert address, such as \"alertreaders\".\n"), errors);
		return EXIT_SYNTAX;
		}

	frequency = atoi(argv[0]);
	method = argv[1];
	address = argv[2];

	if((newprn = fopen(NEWPRN_CONFIG,"w")) == (FILE*)NULL)
		{
		fprintf(errors, _("Unable to create \"%s/%s\", errno=%d (%s).\n"), HOMEDIR, NEWPRN_CONFIG, errno, gu_strerror(errno));
		return EXIT_INTERNAL;
		}

	fprintf(newprn,"Alert: %d %s %s\n",frequency,method,address);

	fclose(newprn);
	return EXIT_OK;
	} /* end of printer_new_alerts() */

/*===================================================================
** Code for ppad show begins here.
===================================================================*/

#define FEEDBACK_NO 0
#define FEEDBACK_YES 1
#define FEEDBACK_DEFAULT -1
#define FEEDBACK_INVALID -100

#define OUTPUTORDER_NORMAL 1
#define OUTPUTORDER_REVERSE -1
#define OUTPUTORDER_PPD 0
#define OUTPUTORDER_INVALID -100

/*
** Take a BANNER_* value and return a description
*/
static const char *flag_description(int code)
	{
	switch(code)
		{
		case BANNER_FORBIDDEN:
			return "never";
		case BANNER_DISCOURAGED:
			return "no";
		case BANNER_ENCOURAGED:
			return "yes";
		case BANNER_REQUIRED:
			return "always";
		default:
			return "<invalid>";
		}
	} /* end of flag_description() */

static const char *long_flag_description(int code)
	{
	switch(code)
		{
		case BANNER_FORBIDDEN:
			return "forbidden";
		case BANNER_DISCOURAGED:
			return "discouraged";
		case BANNER_ENCOURAGED:
			return "encouraged";
		case BANNER_REQUIRED:
			return "required";
		default:
			return "<invalid>";
		}
	} /* end of flag_description() */

static const char *feedback_description(int feedback)
	{
	switch(feedback)
		{
		case FEEDBACK_NO:
			return "no";
		case FEEDBACK_YES:
			return "yes";
		case FEEDBACK_DEFAULT:
			return "default";
		default:
			return "<invalid>";
		}
	}

static const char *jobbreak_description(int jobbreak)
	{
	switch(jobbreak)
		{
		case JOBBREAK_DEFAULT:
			return "default";
		case JOBBREAK_NONE:
			return "none";
		case JOBBREAK_SIGNAL:
			return "signal";
		case JOBBREAK_CONTROL_D:
			return "control-d";
		case JOBBREAK_PJL:
			return "pjl";
		case JOBBREAK_SIGNAL_PJL:
			return "signal/pjl";
		case JOBBREAK_SAVE_RESTORE:
			return "save/restore";
		case JOBBREAK_NEWINTERFACE:
			return "newinterface";
		default:
			return "<invalid>";
		}
	} /* end of jobbreak_description() */

static const char *outputorder_description(int outputorder)
	{
	switch(outputorder)
		{
		case OUTPUTORDER_PPD:			/* default */
			return "PPD";
		case OUTPUTORDER_REVERSE:
			return "Reverse";
		case OUTPUTORDER_NORMAL:
			return "Normal";
		default:
			return "<invalid>";
		}
	} /* end of outputorder_description() */

static const char *codes_description(int codes)
	{
	switch(codes)
		{
		case CODES_DEFAULT:
			return "default";
		case CODES_UNKNOWN:				/* PPR 1.31 behavior */
			return "ignore";
		case CODES_Clean7Bit:
			return "Clean7Bit";
		case CODES_Clean8Bit:
			return "Clean8Bit";
		case CODES_Binary:
			return "Binary";
		case CODES_TBCP:
			return "TBCP";
		default:						/* impossible value */
			return "<invalid>";
		}
	} /* end of codes_description() */

/*
** Read a printer's configuration file and
** print a report.
** It is not necessary to be an operator in order to
** execute this command.
*/
int printer_show(const char *argv[])
	{
	const char function[] = "printer_show";
	const char *printer = argv[0];		/* Argument is printer to show. */

	int count;							/* general use */
	char *ptr;							/* general use */
	float tf1, tf2;
	char scratch[10];

	/* These variables hold the gathered information */
	char *comment = (char*)NULL;
	char *location = (char*)NULL;
	char *department = (char*)NULL;
	char *contact = (char*)NULL;
	char *interface = (char*)NULL;
	char *address = (char*)NULL;
	char *options = (char*)NULL;
	int feedback = FEEDBACK_DEFAULT;
	int feedback_default;
	int jobbreak = JOBBREAK_DEFAULT;
	int jobbreak_default;
	int codes = CODES_DEFAULT;
	int codes_default;
	char *rip_name = NULL;
	char *rip_output_language = NULL;
	char *rip_options = NULL;
	char *rip_ppd_name = NULL;
	char *rip_ppd_output_language = NULL;
	char *rip_ppd_options = NULL;
	char *PPDFile = (char*)NULL;
	char *bins[MAX_BINS];
	int outputorder = 0;				/* unknown outputorder */
	int bincount = 0;
	int banner = BANNER_DISCOURAGED;
	char *deffiltopts = (char*)NULL;
	char *ppdopts[100];
	int ppdopts_count = 0;
	int trailer = BANNER_DISCOURAGED;
	int alerts_frequency = 0;
	char *alerts_method = (char*)NULL;
	char *alerts_address = (char*)NULL;
	int charge_duplex_sheet = -1;		/* charge in cents */
	int charge_simplex_sheet = -1;
	char *switchset = (char*)NULL;
	char *passthru = (char*)NULL;
	int limitpages_lower = 0, limitpages_upper = 0;
	int limitkilobytes_lower = 0, limitkilobytes_upper = 0;
	gu_boolean grayok = TRUE;
	char *acls = (char*)NULL;
	int pagetimelimit = 0;
	char *userparams = (char*)NULL;
	#define MAX_ADDONS 32
	char *addon[MAX_ADDONS];
	int addon_count = 0;

	if(! printer)
		{
		fputs(_("You must specify a printer.\n"), errors);
		return EXIT_SYNTAX;
		}

	if(prnopen(printer, FALSE) == -1)	/* if printer does not exist, */
		{
		fprintf(errors, _("The printer \"%s\" does not exist.\n"), printer);
		return EXIT_BADDEST;
		}

	/*
	** This loop examines each line in the printer's configuration
	** file and gathers information.
	*/
	while(confread())
		{
		if(gu_sscanf(confline, "Interface: %S", &ptr) == 1)
			{
			if(interface) gu_free(interface);
			interface = ptr;

			/* Invalidate lines which preceed "Interface:". */
			jobbreak = JOBBREAK_DEFAULT;
			feedback = FEEDBACK_DEFAULT;
			codes = CODES_DEFAULT;
			}
		else if(gu_sscanf(confline, "Address: %A", &ptr) == 1)
			{
			if(address) gu_free(address);		/* discard any preceding address lines */
			address = ptr;
			}
		else if(gu_sscanf(confline, "Options: %Z", &ptr) == 1)
			{
			if(options) gu_free(options);
			options = ptr;
			}
		else if(lmatch(confline, "Feedback:"))
			{
			switch(gu_torf(&confline[9]))
				{
				case ANSWER_TRUE:
					feedback = FEEDBACK_YES;
					break;
				case ANSWER_FALSE:
					feedback = FEEDBACK_NO;
					break;
				default:
					feedback = FEEDBACK_INVALID;
					break;
				}
			}
		else if(gu_sscanf(confline, "JobBreak: %d", &jobbreak) == 1)
			{
			/* nothing to do */
			}
		else if(gu_sscanf(confline, "Codes: %d", &codes) == 1)
			{
			/* nothing to do */
			}
		else if((ptr = lmatchp(confline, "RIP:")))
			{
			if(rip_name) gu_free(rip_name);
			if(rip_output_language) gu_free(rip_output_language);
			if(rip_options) gu_free(rip_options);
			gu_sscanf(ptr, "%S %S %Z", &rip_name, &rip_output_language, &rip_options);
			}
		else if(gu_sscanf(confline, "PPDFile: %A", &ptr) == 1)
			{
			if(PPDFile) gu_free(PPDFile);
			PPDFile = ptr;
			}
		else if(gu_sscanf(confline, "Bin: %Z", &ptr) == 1)
			{
			if(bincount<MAX_BINS)
				bins[bincount++] = ptr;
			}
		else if(gu_sscanf(confline, "Comment: %A", &ptr) == 1)
			{
			if(comment) gu_free(comment);
			comment = ptr;
			}
		else if(gu_sscanf(confline, "Location: %A", &ptr) == 1)
			{
			if(location) gu_free(location);
			location = ptr;
			}
		else if(gu_sscanf(confline, "Department: %A", &ptr) == 1)
			{
			if(department) gu_free(department);
			department = ptr;
			}
		else if(gu_sscanf(confline, "Contact: %A", &ptr) == 1)
			{
			if(contact) gu_free(contact);
			contact = ptr;
			}
		else if(lmatch(confline, "FlagPages:"))
			{
			sscanf(confline, "FlagPages: %d %d", &banner, &trailer);
			}
		else if(lmatch(confline, "Alert:"))
			{
			int x=6;
			int len;

			if(alerts_method) gu_free(alerts_method);
			if(alerts_address) gu_free(alerts_address);

			x+=strspn(&confline[x]," \t");				/* eat up space */
			alerts_frequency=atoi(&confline[x]);

			x+=strspn(&confline[x]," \t-0123456789");	/* skip spaces and */
														/* digits */
			len=strcspn(&confline[x]," \t");			/* get word length */
			alerts_method = (char*)gu_alloc(len+1,sizeof(char));
			strncpy(alerts_method,&confline[x],len);	 /* copy */
			alerts_method[len] = '\0';					/* terminate */
			x+=len;										/* move past word */
			x+=strspn(&confline[x]," \t");				/* skip spaces */

			len=strcspn(&confline[x]," \t\n");			/* get length */
			alerts_address = (char*)gu_alloc(len+1,sizeof(char));
			strncpy(alerts_address,&confline[x],len);	/* copy */
			alerts_address[len] = '\0';					/* terminate */
			}
		else if(gu_sscanf(confline, "OutputOrder: %#s", sizeof(scratch), scratch) == 1)
			{
			if(strcmp(scratch, "Normal") == 0)
				outputorder = 1;
			else if(strcmp(scratch, "Reverse") == 0)
				outputorder = -1;
			else
				outputorder = -100;
			}
		else if((count = sscanf(confline, "Charge: %f %f", &tf1, &tf2)) > 0)
			{
			/* Convert dollars to cents, pounds to pence, etc.: */
			charge_duplex_sheet = (int)(tf1 * 100.0 + 0.5);

			/* If there is a second parameter, convert it too: */
			if(count == 2)
				charge_simplex_sheet = (int)(tf2 * 100.0 + 0.5);
			else
				charge_duplex_sheet = charge_simplex_sheet;
			}
		else if(gu_sscanf(confline, "Switchset: %Z", &ptr) == 1)
			{
			if(switchset) gu_free(switchset);
			switchset = ptr;
			}
		else if(gu_sscanf(confline, "DefFiltOpts: %Z", &ptr) == 1)
			{
			if(deffiltopts) gu_free(deffiltopts);
			deffiltopts = ptr;
			}
		else if(gu_sscanf(confline, "PPDOpt: %Z", &ptr) == 1)
			{
			if(ppdopts_count >= (sizeof(ppdopts) / sizeof(ppdopts[0])))
				{
				fprintf(errors, "%s(): PPDOpts overflow\n", function);
				}
			else
				{
				ppdopts[ppdopts_count++] = ptr;
				}
			}
		else if(gu_sscanf(confline, "PassThru: %Z", &ptr) == 1)
			{
			if(passthru) gu_free(passthru);
			passthru = ptr;
			}
		else if(gu_sscanf(confline, "LimitPages: %d %d", &limitpages_lower, &limitpages_upper) == 2)
			{
			/* nothing more to do */
			}
		else if(gu_sscanf(confline, "LimitKilobytes: %d %d", &limitkilobytes_lower, &limitkilobytes_upper) == 2)
			{
			/* nothing more to do */
			}
		else if((ptr = lmatchp(confline, "GrayOK:")))
			{
			if(gu_torf_setBOOL(&grayok, ptr) == -1)
				fprintf(errors, _("WARNING: invalid \"%s\" setting: %s\n"), "GrayOK", ptr);
			}
		else if(gu_sscanf(confline, "ACLs: %Z", &ptr) == 1)
			{
			if(acls) gu_free(acls);
			acls = ptr;
			}
		else if(gu_sscanf(confline, "PageTimeLimit: %d", &pagetimelimit) == 1)
			{
			/* nothing more to do */
			}
		else if(gu_sscanf(confline, "Userparams: %Z", &ptr) == 1)
			{
			if(userparams) gu_free(userparams);
			userparams = ptr;
			}
		else if(confline[0] >= 'a' && confline[0] <= 'z')		/* if in addon name space */
			{
			if(addon_count >= MAX_ADDONS)
				{
				fprintf(errors, "%s(): addon[] overflow\n", function);
				}
			else
				{
				addon[addon_count++] = gu_strdup(confline);
				}
			}

		} /* end of loop for each configuration file line */

	confclose();

	/*
	** Determine the jobbreak, feedback, and codes
	** defaults for this interface.
	*/
	{
	int ret;
	char *pline, *p;
	char *cups_filter = NULL;
	int default_resolution = 0;
	struct PPD_PROTOCOLS prot;
	prot.TBCP = FALSE;
	prot.PJL = FALSE;

	/* If there is a PPD file defined, read its "*Protocols:"
	   line to help us arrive at the defaults. */
	if(PPDFile)
		{
		if((ret = ppd_open(PPDFile, errors)) != EXIT_OK)
			return ret;

		while((pline = ppd_readline()))
			{
			if((p = lmatchp(pline, "*Protocols:")))
				{
				char *f;
				while((f = gu_strsep(&p, " \t")))
					{
					if(strcmp(f, "TBCP") == 0)
						prot.TBCP = TRUE;
					if(strcmp(f, "PJL") == 0)
						prot.PJL = TRUE;
					}
				continue;
				} /* "*Protocols:" */

			if((p = lmatchp(pline, "*pprRIP:")) && !rip_ppd_name)
				{
				char *f1, *f2, *f3;
				if((f1 = gu_strsep(&p, " \t")) && (f2 = gu_strsep(&p, " \t")))
					{
					rip_ppd_name = gu_strdup(f1);
					rip_ppd_output_language = gu_strdup(f2);
					if((f3 = gu_strsep(&p, "")))
						rip_ppd_options = gu_strdup(f3);
					}
				else
					{
					fprintf(errors, _("WARNING: can't parse RIP information in PPD file\n"));
					}
				continue;
				} /* "*pprRIP:" */

			if((p = lmatchp(pline, "*cupsFilter:")) && !cups_filter)
				{
				char *p1, *p2, *p3;
				if(*p++ == '"' && (p1 = strchr(p, '"')))
					{
					*p1 = '\0';
					if((p1 = gu_strsep(&p, " \t"))										/* first exists */
								&& strcmp(p1, "application/vnd.cups-raster") == 0		/* and mime type matches */
								&& (p2 = gu_strsep(&p, " \t"))							/* and second parameter exists */
								&& strspn(p2, "0123456789") == strlen(p2)				/* and it is numberic */
								&& (p3 = gu_strsep(&p, "\t"))							/* and third parameter exists */
						)
						{
						cups_filter = gu_strdup(p3);
						}
					}
				continue;
				} /* "*cupsFilter:" */

			if(gu_sscanf(pline, "*DefaultResolution: %d", &default_resolution) == 1)
				{
				continue;
				}

			} /* while() */
		} /* if(PPDFile) */

	/* Determine all of the defaults. */
	feedback_default = interface_default_feedback(interface, &prot);
	jobbreak_default = interface_default_jobbreak(interface, &prot);
	codes_default = interface_default_codes(interface, &prot);

	/* If we didn't find a "*pprRIP:" line, use "*cupsFilter:".
	   */
	if(!rip_ppd_name && cups_filter && default_resolution > 0)
		{
		rip_ppd_name = gu_strdup("ppr-gs");						/* !!! */
		rip_ppd_output_language = gu_strdup("pcl");				/* !!! */
		gu_asprintf(&rip_ppd_options, "cups=%s -r%d", cups_filter, default_resolution);
		}

	/* If the PPD file specifies a RIP and the printer configuration file
	   doesn't override it, use the info from the PPD file.
	   */
	if(!rip_name && rip_ppd_name)
		{
		rip_name = rip_ppd_name;
		rip_output_language = rip_ppd_output_language;
		rip_options = rip_ppd_options;
		}

	if(cups_filter) gu_free(cups_filter);
	}

	/*
	** Now that we have retrieved the information, we will print it.
	*/
	if( ! machine_readable )
		{
		printf(_("Printer name: %s\n"), printer);
		PUTS("  ");
			printf(_("Comment: %s\n"), comment ? comment : "");
		if(location)
			{
			PUTS("  ");
			printf(_("Location: %s\n"), location);
			}
		if(department)
			{
			PUTS("  ");
			printf(_("Department: %s\n"), department);
			}
		if(contact)
			{
			PUTS("  ");
			printf(_("Contact: %s\n"), contact);
			}

		/*-------------------------------------
		** Communication related things
		-------------------------------------*/
		printf(_("Interface: %s\n"), interface ? interface : _("<undefined>"));

		PUTS("  ");
			printf(_("Address: \"%s\"\n"), address ? address : _("<undefined>"));

		PUTS("  ");
			printf(_("Options: %s\n"), options ? options : "");

		PUTS("  ");
			PUTS(_("JobBreak: "));
			if(jobbreak == JOBBREAK_DEFAULT)
				printf(_("%s (by default)"), jobbreak_description(jobbreak_default));
			else
				PUTS(jobbreak_description(jobbreak));
		putchar('\n');

		PUTS("  ");
			PUTS(_("Feedback: "));
			if(feedback == FEEDBACK_DEFAULT)
				printf(_("%s (by default)"), feedback_description(feedback_default));
			else
				PUTS(feedback_description(feedback));
			putchar('\n');

		PUTS("  ");
			PUTS(_("Codes: "));
			if(codes == CODES_DEFAULT)
				printf(_("%s (by default)"), codes_description(codes_default));
			else
				PUTS(codes_description(codes));
			putchar('\n');

		/*-------------------------------------
		** PPD File Related Things
		-------------------------------------*/
		printf(_("PPDFile: %s\n"), PPDFile ? PPDFile : _("<undefined>"));

		PUTS("  ");
		{
		const char *s = _("Default Filter Options: ");
		PUTS(s);
		if(deffiltopts)
			print_wrapped(deffiltopts, strlen(s));
		putchar('\n');
		}

		/* RIP */
		if(rip_name)
			{
			PUTS("  ");
			printf(rip_name==rip_ppd_name ? _("RIP: %s %s \"%s\" (from PPD)\n") : _("RIP: %s %s \"%s\"\n"),
				rip_name,
				rip_output_language ? rip_output_language : "?",
				rip_options ? rip_options : "");
			}

		/* Optional printer equipment. */
		{
		int x;
		for(x=0; x < ppdopts_count; x++)
			{
			PUTS("  ");
			printf(_("PPDOpts: %s\n"), ppdopts[x]);
			gu_free(ppdopts[x]);
			}
		}

		PUTS("  ");
		PUTS(_("Bins: "));				/* print a list of all */
		{
		int x;
		for(x=0; x < bincount; x++)		/* the bins we found */
			{
			if(x==0)
				printf("%s", bins[x]);
			else
				printf(", %s", bins[x]);

			gu_free(bins[x]);
			}
		putchar('\n');
		}

		PUTS("  ");
		printf(_("OutputOrder: %s\n"), outputorder_description(outputorder));

		/*---------------------------------
		** Alerts
		---------------------------------*/
		printf(_("Alert frequency: %d "), alerts_frequency);
		switch(alerts_frequency)
			{
			case 0:
				fputs(_("(never send alerts)"), stdout);
				break;
			case 1:
				fputs(_("(send alert on every error)"), stdout);
				break;
			case -1:
				fputs(_("(send alert on first error, send notice on recovery)"), stdout);
				break;
			default:
				if(alerts_frequency > 0)
					printf(_("(send alert every %d errors)"), alerts_frequency);
				else
					printf(_("(send alert after %d errors, send notice on recovery)"), (alerts_frequency * -1));
				break;
			}
		putchar('\n');
		PUTS("  "); printf(_("Alert method: %s\n"), alerts_method ? alerts_method : _("none"));
		PUTS("  "); printf(_("Alert address: %s\n"), alerts_address ? alerts_address : _("none"));

		/*---------------------------------
		** Accounting things
		---------------------------------*/
		printf(_("Flags: %s %s (banners %s, trailers %s)\n"),
			flag_description(banner),
			flag_description(trailer),
			long_flag_description(banner),
			long_flag_description(trailer));
		printf(_("Charge: "));
		if(charge_duplex_sheet == -1)
			{
			printf(_("no charge\n"));
			}
		else
			{		/* money() uses static array for result */
			printf(_("%s per duplex sheet, "), money(charge_duplex_sheet));
			printf(_("%s per simplex sheet\n"), money(charge_simplex_sheet));
			}

		/*--------------------------------------------
		** discretionary settings
		---------------------------------------------*/

		/* Uncompress and print the switchset. */
		PUTS(_("Switchset: "));
		if(switchset)
			print_switchset(switchset);
		putchar('\n');

		/* Rare option is not show unless set. */
		if(passthru)
			printf(_("PassThru types: %s\n"), passthru);

		/* Rare option is not show unless set. */
		if(limitpages_lower > 0 || limitpages_upper > 0)
			printf(_("LimitPages: %d %d\n"), limitpages_lower, limitpages_upper);

		/* Rare option is not show unless set. */
		if(limitkilobytes_lower > 0 || limitkilobytes_upper > 0)
			printf(_("LimitKilobytes: %d %d\n"), limitkilobytes_lower, limitkilobytes_upper);

		/* Another rare option. */
		if(!grayok)
			printf(_("GrayOK: %s\n"), grayok ? _("yes") : _("no"));

		/* This rare option is not show until set. */
		if(acls)
			printf(_("ACLs: %s\n"), acls);

		/* Another rare option */
		if(pagetimelimit > 0)
			printf(_("PageTimeLimit: %d\n"), pagetimelimit);

		/* Another rare option */
		if(userparams)
			printf(_("Userparams: %s\n"), userparams);

		/* Print the assembed addon settings. */
		if(addon_count > 0)
			{
			int x;
			PUTS(_("Addon:"));
			for(x = 0; x < addon_count; x++)
				{
				printf("\t%s\n", addon[x]);
				gu_free(addon[x]);
				}
			}
		}

	/* Machine readable output. */
	else
		{
		printf("name\t%s\n", printer);
		printf("comment\t%s\n", comment ? comment : "");
		printf("location\t%s\n", location ? location : "");
		printf("department\t%s\n", department ? department : "");
		printf("contact\t%s\n", contact ? contact : "");

		/* Communications related things */
		printf("interface\t%s\n", interface ? interface : "");
		printf("address\t%s\n", address ? address : "");
		printf("options\t%s\n", options ? options : "");
		printf("jobbreak\t%s %s\n",jobbreak_description(jobbreak), jobbreak_description(jobbreak_default));
		printf("feedback\t%s %s\n", feedback_description(feedback), feedback_description(feedback_default));
		printf("codes\t%s %s\n", codes_description(codes), codes_description(codes_default));

		/* RIP */
		printf("rip\t%s\t%s\t%s\n",
				rip_name ? rip_name : "",
				rip_output_language ? rip_output_language : "",
				rip_options ? rip_options : "");
		printf("rip_ppd\t%s\t%s\t%s\n",
				rip_ppd_name ? rip_ppd_name : "",
				rip_ppd_output_language ? rip_ppd_output_language : "",
				rip_ppd_options ? rip_ppd_options : "");
		printf("rip_which\t%s\n",
				rip_name==rip_ppd_name ? "PPD" : "CONFIG");

		/* Alerts */
		printf("alerts\t%d %s %s\n",
				alerts_frequency,
				alerts_method ? alerts_method : "",
				alerts_address ? alerts_address : "");

		/* Accounting related things */
		printf("flags\t%s %s\n", flag_description(banner), flag_description(trailer));
		PUTS("charge\t");
		if(charge_duplex_sheet != -1)
			{			/* money() returns pointer to static array! */
			PUTS(money(charge_duplex_sheet));
			putchar(' ');
			PUTS(money(charge_simplex_sheet));
			}
		else
			{
			putchar(' ');
			}
		putchar('\n');

		/* PPD file related things */
		printf("ppd\t%s\n", PPDFile ? PPDFile : "");

		PUTS("ppdopts\t");
		{
		int x;
		for(x=0; x < ppdopts_count; x++)
			{
			char *p1, *p2;
			int len1, len2;
			p1 = ppdopts[x];
			len1 = strcspn(p1, "/ \t");
			p2 = p1 + len1;
			p2 += strcspn(p2, " \t");
			p2 += strspn(p2, " \t");
			len2 = strcspn(p2, "/ \t");
			if(x)
				putchar(' ');
			printf("%.*s %.*s", len1, p1, len2, p2);
			gu_free(ppdopts[x]);
			}
		}
		putchar('\n');

		PUTS("bins\t");
		{
		int x;
		for(x=0; x < bincount; x++)
			{
			if(x==0)
				printf("%s", bins[x]);
			else
				printf(" %s", bins[x]);
			gu_free(bins[x]);
			}
		}
		putchar('\n');

		printf("outputorder\t%s\n", outputorder_description(outputorder));

		printf("deffiltopts\t%s\n", deffiltopts ? deffiltopts : "");

		/* Other things */
		PUTS("switchset\t"); if(switchset) print_switchset(switchset); putchar('\n');
		printf("passthru\t%s\n", passthru ? passthru : "");
		printf("limitpages\t%d %d\n", limitpages_lower, limitpages_upper);
		printf("limitkilobytes\t%d %d\n", limitkilobytes_lower, limitkilobytes_upper);
		printf("grayok\t%s\n", grayok ? "yes" : "no");
		printf("acls\t%s\n", acls ? acls : "");
		printf("pagetimelimit\t%d\n", pagetimelimit);
		printf("userparams\t%s\n", userparams ? userparams : "");

		/* Addon lines */
		if(addon_count > 0)
			{
			int x;
			char *p;
			for(x = 0; x < addon_count; x++)
				{
				if((p = strchr(addon[x], ':')))
					{
					*p = '\0';
					p++;
					p += strspn(p, " \t");
					printf("addon %s\t%s\n", addon[x], p);
					}
				else
					{
					printf("addon\t%s\n", addon[x]);
					}
				gu_free(addon[x]);
				}
			}
		} /* end of machine_readable */

	/* Free some memory which we allocated: */
	if(comment) gu_free(comment);
	if(location) gu_free(location);
	if(department) gu_free(department);
	if(contact) gu_free(contact);
	if(interface) gu_free(interface);
	if(address) gu_free(address);
	if(options) gu_free(options);
	if(rip_name != rip_ppd_name)
		{
		if(rip_name) gu_free(rip_name);
		if(rip_output_language) gu_free(rip_output_language);
		if(rip_options) gu_free(rip_options);
		}
	if(rip_ppd_name) gu_free(rip_ppd_name);
	if(rip_ppd_output_language) gu_free(rip_ppd_output_language);
	if(rip_ppd_options) gu_free(rip_ppd_options);
	if(PPDFile) gu_free(PPDFile);
	if(alerts_method) gu_free(alerts_method);
	if(alerts_address) gu_free(alerts_address);
	if(deffiltopts) gu_free(deffiltopts);
	if(switchset) gu_free(switchset);
	if(passthru) gu_free(passthru);
	if(acls) gu_free(acls);

	return EXIT_OK;
	} /* end of printer_show() */

/*==========================================================================
** Set a printer's comment.
==========================================================================*/
int printer_comment(const char *argv[])
	{
	const char *printer = argv[0];
	const char *comment = argv[1];

	if( ! printer || ! comment )
		{
		fputs(_("You must supply the name of an existing printer and\n"
				"a comment to attach to it.\n"), errors);
		return EXIT_SYNTAX;
		}

	return conf_set_name(QUEUE_TYPE_PRINTER, printer, "Comment", "%s", comment);
	} /* end of printer_comment() */

/*==========================================================================
** Set a printer's location.
==========================================================================*/
int printer_location(const char *argv[])
	{
	const char *printer = argv[0];
	const char *location = argv[1];

	if( ! printer || ! location )
		{
		fputs(_("You must supply the name of an existing printer and\n"
				"a location name to attach to it.\n"), errors);
		return EXIT_SYNTAX;
		}

	return conf_set_name(QUEUE_TYPE_PRINTER, printer, "Location", "%s", location);
	} /* end of printer_location() */

/*==========================================================================
** Set a printer's department.
==========================================================================*/
int printer_department(const char *argv[])
	{
	const char *printer = argv[0];
	const char *department = argv[1];

	if( ! printer || ! department )
		{
		fputs(_("You must supply the name of an existing printer and\n"
				"a department name to attach to it.\n"), errors);
		return EXIT_SYNTAX;
		}

	return conf_set_name(QUEUE_TYPE_PRINTER, printer, "Department", "%s", department);
	} /* end of printer_department() */

/*==========================================================================
** Set a printer's contact.
==========================================================================*/
int printer_contact(const char *argv[])
	{
	const char *printer = argv[0];
	const char *contact = argv[1];

	if( ! printer || ! contact )
		{
		fputs(_("You must supply the name of an existing printer and\n"
				"a contact name to attach to it.\n"), errors);
		return EXIT_SYNTAX;
		}

	return conf_set_name(QUEUE_TYPE_PRINTER, printer, "Contact", "%s", contact);
	} /* end of printer_contact() */

/*=========================================================================
** Set a printer's interface and the interface's address.
** We will delete "JobBreak:" and "Feedback:" lines if the
** interface is changed or if there was previously no "Interface:"
** line.  We will also delete all "JobBreak:" and "Feedback:" lines
** which appear before the "Interface:" line.
**
** We will only ask the spooler to re-read the configuration file
** if this command creates the printer.
=========================================================================*/
int printer_interface(const char *argv[])
	{
	const char *printer = argv[0];
	const char *interface = argv[1];
	const char *address = argv[2];

	if( ! am_administrator() )
		return EXIT_DENIED;

	if( ! printer || ! interface || ! address )
		{
		fputs(_("You must specify a printer, either new or existing,\n"
				"an interface, and an address for the interface\n"
				"to send the job to.\n"), errors);
		return EXIT_SYNTAX;
		}

	if(strlen(printer) > MAX_DESTNAME)
		{
		fputs(_("The printer name is too long.\n"), errors);
		return EXIT_SYNTAX;
		}

	if(strpbrk(printer, DEST_DISALLOWED) != (char*)NULL)
		{
		fputs(_("The printer name contains a disallowed character.\n"), errors);
		return EXIT_SYNTAX;
		}

	if(strchr(DEST_DISALLOWED_LEADING, (int)printer[0]) != (char*)NULL)
		{
		fputs(_("The printer name begins with a disallowed character.\n"), errors);
		return EXIT_SYNTAX;
		}

	/* Make sure the interface exists. */
	{
	char fname[MAX_PPR_PATH];
	const char *p;
	struct stat statbuf;

	if(interface[0] == '/')
		{
		p = interface;
		}
	else
		{
		ppr_fnamef(fname, "%s/%s", INTDIR, interface);
		p = fname;
		}

	if(stat(p, &statbuf) < 0)
		{
		fprintf(errors, _("The interface \"%s\" does not exist.\n"), interface);
		return EXIT_NOTFOUND;
		}
	if(! (statbuf.st_mode & S_IXUSR) )
		{
		fprintf(errors, _("The interface \"%s\" not executable.\n"), interface);
		return EXIT_NOTFOUND;
		}
	}

	/* if new printer */
	if( prnopen(printer, TRUE) )		/* try to open printer config for write */
		{								/* if we fail, create a new printer */
		FILE *newconf;
		FILE *defaults;
		char fname[MAX_PPR_PATH];
		int c;

		/* create a config file */
		ppr_fnamef(fname, "%s/%s", PRCONF, printer);
		if((newconf = fopen(fname,"w")) == (FILE*)NULL)
			{
			fprintf(errors, _("Failed to create printer config file \"%s\", errno=%d (%s).\n"), fname, errno, gu_strerror(errno));
			return EXIT_INTERNAL;
			}

		fprintf(newconf, "Interface: %s\n", interface);			/* write the interface */
		fprintf(newconf, "Address: \"%s\"\n", address);
		fprintf(newconf, "PPDFile: Apple LaserWriter Plus\n");	/* specify a good generic PPD file */
		fprintf(newconf, "DefFiltOpts: level=1 colour=False resolution=300 freevm=172872 mfmode=CanonCX\n");

		if((defaults = fopen(NEWPRN_CONFIG,"r")) != (FILE*)NULL)
			{
			while( (c=fgetc(defaults)) != -1 )
				fputc(c,newconf);
			fclose(defaults);
			}

		fclose(newconf);
		reread_printer(printer);		/* tell pprd to re-read the configuration */
		}								/* (really for the 1st time) */

	/* if old printer */
	else
		{
		gu_boolean different_interface = TRUE;

		/* Copy up to the 1st "Interface:". */
		while(confread())
			{
			if(lmatch(confline, "Interface:"))
				{
				char *p = &confline[10];
				p += strspn(p, " \t");
				if(strcmp(interface, p) == 0)
					different_interface = FALSE;
				break;
				}
			else				/* while deleting spurious "JobBreak:" and */
				{				/* "Feedback:" lines as we go */
				if(!lmatch(confline, "Address:")
						&& !lmatch(confline, "Options:")
						&& !lmatch(confline, "JobBreak:")
						&& !lmatch(confline, "Feedback:")
						&& !lmatch(confline, "Codes:") )
					{
					conf_printf("%s\n", confline);
					}
				}
			}

		/* Write the new lines. */
		conf_printf("Interface: %s\n", interface);
		conf_printf("Address: \"%s\"\n", address);

		/* And copy the rest of the file while removing certain old lines. */
		while(confread())
			{
			if(!lmatch(confline, "Interface:")
						&& !lmatch(confline, "Address:")
						&& (!lmatch(confline, "Options:") || !different_interface)
						&& (!lmatch(confline, "JobBreak:") || !different_interface)
						&& (!lmatch(confline, "Feedback:") || !different_interface)
						&& (!lmatch(confline, "Codes:") || !different_interface) )
				conf_printf("%s\n", confline);
			}
		confclose();
		}

	return EXIT_OK;
	} /* end of printer_interface() */

/*===========================================================================
** Set a printer's interface options string.
===========================================================================*/
int printer_options(const char *argv[])
	{
	const char *printer = argv[0];

	if( ! am_administrator() )
		return EXIT_DENIED;

	if(! printer)
		{
		fputs(_("Insufficient parameters.\n"), errors);
		return EXIT_SYNTAX;
		}

	/* make sure the printer exists */
	if(prnopen(printer, TRUE))
		{
		fprintf(errors, _("The printer \"%s\" does not exist.\n"), printer);
		return EXIT_BADDEST;
		}

	/* Modify the printer's configuration file. */
	while(confread())
		{
		if(lmatch(confline, "Options:"))				/* Delete "Options: " lines before */
			continue;									/* the "Interface: " line. */
		if(lmatch(confline, "Interface:"))				/* Break loop after copying */
			{											/* the "Interface: " line */
			conf_printf("%s\n", confline);
			break;
			}
		conf_printf("%s\n", confline);
		}

	/* If we have a meaningful new line to write, write it now. */
	{
	char *p = list_to_string(&argv[1]);
	if(p)
		{
		if(strcmp(p, "none") == 0)
			fprintf(errors, X_("Warning: setting options to \"none\" is deprecated\n"));
		else
			conf_printf("Options: %s\n", p);
		gu_free(p);
		}
	}

	while(confread())							/* copy rest of file, */
		{
		if(lmatch(confline, "Options:"))
			continue;
		else
			conf_printf("%s\n", confline);
		}
	confclose();

	return EXIT_OK;
	} /* end of printer_options() */

/*
** Set a printer's jobbreak flag.
*/
static void printer_jobbreak_help(void)
	{
	fputs(_("You must supply the name of an existing printer and a jobbreak mode setting.\n"
		"Valid jobbreak modes are \"signal\", \"control-d\", \"pjl\", \"signal/pjl\",\n"
		"\"save/restore\", \"newinterface\", and \"default\".\n"), errors);
	} /* end of printer_jobbreak_help() */

int printer_jobbreak(const char *argv[])
	{
	const char *printer = argv[0];
	int newstate;

	if( ! am_administrator() )
		return EXIT_DENIED;

	if(! printer || ! argv[1])
		{
		printer_jobbreak_help();
		return EXIT_SYNTAX;
		}

	if(gu_strcasecmp(argv[1], "none") == 0)
		newstate = JOBBREAK_NONE;
	else if(gu_strcasecmp(argv[1], "signal") == 0)
		newstate = JOBBREAK_SIGNAL;
	else if(gu_strcasecmp(argv[1], "control-d")==0)
		newstate = JOBBREAK_CONTROL_D;
	else if(gu_strcasecmp(argv[1], "pjl")==0)
		newstate = JOBBREAK_PJL;
	else if(gu_strcasecmp(argv[1], "signal/pjl")==0)
		newstate = JOBBREAK_SIGNAL_PJL;
	else if(gu_strcasecmp(argv[1], "save/restore")==0)
		newstate = JOBBREAK_SAVE_RESTORE;
	else if(gu_strcasecmp(argv[1], "newinterface")==0)
		newstate = JOBBREAK_NEWINTERFACE;
	else if(gu_strcasecmp(argv[1], "default")==0)
		newstate = JOBBREAK_DEFAULT;
	else
		{
		printer_jobbreak_help();
		return EXIT_SYNTAX;
		}

	/* Make sure the printer exists. */
	if(prnopen(printer, TRUE))
		{
		fprintf(errors, _("The printer \"%s\" does not exist.\n"), printer);
		return EXIT_BADDEST;
		}

	/* Modify the printer's configuration file. */
	while(confread())
		{
		if(lmatch(confline, "JobBreak:"))				/* delete "JobBreak:" lines */
			continue;
		if(lmatch(confline, "Interface:"))				/* stop after "Interface:" line */
			{
			conf_printf("%s\n", confline);
			break;
			}
		conf_printf("%s\n", confline);
		}

	/*
	** If the new jobbreak setting is not "default",
	** write a new "JobBreak:" line.
	*/
	if(newstate != JOBBREAK_DEFAULT)
		conf_printf("JobBreak: %d\n", newstate);

	while(confread())							/* copy rest of file, */
		{
		if(lmatch(confline, "JobBreak:"))		/* deleting any further "JobBreak: " lines */
			continue;
		else
			conf_printf("%s\n", confline);
		}
	confclose();

	return EXIT_OK;
	} /* end of printer_jobbreak() */

/*
** Set a printer's feedback flag.
*/
int printer_feedback(const char *argv[])
	{
	const char *printer = argv[0];
	int newstate;

	if( ! am_administrator() )
		return EXIT_DENIED;

	if(! printer || ! argv[1]
		|| ( (newstate=gu_torf(argv[1]))==ANSWER_UNKNOWN && gu_strcasecmp(argv[1],"default") ) )
		{
		fputs(_("You must supply the name of an existing printer\n"
				"and \"true\", \"false\", or \"default\".\n"), errors);
		return EXIT_SYNTAX;
		}

	/* make sure the printer exists */
	if(prnopen(printer, TRUE))
		{
		fprintf(errors, _("The printer \"%s\" does not exist.\n"), printer);
		return EXIT_BADDEST;
		}

	/* Modify the printer's configuration file. */
	while(confread())
		{
		if(lmatch(confline, "Feedback:"))				/* delete "Feedback:" lines */
			continue;
		if(lmatch(confline, "Interface:"))				/* stop after copying the */
			{											/* "Interface:" line */
			conf_printf("%s\n", confline);
			break;
			}
		conf_printf("%s\n", confline);
		}

	/* If the new feedback setting is not "default", */
	/* write a new "Feedback:" line.				 */
	if(newstate != ANSWER_UNKNOWN)
		conf_printf("Feedback: %s\n", newstate ? "True" : "False");

	while(confread())							/* copy rest of file, */
		{
		if(lmatch(confline, "Feedback:"))
			continue;
		else
			conf_printf("%s\n", confline);
		}
	confclose();

	return EXIT_OK;
	} /* end of printer_feedback() */

/*
** Set a printer's codes setting.
*/
static void printer_codes_help(void)
	{
	fputs(_("You must supply the name of an existing printer and a codes setting.\n"
		"Valid codes settings are \"Clean7Bit\", \"Clean8Bit\", \"Binary\", \"TBCP\",\n"
		"\"UNKNOWN\", and \"default\".\n"), errors);
	} /* end of printer_jobbreak_help() */

int printer_codes(const char *argv[])
	{
	const char *printer = argv[0];
	enum CODES newcodes;

	if( ! am_administrator() )
		return EXIT_DENIED;

	if( ! printer || ! argv[1] )
		{
		printer_codes_help();
		return EXIT_SYNTAX;
		}

	if(gu_strcasecmp(argv[1], "default") == 0)
		newcodes = CODES_DEFAULT;
	else if(gu_strcasecmp(argv[1], "UNKNOWN") == 0)
		newcodes = CODES_UNKNOWN;
	else if(gu_strcasecmp(argv[1], "Clean7Bit") == 0)
		newcodes = CODES_Clean7Bit;
	else if(gu_strcasecmp(argv[1], "Clean8Bit") == 0)
		newcodes = CODES_Clean8Bit;
	else if(gu_strcasecmp(argv[1], "Binary")==0)
		newcodes = CODES_Binary;
	else if(gu_strcasecmp(argv[1], "TBCP")==0)
		newcodes = CODES_TBCP;
	else
		{
		printer_codes_help();
		return EXIT_SYNTAX;
		}

	/* Make sure the printer exists. */
	if(prnopen(printer, TRUE))
		{
		fprintf(errors, _("The printer \"%s\" does not exist.\n"), printer);
		return EXIT_BADDEST;
		}

	/* Modify the printer's configuration file. */
	while(confread())
		{
		if(lmatch(confline, "Codes:"))					/* delete "JobBreak:" lines */
			continue;
		if(lmatch(confline, "Interface:"))				/* stop after "Interface:" line */
			{
			conf_printf("%s\n", confline);
			break;
			}
		conf_printf("%s\n", confline);
		}

	/*
	** If the new codes setting is not "DEFAULT",
	** write a new "JobBreak:" line.
	*/
	if(newcodes != CODES_DEFAULT)
		conf_printf("Codes: %d\n", (int)newcodes);

	while(confread())							/* copy rest of file, */
		{
		if(lmatch(confline, "Codes:"))			/* deleting any further "JobBreak: " lines */
			continue;
		else
			conf_printf("%s\n", confline);
		}
	confclose();

	return EXIT_OK;
	} /* end of printer_jobbreak() */

/*
** Set the RIP.
*/
int printer_rip(const char *argv[])
	{
	const char *printer = argv[0], *rip = argv[1], *output_language = argv[2], *options = argv[3];

	if(!printer || (printer && rip && !output_language))
		{
		fputs(_("You must supply the name of an existing printer.  If you supply no other\n"
				"parameters, the RIP setting will revert to the PPD file default.  To select\n"
				"a different RIP, supply the RIP name (such as \"gs\" or \"ppr-gs\"), an\n"
				"output language (such as \"pcl\" or \"other\"), and a RIP options string.\n"), errors);
		return EXIT_SYNTAX;
		}

	if(rip && strlen(rip) == 0)
		{
		fputs(_("The RIP name may not be an empty string.\n"), errors);
		return EXIT_SYNTAX;
		}

	if(output_language && strlen(output_language) == 0)
		{
		fputs(_("The RIP output language may not be an empty string.\n"), errors);
		return EXIT_SYNTAX;
		}

	if(rip && output_language && options && argv[4])
		{
		fputs(_("Too many parameters.  Did you forget to quote the list of options?\n"), errors);
		return EXIT_SYNTAX;
		}

	return conf_set_name(QUEUE_TYPE_PRINTER, printer, "RIP", rip ? "%s %s %s" : NULL, rip, output_language, options ? options : "");
	} /* end of printer_rip() */

/*
** Set a printer's PPD file.
**
** We do this by modifying the configuration file's "PPDFile:" line.  The 
** spooler doesn't keep track of the PPD file, but it needs to know if
** we change it because a printer that wasn't capable of printing a given
** job with one PPD file may be capable according to the new PPD file.
** Thus we tell the spooler that the config file has changed so that it
** will clear the "never" bits.
*/
int printer_ppd(const char *argv[])
	{
	const char *printer = argv[0];
	const char *ppdname = argv[1];
	int retval;

	if( ! am_administrator() )
		return EXIT_DENIED;

	if( ! printer || ! ppdname )
		{
		fputs(_("You must supply the name of an existing printer and a PPD file.\n"), errors);
		return EXIT_SYNTAX;
		}

	/*
	** Make sure the printer exists,
	** opening its configuration file if it does.
	*/
	if(prnopen(printer, TRUE))
		{
		fprintf(errors, _("The printer \"%s\" does not exist.\n"), printer);
		return EXIT_BADDEST;
		}

	/* make sure the PPD file exists */
	{
	FILE *testopen;
	char *ppdfname = ppd_find_file(ppdname);
	if(!(testopen = fopen(ppdfname,"r")))
		fprintf(errors, _("The PPD file \"%s\" does not exist.\n"), ppdname);
	gu_free(ppdfname);
	if(!testopen)
		{
		confabort();
		return EXIT_NOTFOUND;
		}
	fclose(testopen);
	}

	/* Get ready to collect information for a "DefFiltOpts:" line. */
	deffiltopts_open();

	/* Consider the printer's PPD file.  If this fails, stop. */
	if((retval = deffiltopts_add_ppd(printer, ppdname, (char*)NULL)) != EXIT_OK)
		{
		confabort();
		deffiltopts_close();
		return retval;
		}

	/*
	** Modify the printer's configuration file.
	**
	** First, copy up to the first "PPDFile:" lines, deleting
	** "DefFiltOpts:" lines as we go.
	*/
	while(confread())
		{
		if(lmatch(confline, "PPDFile:"))				/* stop at */
			break;

		if(lmatch(confline, "DefFiltOpts:"))			/* delete */
			continue;

		if(lmatch(confline, "PPDOpt:"))					/* delete */
			continue;

		conf_printf("%s\n", confline);
		}

	/* Write the new "PPDFile:" lines. */
	conf_printf("PPDFile: %s\n", ppdname);

	/*
	** Copy the rest of the file, deleting "DefFiltOpts:" lines
	** and extra "PPDFile:" lines.
	*/
	while(confread())
		{
		if(lmatch(confline, "PPDFile:"))				/* delete */
			continue;

		if(lmatch(confline, "DefFiltOpts:"))			/* delete */
			continue;

		if(lmatch(confline, "PPDOpt:"))					/* delete */
			continue;

		conf_printf("%s\n", confline);
		}

	/* Insert a new "DefFiltOpts:" line. */
	conf_printf("DefFiltOpts: %s\n", deffiltopts_line() );

	/* Free any remaining deffiltopts data space. */
	deffiltopts_close();

	/* Close the printer configuration file. */
	confclose();

	/* Tell pprd we have changed the printer's configuration so that
	   it can clear the never bits for this printer.
	   */
	reread_printer(printer);

	/* Update any groups which have this printer as a member. */
	return update_groups_deffiltopts(printer);
	} /* end of printer_ppd() */

/*
** Change the configuration file "Alert: " lines which
** tells where to send alerts and how often.
*/
int printer_alerts(const char *argv[])
	{
	const char *printer = argv[0];
	int frequency;
	const char *method = argv[2];
	const char *address = argv[3];

	if( ! am_administrator() )
		return EXIT_DENIED;

	if( ! printer || ! argv[1]
				|| (strspn(argv[1], "-0123456789") != strlen(argv[1]))
				|| ! method || ! address )
		{
		fputs(_("You must supply the name of a printer, an alert frequency, such as \"7\",\n"
				"an alert method, such as \"mail\", and an alert address,\n"
				"such as \"alertreaders@domain.com\".\n"), errors);
		return EXIT_SYNTAX;
		}

	frequency = atoi(argv[1]);

	/* make sure the printer exists */
	if(prnopen(printer, TRUE))
		{
		fprintf(errors, _("The printer \"%s\" does not exist.\n"), printer);
		return EXIT_BADDEST;
		}

	/* Modify the printer's configuration file. */
	while(confread())
		{
		if(lmatch(confline, "Alert:"))
			break;
		else
			conf_printf("%s\n", confline);
		}

	conf_printf("Alert: %d %s %s\n", frequency, method, address);

	while(confread())							/* copy rest of file, */
		{										/* deleting further */
		if(lmatch(confline, "Alert:"))			/* "Alert:" lines. */
			continue;
		else
			conf_printf("%s\n", confline);
		}

	confclose();

	reread_printer(printer);	/* tell pprd to re-read the configuration */

	return EXIT_OK;
	} /* end of printer_alert() */

/*
** Change the configuration file "Alert:" line to change the
** alert frequency.
** The absence of an  "Alert:" line is an error.
*/
int printer_frequency(const char *argv[])
	{
	const char *printer;
	int frequency;						/* dispatch every frequency alerts */
	char *method = (char*)NULL;			/* by method */
	char *address = (char*)NULL;		/* to address */

	if( ! am_administrator() )
		return EXIT_DENIED;

	if( ! argv[0] || ! argv[1] )
		{
		fputs(_("You must specify a printer and a new alert frequency.\n"), errors);
		return EXIT_SYNTAX;
		}

	printer = argv[0];
	frequency = atoi(argv[1]);

	/* make sure the printer exists */
	if(prnopen(printer, TRUE))
		{
		fprintf(errors, _("The printer \"%s\" does not exist.\n"), printer);
		return EXIT_BADDEST;
		}

	/* Modify the printer's configuration file. */
	while(confread())
		{
		if(lmatch(confline, "Alert:"))
			{
			int x=6;	/* 7 is length of "Alert: " */
			int len;

			x += strspn(&confline[x]," \t0123456789");	/* skip spaces and */
														/* digits */
			len=strcspn(&confline[x]," \t");			/* get word length */
			method = gu_strndup(&confline[x], len);
			x += len;									/* move past word */
			x += strspn(&confline[x]," \t");			/* skip spaces */

			len = strcspn(&confline[x]," \t\n");		/* get length */
			address = gu_strndup(&confline[x],len);

			break;
			}
		else
			{
			conf_printf("%s\n",confline);
			}
		}

	if(! method || ! address)
		{
		confabort();
		fputs(_("No alert method and address defined, use \"ppad alerts\".\n"), errors);
		return EXIT_NOTFOUND;
		}

	conf_printf("Alert: %d %s %s\n",frequency,method,address);

	while(confread())							/* copy rest of file, */
		{										/* deleting further */
		if(lmatch(confline, "Alert:"))			/* "Alert:" lines. */
			continue;
		else
			conf_printf("%s\n",confline);
		}

	confclose();				/* put new file into place */

	reread_printer(printer);	/* tell pprd to re-read the configuration */

	return EXIT_OK;
	} /* end of printer_frequency() */

/*
** Change the configuration file "FlagPages:" line which
** tells whether or not to print banner and trailer pages.
*/
int printer_flags(const char *argv[])
	{
	const char *printer = argv[0];
	int banner;
	#ifdef GNUC_HAPPY
	int trailer=0;
	#else
	int trailer;
	#endif

	if(! printer || ! argv[1] || ! argv[2])
		{
		fputs(_("You must supply the name of an existing printer, a new banner\n"
				"option, and a new trailer option.  Valid banner and trailer\n"
				"options are \"never\", \"no\", \"yes\", and \"always\".\n"), errors);
		return EXIT_SYNTAX;
		}

	if((banner = flag_code(argv[1])) == BANNER_INVALID
				|| (trailer = flag_code(argv[2])) == BANNER_INVALID)
		{
		fputs(_("Banner and trailer must be set to \"never\", \"no\",\n"
				"\"yes\", or \"always\".\n"), errors);
		return EXIT_SYNTAX;
		}

	return conf_set_name(QUEUE_TYPE_PRINTER, printer, "FlagPages", "%d %d", banner, trailer);
	} /* end of printer_flags() */

/*
** Change the configuration "OutputOrder:" line.
** The direction may be "Normal", "Reverse" or "ppd".
** If the direction is "ppd", the "OutputOrder:" line is deleted
** from the configuration file.
*/
int printer_outputorder(const char *argv[])
	{
	const char *printer = argv[0];
	const char *newstate;

	if(! printer || ! argv[1])
		{
		fputs(_("You must supply the name of an existing printer and\n"
				"a new outputorder.\n"), errors);
		return EXIT_SYNTAX;
		}

	if(gu_strcasecmp(argv[1], "Normal") == 0)
		newstate = "Normal";
	else if(gu_strcasecmp(argv[1], "Reverse") == 0)
		newstate = "Reverse";
	else if(gu_strcasecmp(argv[1], "PPD") == 0)
		newstate = NULL;
	else
		{
		fputs(_("Set outputorder to \"Normal\", \"Reverse\", or \"PPD\".\n"), errors);
		return EXIT_SYNTAX;
		}

	if(newstate)
		return conf_set_name(QUEUE_TYPE_PRINTER, printer, "OutputOrder", "%s", newstate);
	else
		return conf_set_name(QUEUE_TYPE_PRINTER, printer, "OutputOrder", NULL);
	} /* end of printer_direction() */

/*
** Change the charge made for printing.
**
** !!! BUG BUG BUG !!!
** We get the printer configuration re-read, but
** we really ought to get the group configurations
** re-read too.
*/
int printer_charge(const char *argv[])
	{
	const char *printer = argv[0];
	int retval;

	if(! printer || ! argv[1])
		{
		fputs(_("Insufficient parameters.  You must supply the name of a printer\n"
				"and an amount to change per sheet.  If you wish, you may specify\n"
				"an amount to charge per duplex sheet and then an amount to charge\n"
				"per simplex sheet.  If you specify only one amount, it will apply\n"
				"to both.  To remove a printer charge, set the charge to \"none\".\n"
				"In contrast, setting the charge to \"0.00\" will mean that only users\n"
				"with charge accounts may print, even though they will not be charged\n"
				"any money.\n"), errors);
		return EXIT_SYNTAX;
		}

	if(argv[1][0] == '\0' || strcmp(argv[1], "none") == 0)
		{
		if(argv[2] && strlen(argv[2]) > 0)
			{
			fprintf(errors, _("A second parameter is not allowed because first is \"%s\"."), argv[1]);
			return EXIT_SYNTAX;
			}
		retval = conf_set_name(QUEUE_TYPE_PRINTER, printer, "Charge", NULL);
		}
	else
		{
		int newcharge_duplex_sheet, newcharge_simplex_sheet;

		/* Check for proper syntax of values.  This check isn't really rigorous enough!!! */
		if(strspn(argv[1], "0123456789.") != strlen(argv[1]))
			{
			fprintf(errors, _("The value \"%s\" is not in the correct format for decimal currency."), argv[1]);
			return EXIT_SYNTAX;
			}
		if(argv[2] && strspn(argv[2], "0123456789.") != strlen(argv[2]))
			{
			fprintf(errors, _("The value \"%s\" is not in the correct format for decimal currency."), argv[2]);
			return EXIT_SYNTAX;
			}

		/* Convert to floating point dollars, convert to cents, and truncate to int. */
		newcharge_duplex_sheet = (int)(gu_getdouble(argv[1]) * 100.0 + 0.5);

		if(argv[2])
			newcharge_simplex_sheet = (int)(gu_getdouble(argv[2]) * 100.0 + 0.5);
		else
			newcharge_simplex_sheet = newcharge_duplex_sheet;

		retval = conf_set_name(QUEUE_TYPE_PRINTER, printer, "Charge", "%d.%02d %d.%02d",
				newcharge_duplex_sheet / 100,
				newcharge_duplex_sheet % 100,
				newcharge_simplex_sheet / 100,
				newcharge_simplex_sheet % 100);
		}

	if(retval == EXIT_OK)
		reread_printer(printer);

	return retval;
	} /* end of printer_charge() */

/*
** Extract the printer bins list from the PPD file
** and put it in the configuration file.
**
** This will overwrite any bin list in the configuration file.
*/
int printer_bins_ppd(const char *argv[])
	{
	const char *printer;				/* name of printer whose configuration should be changed */
	char *ppdname = (char*)NULL;
	char *ppdline;						/* a line read from the PPD file */
	int x;
	int ret;

	if( ! am_administrator() )
		return EXIT_DENIED;

	if(! (printer=argv[0]))
		{
		fputs(_("You must supply the name of an existing printer.\n"), errors);
		return EXIT_SYNTAX;
		}

	if(argv[1])
		{
		fputs(_("Too many parameters.\n"), errors);
		return EXIT_SYNTAX;
		}

	/* make sure the printer exists */
	if(prnopen(printer, TRUE))
		{
		fprintf(errors, _("The printer \"%s\" does not exist.\n"), printer);
		return EXIT_BADDEST;
		}

	/* Modify the printer's configuration file. */
	while(confread())							/* copy all lines */
		{
		if(lmatch(confline, "PPDFile:"))
			{
			gu_sscanf(confline,"PPDFile: %Z", &ppdname);
			}

		if(lmatch(confline, "Bin:"))			/* don't copy those */
			continue;							/* that begin with "Bin: " */
		else
			conf_printf("%s\n", confline);
		}

	if(! ppdname)
		{
		fputs(_("Printer configuration file does not have a \"PPDFile:\" line.\n"), errors);
		confabort();
		return EXIT_NOTFOUND;
		}

	if((ret = ppd_open(ppdname, errors)))
		{
		confabort();
		return ret;
		}

	while((ppdline = ppd_readline()))
		{
		if(lmatch(ppdline, "*InputSlot"))		/* Use only "*InputSlot" */
			{
			x = 10;
			x += strspn(&ppdline[x]," \t");		/* Skip to start of options keyword */

			ppdline[x+strcspn(&ppdline[x],":/")] = '\0';
												/* terminate at start of translation or code */
			conf_printf("Bin: %s\n", &ppdline[x]);
			}
		}

	confclose();

	/* Instruct pprd to reread the printer configuration file. */
	reread_printer(printer);

	return EXIT_OK;
	} /* end of printer_bins_ppd() */

/*
** Add bin(s) to a printer's bin list.
*/
int printer_bins_set_or_add(gu_boolean add, const char *argv[])
	{
	const char *printer, *bin;
	int idx;					/* the bin we are working on */
	int count = 0;				/* number of bins in conf file */
	gu_boolean duplicate;

	if(! am_administrator())
		return EXIT_DENIED;

	if(! (printer=argv[0]) || ! argv[1])
		{
		fputs(_("You must specify an existing printer and a new bin name or names.\n"), errors);
		return EXIT_SYNTAX;
		}

	if(prnopen(printer, TRUE))
		{
		fprintf(errors, _("The printer \"%s\" does not exist.\n"), printer);
		return EXIT_BADDEST;
		}

	/* Copy up to the first "Bin:" line. */
	while(confread())
		{
		if(lmatch(confline, "Bin:"))
			break;
		conf_printf("%s\n", confline);
		}

	/* Delete or copy all the "Bin:" lines */
	duplicate = FALSE;
	do	{
		if(!(bin = lmatchp(confline, "Bin:")))
			break;

		if(add)
			{
			for(idx=1; argv[idx]; idx++)
				{
				if(strcmp(bin, argv[idx]) == 0)
					{
					fprintf(errors, _("Printer \"%s\" already has a bin called \"%s\".\n"), printer, argv[idx]);
					duplicate = TRUE;
					}
				}

			conf_printf("%s\n", confline);
			count++;
			}
		} while(confread());

	if(duplicate)
		{
		confabort();
		return EXIT_ALREADY;
		}

	/* Add the new "Bin:" lines. */
	for(idx=1; argv[idx]; idx++, count++)
		conf_printf("Bin: %s\n", argv[idx]);

	/* If this would make for too many bins, abort the changes. */
	if(count > MAX_BINS)
		{
		fprintf(errors, _("Can't add these %d bin%s to \"%s\" because then the printer would\n"
				"have %d bins and the %d bin limit would be exceeded.\n"),
				idx - 1, idx == 2 ? "" : "s",
				printer, count, MAX_BINS);
		confabort();
		return EXIT_OVERFLOW;
		}

	/* Copy the rest of the file */
	while(confread())
		conf_printf("%s\n",confline);

	/* Commit to the changes. */
	confclose();

	/* We must inform pprd because it keeps track of mounted media. */
	reread_printer(printer);

	return EXIT_OK;
	} /* end of printer_bins_set_or_add() */

/*
** Remove bin(s) from a printer's bin list.
*/
int printer_bins_delete(const char *argv[])
	{
	const char *printer;
	int idx;
	int misses;
	char *ptr;
	int found[MAX_BINS];

	if(! am_administrator())
		return EXIT_DENIED;

	if(! (printer=argv[0]) || ! argv[1])
		{
		fputs(_("You must specify a printer and a bin or bins to remove from it.\n"), errors);
		return EXIT_SYNTAX;
		}

	/* Set a checklist entry for each bin to be removed to no. */
	for(idx=1; argv[idx] != (char*)NULL; idx++)
		{
		if(idx > MAX_BINS)
			{
			fprintf(errors, _("You can't delete more than %d bins at a time.\n"), MAX_BINS);
			return EXIT_SYNTAX;
			}
		found[idx - 1] = FALSE;
		}

	if(prnopen(printer, TRUE))
		{
		fprintf(errors, _("The printer \"%s\" does not exist.\n"), printer);
		return EXIT_BADDEST;
		}

	/* Copy the file, removing the bin lines that match as we go. */
	while(confread())
		{
		if(lmatch(confline, "Bin:"))
			{
			/* Make a pointer to the bin name */
			ptr = &confline[5 + strspn(&confline[5], " \t")];

			/* Search the delete list for the bin. */
			for(idx=1; argv[idx]; idx++)
				{
				if(strcmp(ptr, argv[idx]) == 0)
					{
					found[idx - 1] = TRUE;
					break;
					}
				}

			/* If this bin was in the delete list, don't keep it. */
			if(argv[idx])
				continue;
			}

		conf_printf("%s\n", confline);
		}

	/* Check and see if all of the bins were found. */
	misses = 0;
	for(idx=1; argv[idx]; idx++)
		{
		if(! found[idx - 1])
			{
			fprintf(errors, _("The printer \"%s\" does not have a bin called \"%s\".\n"), printer, argv[idx]);
			misses++;
			}
		}

	/* If any of the bins to be deleted are not found, cancel whole command. */
	if(misses)
		{
		confabort();
		return EXIT_BADBIN;
		}

	/* Commit to the changes */
	confclose();

	/* We must inform pprd because it keeps track of mounted media. */
	reread_printer(printer);

	return EXIT_OK;
	} /* end of printer_bins_delete() */

/*
** Delete a printer configuration file and inform the spooler
** that we have deleted it so it can do so too.
*/
int printer_delete(const char *argv[])
	{
	const char function[] = "printer_delete";
	const char *printer = argv[0];
	char fname[MAX_PPR_PATH];
	DIR *dir;
	struct dirent *direntp;
	int len;
	int is_a_member;

	if( ! am_administrator() )
		return EXIT_DENIED;

	if(!printer)
		{
		fputs(_("You must specify a printer to delete.\n"), errors);
		return EXIT_SYNTAX;
		}

	ppop2("halt", printer);				/* halt printer */
	ppop2("reject", printer);			/* accept no more jobs */
	ppop2("purge", printer);			/* cancel all existing jobs */

	/* Remove the printer from membership in each */
	/* and every group.  Rather laborious, don't */
	/* you think? */
	if((dir = opendir(GRCONF)) == (DIR*)NULL)
		{
		fprintf(errors, "%s(): opendir() failed\n", function);
		return EXIT_INTERNAL;
		}

	while((direntp = readdir(dir)) != (struct dirent*)NULL)
		{
		if( direntp->d_name[0] == '.' )			/* skip . and .. */
			continue;							/* and temporary files */

		len=strlen(direntp->d_name);			/* Emacs style backup files */
		if( len > 0 && direntp->d_name[len-1]=='~' )
			continue;							/* should be skipt. */

		is_a_member = FALSE;

		if( grpopen(direntp->d_name,FALSE) )
			{
			fprintf(errors, "%s(): grpopen(\"%s\",FALSE) failed", function, direntp->d_name);
			closedir(dir);
			return EXIT_INTERNAL;
			}

		while(confread())
			{
			if(lmatch(confline, "Printer:"))
				{
				char *p = &confline[8];
				p += strspn(confline, " \t");
				if(strcmp(p, printer) == 0)
					{
					is_a_member = TRUE;
					break;
					}
				}
			}

		confclose();

		if(is_a_member)
			_group_remove(direntp->d_name,printer);
		}

	closedir(dir);

	/* Remove the printer configuration file. */
	ppr_fnamef(fname, "%s/%s", PRCONF, printer);
	if(unlink(fname))
		{
		if(errno==ENOENT)
			{
			fprintf(errors, _("The printer \"%s\" does not exist.\n"), printer);
			return EXIT_BADDEST;
			}
		else
			{
			fprintf(errors, "unlink(\"%s\") failed, errno=%d (%s)\n", fname, errno, gu_strerror(errno));
			return EXIT_INTERNAL;
			}
		}
	else				/* It worked, now remove the mounted file, */
		{				/* alert log, and status file. */
		ppr_fnamef(fname, "%s/%s", MOUNTEDDIR, printer);
		unlink(fname);

		ppr_fnamef(fname, "%s/%s", ALERTDIR, printer);
		unlink(fname);

		ppr_fnamef(fname, "%s/%s", STATUSDIR, printer);
		unlink(fname);

		ppr_fnamef(fname, "%s/%s", ADDRESS_CACHE, printer);
		unlink(fname);

		reread_printer(printer);

		return EXIT_OK;
		}
	} /* end of printer_delete() */

/*
** Just tell the spooler to re-read the configuration file.
*/
int printer_touch(const char *argv[])
	{
	const char *printer = argv[0];

	if( ! am_administrator() )
		return EXIT_DENIED;

	if(! printer)
		{
		fputs(_("You must supply the name of a printer.\n"), errors);
		return EXIT_SYNTAX;
		}

	/* make sure the printer exists */
	if(prnopen(printer, FALSE))
		{
		fprintf(errors, _("The printer \"%s\" does not exist.\n"), printer);
		return EXIT_BADDEST;
		}

	confclose();

	reread_printer(printer);	/* tell pprd to re-read the configuration */

	return EXIT_OK;
	} /* end of printer_touch() */

/*
** Change the switchset line in the configuration file.
*/
int printer_switchset(const char *argv[])
	{
	const char *printer = argv[0];		/* name of printer */
	char newset[256];					/* new set of switches */

	if( ! printer )
		{
		fputs(_("You must supply the name of a printer and a set of switches.\n"), errors);
		return EXIT_SYNTAX;
		}

	/* convert the switch set to a line */
	if(make_switchset_line(newset, &argv[1]))
		{
		fputs(_("Bad set of switches.\n"), errors);
		return EXIT_SYNTAX;
		}

	return conf_set_name(QUEUE_TYPE_PRINTER, printer, "Switchset", newset[0] ? "%s" : NULL, newset);
	} /* end of printer_switchset() */

/*
** This command is called to re-read the printer's "PPDFile:"
** line and construct an appropriate "DefFiltOpts:" line.
**
** The "ppad ppd" and "ppad ppdopts" commands do this too.
*/
int printer_deffiltopts(const char *argv[])
	{
	const char *printer = argv[0];
	char *PPDFile = (char*)NULL;
	char *InstalledMemory = (char*)NULL;

	if( ! am_administrator() )
		return EXIT_DENIED;

	if(! printer)
		{
		fputs(_("You must supply the name of a printer.\n"), errors);
		return EXIT_SYNTAX;
		}

	if(prnopen(printer, TRUE))
		{
		fprintf(errors, _("The printer \"%s\" does not exist.\n"), printer);
		return EXIT_BADDEST;
		}

	/* Get ready to collect information for the line. */
	deffiltopts_open();

	/* Modify the printer's configuration file. */
	while(confread())
		{
		if(lmatch(confline, "DefFiltOpts:"))			/* delete */
			continue;

		if(lmatch(confline, "PPDFile:"))
			{
			int len;

			if(PPDFile)
				{
				fputs(_("WARNING: all but the last \"PPDFile:\" line ignored.\n"), errors);
				gu_free(PPDFile);
				}

			PPDFile = &confline[8];
			PPDFile += strspn(PPDFile, " \t");
			len = strlen(PPDFile);
			while( --len >= 0 )
				if( isspace( PPDFile[len] ) )
					PPDFile[len] = '\0';
				else
					break;

			PPDFile = gu_strdup(PPDFile);
			}

		else if(lmatch(confline, "PPDOpt: *InstalledMemory "))
			{
			if(InstalledMemory) gu_free(InstalledMemory);
			InstalledMemory = gu_strndup(&confline[25], strcspn(&confline[25], " "));
			}

		conf_printf("%s\n",confline);
		}

	if(PPDFile)
		{
		deffiltopts_add_ppd(printer, PPDFile, InstalledMemory);
		gu_free(PPDFile);
		}

	if(InstalledMemory) gu_free(InstalledMemory);

	conf_printf("DefFiltOpts: %s\n", deffiltopts_line() );

	deffiltopts_close();

	confclose();

	return EXIT_OK;
	} /* end of printer_deffiltopts() */

/*
** Set the printer "PassThru:" line.
*/
int printer_passthru(const char *argv[])
	{
	const char *printer = argv[0];
	char *passthru;
	int retval;

	if(!printer)
		{
		fputs(_("You must specify a printer and a (possibly empty) list\n"
				"of file types.  These file types should be the same as\n"
				"those used with the \"ppr -T\" option.\n"), errors);
		return EXIT_SYNTAX;
		}

	passthru = list_to_string(&argv[1]);
	retval = conf_set_name(QUEUE_TYPE_PRINTER, printer, "PassThru", passthru ? "%s" : NULL, passthru);
	if(passthru) gu_free(passthru);

	return retval;
	} /* end of printer_passthru() */

/*
** This command opens the PPD file and asks the user for an answer for each
** option found therein.  For each answer it generates a "PPDOpts:" line
** for the printer configuration file.
*/
int printer_ppdopts(const char *argv[])
	{
	const char function[] = "printer_ppdopts";
	const char *printer;				/* printer whose configuration should be edited */
	const char **answers = (const char **)NULL;
	char *PPDFile = (char*)NULL;		/* name of PPD file to open */
	char *InstalledMemory = (char*)NULL;
	char *ppdline;						/* a line read from the PPD file */
	char *ui_open;						/* the UI section we are int, NULL otherwise */
	int ui_open_mrlen;
	char *ptr;
	unsigned int next_value;
	int c;
	char *values[100];					/* the list of possible values */
	int ret;

	/* Make sure we have the necessary authority. */
	if( ! am_administrator() )
		return EXIT_DENIED;

	/* Make sure the required parameter is supplied. */
	if(! (printer = argv[0]))
		{
		fputs(_("You must supply the name of a printer.\n"), errors);
		return EXIT_SYNTAX;
		}

	/* Are there answers on the command line? */
	if(argv[1])
		{
		answers = &argv[1];
		}

	/* Open the printer's configuration file for modification. */
	if(prnopen(printer, TRUE))
		{
		fprintf(errors, _("The printer \"%s\" does not exist.\n"), printer);
		return EXIT_BADDEST;
		}

	/*
	** Set whole values array to NULL so we will
	** later know what we must gu_free().
	*/
	for(next_value=0; next_value < (sizeof(values)/sizeof(char*)); next_value++)
		values[next_value] = (char*)NULL;

	/*
	** Copy the existing printer configuration file, discarding
	** "PPDOpt:" and "DefFiltOpts:" lines and noting which
	** PPD file is to be used.
	*/
	while(confread())
		{
		if(lmatch(confline, "PPDOpt:"))					/* delete */
			continue;

		if(lmatch(confline, "DefFiltOpts:"))			/* delete */
			continue;

		conf_printf("%s\n", confline);					/* copy to output file */

		if(gu_sscanf(confline, "PPDFile: %A", &ptr) == 1)
			{
			if(PPDFile) gu_free(PPDFile);
			PPDFile = ptr;
			}
		}

	/* If there was no "PPDFile:" line, we can't continue. */
	if(!PPDFile)
		{
		fprintf(errors, _("The printer \"%s\" has no \"PPDFile:\" line in its configuration file.\n"), printer);
		confabort();
		return EXIT_BADDEST;
		}

	/* Open the PPD file. */
	if((ret = ppd_open(PPDFile, errors)))
		{
		confabort();
		return ret;
		}

	/*
	** Read the PPD file, ask the questions, and
	** generate "PPDOpts:" lines.
	*/
	ui_open = (char*)NULL;
	next_value = 0;
	if(!answers) putchar('\n');
	while((ppdline = ppd_readline()))
		{
		if((ptr = lmatchp(ppdline, "*OpenUI")))
			{
			/*
			** If we already have a pointer to the name of the current
			** UI block, since we have just seen the start of a new one,
			** we know that there was an unclosed one since if it had been
			** closed the pointer would have been set to NULL.
			**
			** After printing the warning, we need only free the string.
			** In order to discard the values from the unclosed block,
			** next_value is set to 1 a little later.  We must set ui_open
			** to NULL here since if we aren't interested in the new section,
			** we will not save its name in ui_open.
			*/
			if(ui_open)
				{
				fprintf(errors, _("WARNING: Unclosed UI block \"%s\" in PPD file.\n"), ui_open);
				gu_free(ui_open);
				ui_open = (char*)NULL;
				}

			/* If this is not an option UI block, we are not interested. */
			if(!lmatch(ptr, "*Option") && !lmatch(ptr, "*InstalledMemory"))
				continue;

			/* Truncate after the end of the translation string. */
			ptr[strcspn(ptr, ":")] = '\0';

			/* Print the option name and its translation string. */
			if(!answers)
				printf("%s\n", ptr);

			/* Save the option name and translation string. */
			ui_open = gu_strdup(ptr);
			ui_open_mrlen = strcspn(ptr, "/");

			/* Set to indicated no accumulated values. */
			next_value = 1;

			continue;
			}

		/* If between "*OpenUI" and "*CloseUI" and this is an "*Option" or "*InstalledMemory" line, */
		if(ui_open && (lmatch(ppdline, "*Option") || lmatch(ppdline, "*InstalledMemory")) )
			{
			/* If the first part of the name of this option doesn't match the part of the
			   UI section name before the start of the translation string, */
			if(strcspn(ppdline, "/ \t") != ui_open_mrlen || strncmp(ppdline, ui_open, ui_open_mrlen))
				{
				fprintf(errors, _("WARNING: spurious option found in UI section \"%s\".\n"), ui_open);
				continue;
				}

			ptr = ppdline;
			ptr += strcspn(ptr, " \t");
			ptr += strspn(ptr, " \t");

			/* Truncate after option name and translation string. */
			ptr[strcspn(ptr, ":")] = '\0';

			if(!answers) printf("%d) %s\n", next_value, ptr);

			if(next_value >= (sizeof(values) / sizeof(char*)))
				{
				fprintf(errors, "%s(): values[] overflow\n", function);
				confabort();
				return EXIT_INTERNAL;
				}

			if(values[next_value]) gu_free(values[next_value]);
			values[next_value++] = gu_strdup(ptr);

			continue;
			}

		/* If this is the end of an option, ask for a choice. */
		if(ui_open && (ptr = lmatchp(ppdline, "*CloseUI:")))
			{
			if( strncmp(ptr, ui_open, strcspn(ui_open, "/")) )
				{
				fputs(_("WARNING: mismatched \"*OpenUI\", \"*CloseUI\" in PPD file.\n"), errors);
				fprintf(errors, "(\"%s\" closed by \"%.*s\".)\n", ui_open, (int)strcspn(ptr,"/\n"), ptr);
				}

			/* If there are answers on the command line, */
			if(answers)
				{
				int x, y;
				gu_boolean found = FALSE;
				gu_boolean valid = FALSE;
				int value_len;
				c = 0;
				for(x=0; answers[x] && answers[x+1]; x+=2)
					{
					for(y=1; y < next_value; y++)
						{
						#if 0
						printf("x=%d, y=%d\n", x, y);
						printf("answers[x]=\"%s\", answers[x+1]=\"%s\", ui_open=\"%s\", ui_open_mrlen=%d, values[y]=\"%s\"\n", answers[x], answers[x+1], ui_open, ui_open_mrlen, values[y]);
						#endif
						if(strlen(answers[x]) == ui_open_mrlen && strncmp(answers[x], ui_open, ui_open_mrlen) == 0)
							{
							found = TRUE;
							if(strlen(answers[x+1]) == (value_len = strcspn(values[y], "/")) && strncmp(answers[x+1], values[y], value_len) == 0)
								{
								c = y;
								valid = TRUE;	/* set flag but don't break */
								}
							}
						}
					}
				if(!found)
					{
					fprintf(errors, _("Warning:	 No value provided for \"%s\".\n"), ui_open);
					}
				else if(!valid)
					{
					fprintf(errors, _("Value provided for \"%s\" is not among the possible values.\n"), ui_open);
					confabort();
					return EXIT_SYNTAX;
					}
				}

			/* Otherwise, present the user with the list of options we have gathered. */
			else
				{
				char temp[10];
				do	{
					printf(_("Choose 1 thru %d or 0 if unknown> "), next_value-1);
					fflush(stdout);
					if(fgets(temp, sizeof(temp), stdin) == (char*)NULL)
						{
						c = 0;
						break;
						}
					c = atoi(temp);
					} while( ! isdigit( temp[strspn(temp," \t")] ) || c >= next_value );

				fputc('\n', stdout);
				}

			/*
			** If the user did not choose zero (unknown), write
			** a line containing the option and the chosen value
			** into the printer configuration file.
			*/
			if(c != 0)
				{
				/* Print a line with the selected option. */
				conf_printf("PPDOpt: %.*s %.*s", strcspn(ui_open, "/"), ui_open, strcspn(values[c],"/"), values[c]);

				/* If translation strings are provided for both the category and the
				   selected option, print them afterward so that they can later
				   be shown in the "ppad show" output. */
				if( strchr(ui_open, '/') && strchr(values[c],'/'))
					conf_printf(" (%s %s)", (strchr(ui_open, '/') + 1), (strchr(values[c], '/') + 1));

				/* End the configuration file line. */
				conf_printf("\n");

				/* If this is the amount of installed memory, feed the
				   value to the code which is generating the "DefFiltOpts:" line. */
				if(lmatch(ui_open, "*InstalledMemory"))
					 InstalledMemory = gu_strndup(values[c], strcspn(values[c], "/"));
				}

			gu_free(ui_open);
			ui_open = (char*)NULL;
			}
		} /* end of PPD reading loop */

	/* Sanity check. */
	if(ui_open)
		{
		fprintf(errors, _("WARNING: Unclosed UI block in PPD file: \"%s\".\n"), ui_open);
		gu_free(ui_open);
		}

	/* Emmit a new "DefFiltOpts:" line. */
	deffiltopts_open();
	if((ret = deffiltopts_add_ppd(printer, PPDFile, InstalledMemory)))
		{
		confabort();
		return ret;
		}
	conf_printf("DefFiltOpts: %s\n", deffiltopts_line() );
	deffiltopts_close();

	/* Close the new configuration file and move it into place. */
	confclose();

	/* Tell pprd, since it might want to clear the never flag for this printer. */
	reread_printer(printer);

	/* Free any lingering memory blocks */
	if(PPDFile) gu_free(PPDFile);
	if(InstalledMemory) gu_free(InstalledMemory);
	for(next_value=0; next_value < (sizeof(values)/sizeof(char*)); next_value++)
		{
		if(values[next_value])
			gu_free(values[next_value]);
		}

	/*
	** Update the default filter options of any
	** groups which have this printer as a member.
	*/
	return update_groups_deffiltopts(printer);
	} /* end of printer_ppdopts() */

/*
** Set a printer's pages limit.
*/
int printer_limitpages(const char *argv[])
	{
	const char *printer = argv[0];
	int limit_lower, limit_upper;
	int ret;

	if(!printer || !argv[1] || !argv[2])
		{
		fputs(_("You must supply the name of an existing printer and the minimum and maximum number\n"
				"of pages of the documents it should be allowed to print.  To clear either limit,\n"
				"set it to 0.\n"), errors);
		return EXIT_SYNTAX;
		}

	if((limit_lower = atoi(argv[1])) < 0)
		{
		fputs(_("The lower limit must be 0 (unlimited) or a positive integer.\n"), errors);
		return EXIT_SYNTAX;
		}

	if((limit_upper = atoi(argv[2])) < 0)
		{
		fputs(_("The upper limit must be 0 (unlimited) or a positive integer.\n"), errors);
		return EXIT_SYNTAX;
		}

	ret = conf_set_name(QUEUE_TYPE_PRINTER, printer, "LimitPages", (limit_lower > 0 || limit_upper > 0) ? "%d %d" : NULL, limit_lower, limit_upper);

	reread_printer(printer);

	return ret;
	} /* end or printer_limitpages() */

/*
** Set a printer's kilobytes limit.
*/
int printer_limitkilobytes(const char *argv[])
	{
	const char *printer = argv[0];
	int limit_lower, limit_upper;
	int ret;

	if(!printer || !argv[1] || !argv[2])
		{
		fputs(_("You must supply the name of an existing printer and the minimum and maximum number\n"
				"of kilobytes of the documents it should be allowed to print.  To clear either limit,\n"
				"set it to 0.\n"), errors);
		return EXIT_SYNTAX;
		}

	if((limit_lower = atoi(argv[1])) < 0)
		{
		fputs(_("The lower limit must be 0 (unlimited) or a positive integer.\n"), errors);
		return EXIT_SYNTAX;
		}

	if((limit_upper = atoi(argv[2])) < 0)
		{
		fputs(_("The upper limit must be 0 (unlimited) or a positive integer.\n"), errors);
		return EXIT_SYNTAX;
		}

	ret = conf_set_name(QUEUE_TYPE_PRINTER, printer, "LimitKilobytes", (limit_lower > 0 || limit_upper > 0) ? "%d %d" : NULL, limit_lower, limit_upper);

	reread_printer(printer);

	return ret;
	} /* end or printer_limitkilobytes() */

/*
** Set the printer's "GrayOK:" line.
*/
int printer_grayok(const char *argv[])
	{
	const char *printer = argv[0];
	gu_boolean grayok;
	int ret;

	if(!printer || !argv[1])
		{
		fputs(_("You must specify a printer and a boolean value.\n"),
				errors);
		return EXIT_SYNTAX;
		}

	if(gu_torf_setBOOL(&grayok, argv[1]) == -1)
		{
		fputs(_("Value is not boolean.\n"), errors);
		return EXIT_SYNTAX;
		}

	/* If FALSE, set to 0, otherwise delete line. */
	ret = conf_set_name(QUEUE_TYPE_PRINTER, printer, "GrayOK", grayok ? NULL : "false");

	/* This change may change the eligibility of some jobs. */
	if(grayok)
		reread_printer(printer);

	return ret;
	} /* end of printer_grayok() */
/*
** Set the printer's "ACLs:" line.
*/
int printer_acls(const char *argv[])
	{
	const char *printer = argv[0];
	char *acls;
	int retval;

	if(!printer)
		{
		fputs(_("You must specify a printer and a (possibly empty) list\n"
				"of PPR access control lists.\n"), errors);
		return EXIT_SYNTAX;
		}

	acls = list_to_string(&argv[1]);
	retval = conf_set_name(QUEUE_TYPE_PRINTER, printer, "ACLs", acls ? "%s" : NULL, acls);
	if(acls) gu_free(acls);

	return retval;
	} /* end of printer_acls() */

/*
** Set the printer's timeout values.
*/
int printer_userparams(const char *argv[])
	{
	const char *printer = argv[0];
	char *userparams;
	int result;

	if(!printer)
		{
		fputs(_("You must supply the name of a printer.  You may also supply a new\n"
				"quoted, space delimited list of name=value pairs.  If you don't\n"
				"supply such a list, any existing list will be deleted.\n"), errors);
		}

	userparams = list_to_string(&argv[1]);
	result = conf_set_name(QUEUE_TYPE_PRINTER, printer, "Userparams", userparams ? "%s" : NULL, userparams);
	if(userparams) gu_free(userparams);

	return result;
	} /* end of printer_userparams() */

/*
** Set a printer's per-page time limit
*/
int printer_pagetimelimit(const char *argv[])
	{
	const char *printer = argv[0];
	int limit;
	int ret;

	if(!printer || !argv[1])
		{
		fputs(_("You must supply the name of an existing printer and the maximum number of\n"
				"seconds to allow per page.\n"), errors);
		return EXIT_SYNTAX;
		}

	if((limit = atoi(argv[1])) < 0)
		{
		fputs(_("The limit must be 0 (unlimited) or a positive integer.\n"), errors);
		return EXIT_SYNTAX;
		}

	ret = conf_set_name(QUEUE_TYPE_PRINTER, printer, "PageTimeLimit", (limit > 0) ? "%d" : NULL, limit);

	return ret;
	} /* end or printer_pagetimelimit() */

/*
** Set a printer addon option.
*/
int printer_addon(const char *argv[])
	{
	const char *printer = argv[0];
	const char *name = argv[1];
	const char *value = argv[2];

	if(!printer || !name || (value && argv[3]))
		{
		fputs(_("You must supply the name of an existing printer, the name of an addon\n"
				"parameter.  A value for the paremeter is optional.  If you do not\n"
				"supply a value, the parameter will be unset.\n"), errors);
		return EXIT_SYNTAX;
		}

	if(!(name[0] >= 'a' && name[0] <= 'z'))
		{
		fputs(_("Addon parameter names must begin with a lower-case ASCII letter.\n"), errors);
		return EXIT_SYNTAX;
		}

	return conf_set_name(QUEUE_TYPE_PRINTER, printer, name, (value && value[0]) ? "%s" : NULL, value);
	} /* end of printer_addon() */

/*
** Send a query to the printer and produce a list of suitable PPD files.
*/
int printer_ppdq(const char *argv[])
	{
	const char *printer = argv[0];
	struct QUERY *q = NULL;
	int ret = EXIT_OK;

	if(!printer || argv[1])
		{
		fputs(_("You must supply only the name of an existing printer.\n"), errors);
		return EXIT_SYNTAX;
		}

	gu_Try
		{
		/* Create an object from the printer's configuration. */
		q = query_new_byprinter(printer);

		/* Now call the function in ppad_ppd.c that does the real work. */
		ret = ppd_query_core(printer, q);

		query_delete(q);
		}
	gu_Catch
		{
		fprintf(stderr, _("Query failed: %s\n"), gu_exception);
		return EXIT_INTERNAL;
		}

	return EXIT_OK;
	} /* end of printer_ppdq */

/* end of file */

