/*
** mouse:~ppr/src/filters_misc/make_dvips_conf.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 20 February 2001.
*/

/*
** This program is called from the DVI filter (dvi.sh).  Its purpose is to take the MetaFont
** mode, resolution, and available memory specified on the command line and generate a
** DVIPS configuration file.  This program is setuid ppr and creates the file in a PPR
** directory set asside for that purpose.
*/

#include "config.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

const char myname[] = "make_dvips_conf";

int main(int argc, char *argv[])
	{
	char fname[MAX_PPR_PATH];
	FILE *f;

	char *PageSize;								/* "Letter" for US Letter Size */
	double phys_pu_width;						/* 612 for letter */
	double phys_pu_height;						/* 792 for letter */
	double MediaWeight;							/* probably 75 */
	const char *MediaColor;						/* probably "white" */
	const char *MediaType;						/* probably "" */

	if(argc != 4)
		{
		fprintf(stderr, "%s: wrong number of parameters\n", myname);
		fprintf(stderr, "Usage:\n\tmake_dvips_conf <mfmode> <resolution> <bytes_free>\n");
		return 1;
		}

	if(strchr(argv[1], '/') || strchr(argv[2], '/') || strchr(argv[3], '/'))
		{
		fprintf(stderr, "%s: slash in parameter!\n", myname);
		return 1;
		}

	{
	const char file[] = PPR_CONF;
	const char section[] = "internationalization";
	const char key[] = "defaultmedium";
	const char *error_message;

	error_message = gu_ini_scan_list(file, section, key,
		GU_INI_TYPE_NONEMPTY_STRING, &PageSize,
		GU_INI_TYPE_POSITIVE_DOUBLE, &phys_pu_width,
		GU_INI_TYPE_POSITIVE_DOUBLE, &phys_pu_height,
		GU_INI_TYPE_NONNEG_DOUBLE, &MediaWeight,
		GU_INI_TYPE_STRING, &MediaColor,
		GU_INI_TYPE_STRING, &MediaType,
		GU_INI_TYPE_END);

	if(error_message)
		{
		fprintf(stderr, _("%s: %s\n"
						"\twhile attempting to read \"%s\"\n"
						"\t\t[%s]\n"
						"\t\t%s =\n"),
				myname, gettext(error_message), file, section, key);
		return 1;
		}
	}

	ppr_fnamef(fname, "%s/dvips/config.%s-%s-%s", VAR_SPOOL_PPR, argv[1], argv[2], argv[3]);

	if(!(f = fopen(fname, "w")))
		{
		fprintf(stderr, "%s: can't create %s, errno=%d (%s)\n", argv[0], fname, errno, gu_strerror(errno));
		return 1;
		}

	{
	char datebuffer[128];
	time_t seconds_now;
	time(&seconds_now);
	strftime(datebuffer, sizeof(datebuffer), "%x %X", localtime(&seconds_now));
	fprintf(f, "%%\n"
				"%% This is an automatically generated DVIPS config file.\n"
				"%% It was generated at %s.\n"
				"%%\n"
				"\n",
				datebuffer);
	}

	fprintf(f,
				"%% MetaFont mode\n"
				"M %s\n"
				"\n"
				"%% Selected resolution\n"
				"D %s\n"
				"\n"
				"%% Memory available\n"
				"m %s\n"
				"\n"
				"%% Printer offset\n"
				"O 0pt,0pt\n"
				"\n"
				"%% Don't compress the bitmaps\n"
				"%% Z0\n"
				"\n"
				"%% Don't remove comments in included files\n"
				"K0\n"
				"\n"
				"@\n"
				"\n",
				argv[1], argv[2], argv[3]);
				
		fprintf(f,
				"@ %s %.1fbp %.1fbp\n"
				"@+ %%%%IncludeFeature: *PageSize %s\n"
				"\n",
				PageSize, phys_pu_width, phys_pu_height, PageSize);
				

		fprintf(f,
				"@ letter 8.5in 11in\n"
				"@+ %%%%IncludeFeature: *PageSize Letter\n"
				"\n"
				"@ a4 210mm 297mm\n"
				"@+ %%%%IncludeFeature: *PageSize A4\n"
				"\n"
				"@ a3 297mm 420mm\n"
				"@+ %%%%IncludeFeature: *PageSize A3\n"
				"\n"
				"@ legal 8.5in 14in\n"
				"@+ %%%%IncludeFeature: *PageSize Legal\n"
				"\n"
				"@ ledger 17in 11in\n"
				"@+ %%%%IncludeFeature: *PageSize Ledger\n"
				"\n"
				"@ tabloid 11in 17in\n"
				"@+ %%%%IncludeFeature: *PageSize Tabloid\n");

	chmod(fname, 0664);

	return 0;
	}

/* end of file */
