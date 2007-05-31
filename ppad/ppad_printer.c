/*
** mouse:~ppr/src/ppad/ppad_printer.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 22 May 2006.
*/

/*==============================================================
** This module is part of the printer administrator's utility.
** It contains the code to implement those sub-commands which
** manipulate printers.
<helptopic>
	<name>printer</name>
	<desc>all settings for printers</desc>
</helptopic>
<helptopic>
	<name>printer-new</name>
	<desc>default settings for new printers</desc>
</helptopic>
<helptopic>
	<name>printer-basic</name>
	<desc>basic printer commands</desc>
</helptopic>
<helptopic>
	<name>printer-advanced</name>
	<desc>expert-level printer commands</desc>
</helptopic>
<helptopic>
	<name>printer-ppd</name>
	<desc>commands related to PostScript Printer Descriptions</desc>
</helptopic>
<helptopic>
	<name>printer-description</name>
	<desc>setting printer description fields</desc>
</helptopic>
<helptopic>
	<name>printer-flags</name>
	<desc>setting options for banner and trailer pages</desc>
</helptopic>
<helptopic>
	<name>printer-limits</name>
	<desc>limits consumption (of time, pages, etc.) by print jobs</desc>
</helptopic>
==============================================================*/

#include "config.h"
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
#include "queueinfo.h"
#include "ppad.h"
#include "dispatch_table.h"

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
	struct CONF_OBJ *obj;
	char *line, *p;
	int is_a_member;

	if(debug_level > 0)
		gu_utf8_printf("Updating \"DefFiltOpts:\" for groups of which \"%s\" is a member.\n", printer);

	if(!(dir = opendir(GRCONF)))
		{
		gu_utf8_fprintf(stderr, _("%s(): %s() failed, errno=%d (%s)\n"), function, "opendir", errno, strerror(errno));
		return EXIT_INTERNAL;
		}

	while((direntp = readdir(dir)))
		{
		if(direntp->d_name[0] == '.')			/* skip . and .. */
			continue;							/* and temporary files */

		len=strlen(direntp->d_name);			/* Emacs style backup files */
		if(len > 0 && direntp->d_name[len-1]=='~')
			continue;							/* should be skipt. */

		is_a_member = FALSE;					/* start by assuming it is not */

		/* Open group config file for read. */
		if(!(obj = conf_open(QUEUE_TYPE_GROUP, direntp->d_name, 0)))
			fatal(EXIT_INTERNAL, "%s(): conf_open(QUEUE_TYPE_GROUP, \"%s\", 0) failed", function, direntp->d_name);

		while((line = conf_getline(obj)))		/* read until end of file */
			{
			if((p = lmatchp(line, "Printer:")))
				{
				if(strcmp(p, printer) == 0)
					{
					is_a_member = TRUE;
					break;
					}
				}
			}

		conf_close(obj);						/* close the group file */

		/*
		** If membership was detected, then call the routine in
		** ppad_group.c which group_deffiltopts() calls.
		*/
		if(is_a_member)
			{
			if(debug_level > 0)
				gu_utf8_printf("  Updating \"DefFiltOpts:\" for group \"%s\".\n", direntp->d_name);
			group_deffiltopts_internal(direntp->d_name);
			}
		}

	closedir(dir);

	return EXIT_OK;
	} /* end of update_groups_deffiltopts() */

/*===================================================================
** Functions for translating between the textual and integer 
** versions of PPR enums.
===================================================================*/

#define FEEDBACK_NO 0
#define FEEDBACK_YES 1
#define FEEDBACK_DEFAULT -1
#define FEEDBACK_INVALID -100

#define OUTPUTORDER_NORMAL 1
#define OUTPUTORDER_REVERSE -1
#define OUTPUTORDER_PPD 0
#define OUTPUTORDER_INVALID -100

#define JOBBREAK_INVALID -100

#define CODES_INVALID -100

/*
** Read the word provided and return BANNER_*
*/
static int flag_code(const char option[])
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
	} /* flag_code() */

/*
 * Read the word provided and return JOBBREAK_*.
 */
static int jobbreak_code(const char jobbreak[])
	{
	if(gu_strcasecmp(jobbreak, "default")==0)
		return JOBBREAK_DEFAULT;
	else if(gu_strcasecmp(jobbreak, "none") == 0)
		return JOBBREAK_NONE;
	else if(gu_strcasecmp(jobbreak, "signal") == 0)
		return JOBBREAK_SIGNAL;
	else if(gu_strcasecmp(jobbreak, "control-d")==0)
		return JOBBREAK_CONTROL_D;
	else if(gu_strcasecmp(jobbreak, "pjl")==0)
		return JOBBREAK_PJL;
	else if(gu_strcasecmp(jobbreak, "signal/pjl")==0)
		return JOBBREAK_SIGNAL_PJL;
	else if(gu_strcasecmp(jobbreak, "save/restore")==0)
		return JOBBREAK_SAVE_RESTORE;
	else if(gu_strcasecmp(jobbreak, "newinterface")==0)
		return JOBBREAK_NEWINTERFACE;
	else
		return JOBBREAK_INVALID;
	} /* jobbreak_code() */

/*
 * Read the word provided and return FEEDBACK_*.
 */
static int feedback_code(const char feedback[])
	{
	gu_boolean temp;
	if(gu_strcasecmp(feedback, "default") == 0)
		return FEEDBACK_DEFAULT;
	if(gu_torf_setBOOL(&temp, feedback) == -1)
	   return FEEDBACK_INVALID;
	if(temp)
		return FEEDBACK_YES;
	return FEEDBACK_NO;
	}

/*
 * Read the word provided and return FEEDBACK_*.
 */
static int codes_code(const char codes[])
	{
	if(gu_strcasecmp(codes, "default") == 0)
		return CODES_DEFAULT;
	else if(gu_strcasecmp(codes, "UNKNOWN") == 0)
		return CODES_UNKNOWN;
	else if(gu_strcasecmp(codes, "Clean7Bit") == 0)
		return CODES_Clean7Bit;
	else if(gu_strcasecmp(codes, "Clean8Bit") == 0)
		return CODES_Clean8Bit;
	else if(gu_strcasecmp(codes, "Binary")==0)
		return CODES_Binary;
	else if(gu_strcasecmp(codes, "TBCP")==0)
		return CODES_TBCP;
	return CODES_INVALID;
	}

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
<command acl="ppad" helptopics="printer,printer-new">
	<name><word>new</word><word>alerts</word></name>
	<desc>set default alerts settings for new printers</desc>
	<args>
		<arg><name>frequency</name><desc>error count at which to send alerts</desc></arg>
		<arg><name>method</name><desc>method by which to send alerts</desc></arg>
		<arg><name>address</name><desc>address to which to send alerts</desc></arg>
	</args>
</command>
*/
int command_new_alerts(const char *argv[])
	{
	int frequency;
	const char *method;
	const char *address;
	FILE *newprn;

	if(strspn(argv[0],"-0123456789") != strlen(argv[0]))
		{
		gu_utf8_fprintf(stderr, _("Alerts interval must be an integer.\n"));
		return EXIT_SYNTAX;
		}

	frequency = atoi(argv[0]);
	method = argv[1];
	address = argv[2];

	if(!(newprn = fopen(NEWPRN_CONFIG,"w")))
		{
		gu_utf8_fprintf(stderr, _("Unable to create \"%s\", errno=%d (%s).\n"), NEWPRN_CONFIG, errno, gu_strerror(errno));
		return EXIT_INTERNAL;
		}

	gu_utf8_fprintf(newprn, "Alert: %d %s %s\n", frequency, method, address);

	fclose(newprn);
	return EXIT_OK;
	} /* command_new_alerts() */

