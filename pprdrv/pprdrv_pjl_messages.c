/*
** mouse:~ppr/src/pprdrv/pprdrv_pjl_messages.c
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

#include "before_system.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "pprdrv.h"

/*
** This function decodes PJL printer status codes.  It is passed the code
** and the message from the printer's control panel.  The latter is used
** for debugging purposes.	(Since it is internationalized by the printer,
** it is not machine readable.)
*/
int translate_pjl_message(int code, const char display[], int *value1, int *value2, int *value3, const char **details)
	{
	FUNCTION4DEBUG("translate_pjl_message")
	static char static_details[40];
	char code_str[6];

	const char filename[] = PJL_MESSAGES_CONF;
	FILE *file;
	char *line = NULL; int line_len = 80; int linenum = 0;
	char *p, *f1, *f2, *f3, *f4, *f5;

	DODEBUG_LW_MESSAGES(("%s(code=%d, display[]=\"%s\", ...)", function, code, display));

	*value1 = *value2 = *value3 = -1;
	*details = static_details;
	static_details[0] = '\0';

	snprintf(code_str, sizeof(code_str), "%d", code);

	if(!(file = fopen(filename, "r")))
		{
		error(_("Can't open \"%s\", errno=%d (%s)"), filename, errno, gu_strerror(errno));
		return -1;
		}

	while((line = gu_getline(line, &line_len, file)))
		{
		linenum++;

		/* Skip blank and comment lines. */
		if(*line == '\0' || *line == '#' || *line == ';')
		   continue;

		if(strlen(line) == 0)
		   continue;

		p = line;
		if(!(f1 = gu_strsep(&p, ":"))							/* code */
				|| !(f2 = gu_strsep(&p, ":"))					/* value1 */
				|| !(f3 = gu_strsep(&p, ":"))					/* value2 */
				|| !(f4 = gu_strsep(&p, ":"))					/* value3 */
				|| !(f5 = gu_strsep_quoted(&p, ":", NULL)))		/* details */
			{
			error(_("Not enough fields in \"%s\" line %d"), filename, linenum);
			continue;
			}

		/* DODEBUG_LW_MESSAGES(("%s(): f1=\"%s\"", function, f1)); */

		if(gu_wildmat(code_str, f1))
			{
			DODEBUG_LW_MESSAGES(("%s(): match on %s!", function, f1));

			if(*f2 && (*value1 = snmp_DeviceStatus(f2)) == -1)
				error(_("Unrecognized hrDeviceStatus \"%s\" in \"%s\" line %d field 2"), f2, filename, linenum);
			if(*f3 && (*value2 = snmp_PrinterStatus(f3)) == -1)
				error(_("Unrecognized hrPrinterStatus \"%s\" in \"%s\" line %d field 3"), f3, filename, linenum);
			if(*f4 && (*value3 = snmp_PrinterDetectedErrorState(f4)) == -1)
				error(_("Unrecognized hrPrinterDetectedErrorState \"%s\" in \"%s\" line %d field 4"), f4, filename, linenum);

			if(*f5)
				gu_snprintfcat(static_details, sizeof(static_details), "%s", f5);

			/* Stop searching unless the details string has a space at the end. */
			if(strlen(static_details) > 0 && static_details[strlen(static_details) - 1] != ' ')
				break;
			}
		}

	fclose(file);

	if(line)
		gu_free(line);
	else
		error(_("%s: no match for %d \"%s\""), filename, code, display);

	DODEBUG_LW_MESSAGES(("%s(): %d \"%s\" --> %d %d %d \"%s\"", function, code, display, *value1, *value2, *value3, static_details));

	return 0;
	}

/* end of file */
