/*
** mouse:~ppr/src/pprdrv/pprdrv_reason.c
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
** Last modified 6 February 2003.
*/

/*===========================================================================
** The primary work of this module is to append a "Reason:" line to the
** job's queue file when it is arrested.  In some places the code calls
** give_reason() directly.	However pprdrv_feedback.c calls 
** describe_postscript_error() which calls give_reason() itself.
===========================================================================*/

#include "before_system.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "pprdrv.h"
#include "interface.h"

/*
** Append a "Reason:" line with the provided content to the
** job's queue file.  This will tell why the job was arrested.
**
** This routine is called by described_postscript_error(), below,
** but it is also called from pprdrv_capable.c.
*/
void give_reason(const char reason[])
	{
	const char *function = "give_reason";
	char fname[MAX_PPR_PATH];
	FILE *f;

	ppr_fnamef(fname, "%s/%s", QUEUEDIR, QueueFile);
	if(!(f = fopen(fname, "a")))
		fatal(EXIT_PRNERR_NORETRY, "%s(): can't open queue file, errno=%d (%s)", function, errno, gu_strerror(errno) );

	fprintf(f, "Reason: %s\n", reason);

	fclose(f);
	} /* end of give_reason() */

/*
** This routine is passed information about a PostScript error.	 It looks up 
** the error in a list in a file and hopefully finds a description.	 In 
** addition to the name of the failed PostScript command and the PostScript 
** error message it receives the string from the "%%Creator:" line in the 
** document.  Since this often indicates the program or printer driver that
** generated the document, it can help us to explain the error in context.
**
** Information about the error is sent to the job log and to the queue file.
**
** This function is called from pprdrv_feedback.c _before_ the PostScript 
** error is logged.
*/
void describe_postscript_error(const char creator[], const char errormsg[], const char command[])
	{
	const char function[] = "describe_postscript_error";
	const char filename[] = PSERRORS_CONF;
	FILE *f;
	gu_boolean found = FALSE;

	#if 0
	debug("describe_postscript_error(creator=\"%s\",errormsg=\"%s\",command=\"%s\")",
		creator ? creator : "",
		errormsg ? errormsg : "",
		command ? command : "");
	#endif

	log_puts("==============================================================================\n");
	log_printf(_("The PostScript error indicated below occured while printing on \"%s\".\n"), printer.Name);

	/* If we have someting to go on... */
	if(errormsg && command)
		{
		/* Look in. */
		if((f = fopen(filename, "r")))
			{
			const char *safe_creator = creator ? creator : "";
			char *line = NULL;
			int line_space = 80;
			int linenum = 0;
			char *p, *f1, *f2, *f3, *f4;

			while((line = gu_getline(line, &line_space, f)))
				{
				linenum++;
				if(line[0] == ';' || line[0] == '#' || line[0] == '\0') continue;

				/* Parse the line into three colon separated fields. */
				p = line;
				if(!(f1 = gu_strsep(&p, ":")) || !(f2 = gu_strsep(&p, ":"))
							|| !(f3 = gu_strsep(&p, ":")) || !(f4 = gu_strsep(&p, ":")))
					{
					error("%s(): syntax error in \"%s\" line %d", function, filename, linenum);
					continue;
					}

				if(ppr_wildmat(safe_creator, f1) && ppr_wildmat(errormsg, f2) && ppr_wildmat(command, f3))
					{
					int i;

					log_puts(_("Probable cause: "));
					for(i=0; f4[i]; i++)
						{
						switch(f4[i])
							{
							case ',':
								log_puts(", ");
								break;
							case '|':
								log_putc(' ');
								break;
							default:
								log_putc(f4[i]);
								break;
							}
						}
					log_putc('\n');

					give_reason(f4);

					gu_free(line);
					found = TRUE;
					break;
					}
				}

			fclose(f);
			}
		}

	log_puts("==============================================================================\n");

	/* If nothing was found above, use the generic message. */
	if(!found)
		give_reason("PostScript error");

	} /* end of describe_postscript_error() */

/* end of file */