/*
<command acl="ppad" helptopics="printer,printer-basic">
	<name><word>show</word></name>
	<desc>show configuration of <arg>printer</arg></desc>
	<args>
		<arg><name>printer</name><desc>printer to show</desc></arg>
	</args>
</command>
**
** We might someday be able to replace much of this function with a
** queueinfo object (../libppr/queueinfo.c) but as of 30 January 2004 it
** isn't mature enough.
*/
int command_show(const char *argv[])
	{
	const char function[] = "printer_show";
	void *pool;
	const char *printer = argv[0];		/* Argument is printer to show. */

	struct CONF_OBJ *obj;
	char *line, *p;
	int count;							/* general use */
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

	if(!(obj = conf_open(QUEUE_TYPE_PRINTER, printer, CONF_ENOENT_PRINT)))
		return EXIT_BADDEST;

	pool = gu_pool_push(gu_pool_new());

	/*
	** This loop examines each line in the printer's configuration
	** file and gathers information.
	*/
	while((line = conf_getline(obj)))
		{
		if(gu_sscanf(line, "Interface: %S", &p) == 1)
			{
			interface = p;
			/* Invalidate lines which preceed "Interface:". */
			jobbreak = JOBBREAK_DEFAULT;
			feedback = FEEDBACK_DEFAULT;
			codes = CODES_DEFAULT;
			}
		else if(gu_sscanf(line, "Address: %A", &p) == 1)
			{
			address = p;
			}
		else if(gu_sscanf(line, "Options: %T", &p) == 1)
			{
			options = p;
			}
		else if((p = lmatchp(line, "Feedback:")))
			{
			gu_boolean temp;
			if(gu_torf_setBOOL(&temp, p) == -1)
				feedback = FEEDBACK_INVALID;
			if(temp)
				feedback = FEEDBACK_YES;
			else
				feedback = FEEDBACK_NO;
			}
		else if(gu_sscanf(line, "JobBreak: %d", &jobbreak) == 1)
			{
			/* nothing to do */
			}
		else if(gu_sscanf(line, "Codes: %d", &codes) == 1)
			{
			/* nothing to do */
			}
		else if((p = lmatchp(line, "RIP:")))
			{
			rip_name = NULL;
			rip_output_language = NULL;
			rip_options = NULL;
			gu_sscanf(p, "%S %S %T", &rip_name, &rip_output_language, &rip_options);
			}
		else if(gu_sscanf(line, "PPDFile: %A", &p) == 1)
			{
			PPDFile = p;
			}
		else if(gu_sscanf(line, "Bin: %A", &p) == 1)
			{
			if(bincount<MAX_BINS)
				bins[bincount++] = p;
			}
		else if(gu_sscanf(line, "Comment: %T", &p) == 1)
			{
			comment = p;
			}
		else if(gu_sscanf(line, "Location: %T", &p) == 1)
			{
			location = p;
			}
		else if(gu_sscanf(line, "Department: %T", &p) == 1)
			{
			department = p;
			}
		else if(gu_sscanf(line, "Contact: %T", &p) == 1)
			{
			contact = p;
			}
		else if(lmatch(line, "FlagPages:"))
			{
			gu_sscanf(line, "FlagPages: %d %d", &banner, &trailer);
			}
		else if(lmatch(line, "Alert:"))
			{
			alerts_method = NULL;
			alerts_address = NULL;
			gu_sscanf(line, "Alert: %d %S %S", &alerts_frequency, &alerts_method, &alerts_address);
			}
		else if(gu_sscanf(line, "OutputOrder: %@s", sizeof(scratch), scratch) == 1)
			{
			if(strcmp(scratch, "Normal") == 0)
				outputorder = 1;
			else if(strcmp(scratch, "Reverse") == 0)
				outputorder = -1;
			else
				outputorder = -100;
			}
		else if((count = gu_sscanf(line, "Charge: %f %f", &tf1, &tf2)) > 0)
			{
			/* Convert dollars to cents, pounds to pence, etc.: */
			charge_duplex_sheet = (int)(tf1 * 100.0 + 0.5);

			/* If there is a second parameter, convert it too: */
			if(count == 2)
				charge_simplex_sheet = (int)(tf2 * 100.0 + 0.5);
			else
				charge_duplex_sheet = charge_simplex_sheet;
			}
		else if(gu_sscanf(line, "Switchset: %T", &p) == 1)
			{
			switchset = p;
			}
		else if(gu_sscanf(line, "DefFiltOpts: %T", &p) == 1)
			{
			deffiltopts = p;
			}
		else if(gu_sscanf(line, "PPDOpt: %T", &p) == 1)
			{
			if(ppdopts_count >= (sizeof(ppdopts) / sizeof(ppdopts[0])))
				{
				gu_utf8_fprintf(stderr, "%s(): PPDOpts overflow\n", function);
				}
			else
				{
				ppdopts[ppdopts_count++] = p;
				}
			}
		else if(gu_sscanf(line, "PassThru: %T", &p) == 1)
			{
			passthru = p;
			}
		else if(gu_sscanf(line, "LimitPages: %d %d", &limitpages_lower, &limitpages_upper) == 2)
			{
			/* nothing more to do */
			}
		else if(gu_sscanf(line, "LimitKilobytes: %d %d", &limitkilobytes_lower, &limitkilobytes_upper) == 2)
			{
			/* nothing more to do */
			}
		else if((p = lmatchp(line, "GrayOK:")))
			{
			if(gu_torf_setBOOL(&grayok,p) == -1)
				gu_utf8_fprintf(stderr, _("WARNING: invalid \"%s\" setting: %s\n"), "GrayOK", p);
			}
		else if(gu_sscanf(line, "ACLs: %T", &p) == 1)
			{
			acls = p;
			}
		else if(gu_sscanf(line, "PageTimeLimit: %d", &pagetimelimit) == 1)
			{
			/* nothing more to do */
			}
		else if(gu_sscanf(line, "Userparams: %T", &p) == 1)
			{
			userparams = p;
			}
		else if(line[0] >= 'a' && line[0] <= 'z')		/* if in addon name space */
			{
			if(addon_count >= MAX_ADDONS)
				{
				gu_utf8_fprintf(stderr, "%s(): addon[] overflow\n", function);
				}
			else
				{
				addon[addon_count++] = gu_strdup(line);
				}
			}

		} /* end of loop for each configuration file line */

	conf_close(obj);

	/*
	** Determine the jobbreak, feedback, and codes
	** defaults for this interface.
	*/
	{
	char *cups_raster_filter = NULL;
	char *cups_postscript_filter = NULL;
	gu_boolean ColorDevice = FALSE;
	struct PPD_PROTOCOLS prot;
	prot.TBCP = FALSE;
	prot.PJL = FALSE;

	/* If there is a PPD file defined, read its "*Protocols:"
	   line to help us arrive at the defaults. */
	if(PPDFile)
		{
		void *ppdobj = NULL;
		
		gu_Try {
			ppdobj = ppdobj_new(PPDFile);

			while((line = ppdobj_readline(ppdobj)))
				{
				if((p = lmatchp(line, "*Protocols:")))
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
	
				if((p = lmatchp(line, "*pprRIP:")) && !rip_ppd_name)
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
						gu_utf8_fprintf(stderr, _("WARNING: can't parse RIP information in PPD file\n"));
						}
					continue;
					} /* "*pprRIP:" */
	
				if((p = lmatchp(line, "*cupsFilter:")))
					{
					char *p1, *p2, *p3;
					if(*p++ == '"' && (p1 = strchr(p, '"')))
						{
						*p1 = '\0';
						if((p1 = gu_strsep(&p, " \t"))										/* first exists */
								&& (p2 = gu_strsep(&p, " \t"))							/* and second parameter exists */
								&& strspn(p2, "0123456789") == strlen(p2)				/* and it is numberic */
								&& (p3 = gu_strsep(&p, "\t"))							/* and third parameter exists */
							)
							{
							if(!cups_raster_filter && strcmp(p1, "application/vnd.cups-raster") == 0)
								cups_raster_filter = gu_strdup(p3);
							else if(!cups_postscript_filter && strcmp(p1, "application/vnd.cups-postscript") == 0)
								cups_postscript_filter = gu_strdup(p3);
							}
						}
					continue;
					} /* "*cupsFilter:" */
	
				if((p = lmatchp(line, "*ColorDevice:")))
					{
					gu_torf_setBOOL(&ColorDevice,p);
					continue;
					}
	
				} /* while() */
			}
		gu_Final {
			if(ppdobj)
				ppdobj_free(ppdobj);
			}
		gu_Catch {
			gu_utf8_fprintf(stderr, "%s: %s\n", myname, gu_exception);
			}

		} /* if(PPDFile) */

	/* Determine all of the defaults. */
	feedback_default = interface_default_feedback(interface, &prot);
	jobbreak_default = interface_default_jobbreak(interface, &prot);
	codes_default = interface_default_codes(interface, &prot);

	/* If we didn't find a "*pprRIP:" line, use "*cupsFilter:". */
	if(!rip_ppd_name)
		{
		if(cups_raster_filter)
			{
			rip_ppd_name = gu_strdup("ppr-gs");
			rip_ppd_output_language = gu_strdup("PCL");	/* !!! a wild guess !!! */
			gu_asprintf(&rip_ppd_options, "cups=%s", cups_raster_filter);
			}
		else if(cups_postscript_filter && strcmp(cups_postscript_filter, "pstopxl") == 0)
			{
			rip_ppd_name = gu_strdup("ppr-gs");
			rip_ppd_output_language = gu_strdup("PCLXL");
			if(ColorDevice)
				gu_asprintf(&rip_ppd_options, "-sDEVICE=pxlcolor");
			else
				gu_asprintf(&rip_ppd_options, "-sDEVICE=pxlmono");
			}
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
	}

	/*
	** Now that we have retrieved the information, we will print it.
	*/
	if( ! machine_readable )
		{
		gu_utf8_printf(_("Printer name: %s\n"), printer);
		gu_utf8_puts("  ");
			gu_utf8_printf(_("Comment: %s\n"), comment ? comment : "");
		if(location)
			{
			gu_utf8_puts("  ");
			gu_utf8_printf(_("Location: %s\n"), location);
			}
		if(department)
			{
			gu_utf8_puts("  ");
			gu_utf8_printf(_("Department: %s\n"), department);
			}
		if(contact)
			{
			gu_utf8_puts("  ");
			gu_utf8_printf(_("Contact: %s\n"), contact);
			}

		/*-------------------------------------
		** Communication related things
		-------------------------------------*/
		gu_utf8_printf(_("Interface: %s\n"), interface ? interface : _("<undefined>"));

		gu_utf8_puts("  ");
			gu_utf8_printf(_("Address: \"%s\"\n"), address ? address : _("<undefined>"));

		gu_utf8_puts("  ");
			gu_utf8_printf(_("Options: %s\n"), options ? options : "");

		gu_utf8_puts("  ");
			gu_utf8_puts(_("JobBreak: "));
			if(jobbreak == JOBBREAK_DEFAULT)
				gu_utf8_printf(_("%s (by default)"), jobbreak_description(jobbreak_default));
			else
				gu_utf8_puts(jobbreak_description(jobbreak));
		gu_putwc('\n');

		gu_utf8_puts("  ");
			gu_utf8_puts(_("Feedback: "));
			if(feedback == FEEDBACK_DEFAULT)
				gu_utf8_printf(_("%s (by default)"), feedback_description(feedback_default));
			else
				gu_utf8_puts(feedback_description(feedback));
			gu_putwc('\n');

		gu_utf8_puts("  ");
			gu_utf8_puts(_("Codes: "));
			if(codes == CODES_DEFAULT)
				gu_utf8_printf(_("%s (by default)"), codes_description(codes_default));
			else
				gu_utf8_puts(codes_description(codes));
			gu_putwc('\n');

		/*-------------------------------------
		** PPD File Related Things
		-------------------------------------*/
		gu_utf8_printf(_("PPDFile: %s\n"), PPDFile ? PPDFile : _("<undefined>"));

		gu_utf8_puts("  ");
		{
		const char *s = _("Default Filter Options: ");
		gu_utf8_puts(s);
		if(deffiltopts)
			print_wrapped(deffiltopts, strlen(s));
		gu_putwc('\n');
		}

		/* RIP */
		if(rip_name)
			{
			gu_utf8_puts("  ");
			gu_utf8_printf(rip_name==rip_ppd_name ? _("RIP: %s %s \"%s\" (from PPD)\n") : _("RIP: %s %s \"%s\"\n"),
				rip_name,
				rip_output_language ? rip_output_language : "?",
				rip_options ? rip_options : "");
			}

		/* Optional printer equipment. */
		{
		int x;
		for(x=0; x < ppdopts_count; x++)
			{
			gu_utf8_puts("  ");
			gu_utf8_printf(_("PPDOpts: %s\n"), ppdopts[x]);
			}
		}

		gu_utf8_puts("  ");
		gu_utf8_puts(_("Bins: "));		/* print a list of all */
		{
		int x;
		for(x=0; x < bincount; x++)		/* the bins we found */
			{
			if(x==0)
				gu_utf8_printf("%s", bins[x]);
			else
				gu_utf8_printf(", %s", bins[x]);
			}
		gu_putwc('\n');
		}

		gu_utf8_puts("  ");
		gu_utf8_printf(_("OutputOrder: %s\n"), outputorder_description(outputorder));

		/*---------------------------------
		** Alerts
		---------------------------------*/
		gu_utf8_printf(_("Alert frequency: %d "), alerts_frequency);
		switch(alerts_frequency)
			{
			case 0:
				gu_utf8_puts(_("(never send alerts)"));
				break;
			case 1:
				gu_utf8_puts(_("(send alert on every error)"));
				break;
			case -1:
				gu_utf8_puts(_("(send alert on first error, send notice on recovery)"));
				break;
			default:
				if(alerts_frequency > 0)
					gu_utf8_printf(_("(send alert every %d errors)"), alerts_frequency);
				else
					gu_utf8_printf(_("(send alert after %d errors, send notice on recovery)"), (alerts_frequency * -1));
				break;
			}
		gu_putwc('\n');
		gu_utf8_puts("  "); gu_utf8_printf(_("Alert method: %s\n"), alerts_method ? alerts_method : _("none"));
		gu_utf8_puts("  "); gu_utf8_printf(_("Alert address: %s\n"), alerts_address ? alerts_address : _("none"));

		/*---------------------------------
		** Accounting things
		---------------------------------*/
		gu_utf8_printf(_("Flags: %s %s (banners %s, trailers %s)\n"),
			flag_description(banner),
			flag_description(trailer),
			long_flag_description(banner),
			long_flag_description(trailer));
		gu_utf8_printf(_("Charge: "));
		if(charge_duplex_sheet == -1)
			{
			gu_utf8_printf(_("no charge\n"));
			}
		else
			{		/* money() uses static array for result */
			gu_utf8_printf(_("%s per duplex sheet, "), money(charge_duplex_sheet));
			gu_utf8_printf(_("%s per simplex sheet\n"), money(charge_simplex_sheet));
			}

		/*--------------------------------------------
		** discretionary settings
		---------------------------------------------*/

		/* Uncompress and print the switchset. */
		gu_utf8_puts(_("Switchset: "));
		if(switchset)
			print_switchset(switchset);
		gu_putwc('\n');

		/* Rare option is not show unless set. */
		if(passthru)
			gu_utf8_printf(_("PassThru types: %s\n"), passthru);

		/* Rare option is not show unless set. */
		if(limitpages_lower > 0 || limitpages_upper > 0)
			gu_utf8_printf(_("LimitPages: %d %d\n"), limitpages_lower, limitpages_upper);

		/* Rare option is not show unless set. */
		if(limitkilobytes_lower > 0 || limitkilobytes_upper > 0)
			gu_utf8_printf(_("LimitKilobytes: %d %d\n"), limitkilobytes_lower, limitkilobytes_upper);

		/* Another rare option. */
		if(!grayok)
			gu_utf8_printf(_("GrayOK: %s\n"), grayok ? _("yes") : _("no"));

		/* This rare option is not show until set. */
		if(acls)
			gu_utf8_printf(_("ACLs: %s\n"), acls);

		/* Another rare option */
		if(pagetimelimit > 0)
			gu_utf8_printf(_("PageTimeLimit: %d\n"), pagetimelimit);

		/* Another rare option */
		if(userparams)
			gu_utf8_printf(_("Userparams: %s\n"), userparams);

		/* Print the assembed addon settings. */
		if(addon_count > 0)
			{
			int x;
			gu_utf8_puts(_("Addon:"));
			for(x = 0; x < addon_count; x++)
				{
				gu_utf8_printf("\t%s\n", addon[x]);
				}
			}
		}

	/* Machine readable output. */
	else
		{
		gu_utf8_printf("name\t%s\n", printer);
		gu_utf8_printf("comment\t%s\n", comment ? comment : "");
		gu_utf8_printf("location\t%s\n", location ? location : "");
		gu_utf8_printf("department\t%s\n", department ? department : "");
		gu_utf8_printf("contact\t%s\n", contact ? contact : "");

		/* Communications related things */
		gu_utf8_printf("interface\t%s\n", interface ? interface : "");
		gu_utf8_printf("address\t%s\n", address ? address : "");
		gu_utf8_printf("options\t%s\n", options ? options : "");
		gu_utf8_printf("jobbreak\t%s %s\n",jobbreak_description(jobbreak), jobbreak_description(jobbreak_default));
		gu_utf8_printf("feedback\t%s %s\n", feedback_description(feedback), feedback_description(feedback_default));
		gu_utf8_printf("codes\t%s %s\n", codes_description(codes), codes_description(codes_default));

		/* RIP */
		gu_utf8_printf("rip\t%s\t%s\t%s\n",
				rip_name ? rip_name : "",
				rip_output_language ? rip_output_language : "",
				rip_options ? rip_options : "");
		gu_utf8_printf("rip_ppd\t%s\t%s\t%s\n",
				rip_ppd_name ? rip_ppd_name : "",
				rip_ppd_output_language ? rip_ppd_output_language : "",
				rip_ppd_options ? rip_ppd_options : "");
		gu_utf8_printf("rip_which\t%s\n",
				rip_name==rip_ppd_name ? "PPD" : "CONFIG");

		/* Alerts */
		gu_utf8_printf("alerts\t%d %s %s\n",
				alerts_frequency,
				alerts_method ? alerts_method : "",
				alerts_address ? alerts_address : "");

		/* Accounting related things */
		gu_utf8_printf("flags\t%s %s\n", flag_description(banner), flag_description(trailer));
		gu_utf8_puts("charge\t");
		if(charge_duplex_sheet != -1)
			{			/* money() returns pointer to static array! */
			gu_utf8_puts(money(charge_duplex_sheet));
			gu_putwc(' ');
			gu_utf8_puts(money(charge_simplex_sheet));
			}
		else
			{
			gu_putwc(' ');
			}
		gu_putwc('\n');

		/* PPD file related things */
		gu_utf8_printf("ppd\t%s\n", PPDFile ? PPDFile : "");

		gu_utf8_puts("ppdopts\t");
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
				gu_putwc(' ');
			gu_utf8_printf("%.*s %.*s", len1, p1, len2, p2);
			}
		}
		gu_putwc('\n');

		gu_utf8_puts("bins\t");
		{
		int x;
		for(x=0; x < bincount; x++)
			{
			if(x==0)
				gu_utf8_puts(bins[x]);
			else
				gu_utf8_puts(bins[x]);
			}
		}
		gu_putwc('\n');

		gu_utf8_printf("outputorder\t%s\n", outputorder_description(outputorder));

		gu_utf8_printf("deffiltopts\t%s\n", deffiltopts ? deffiltopts : "");

		/* Other things */
		gu_utf8_puts("switchset\t"); if(switchset) print_switchset(switchset); gu_putwc('\n');
		gu_utf8_printf("passthru\t%s\n", passthru ? passthru : "");
		gu_utf8_printf("limitpages\t%d %d\n", limitpages_lower, limitpages_upper);
		gu_utf8_printf("limitkilobytes\t%d %d\n", limitkilobytes_lower, limitkilobytes_upper);
		gu_utf8_printf("grayok\t%s\n", grayok ? "yes" : "no");
		gu_utf8_printf("acls\t%s\n", acls ? acls : "");
		gu_utf8_printf("pagetimelimit\t%d\n", pagetimelimit);
		gu_utf8_printf("userparams\t%s\n", userparams ? userparams : "");

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
					gu_utf8_printf("addon %s\t%s\n", addon[x], p);
					}
				else
					{
					gu_utf8_printf("addon\t%s\n", addon[x]);
					}
				}
			}
		} /* end of machine_readable */

	gu_pool_free(gu_pool_pop(pool));

	return EXIT_OK;
	} /* command_show() */

