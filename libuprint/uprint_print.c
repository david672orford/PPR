/*
** mouse:~ppr/src/libuprint/uprint_print.c
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
** Last modified 20 February 2003.
*/

#include "config.h"
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "uprint.h"
#include "uprint_private.h"

#define MAX_ARGV 100

/*
** Function to make a log entry for debugging purposes:
*/
static void logit(const char *command_path, const char **argv)
	{
	#ifdef UPRINT_LOGFILE
	FILE *f;

	if((f = fopen(UPRINT_LOGFILE, "a")) != (FILE*)NULL)
		{
		int x;
		fprintf(f, "--> %s", command_path);
		for(x=1; argv[x]; x++)
			{
			if(strlen(argv[x]) == 0 || strchr(argv[x], ' '))
				fprintf(f, " \"%s\"", argv[x]);
			else
				fprintf(f, " %s", argv[x]);
			}
		fputc('\n', f);
		fclose(f);
		}
	#endif
	}

/*
** Send the job to the correct spooling system.
*/
int uprint_print(void *p, gu_boolean remote_too)
	{
	const char function[] = "uprint_print";
	struct UPRINT *upr = (struct UPRINT *)p;
	const char *argv[MAX_ARGV];
	int i = 0;
	const char *command_path;
	gu_boolean one_at_a_time = FALSE;			/* general case is FALSE */
	int ret;
	struct REMOTEDEST info;

	DODEBUG(("%s(p=%p)", function, p));

	if(upr->dest == (const char *)NULL)
		{
		uprint_errno = UPE_NODEST;
		uprint_error_callback("%s(): dest not set", function);
		return -1;
		}

	if(upr->user == (const char *)NULL)
		{
		uprint_errno = UPE_BADORDER;
		uprint_error_callback("%s(): user not set", function);
		return -1;
		}

	/* PPR spooling system: */
	if(printdest_claim_ppr(upr->dest))
		{
		one_at_a_time = TRUE;
		command_path = PPR_PATH;
		if((i = uprint_print_argv_ppr(p, argv, MAX_ARGV)) == -1)
			return -1;
		}

	/* System V spooler: */
	else if(printdest_claim_sysv(upr->dest))
		{
		command_path = uprint_path_lp();
		if(strcmp(upr->fakername, "lp") == 0)
			{
			upr->argv[0] = "lp";
			}
		else
			{
			if((i = uprint_print_argv_sysv(p, argv, MAX_ARGV)) == -1)
				return -1;
			}
		}

	/* BSD lpr spooler: */
	else if(printdest_claim_bsd(upr->dest))
		{
		command_path = uprint_path_lpr();
		if(strcmp(upr->fakername, "lpr") == 0)
			{
			upr->argv[0] = "lpr";
			}
		else
			{
			if((i = uprint_print_argv_bsd(p, argv, MAX_ARGV)) == -1)
				return -1;
			}
		}

	/* Remote network spooler: */
	else if(remote_too && printdest_claim_remote(uprint_get_dest(upr), &info))
		{
		command_path = (char*)NULL;
		}

	/* Destination unknown. */
	else
		{
		uprint_errno = UPE_UNDEST;
		return -1;
		}

	/* If to remote queue, */
	if(command_path == (char*)NULL)
		{
		FILE *f;
		if((f = fopen(UPRINT_LOGFILE, "a")) != (FILE*)NULL)
			{
			fprintf(f, "to remote %s@%s\n", info.printer, info.node);
			fclose(f);
			}
		if(uprint_print_rfc1179(upr, &info) == -1)
			ret = 1;
		else
			ret = 0;
		gu_free(info.node);
		}

	/*
	** If i is still 0 then we run the command for which
	** we are the fake and pass it the arguments we got.
	*/
	else if(i == 0)
		{
		if(upr->argc > 0)
			logit(command_path, upr->argv);
		ret = uprint_run(upr->uid, upr->gid, command_path, upr->argv);
		}

	/*
	** Append the file names and then run the
	** command.
	*/

	/* If no files specified, just run the command
	   and let it read stdin: */
	else if(upr->files == (const char **)NULL)
		{
		if(command_path == PPR_PATH)
			{
			argv[i++] = "--lpq-filename";
			argv[i++] = "stdin";
			}
		argv[i] = (const char *)NULL;
		if(upr->argc > 0)
			logit(command_path, argv);
		ret = uprint_run(upr->uid, upr->gid, command_path, argv);
		}

	/* If files specified and the command must
	   be run once for each one: */
	else if(one_at_a_time)
		{
		int x;
		ret = 0;
		for(x=0; upr->files[x]; x++)
			{
			if(command_path == PPR_PATH)
				{
				argv[i++] = "--lpq-filename";
				argv[i++] = strcmp(upr->files[x], "-") ? upr->files[x] : "stdin";
				}

			argv[i] = upr->files[x];
			argv[i+1] = (const char *)NULL;

			if(upr->argc > 0)
				logit(command_path, argv);

			if((ret = uprint_run(upr->uid, upr->gid, command_path, argv)) != 0)
				break;
			}
		}

	/* If they can all go on the command line: */
	else
		{
		int x;
		for(x=0; upr->files[x] && i < (MAX_ARGV - 1); x++, i++)
			argv[i] = upr->files[x];
		argv[i] = (const char*)NULL;

		if(upr->files[x] != (const char*)NULL)
			{
			uprint_errno = UPE_TOOMANY;
			uprint_error_callback("%s(): argv[] overflow", function);
			ret = -1;
			}

		if(upr->argc > 0)
			logit(command_path, argv);

		ret = uprint_run(upr->uid, upr->gid, command_path, argv);
		}

	#ifdef UPRINT_LOGFILE
	if(upr->argc > 0)
		{
		FILE *f;
		if((f = fopen(UPRINT_LOGFILE, "a")) != (FILE*)NULL)
			{
			if(ret == -1)
				fprintf(f, "result=%d, uprint_errno=%d (%s)\n\n", ret, uprint_errno, uprint_strerror(uprint_errno));
			else
				fprintf(f, "result=%d\n\n", ret);
			fclose(f);
			}
		}
	#endif

	/* uprint-lpr and uprint-lp will call exit() with this value. */
	return ret;
	} /* end of uprint_print() */

/* end of file */
