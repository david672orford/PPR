/*
** mouse:~ppr/src/pprdrv/pprdrv_lw_messages.c
** Copyright 1995--2004, Trinity College Computing Center.
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
** Last modified 22 May 2004.
*/

#include "config.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "pprdrv.h"

/*
** These routines look up LaserWriter-style error and status messages up in a
** file and returns SNMP status codes and a details string if it finds a
** match.
*/
int translate_lw_message(const char raw_message[], int *value1, int *value2, int *value3, const char **details)
	{
	FUNCTION4DEBUG("translate_lw_message")
	const char filename[] = LW_MESSAGES_CONF;
	FILE *f; char *line = NULL; int line_space = 80; int linenum = 0;
	static char static_details[64] = {'\0'};
	char *p, *f1, *f2, *f3, *f4, *f5;

	DODEBUG_LW_MESSAGES(("%s(raw_message=\"%s\", severity=?)", function, raw_message));

	*value1 = *value2 = *value3 = -1;
	*details = "";

	/* Open the message translation file: */
	if((f = fopen(filename, "r")) == (FILE*)NULL)
		{
		error(_("Can't open \"%s\", errno=%d (%s)"), filename, errno, gu_strerror(errno) );
		return -1;
		}

	/* This loop reads all the lines in the file. */
	while((line = gu_getline(line, &line_space, f)))
		{
		linenum++;

		/* Skip blank and comment lines. */
		if(*line == '\0' || *line == ';' || *line == '#')
			continue;

		p = line;
		if(!(f1 = gu_strsep_quoted(&p, ":", " \t"))
				|| !(f2 = gu_strsep(&p, ":"))
				|| !(f3 = gu_strsep(&p, ":"))
				|| !(f4 = gu_strsep(&p, ":"))
				|| !(f5 = gu_strsep_quoted(&p, ":", NULL)))
			{
			error(_("Not enough fields in \"%s\" line %d"), filename, linenum);
			continue;
			}

		if(gu_wildmat(raw_message, f1))
			{
			DODEBUG_LW_MESSAGES(("%s(): raw message string matched to \"%s\"", function, f1));

			if(*f2 && (*value1 = snmp_DeviceStatus(f2)) == -1)
				error(_("Unrecognized hrDeviceStatus \"%s\" in \"%s\" line %d field 2"), f2, filename, linenum);
			if(*f3 && (*value2 = snmp_PrinterStatus(f3)) == -1)
				error(_("Unrecognized hrPrinterStatus \"%s\" in \"%s\" line %d field 3"), f3, filename, linenum);
			if(*f4 && (*value3 = snmp_PrinterDetectedErrorState(f4)) == -1)
				error(_("Unrecognized hrPrinterDetectedErrorState \"%s\" in \"%s\" line %d field 4"), f4, filename, linenum);

			snprintf(static_details, sizeof(static_details), "%s", f5);
			*details = static_details;
			break;
			}

		} /* end of line loop */

	fclose(f);

	if(line)		/* if found before end of file, */
		{
		gu_free(line);
		DODEBUG_LW_MESSAGES(("%s(): %d %d %d \"%s\"", function, *value1, *value2, *value3, *details));
		return 0;
		}

	error(_("%s: no match for \"%s\""), filename, raw_message);
	return -1;
	} /* end of translate_lw_message() */

/* end of file */