/*
<command acl="ppad" helptopics="printer,printer-basic">
	<name><word>copy</word></name>
	<desc>copy printer <arg>existing</arg> creating printer <arg>new</arg></desc>
	<args>
		<arg><name>existing</name><desc>name of existing printer</desc></arg>
		<arg><name>new</name><desc>name of new printer</desc></arg>
	</args>
</command>
*/
int command_copy(const char *argv[])
	{
	return conf_copy(QUEUE_TYPE_PRINTER, argv[0], argv[1]);
	}

/*
<command acl="ppad" helptopics="printer,printer-basic,printer-description">
	<name><word>comment</word></name>
	<desc>modify a printer's comment field</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg flags="optional"><name>comment</name><desc>comment to attach (ommit to delete)</desc></arg>
	</args>
</command>
*/
int command_comment(const char *argv[])
	{
	return conf_set_name(QUEUE_TYPE_PRINTER, argv[0], 0, "Comment", argv[1] ? "%s" : NULL, argv[1]);
	} /* command_comment() */

/*
<command acl="ppad" helptopics="printer,printer-description">
	<name><word>location</word></name>
	<desc>modify a printer's location field</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg flags="optional"><name>location</name><desc>new location description (ommit to delete)</desc></arg>
	</args>
</command>
*/
int command_location(const char *argv[])
	{
	return conf_set_name(QUEUE_TYPE_PRINTER, argv[0], 0, "Location", argv[1] ? "%s" : NULL, argv[1]);
	} /* command_location() */

/*
<command acl="ppad" helptopics="printer,printer-description">
	<name><word>department</word></name>
	<desc>modify a printer's department field</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg flags="optional"><name>department</name><desc>new department description (ommit to delete)</desc></arg>
	</args>
</command>
*/
int command_department(const char *argv[])
	{
	return conf_set_name(QUEUE_TYPE_PRINTER, argv[0], 0, "Department", argv[1] ? "%s" : NULL, argv[1]);
	} /* command_department() */

/*
<command acl="ppad" helptopics="printer,printer-description">
	<name><word>contact</word></name>
	<desc>modify a printer's contact field</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg flags="optional"><name>contact</name><desc>new contact description (ommit to delete)</desc></arg>
	</args>
</command>
*/
int command_contact(const char *argv[])
	{
	return conf_set_name(QUEUE_TYPE_PRINTER, argv[0], 0, "Contact", argv[1] ? "%s" : NULL, argv[1]);
	} /* command_contact() */

