/*
** mouse:~ppr/src/nonppr_tcl/ppr-tclsh.c
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
** Last modified 29 March 2005.
*/

#include "config.h"
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "gu.h"
#include "global_defines.h"
#include "version.h"
#include "tcl.h"
#include "tclInt.h"

const char myname[] = "ppr-tclsh";

int main(int argc, char **argv)
	{
	Tcl_Main(argc, argv, Tcl_AppInit);
	return 0;
	}

static int ppr_version(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
	{
	gu_strlcpy(interp->result, SHORT_VERSION, TCL_RESULT_SIZE);
	return TCL_OK;
	}

static int ppr_conf_query(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
	{
	struct GU_INI_ENTRY *section = NULL;
	char *section_name;
	char *key_name = NULL;
	int value_index = 0;
	char *default_value = "";
	const struct GU_INI_ENTRY *value;

	if(argc < 3)
		{
		gu_snprintf(interp->result, TCL_RESULT_SIZE, "wrong number of parameters (2 or more required, %d received)", argc-1);
		return TCL_ERROR;
		}

	section_name = argv[1];
	key_name = argv[2];
	if(argc >= 4)
		value_index = atoi(argv[3]);
	if(argc >= 5)
		default_value = argv[4];

	do	{
		FILE *cf;

		if(!(cf = fopen(PPR_CONF, "r")))
			{
			gu_snprintf(interp->result, TCL_RESULT_SIZE, "can't open \"%s\", errno=%d (%s)", PPR_CONF, errno, gu_strerror(errno));
			return TCL_ERROR;
			}

		if(!(section = gu_ini_section_load(cf, section_name)))
			{
			fprintf(stderr, "%s: warning: there is no section named [%s]\n", myname, section_name);
			}

		fclose(cf);
		} while(0);

	if(!(value = gu_ini_section_get_value(section, key_name)))
		fprintf(stderr, "%s: warning: there is no item named \"%s\" in section [%s]\n", myname, key_name, section_name);
	gu_strlcpy(interp->result, gu_ini_value_index(value, value_index, default_value), TCL_RESULT_SIZE);

	return TCL_OK;
	}

static int ppr_alert(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
	{
	gu_boolean stamp;

	if(argc != 4)
		{
		gu_snprintf(interp->result, TCL_RESULT_SIZE, "wrong number of parameters (3 required, %d received)", argc-1);
		return TCL_ERROR;
		}

	if(gu_torf_setBOOL(&stamp, argv[2]) == -1)
		{
		gu_snprintf(interp->result, TCL_RESULT_SIZE, "second parameter must be TRUE or FALSE");
		return TCL_ERROR;
		}

	/* Call the library alert function to post the alert. */
	alert(argv[1], stamp, "%s", argv[3]);

	interp->result[0] = '\0';
	return TCL_OK;
	}

static int ppr_wordwrap(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
	{
	int width;
	int temp_len;
	char *temp;

	if(argc != 3)
		{
		gu_snprintf(interp->result, TCL_RESULT_SIZE, "wrong number of parameters (2 required, %d received)", argc-1);
		return TCL_ERROR;
		}

	if((width = atoi(argv[2])) < 0)
		{
		gu_snprintf(interp->result, TCL_RESULT_SIZE, "width is less than zero");
		return TCL_ERROR;
		}

	temp_len = strlen(argv[1]) + 1;
	temp = ckalloc(temp_len);
	gu_strlcpy(temp, argv[1], temp_len);

	gu_wordwrap(temp, width);

	Tcl_SetResult(interp, temp, TCL_DYNAMIC);
	return TCL_OK;
	}

static int ppr_popen_w(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
	{
	int fds[2];
	pid_t pid;
	char **exec_argv;
	int exec_argc;

	if(argc != 2)
		{
		gu_snprintf(interp->result, TCL_RESULT_SIZE, "wrong number of parameters (1 required, %d received)", argc-1);
		return TCL_ERROR;
		}
	if(Tcl_SplitList(interp, argv[1], &exec_argc, &exec_argv) != TCL_OK)
		{
		return TCL_ERROR;
		}
	if(pipe(fds) == -1)
		{
		gu_snprintf(interp->result, TCL_RESULT_SIZE, "pipe() failed, errno=%d (%s)", errno, gu_strerror(errno));
		return TCL_ERROR;
		}
	if((pid = fork()) == -1)
		{
		gu_snprintf(interp->result, TCL_RESULT_SIZE, "fork() failed, errno=%d (%s)", errno, gu_strerror(errno));
		return TCL_ERROR;
		}
	if(pid == 0)		/* if child, */
		{
		close(fds[1]);	/* write end */
		if(fds[0] != 0)
			{
			dup2(fds[0], 0);
			close(fds[0]);
			}
		execvp(exec_argv[0], exec_argv);
		fprintf(stderr, "execvp(\"%s\", ...) failed, errno=%d (%s)\n", exec_argv[0], errno, gu_strerror(errno));
		exit(242);
		}
	else				/* if not child, */
		{
		FILE *f;
		OpenFile *oFilePtr;

		ckfree(exec_argv);
		close(fds[0]);	/* read end */
		f = fdopen(fds[1], "w");
		Tcl_EnterFile(interp, f, TCL_FILE_WRITABLE);
		oFilePtr = tclOpenFiles[fileno(f)];	
		oFilePtr->numPids = 1;
		oFilePtr->pidPtr = ckalloc(sizeof(pid_t));
		memcpy(oFilePtr->pidPtr, &pid, sizeof(pid_t));
		}

	return TCL_OK;
	}

int Tcl_AppInit(Tcl_Interp *interp)
	{
	/*
	 * Set up the Tcl library facility.  This is disabled for now.
	 */
	#if 0
	if (Tcl_Init(interp) == TCL_ERROR) {
		return TCL_ERROR;
		}
	#endif

	/*
	 * Add PPR commands
	 */
	Tcl_CreateCommand(interp, "ppr_version", ppr_version, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp, "ppr_conf_query", ppr_conf_query, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp, "ppr_alert", ppr_alert, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp, "ppr_wordwrap", ppr_wordwrap, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp, "ppr_popen_w", ppr_popen_w, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

	/*
	 * Interactive mode startup file.  This is disabled for now.
	 */
	#if 0
	tcl_RcFileName = "~/.tclshrc";
	#endif
	
	return TCL_OK;
}
