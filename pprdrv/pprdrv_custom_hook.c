/*
** mouse:~ppr/src/pprdrv/pprdrv_custom_hook.c
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
** Last modified 15 December 2005.
*/

#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
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
		fatal(EXIT_PRNERR, _("%s(): %s() failed, errno=%d (%s)"), function, "pipe", errno, gu_strerror(errno));

	if((pid = fork()) == -1)
		fatal(EXIT_PRNERR, "%s(): %s() failed, errno=%d (%s)", function, "fork", errno, gu_strerror(errno));

	if(pid == 0)						/* child */
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

	else								/* parent */
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

/*
 * This function is called at each point at which a custom hook
 * could insert text into the job stream.  This function determines
 * if a custom hook is enabled and invokes it if one is.  If a custom
 * hook is actually invoked, this function returns TRUE.  If not
 * it returns FALSE and the default routine (if any) is invoked in
 * its stead.
 */
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
		fatal(EXIT_PRNERR, "CustomHook path not defined");
		}
	else if(access(printer.custom_hook.path, X_OK) == -1)
		{
		fatal(EXIT_PRNERR, "CustomHook program \"%s\" not executable", printer.custom_hook.path);
		}
	else
		{
		/* The banner and trailer hooks generate complete PostScript
		 * documents, so we must do the job start and end stuff. */
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