/*
<command acl="ppad" helptopics="printer,printer-basic">
	<name><word>interface</word></name>
	<desc>set a printers interface program and address</desc>
	<args>
		<arg><name>printer</name><desc>printer to modify or create</desc></arg>
		<arg><name>interface</name><desc>interface program to use</desc></arg>
		<arg><name>address</name><desc>address to pass to interface program</desc></arg>
	</args>
</command>
**
** We will delete "JobBreak:" and "Feedback:" lines if the
** interface is changed or if there was previously no "Interface:"
** line.  We will also delete all "JobBreak:" and "Feedback:" lines
** which appear before the "Interface:" line.
**
** We will only ask the spooler to re-read the configuration file
** if this command creates the printer.
*/
int command_interface(const char *argv[])
	{
	const char *printer = argv[0];
	const char *interface = argv[1];
	const char *address = argv[2];
	struct CONF_OBJ *obj;

	if(strpbrk(printer, DEST_DISALLOWED))
		{
		gu_utf8_fputs(_("The printer name contains a disallowed character.\n"), stderr);
		return EXIT_SYNTAX;
		}

	if(strchr(DEST_DISALLOWED_LEADING, (int)printer[0]))
		{
		gu_utf8_fputs(_("The printer name begins with a disallowed character.\n"), stderr);
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
		gu_utf8_fprintf(stderr, _("The interface \"%s\" does not exist.\n"), interface);
		return EXIT_NOTFOUND;
		}
	if(! (statbuf.st_mode & S_IXUSR) )
		{
		gu_utf8_fprintf(stderr, _("The interface \"%s\" not executable.\n"), interface);
		return EXIT_NOTFOUND;
		}
	}

	/* Is there an existing printer? */
	if((obj = conf_open(QUEUE_TYPE_PRINTER, printer, CONF_MODIFY | CONF_RELOAD)))
		{
		char *line, *p;
		gu_boolean different_interface = TRUE;

		/* Copy up to the 1st "Interface:". */
		while((line  = conf_getline(obj)))
			{
			if((p = lmatchp(line, "Interface:")))
				{
				if(strcmp(interface, p) == 0)
					different_interface = FALSE;
				break;
				}
			else				/* while deleting spurious "JobBreak:" and */
				{				/* "Feedback:" lines as we go */
				if(!lmatch(line, "Address:")
						&& !lmatch(line, "Options:")
						&& !lmatch(line, "JobBreak:")
						&& !lmatch(line, "Feedback:")
						&& !lmatch(line, "Codes:") )
					{
					conf_printf(obj, "%s\n", line);
					}
				}
			}

		/* Write the new lines. */
		conf_printf(obj, "Interface: %s\n", interface);
		conf_printf(obj, "Address: \"%s\"\n", address);

		/* And copy the rest of the file while removing certain old lines. */
		while((line = conf_getline(obj)))
			{
			if(!lmatch(line, "Interface:")
						&& !lmatch(line, "Address:")
						&& (!lmatch(line, "Options:") || !different_interface)
						&& (!lmatch(line, "JobBreak:") || !different_interface)
						&& (!lmatch(line, "Feedback:") || !different_interface)
						&& (!lmatch(line, "Codes:") || !different_interface) )
				conf_printf(obj, "%s\n", line);
			}
		conf_close(obj);
		}

	/* Nope, create new printer. */
	else
		{
		FILE *defaults;

		if(!(obj = conf_open(QUEUE_TYPE_PRINTER, printer, CONF_CREATE | CONF_RELOAD)))
			return EXIT_INTERNAL;

		conf_printf(obj, "Interface: %s\n", interface);
		conf_printf(obj, "Address: \"%s\"\n", address);

		/* specify a good generic PPD file */
		conf_printf(obj, "PPDFile: Apple LaserWriter Plus\n");
		conf_printf(obj, "DefFiltOpts: level=1 colour=False resolution=300 freevm=172872 mfmode=CanonCX\n");

		/* if there is a defaults file, include it */
		if((defaults = fopen(NEWPRN_CONFIG,"r")))
			{
			char *line = NULL;
			int line_space = 80;
			while((line = gu_getline(line, &line_space, defaults)))
				conf_printf(obj, "%s\n", line);
			fclose(defaults);
			}

		conf_close(obj);
		}								/* (really for the 1st time) */

	return EXIT_OK;
	} /* command_interface() */

/*
<command acl="ppad" helptopics="printer,printer-basic">
	<name><word>options</word></name>
	<desc>set a printer's interface options string</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg flags="optional,repeat"><name>options</name><desc>interface options (ommit to clear)</desc></arg>
	</args>
</command>
*/
int command_options(const char *argv[])
	{
	const char *printer = argv[0];
	struct CONF_OBJ *obj;
	char *line;

	/* make sure the printer exists */
	if(!(obj = conf_open(QUEUE_TYPE_PRINTER, printer, CONF_MODIFY | CONF_ENOENT_PRINT)))
		return EXIT_BADDEST;

	/* Modify the printer's configuration file. */
	while((line = conf_getline(obj)))
		{
		if(lmatch(line, "Options:"))					/* Delete "Options: " lines before */
			continue;									/* the "Interface: " line. */
		if(lmatch(line, "Interface:"))					/* Break loop after copying */
			{											/* the "Interface: " line */
			conf_printf(obj, "%s\n", line);
			break;
			}
		conf_printf(obj, "%s\n", line);
		}

	/* If we have a meaningful new line to write, write it now. */
	{
	char *p = list_to_string(&argv[1]);
	if(p)
		{
		if(strcmp(p, "none") == 0)
			gu_utf8_fprintf(stderr, X_("Warning: setting options to \"none\" is deprecated\n"));
		else
			conf_printf(obj, "Options: %s\n", p);
		gu_free(p);
		}
	}

	while((line = conf_getline(obj)))						/* copy rest of file, */
		{
		if(lmatch(line, "Options:"))
			continue;
		else
			conf_printf(obj, "%s\n", line);
		}

	conf_close(obj);

	return EXIT_OK;
	} /* command_options() */

/*
 * Set a numberic Interface parameter.  If the value is less than zero, then
 * the setting is simply removed.
 */
static int printer_interface_param(const char printer[], const char param[], int value)
	{
	struct CONF_OBJ *obj;
	char *line;
	int param_len = strlen(param);

	/* Make sure the printer exists. */
	if(!(obj = conf_open(QUEUE_TYPE_PRINTER, printer, CONF_MODIFY | CONF_ENOENT_PRINT)))
		return EXIT_BADDEST;

	/* Modify the printer's configuration file. */
	while((line = conf_getline(obj)))
		{
		if(strncmp(line, param, param_len) == 0 && line[param_len] == ':')
			continue;
		conf_printf(obj, "%s\n", line);
		if(lmatch(line, "Interface:"))			/* stop after "Interface:" line */
			break;
		}

	/* If the parameter value is not JOBBREAK_DEFAULT, CODES_DEFAULT, or FEEDBACK_DEFAULT, */
	if(value >= 0)
		{
		if(strcmp(param, "Feedback") == 0)
			conf_printf(obj, "%s: %s\n", param, value ? "True" : "False");
		else
			conf_printf(obj, "%s: %d\n", param, value);
		}

	while((line = conf_getline(obj)))			/* copy rest of file, */
		{
		if(strncmp(line, param, param_len) == 0 && line[param_len] == ':')
			continue;
		conf_printf(obj, "%s\n", line);
		}

	conf_close(obj);

	return EXIT_OK;
	} /* printer_interface_param() */

/*
<command acl="ppad" helptopics="printer,printer-advanced">
	<name><word>jobbreak</word></name>
	<desc>set a printer's jobbreak method</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg><name>jobbreak</name><desc>signal, control-d, pjl, signal/pjl, newinterface, or default</desc></arg>
	</args>
</command>
*/
int command_jobbreak(const char *argv[])
	{
	const char *printer = argv[0];
	const char *jobbreak = argv[1];
	int new_value;

	if((new_value = jobbreak_code(jobbreak)) == JOBBREAK_INVALID)
		{
		/* We omit "none" and "save/restore" from this list because
		 * we want to remove them. */
		gu_utf8_fputs(_("Valid jobbreak modes are \"signal\", \"control-d\", \"pjl\", \"signal/pjl\",\n"
			"\"newinterface\", and \"default\".\n"), stderr);
		return EXIT_SYNTAX;
		}

	return printer_interface_param(printer, "JobBreak", new_value);
	} /* command_jobbreak() */

/*
<command acl="ppad" helptopics="printer,printer-advanced">
	<name><word>feedback</word></name>
	<desc>set a printer's feedback flag</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg><name>feedback</name><desc>true, false, or default</desc></arg>
	</args>
</command>
*/
int command_feedback(const char *argv[])
	{
	const char *printer = argv[0];
	const char *feedback = argv[1];
	int new_value;

	if((new_value = feedback_code(feedback)) == FEEDBACK_INVALID)
		{
		gu_utf8_fputs(_("Valid feedback settings are \"true\", \"false\", or \"default\".\n"), stderr);
		return EXIT_SYNTAX;
		}

	return printer_interface_param(printer, "Feedback", new_value);
	} /* command_feedback() */

/*
<command acl="ppad" helptopics="printer,printer-advanced">
	<name><word>codes</word></name>
	<desc>set a printer's passable codes info</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg><name>codes</name><desc>Clean7Bit, Clean8Bit, Binary, TBCP, UNKNOWN, or default</desc></arg>
	</args>
</command>
*/
int command_codes(const char *argv[])
	{
	const char *printer = argv[0];
	const char *codes = argv[1];
	int new_value;

	if((new_value = codes_code(codes)) == CODES_INVALID)
		{
		gu_utf8_fputs(_("Valid codes settings are \"Clean7Bit\", \"Clean8Bit\", \"Binary\", \"TBCP\",\n"
			"\"UNKNOWN\", and \"default\".\n"), stderr);
		return EXIT_SYNTAX;
		}

	return printer_interface_param(printer, "Codes", new_value);
	} /* command_codes() */

/*
<command acl="ppad" helptopics="printer,printer-advanced">
	<name><word>rip</word></name>
	<desc>change external RIP settings for <arg>printer</arg></desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg flags="optional"><name>rip</name><desc>name of external RIP</desc></arg>
		<arg flags="optional"><name>output</name><desc>type of output file produced by <arg>rip</arg> (such as \"pcl\" or \"other\")</desc></arg>
		<arg flags="optional"><name>options</name><desc>space-separated list of options to pass to RIP</desc></arg>
	</args>
</command>
*/
int command_rip(const char *argv[])
	{
	const char *printer = argv[0];
	const char *rip = argv[1];
	const char *output_language = argv[2];
	const char *options = argv[3];

	if(rip && !output_language)
		{
		gu_utf8_fputs(_("If you set a RIP, you must specify the output language.\n"), stderr);
		return EXIT_SYNTAX;
		}

	if(rip && strlen(rip) == 0)
		{
		gu_utf8_fputs(_("The RIP name may not be an empty string.\n"), stderr);
		return EXIT_SYNTAX;
		}

	if(output_language && strlen(output_language) == 0)
		{
		gu_utf8_fputs(_("The RIP output language may not be an empty string.\n"), stderr);
		return EXIT_SYNTAX;
		}

	return conf_set_name(QUEUE_TYPE_PRINTER, printer, 0, "RIP", rip ? "%s %s %s" : NULL, rip, output_language, options ? options : "");
	} /* command_rip() */

