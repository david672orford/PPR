/*
** mouse:~ppr/src/pprdrv/pprdrv_custom_hook.c
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
** Last modified 11 January 2001.
*/

#include "before_system.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"

#include "pprdrv.h"
#include "interface.h"

#ifdef DEBUG_CUSTOM_HOOK
#define DODEBUG_CUSTOM_HOOK(a) debug a
#else
#define DODEBUG_CUSTOM_HOOK(a)
#endif

static void custom_hook_run(const char path[], int code, int parameter)
    {
    const char function[] = "custom_hook_run";
    char code_str[4];
    char parameter_str[10];
    pid_t pid;
    int pipefds[2];

    DODEBUG_CUSTOM_HOOK(("%s(path=\"%s\", code=%d, parameter=%d", function, path, code, parameter));

    snprintf(code_str, sizeof(code_str), "%d", code);
    snprintf(parameter_str, sizeof(parameter_str), "%d", parameter);

    if(pipe(pipefds) == -1)
    	fatal(EXIT_PRNERR, "%s(): pipe() failed, errno=%d (%s)", function, errno, gu_strerror(errno));

    if((pid = fork()) == -1)
    	fatal(EXIT_PRNERR, "%s(): fork() failed, errno=%d (%s)", function, errno, gu_strerror(errno));

    if(pid == 0) 			/* child */
	{
	int fd;

	close(pipefds[0]);

        if((fd = open("/dev/null", O_RDWR)) != -1)
            {
            if(fd != 0)
            	{
            	dup2(fd, 0);
            	close(fd);
            	}
            }

	if(pipefds[1] != 1)
	    {
	    dup2(pipefds[1], 1);
	    close(pipefds[1]);
	    }

        if((fd = open(PPRDRV_LOGFILE, O_WRONLY | O_CREAT | O_APPEND, UNIX_644)) != -1)
            {
            if(fd != 2) dup2(fd, 2);
            if(fd > 2) close(fd);
            }

	execl(path, path, code_str, parameter_str, QueueFile, NULL);
	error("exec(\"%s\", ...) failed, errno=%d (%s)", path, errno, gu_strerror(errno));
	exit(242);
	}

    else				/* parent */
	{
	FILE *f;
	char *line = NULL;
	int line_len = 128;
	int matches;
	char *feature, *option;

	DODEBUG_CUSTOM_HOOK(("%s(): launched process %ld", function, (long)pid));

	if(close(pipefds[1]) == -1)
	    fatal(EXIT_PRNERR, "%s(): close() failed, errno=%d (%s)", function, errno, gu_strerror(errno));

	if(!(f = fdopen(pipefds[0], "r")))
	    fatal(EXIT_PRNERR, "%s(): fdopen() failed, errno=%d (%s)", function, errno, gu_strerror(errno));

	while((line = gu_getline(line, &line_len, f)))
	    {
	    if(line[0] == '%' && (matches = gu_sscanf(line, "%%%%IncludeFeature: %S %S", &feature, &option)) > 0)
	    	{
		if(matches == 2)
		    {
		    include_feature(feature, option);
		    gu_free(feature);
		    gu_free(option);
		    }
		else
		    {
		    include_feature(feature, NULL);
		    gu_free(feature);
		    }
		continue;
	    	}
	    printer_putline(line);
	    }

	if(fclose(f) == EOF)
	    fatal(EXIT_PRNERR, "%s(): fclose() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
	}

    DODEBUG_CUSTOM_HOOK(("%s(): done", function));
    } /* end of custom_hook_run() */

gu_boolean custom_hook(int code, int parameter)
    {
    const char function[] = "custom_hook";
    DODEBUG_CUSTOM_HOOK(("%s(code=%d, parameter=%d)", function, code, parameter));

    if((code & printer.custom_hook.flags) == 0)
    	{
	DODEBUG_CUSTOM_HOOK(("%s(): no hook for this event", function));
    	return FALSE;
    	}
    else if(!printer.custom_hook.path)
    	{
	error("%s(): CustomHook path not defined!", function);
	return FALSE;
    	}
    else
    	{
	if(code & (CUSTOM_HOOK_BANNER | CUSTOM_HOOK_TRAILER))
	    job_start(JOBTYPE_FLAG);

	custom_hook_run(printer.custom_hook.path, code, parameter);

	if(code & (CUSTOM_HOOK_BANNER | CUSTOM_HOOK_TRAILER))
	    job_end();

	DODEBUG_CUSTOM_HOOK(("%s(): hook action complete", function));
	return TRUE;
	}
    } /* end of custom_hook() */

/* end of file */

