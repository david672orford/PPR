/*
** mouse:~ppr/src/pprdrv/pprdrv_reason.c
** Copyright 1995--2001, Trinity College Computing Center
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 13 November 2001.
*/

/*
** This module appends "Reason: " lines to the queue file in order
** to indicate the reason why a job was arrested.
*/

#include "before_system.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "gu.h"
#include "global_defines.h"
#include "pprdrv.h"
#include "interface.h"

/*
** Append a reason line with the provided content to the
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

    fprintf(f, "Reason: (%s)\n", reason);

    fclose(f);
    } /* end of give_reason() */

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

    if(errormsg && command)
	{
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

                p = line;
                if(!(f1 = gu_strsep(&p, ":")) || !(f2 = gu_strsep(&p, ":"))
                            || !(f3 = gu_strsep(&p, ":")) || !(f4 = gu_strsep(&p, ":")))
                    {
                    error("%s(): syntax error in \"%s\" line %d", function, filename, linenum);
                    continue;
                    }

                if(ppr_wildmat(safe_creator, f1) && ppr_wildmat(errormsg, f2) && ppr_wildmat(command, f3))
                    {
                    give_reason(f4);
                    gu_free(line);
                    found = TRUE;
                    break;
                    }
                }

            fclose(f);
            }
	}


    /* If nothing was found above, use the generic message. */
    if(!found)
	give_reason("PostScript error");

    } /* end of describe_postscript_error() */

/* end of file */