/*
<command acl="ppad" helptopics="printer,printer-basic,printer-ppd">
	<name><word>ppd</word></name>
	<desc>set a printer's description (PPD) file</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg><name>modelname</name><desc>name of printer description</desc></arg>
	</args>
</command>
**
** We do this by modifying the configuration file's "PPDFile:" line.  The 
** spooler doesn't keep track of the PPD file, but it needs to know if
** we change it because a printer that wasn't capable of printing a given
** job with one PPD file may be capable according to the new PPD file.
** Thus we tell the spooler that the config file has changed so that it
** will clear the "never" bits.
*/
int command_ppd(const char *argv[])
	{
	const char *printer = argv[0];
	const char *ppdname = argv[1];
	struct CONF_OBJ *obj;
	void *qobj = NULL;
	const char *line, *p;

	/*
	** Make sure the printer exists,
	** opening its configuration file if it does.
	*/
	if(!(obj = conf_open(QUEUE_TYPE_PRINTER, printer, CONF_MODIFY | CONF_ENOENT_PRINT | CONF_RELOAD)))
		return EXIT_BADDEST;

	gu_Try
		{
		/* Get ready to collect information for a "DefFiltOpts:" line. */
		qobj = queueinfo_new(QUEUEINFO_PRINTER, printer);
		queueinfo_set_warnings_file(qobj, stderr);
		queueinfo_set_debug_level(qobj, debug_level);

		/* Consider the new PPD file.  If this fails, it will throw an exception. */
		queueinfo_add_hypothetical_printer(qobj, printer, ppdname, NULL);

		/*
		** Modify the printer's configuration file.
		**
		** First, copy up to (but not including) the first "PPDFile:" line.
		** As we go, delete lines which will be obsolete since the PPD
		** file is changing.
		*/
		while((line = conf_getline(obj)))
			{
			if(lmatch(line, "PPDFile:"))				/* stop at */
				break;
			if(lmatch(line, "DefFiltOpts:"))			/* delete */
				continue;
			if(lmatch(line, "PPDOpt:"))					/* delete */
				continue;
			conf_printf(obj, "%s\n", line);
			}
	
		conf_printf(obj, "PPDFile: %s\n", ppdname);
	
		/*
		** Copy the rest of the file, deleting "PPDFile:" lines and any
		** lines made obsolete by the fact that the PPD file has been
		** changed.
		*/
		while((line = conf_getline(obj)))
			{
			if(lmatch(line, "PPDFile:"))				/* delete */
				continue;
			if(lmatch(line, "DefFiltOpts:"))			/* delete */
				continue;
			if(lmatch(line, "PPDOpt:"))					/* delete */
				continue;
			conf_printf(obj, "%s\n", line);
			}

		/* Insert the new "DefFiltOpts:" line. */
		if((p = queueinfo_computedDefaultFilterOptions(qobj)))
			conf_printf(obj, "DefFiltOpts: %s\n", p);

		/* This will commit the changes. */
		conf_close(obj);
		}
	gu_Final {
		if(qobj)
			queueinfo_free(qobj);
		}
	gu_Catch {
		conf_abort(obj);
		gu_utf8_fprintf(stderr, "%s: %s\n", myname, gu_exception);
		return exception_to_exitcode(gu_exception_code);
		}
	
	/* Update any groups which have this printer as a member. */
	return update_groups_deffiltopts(printer);
	} /* command_ppd() */

/*
<command acl="ppad" helptopics="printer,printer-advanced">
	<name><word>alerts</word></name>
	<desc>set frequency and destination of printer alerts</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg><name>frequency</name><desc>error count at which to send alerts</desc></arg>
		<arg><name>method</name><desc>method by which to send alerts</desc></arg>
		<arg><name>address</name><desc>address to which to send alerts</desc></arg>
	</args>
</command>
*/
int command_alerts(const char *argv[])
	{
	const char *printer = argv[0];
	int frequency;
	const char *method = argv[2];
	const char *address = argv[3];
	struct CONF_OBJ *obj;
	char *line;

	if(strspn(argv[1], "-0123456789") != strlen(argv[1]))
		{
		gu_utf8_fprintf(stderr, _("Alerts interval must be an integer.\n"));
		return EXIT_SYNTAX;
		}

	frequency = atoi(argv[1]);

	if(!(obj = conf_open(QUEUE_TYPE_PRINTER, printer, CONF_MODIFY | CONF_ENOENT_PRINT | CONF_RELOAD)))
		return EXIT_BADDEST;

	/* Modify the printer's configuration file. */
	while((line = conf_getline(obj)))
		{
		if(lmatch(line, "Alert:"))
			break;
		else
			conf_printf(obj, "%s\n", line);
		}

	conf_printf(obj, "Alert: %d %s %s\n", frequency, method, address);

	while((line = conf_getline(obj)))			/* copy rest of file, */
		{										/* deleting further */
		if(lmatch(line, "Alert:"))				/* "Alert:" lines. */
			continue;
		else
			conf_printf(obj, "%s\n", line);
		}

	conf_close(obj);

	return EXIT_OK;
	} /* command_alert() */

/*
<command acl="ppad" helptopics="printer,printer-advanced">
	<name><word>frequency</word></name>
	<desc>alter frequency of printer alerts</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg><name>frequency</name><desc>error count at which to send alerts</desc></arg>
	</args>
</command>
**
** Change the configuration file "Alert:" line to change the
** alert frequency.  The absence of an  "Alert:" line is an error.
*/
int command_frequency(const char *argv[])
	{
	const char *printer;
	int alert_frequency;
	char *alert_method = (char*)NULL;
	char *alert_address = (char*)NULL;
	struct CONF_OBJ *obj;
	char *line;

	if(strspn(argv[1], "-0123456789") != strlen(argv[1]))
		{
		gu_utf8_fprintf(stderr, _("Alerts interval must be an integer.\n"));
		return EXIT_SYNTAX;
		}

	printer = argv[0];
	alert_frequency = atoi(argv[1]);

	if(!(obj = conf_open(QUEUE_TYPE_PRINTER, printer, CONF_MODIFY | CONF_ENOENT_PRINT | CONF_RELOAD)))
		return EXIT_BADDEST;

	/* Modify the printer's configuration file. */
	while((line = conf_getline(obj)))
		{
		if(lmatch(line, "Alert:"))
			{
			gu_free_if(alert_method);
			gu_free_if(alert_address);
			gu_sscanf(line, "Alert: %*d %S %S", &alert_method, &alert_address);
			break;
			}
		else
			{
			conf_printf(obj, "%s\n", line);
			}
		}

	if(! alert_method || ! alert_address)
		{
		gu_utf8_fputs(_("No alert method and address defined, use \"ppad alerts\".\n"), stderr);
		conf_abort(obj);
		return EXIT_NOTFOUND;
		}

	conf_printf(obj, "Alert: %d %s %s\n", alert_frequency, alert_method, alert_address);
	gu_free(alert_method);
	gu_free(alert_address);
 
	while((line = conf_getline(obj)))			/* copy rest of file, */
		{										/* deleting further */
		if(lmatch(line, "Alert:"))				/* "Alert:" lines. */
			continue;
		else
			conf_printf(obj, "%s\n", line);
		}

	conf_close(obj);				/* put new file into place */

	return EXIT_OK;
	} /* command_frequency() */

/*
<command acl="ppad" helptopics="printer,printer-flags">
	<name><word>flags</word></name>
	<desc>set rules for printing flag (banner and trailer) pages</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg><name>banner</name><desc>never, no, yes, or always</desc></arg>
		<arg><name>trailer</name><desc>never, no, yes, or always</desc></arg>
	</args>
</command>
*/
int command_flags(const char *argv[])
	{
	const char *printer = argv[0];
	int banner;
	#ifdef GNUC_HAPPY
	int trailer=0;
	#else
	int trailer;
	#endif

	if(	(banner = flag_code(argv[1])) == BANNER_INVALID
		||
		(trailer = flag_code(argv[2])) == BANNER_INVALID
		)
		{
		gu_utf8_fputs(_("Banner and trailer must be set to \"never\", \"no\",\n"
				"\"yes\", or \"always\".\n"), stderr);
		return EXIT_SYNTAX;
		}

	return conf_set_name(QUEUE_TYPE_PRINTER, printer, 0, "FlagPages", "%d %d", banner, trailer);
	} /* command_flags() */

/*
<command acl="ppad" helptopics="printer,printer-advanced,printer-ppd">
	<name><word>outputorder</word></name>
	<desc>set rules for printing flag (banner and trailer) pages</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg><name>order</name><desc>normal, reverse, or ppd</desc></arg>
	</args>
</command>
**
** If the direction is "ppd", the "OutputOrder:" line is deleted
** from the configuration file.
*/
int command_outputorder(const char *argv[])
	{
	const char *printer = argv[0];
	const char *newstate;

	if(gu_strcasecmp(argv[1], "Normal") == 0)
		newstate = "Normal";
	else if(gu_strcasecmp(argv[1], "Reverse") == 0)
		newstate = "Reverse";
	else if(gu_strcasecmp(argv[1], "PPD") == 0)
		newstate = NULL;
	else
		{
		gu_utf8_fputs(_("Set outputorder to \"Normal\", \"Reverse\", or \"PPD\".\n"), stderr);
		return EXIT_SYNTAX;
		}

	return conf_set_name(QUEUE_TYPE_PRINTER, printer, 0, "OutputOrder", newstate ? "%s" : NULL, newstate);
	} /* command_direction() */

/*
<command acl="ppad" helptopics="printer,printer-advanced">
	<name><word>charge</word></name>
	<desc>set or clear per-sheet charge (possibly 0.00)</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg><name>per-sheet/duplex</name><desc>none or N.NN</desc></arg>
		<arg flags="optional"><name>simplex</name><desc>N.NN</desc></arg>
	</args>
</command>
**
** !!! BUG BUG BUG !!!
** We get the printer configuration re-read, but
** we really ought to get the group configurations
** re-read too.
*/
int command_charge(const char *argv[])
	{
	const char *printer = argv[0];
	int retval;

	if(argv[1][0] == '\0' || strcmp(argv[1], "none") == 0)
		{
		if(argv[2] && strlen(argv[2]) > 0)
			{
			gu_utf8_fprintf(stderr, _("A second parameter is not allowed because first is \"%s\"."), argv[1]);
			return EXIT_SYNTAX;
			}
		retval = conf_set_name(QUEUE_TYPE_PRINTER, printer, CONF_RELOAD, "Charge", NULL);
		}
	else
		{
		int newcharge_duplex_sheet, newcharge_simplex_sheet;

		/* Check for proper syntax of values.  This check isn't really rigorous enough!!! */
		if(strspn(argv[1], "0123456789.") != strlen(argv[1]))
			{
			gu_utf8_fprintf(stderr, _("The value \"%s\" is not in the correct format for decimal currency."), argv[1]);
			return EXIT_SYNTAX;
			}
		if(argv[2] && strspn(argv[2], "0123456789.") != strlen(argv[2]))
			{
			gu_utf8_fprintf(stderr, _("The value \"%s\" is not in the correct format for decimal currency."), argv[2]);
			return EXIT_SYNTAX;
			}

		/* Convert to floating point dollars, convert to cents, and truncate to int. */
		newcharge_duplex_sheet = (int)(gu_getdouble(argv[1]) * 100.0 + 0.5);

		if(argv[2])
			newcharge_simplex_sheet = (int)(gu_getdouble(argv[2]) * 100.0 + 0.5);
		else
			newcharge_simplex_sheet = newcharge_duplex_sheet;

		retval = conf_set_name(QUEUE_TYPE_PRINTER, printer, CONF_RELOAD, "Charge", "%d.%02d %d.%02d",
				newcharge_duplex_sheet / 100,
				newcharge_duplex_sheet % 100,
				newcharge_simplex_sheet / 100,
				newcharge_simplex_sheet % 100);
		}

	return retval;
	} /* command_charge() */

/*
<command acl="ppad" helptopics="printer,printer-advanced,printer-ppd">
	<name><word>bins</word><word>ppd</word></name>
	<desc>set bins list from PPD file</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
	</args>
</command>
*/
int command_bins_ppd(const char *argv[])
	{
	const char *printer = argv[0];		/* name of printer whose configuration should be changed */
	struct CONF_OBJ *obj;
	char *line;
	char *ppdname = NULL;
	void *ppdobj = NULL;				/* initialized so GCC won't complain */
	char *ppdline;						/* a line read from the PPD file */
	int x;

	/* make sure the printer exists */
	if(!(obj = conf_open(QUEUE_TYPE_PRINTER, printer, CONF_MODIFY | CONF_ENOENT_PRINT | CONF_RELOAD)))
		return EXIT_BADDEST;

	/* Modify the printer's configuration file. */
	while((line = conf_getline(obj)))				/* copy all lines */
		{
		if(lmatch(line, "PPDFile:"))
			{
			gu_sscanf(line,"PPDFile: %A", &ppdname);
			}

		if(lmatch(line, "Bin:"))				/* don't copy those */
			continue;							/* that begin with "Bin: " */
		else
			conf_printf(obj, "%s\n", line);
		}

	if(! ppdname)
		{
		gu_utf8_fputs(_("Printer configuration file does not have a \"PPDFile:\" line.\n"), stderr);
		conf_abort(obj);
		return EXIT_NOTFOUND;
		}

	gu_Try
		{
		ppdobj = ppdobj_new(ppdname);
		}
	gu_Catch
		{
		conf_abort(obj);
		gu_ReThrow();
		}

	while((ppdline = ppdobj_readline(ppdobj)))
		{
		if(lmatch(ppdline, "*InputSlot"))		/* Use only "*InputSlot" */
			{
			x = 10;
			x += strspn(&ppdline[x]," \t");		/* Skip to start of options keyword */

			ppdline[x+strcspn(&ppdline[x],":/")] = '\0';
												/* terminate at start of translation or code */
			conf_printf(obj, "Bin: %s\n", &ppdline[x]);
			}
		}

	ppdobj_free(ppdobj);
	
	conf_close(obj);

	return EXIT_OK;
	} /* command_bins_ppd() */

/*
** Add bin(s) to a printer's bin list.
*/
static int printer_bins_set_or_add(gu_boolean add, const char *argv[])
	{
	const char *printer = argv[0], *bin;
	struct CONF_OBJ *obj;
	char *line;
	int idx;					/* the bin we are working on */
	int count = 0;				/* number of bins in conf file */
	gu_boolean duplicate;

	if(!(obj = conf_open(QUEUE_TYPE_PRINTER, printer, CONF_MODIFY | CONF_ENOENT_PRINT | CONF_MODIFY)))
		return EXIT_BADDEST;

	/* Copy up to the first "Bin:" line. */
	while((line = conf_getline(obj)))
		{
		if(lmatch(line, "Bin:"))
			break;
		conf_printf(obj, "%s\n", line);
		}

	/* Delete or copy all the "Bin:" lines */
	duplicate = FALSE;
	do	{
		if(!(bin = lmatchp(line, "Bin:")))
			break;

		if(add)
			{
			for(idx=1; argv[idx]; idx++)
				{
				if(strcmp(bin, argv[idx]) == 0)
					{
					gu_utf8_fprintf(stderr, _("Printer \"%s\" already has a bin called \"%s\".\n"), printer, argv[idx]);
					duplicate = TRUE;
					}
				}

			conf_printf(obj, "%s\n", line);
			count++;
			}
		} while((line = conf_getline(obj)));

	if(duplicate)
		{
		conf_abort(obj);
		return EXIT_ALREADY;
		}

	/* Add the new "Bin:" lines. */
	for(idx=1; argv[idx]; idx++, count++)
		conf_printf(obj, "Bin: %s\n", argv[idx]);

	/* If this would make for too many bins, abort the changes. */
	if(count > MAX_BINS)
		{
		gu_utf8_fprintf(stderr, _("Can't add these %d bin%s to \"%s\" because then the printer would\n"
				"have %d bins and the %d bin limit would be exceeded.\n"),
				idx - 1, idx == 2 ? "" : "s",
				printer, count, MAX_BINS);
		conf_abort(obj);
		return EXIT_OVERFLOW;
		}

	/* Copy the rest of the file */
	while((line = conf_getline(obj)))
		conf_printf(obj, "%s\n", line);

	/* Commit to the changes. */
	conf_close(obj);

	return EXIT_OK;
	} /* command_bins_set_or_add() */

/*
<command acl="ppad" helptopics="printer,printer-advanced,printer-ppd">
	<name><word>bins</word><word>set</word></name>
	<desc>set bins list</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg flags="optional,repeat"><name>bins</name><desc>bins to use</desc></arg>
	</args>
</command>
*/
int command_bins_set(const char *argv[])
	{
	return printer_bins_set_or_add(FALSE, argv);
	} /* command_bins_set() */

/*
<command acl="ppad" helptopics="printer,printer-advanced,printer-ppd">
	<name><word>bins</word><word>add</word></name>
	<desc>add to bins list</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg flags="optional,repeat"><name>bins</name><desc>bins to add</desc></arg>
	</args>
</command>
*/
int command_bins_add(const char *argv[])
	{
	return printer_bins_set_or_add(TRUE, argv);
	} /* command_bins_add() */

/*
<command acl="ppad" helptopics="printer,printer-advanced,printer-ppd">
	<name><word>bins</word><word>delete</word></name>
	<desc>delete from bins list</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg flags="optional,repeat"><name>bins</name><desc>bins to remove</desc></arg>
	</args>
</command>
*/
int command_bins_delete(const char *argv[])
	{
	const char *printer = argv[0];
	struct CONF_OBJ *obj;
	char *line, *p;
	int idx;
	int misses;
	int found[MAX_BINS];

	/* Set a checklist entry for each bin to be removed to no. */
	for(idx=1; argv[idx]; idx++)
		{
		if(idx > MAX_BINS)
			{
			gu_utf8_fprintf(stderr, _("You can't delete more than %d bins at a time.\n"), MAX_BINS);
			return EXIT_SYNTAX;
			}
		found[idx - 1] = FALSE;
		}

	if(!(obj = conf_open(QUEUE_TYPE_PRINTER, printer, CONF_MODIFY | CONF_ENOENT_PRINT | CONF_RELOAD)))
		return EXIT_BADDEST;

	/* Copy the file, removing the bin lines that match as we go. */
	while((line = conf_getline(obj)))
		{
		if((p = lmatchp(line, "Bin:")))
			{
			/* Search the delete list for the bin. */
			for(idx=1; argv[idx]; idx++)
				{
				if(strcmp(p, argv[idx]) == 0)
					{
					found[idx - 1] = TRUE;
					break;
					}
				}

			/* If this bin was in the delete list, don't keep it. */
			if(argv[idx])
				continue;
			}

		conf_printf(obj, "%s\n", line);
		}

	/* Check and see if all of the bins were found. */
	misses = 0;
	for(idx=1; argv[idx]; idx++)
		{
		if(! found[idx - 1])
			{
			gu_utf8_fprintf(stderr, _("The printer \"%s\" does not have a bin called \"%s\".\n"), printer, argv[idx]);
			misses++;
			}
		}

	/* If any of the bins to be deleted are not found, cancel whole command. */
	if(misses)
		{
		conf_abort(obj);
		return EXIT_BADBIN;
		}

	/* Commit to the changes */
	conf_close(obj);

	return EXIT_OK;
	} /* command_bins_delete() */

/*
 * This function clears a directory of files (one level only)
 * and then removes it.  It is used when removing a printer.
 */
static void remove_directory(const char dirname[])
	{
	DIR *dir;
	struct dirent *dent;
	char fname[MAX_PPR_PATH];

	if((dir = opendir(dirname)))
		{
		while((dent = readdir(dir)))
			{
			ppr_fnamef(fname, "%s/%s", dirname, dent->d_name);
			unlink(fname);
			}
		closedir(dir);
		rmdir(dirname);
		}
	}

/*
<command acl="ppad" helptopics="printer,printer-basic">
	<name><word>delete</word></name>
	<desc>delete <arg>printer</arg></desc>
	<args>
		<arg><name>printer</name><desc>printer to be deleted</desc></arg>
	</args>
</command>
*/
int command_delete(const char *argv[])
	{
	const char function[] = "printer_delete";
	const char *printer = argv[0];
	struct CONF_OBJ *obj;
	char *line, *p;
	char fname[MAX_PPR_PATH];
	DIR *dir;
	struct dirent *direntp;
	int len;
	int is_a_member;

	ppop2("halt", printer);				/* halt printer */
	ppop2("reject", printer);			/* accept no more jobs */
	ppop2("purge", printer);			/* cancel all existing jobs */

	/*
	 * Remove the printer from membership in each and every group.  Rather 
	 * laborious, don't you think?
	 */
	if(!(dir = opendir(GRCONF)))
		{
		gu_utf8_fprintf(stderr, _("%s(): %s(\"%s\") failed, errno=%d (%s)\n"), function, "opendir", GRCONF, errno, strerror(errno));
		return EXIT_INTERNAL;
		}

	while((direntp = readdir(dir)))
		{
		if( direntp->d_name[0] == '.' )			/* skip . and .. */
			continue;							/* and temporary files */

		len=strlen(direntp->d_name);			/* Emacs style backup files */
		if( len > 0 && direntp->d_name[len-1]=='~' )
			continue;							/* should be skipt. */

		is_a_member = FALSE;

		if(!(obj = conf_open(QUEUE_TYPE_GROUP, direntp->d_name, 0)))
			{
			gu_utf8_fprintf(stderr, "%s(): conf_open(QUEUE_TYPE_GROUP, \"%s\", 0) failed", function, direntp->d_name);
			closedir(dir);
			return EXIT_INTERNAL;
			}

		while((line = conf_getline(obj)))
			{
			if((p = lmatchp(line, "Printer:")))
				{
				if(strcmp(p, printer) == 0)
					{
					is_a_member = TRUE;
					break;
					}
				}
			}

		conf_close(obj);

		if(is_a_member)
			group_remove_internal(direntp->d_name,printer);
		}

	closedir(dir);

	/* Remove the printer configuration file. */
	ppr_fnamef(fname, "%s/%s", PRCONF, printer);
	if(unlink(fname) == -1)
		{
		if(errno==ENOENT)
			{
			gu_utf8_fprintf(stderr, _("The printer \"%s\" does not exist.\n"), printer);
			return EXIT_BADDEST;
			}
		else
			{
			gu_utf8_fprintf(stderr, "unlink(\"%s\") failed, errno=%d (%s)\n", fname, errno, gu_strerror(errno));
			return EXIT_INTERNAL;
			}
		}

	/* Remove mounted, stop, etc. */
	ppr_fnamef(fname, "%s/%s", PRINTERS_PERSISTENT_STATEDIR, printer);
	remove_directory(fname);

	/* Remove status, address cache, etc. */
	ppr_fnamef(fname, "%s/%s", PRINTERS_PURGABLE_STATEDIR, printer);
	remove_directory(fname);

	/* Send a printer-touch command to pprd. */
	write_fifo("NP %s\n", printer);

	return EXIT_OK;
	} /* command_delete() */

/*
<command acl="ppad" helptopics="printer">
	<name><word>touch</word></name>
	<desc>instruct pprd to reload <arg>printer</arg></desc>
	<args>
		<arg><name>printer</name><desc>printer to reload</desc></arg>
	</args>
</command>
*/
int command_touch(const char *argv[])
	{
	const char *printer = argv[0];
	struct CONF_OBJ *obj;

	/* Just open it in read-only mode with CONF_RELOAD set and close it again. */
	if(!(obj = conf_open(QUEUE_TYPE_PRINTER, printer, CONF_ENOENT_PRINT | CONF_RELOAD)))
		return EXIT_BADDEST;
	conf_close(obj);

	return EXIT_OK;
	} /* command_touch() */

/*
<command acl="ppad" helptopics="printer,printer-advanced">
	<name><word>switchset</word></name>
	<desc>attach a set of switches to a printer</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg flags="optional,repeat"><name>switchset</name><desc>switches to attach (ommit to delete list)</desc></arg>
	</args>
</command>
*/
int command_switchset(const char *argv[])
	{
	const char *printer = argv[0];		/* name of printer */
	char newset[256];					/* new set of switches */

	/* convert the switch set to a line */
	if(make_switchset_line(newset, &argv[1]))
		{
		gu_utf8_fputs(_("Bad set of switches.\n"), stderr);
		return EXIT_SYNTAX;
		}

	return conf_set_name(QUEUE_TYPE_PRINTER, printer, 0, "Switchset", newset[0] ? "%s" : NULL, newset);
	} /* command_switchset() */

/*
<command acl="ppad" helptopics="printer">
	<name><word>deffiltopts</word></name>
	<desc>update a printer's default filter options</desc>
	<args>
		<arg><name>printer</name><desc>printer to update</desc></arg>
	</args>
</command>
*/
int command_deffiltopts(const char *argv[])
	{
	const char *printer = argv[0];
	struct CONF_OBJ *obj;

	if(!(obj = conf_open(QUEUE_TYPE_PRINTER, printer, CONF_MODIFY | CONF_ENOENT_PRINT)))
		return EXIT_BADDEST;

	{
	void *qobj = NULL;
	char *line;
	gu_Try {
		if(!(qobj = queueinfo_new_load_config(QUEUEINFO_PRINTER, printer)))
			gu_Throw(_("Printer \"%s\" does not exist."), printer);

		queueinfo_set_warnings_file(qobj, stderr);
		queueinfo_set_debug_level(qobj, debug_level);

		while((line = conf_getline(obj)))
			{
			if(lmatch(line, "DefFiltOpts:"))			/* delete */
				continue;
			conf_printf(obj, "%s\n", line);
			}

		{
		const char *p;
		if((p = queueinfo_computedDefaultFilterOptions(qobj)))
			conf_printf(obj, "DefFiltOpts: %s\n", p);
		}

		conf_close(obj);
		}
	gu_Final {
		if(qobj)
			queueinfo_free(qobj);
		}
	gu_Catch {
		conf_abort(obj);
		gu_utf8_fprintf(stderr, "%s: %s\n", myname, gu_exception);
		return exception_to_exitcode(gu_exception_code);
		}
	}

	return EXIT_OK;
	} /* command_deffiltopts() */

/*
<command acl="ppad" helptopics="printer,printer-advanced">
	<name><word>passthru</word></name>
	<desc>set a printer's passthru language list</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg flags="optional,repeat"><name>languages</name><desc>languages to pass thru (ommit to delete list)</desc></arg>
	</args>
</command>
*/
int command_passthru(const char *argv[])
	{
	const char *printer = argv[0];
	char *passthru;
	int retval;

	passthru = list_to_string(&argv[1]);
	retval = conf_set_name(QUEUE_TYPE_PRINTER, printer, 0, "PassThru", passthru ? "%s" : NULL, passthru);
	gu_free_if(passthru);

	return retval;
	} /* command_passthru() */

/*
<command acl="ppad" helptopics="printer,printer-advanced,printer-ppd">
	<name><word>ppdopts</word></name>
	<desc>choose optional printer features from PPD file</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg flags="optional,repeat"><name>feature</name><desc>feature name</desc></arg>
		<arg flags="optional,repeat"><name>option</name><desc>condition of <arg>feature</arg></desc></arg>
	</args>
</command>
**
** Note that though it is not reflected in the XML description above, feature
** and option can be repeated as a pair!!!
**
** This command opens the PPD file and asks the user for an answer for each
** option found therein.  For each answer it generates a "PPDOpts:" line
** for the printer configuration file.
*/
int command_ppdopts(const char *argv[])
	{
	const char function[] = "printer_ppdopts";
	const char *printer = argv[0];
	struct CONF_OBJ *obj;
	char *line, *p;
	const char **answers = (const char **)NULL;

	char *PPDFile = (char*)NULL;		/* name of PPD file to open */
	char *InstalledMemory = (char*)NULL;
	unsigned int next_value;
	int c;
	char *values[100];					/* the list of possible values */

	/* Are there answers on the command line? */
	if(argv[1])
		answers = &argv[1];

	/* Set whole values array to NULL so we will later know what we must gu_free(). */
	for(next_value=0; next_value < (sizeof(values)/sizeof(char*)); next_value++)
		values[next_value] = (char*)NULL;

	/* Open the printer's configuration file for modification.  We include
	 * CONF_RELOAD because pprd might want to clear the never flag for
	 * this printer.
	 */ 
	if(!(obj = conf_open(QUEUE_TYPE_PRINTER, printer, CONF_MODIFY | CONF_ENOENT_PRINT | CONF_RELOAD)))
		return EXIT_BADDEST;

	{
	void *ppd_obj = NULL;
	gu_Try {
		/*
		** Copy the existing printer configuration file, discarding
		** "PPDOpt:" and "DefFiltOpts:" lines and noting which
		** PPD file is to be used.
		*/
		while((line = conf_getline(obj)))
			{
			if(lmatch(line, "PPDOpt:"))					/* delete */
				continue;
	
			if(lmatch(line, "DefFiltOpts:"))			/* delete */
				continue;
	
			conf_printf(obj, "%s\n", line);				/* copy to output file */
	
			if(gu_sscanf(line, "PPDFile: %A", &p) == 1)
				{
				gu_free_if(PPDFile);
				PPDFile = p;
				}
			}
	
		/* If there was no "PPDFile:" line, we can't continue. */
		if(!PPDFile)
			gu_Throw(_("The printer \"%s\" has no \"PPDFile:\" line in its configuration file."), printer);
	
		/*
		** Read the PPD file, ask the questions, and
		** generate "PPDOpts:" lines.
		*/
		{
		char *ppdline;
		gu_boolean in_installable_options = FALSE;
		char *ui_open = (char*)NULL;		/* the UI section we are in, NULL otherwise */
		int ui_open_mrlen = 0;

		ppd_obj = ppdobj_new(PPDFile);
		next_value = 0;

		if(!answers)
			gu_putwc('\n');

		while((ppdline = ppdobj_readline(ppd_obj)))
			{
			if(!in_installable_options)
				{
				if((p = lmatchp(ppdline, "*OpenGroup:")))
					{
					if(lmatch(p, "InstallableOptions"))	/* !!! */
						in_installable_options = TRUE;
					}
				continue;
				}

			if((p = lmatchp(ppdline, "*CloseGroup:")))
				{
				in_installable_options = FALSE;
				continue;
				}

			if((p = lmatchp(ppdline, "*OpenUI")))
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
					gu_utf8_fprintf(stderr, _("WARNING: Unclosed UI block \"%s\" in PPD file.\n"), ui_open);
					gu_free(ui_open);
					ui_open = (char*)NULL;
					}
	
				/* Truncate after the end of the translation string. */
				p[strcspn(p, ":")] = '\0';
	
				/* Print the option name and its translation string. */
				if(!answers)
					gu_utf8_printf("%s\n", p);
	
				/* Save the option name and translation string. */
				ui_open = gu_strdup(p);
				ui_open_mrlen = strcspn(p, "/");
	
				/* Set to indicated no accumulated values. */
				next_value = 1;
	
				continue;
				}
	
			/* If this is one of the choices, */
			if(ui_open && strcspn(ppdline, "/ \t") == ui_open_mrlen && strncmp(ppdline, ui_open, ui_open_mrlen) == 0)
				{
				p = ppdline;
				p += strcspn(p, " \t");
				p += strspn(p, " \t");
	
				/* Truncate after option name and translation string. */
				p[strcspn(p, ":")] = '\0';
	
				if(!answers)
					gu_utf8_printf("%d) %s\n", next_value, p);
	
				if(next_value >= (sizeof(values) / sizeof(char*)))
					{
					gu_utf8_fprintf(stderr, "%s(): values[] overflow\n", function);
					conf_abort(obj);
					return EXIT_INTERNAL;
					}
	
				gu_free_if(values[next_value]);
				values[next_value++] = gu_strdup(p);
	
				continue;
				}
	
			/* If this is the end of an option, ask for a choice. */
			if(ui_open && (p = lmatchp(ppdline, "*CloseUI:")))
				{
				if( strncmp(p, ui_open, strcspn(ui_open, "/")) )
					{
					gu_utf8_fputs(_("WARNING: mismatched \"*OpenUI\", \"*CloseUI\" in PPD file.\n"), stderr);
					gu_utf8_fprintf(stderr, "(\"%s\" closed by \"%.*s\".)\n", ui_open, (int)strcspn(p,"/\n"), p);
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
							gu_utf8_printf("x=%d, y=%d\n", x, y);
							gu_utf8_printf("answers[x]=\"%s\", answers[x+1]=\"%s\", ui_open=\"%s\", ui_open_mrlen=%d, values[y]=\"%s\"\n", answers[x], answers[x+1], ui_open, ui_open_mrlen, values[y]);
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
						gu_utf8_fprintf(stderr, _("Warning:	 No value provided for \"%s\".\n"), ui_open);
						}
					else if(!valid)
						{
						gu_utf8_fprintf(stderr, _("Value provided for \"%s\" is not among the possible values.\n"), ui_open);
						conf_abort(obj);
						return EXIT_SYNTAX;
						}
					}
	
				/* Otherwise, present the user with the list of options we have gathered. */
				else
					{
					char temp[10];
					do	{
						gu_utf8_printf(_("Choose 1 thru %d or 0 if unknown> "), next_value-1);
						fflush(stdout);
						if(fgets(temp, sizeof(temp), stdin) == (char*)NULL)
							{
							c = 0;
							break;
							}
						c = atoi(temp);
						} while( ! isdigit( temp[strspn(temp," \t")] ) || c >= next_value );
	
					gu_putwc('\n');
					}
	
				/*
				** If the user did not choose zero (unknown), write
				** a line containing the option and the chosen value
				** into the printer configuration file.
				*/
				if(c != 0)
					{
					/* Print a line with the selected option. */
					conf_printf(obj, "PPDOpt: %.*s %.*s", strcspn(ui_open, "/"), ui_open, strcspn(values[c],"/"), values[c]);
	
					/* If translation strings are provided for both the category and the
					   selected option, print them afterward so that they can later
					   be shown in the "ppad show" output. */
					if(strchr(ui_open, '/') && strchr(values[c],'/'))
						conf_printf(obj, " (%s %s)", (strchr(ui_open, '/') + 1), (strchr(values[c], '/') + 1));
	
					/* End the configuration file line. */
					conf_printf(obj, "\n");
	
					/* If this is the amount of installed memory, feed the
					   value to the code which is generating the "DefFiltOpts:" line. */
					if(lmatch(ui_open, "*InstalledMemory"))
						 InstalledMemory = gu_strndup(values[c], strcspn(values[c], "/"));
					}
	
				gu_free(ui_open);
				ui_open = (char*)NULL;
				ui_open_mrlen = 0;
				}
			} /* end of PPD reading loop */

		/* Sanity check. */
		if(ui_open)
			{
			gu_utf8_fprintf(stderr, _("WARNING: Unclosed UI block in PPD file: \"%s\".\n"), ui_open);
			gu_free(ui_open);
			}
	
		/* Emmit a new "DefFiltOpts:" line. */
		{
		void *qobj = NULL;
		const char *cp;
		gu_Try {
			qobj = queueinfo_new(QUEUEINFO_PRINTER, printer);
			queueinfo_set_warnings_file(qobj, stderr);
			queueinfo_set_debug_level(qobj, debug_level);
			queueinfo_add_hypothetical_printer(qobj, printer, PPDFile, InstalledMemory);
			if((cp = queueinfo_computedDefaultFilterOptions(qobj)))
				conf_printf(obj, "DefFiltOpts: %s\n", cp);
			}
		gu_Final {
			if(qobj)
				queueinfo_free(qobj);
			}
		gu_Catch {
			gu_ReThrow();
			}
		}
		}
		
		/* Close the new configuration file and move it into place. */
		conf_close(obj);
		}
	gu_Final {
		if(ppd_obj)
			ppdobj_free(ppd_obj);
		}
	gu_Catch {
		conf_abort(obj);
		gu_utf8_fprintf(stderr, "%s: %s\n", myname, gu_exception);
		return exception_to_exitcode(gu_exception_code);
		}
	}

	/* Free any lingering memory blocks */
	gu_free_if(PPDFile);
	gu_free_if(InstalledMemory);
	for(next_value=0; next_value < (sizeof(values)/sizeof(char*)); next_value++)
		{
		gu_free_if(values[next_value]);
		}

	/*
	** Update the default filter options of any
	** groups which have this printer as a member.
	*/
	return update_groups_deffiltopts(printer);
	} /* command_ppdopts() */

/*
<command acl="ppad" helptopics="printer,printer-limits">
	<name><word>limitpages</word></name>
	<desc>set lower and upper bounds on pages printed</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg><name>lower</name><desc>minium number of pages, 0 for no limit</desc></arg>
		<arg><name>upper</name><desc>maximum number of pages, 0 for no limit</desc></arg>
	</args>
</command>
*/
int command_limitpages(const char *argv[])
	{
	const char *printer = argv[0];
	int limit_lower, limit_upper;

	if((limit_lower = atoi(argv[1])) < 0)
		{
		gu_utf8_fputs(_("The lower limit must be 0 (unlimited) or a positive integer.\n"), stderr);
		return EXIT_SYNTAX;
		}

	if((limit_upper = atoi(argv[2])) < 0)
		{
		gu_utf8_fputs(_("The upper limit must be 0 (unlimited) or a positive integer.\n"), stderr);
		return EXIT_SYNTAX;
		}

	return conf_set_name(QUEUE_TYPE_PRINTER, printer, CONF_RELOAD, "LimitPages", (limit_lower > 0 || limit_upper > 0) ? "%d %d" : NULL, limit_lower, limit_upper);
	} /* command_limitpages() */

/*
<command acl="ppad">
	<name><word>limitkilobytes</word></name>
	<desc>set lower and upper bounds on job size in kilobytes</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg><name>lower</name><desc>minium number of kilobytes, 0 for no limit</desc></arg>
		<arg><name>upper</name><desc>maximum number of kilobytes, 0 for no limit</desc></arg>
	</args>
</command>
*/
int command_limitkilobytes(const char *argv[])
	{
	const char *printer = argv[0];
	int limit_lower, limit_upper;

	if((limit_lower = atoi(argv[1])) < 0)
		{
		gu_utf8_fputs(_("The lower limit must be 0 (unlimited) or a positive integer.\n"), stderr);
		return EXIT_SYNTAX;
		}

	if((limit_upper = atoi(argv[2])) < 0)
		{
		gu_utf8_fputs(_("The upper limit must be 0 (unlimited) or a positive integer.\n"), stderr);
		return EXIT_SYNTAX;
		}

	return conf_set_name(QUEUE_TYPE_PRINTER, printer, CONF_RELOAD, "LimitKilobytes", (limit_lower > 0 || limit_upper > 0) ? "%d %d" : NULL, limit_lower, limit_upper);
	} /* command_limitkilobytes() */

/*
<command acl="ppad">
	<name><word>grayok</word></name>
	<desc>allow or prohibit grayscale-only (non-color) documents</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg><name>grayok</name><desc>true, or false</desc></arg>
	</args>
</command>
*/
int command_grayok(const char *argv[])
	{
	const char *printer = argv[0];
	gu_boolean grayok;

	if(gu_torf_setBOOL(&grayok, argv[1]) == -1)
		{
		gu_utf8_fputs(_("Value is not boolean.\n"), stderr);
		return EXIT_SYNTAX;
		}

	/* If FALSE, set to "false", otherwise delete line.  A reload is required when
	 * turning on GrayOK since it affects the eligibility of all-grayscale jobs
	 * which may already have been declared inelligible for certain printers.
	 */
	return conf_set_name(QUEUE_TYPE_PRINTER, printer, grayok ? CONF_RELOAD : 0, "GrayOK", grayok ? NULL : "false");
	} /* command_grayok() */

/*
<command acl="ppad">
	<name><word>acls</word></name>
	<desc>set list of ACLs listing those who can submit to <arg>printer</arg></desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg flags="optional,repeat"><name>acls</name><desc>list of ACLs (ommit to remove restrictions)</desc></arg>
	</args>
</command>
*/
int command_acls(const char *argv[])
	{
	const char *printer = argv[0];
	char *acls;
	int retval;

	acls = list_to_string(&argv[1]);
	retval = conf_set_name(QUEUE_TYPE_PRINTER, printer, 0, "ACLs", acls ? "%s" : NULL, acls);
	gu_free_if(acls);

	return retval;
	} /* command_acls() */

/*
<command acl="ppad">
	<name><word>userparams</word></name>
	<desc>set PostScript userparam values</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg flags="optional,repeat"><name>parameters</name><desc>list of name=value pairs</desc></arg>
	</args>
</command>
*/
int command_userparams(const char *argv[])
	{
	const char *printer = argv[0];
	char *userparams;
	int result;
	userparams = list_to_string(&argv[1]);
	result = conf_set_name(QUEUE_TYPE_PRINTER, printer, 0, "Userparams", userparams ? "%s" : NULL, userparams);
	gu_free_if(userparams);
	return result;
	} /* command_userparams() */

/*
<command acl="ppad">
	<name><word>pagetimelimit</word></name>
	<desc>set time limit on processing of each page</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg><name>seconds</name><desc>number of seconds to allow per page</desc></arg>
	</args>
</command>
*/
/*
** Set a printer's per-page time limit
*/
int command_pagetimelimit(const char *argv[])
	{
	const char *printer = argv[0];
	int limit;
	int ret;

	if((limit = atoi(argv[1])) < 0)
		{
		gu_utf8_fputs(_("The limit must be 0 (unlimited) or a positive integer.\n"), stderr);
		return EXIT_SYNTAX;
		}

	ret = conf_set_name(QUEUE_TYPE_PRINTER, printer, 0, "PageTimeLimit", (limit > 0) ? "%d" : NULL, limit);

	return ret;
	} /* command_pagetimelimit() */

/*
<command acl="ppad">
	<name><word>addon</word></name>
	<desc>set printer parameters for use by a PPR extension</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
		<arg><name>param</name><desc>addon parameter to modify</desc></arg>
		<arg flags="optional"><name>value</name><desc>new value for <arg>param</arg> (ommit to delete)</desc></arg>
	</args>
</command>
*/
int command_addon(const char *argv[])
	{
	const char *printer = argv[0];
	const char *name = argv[1];
	const char *value = argv[2];

	if(!(name[0] >= 'a' && name[0] <= 'z'))
		{
		gu_utf8_fputs(_("Addon parameter names must begin with a lower-case ASCII letter.\n"), stderr);
		return EXIT_SYNTAX;
		}

	return conf_set_name(QUEUE_TYPE_PRINTER, printer, 0, name, (value && value[0]) ? "%s" : NULL, value);
	} /* command_addon() */

/*
<command acl="ppad">
	<name><word>ppdq</word></name>
	<desc>send query to printer and produce list of suitable PPD files</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to be modified</desc></arg>
	</args>
</command>
*/
int command_ppdq(const char *argv[])
	{
	const char *printer = argv[0];
	struct QUERY *q = NULL;
	int ret = EXIT_OK;

	gu_Try
		{
		/* Create an object from the printer's configuration. */
		q = query_new_byprinter(printer);

		/* Now call the function in ppad_ppd.c that does the real work. */
		ret = ppd_query_core(printer, q);

		query_free(q);
		}
	gu_Catch
		{
		gu_utf8_fprintf(stderr, _("%s: query failed: %s\n"), myname, gu_exception);
		return EXIT_INTERNAL;
		}

	return EXIT_OK;
	} /* command_ppdq() */

/* end of file */
