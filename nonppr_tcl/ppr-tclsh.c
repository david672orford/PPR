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
** Last modified 20 January 2005.
*/

#include "config.h"
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include "gu.h"
#include "global_defines.h"
#include "version.h"
#include "tcl.h"

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
		gu_snprintf(interp->result, TCL_RESULT_SIZE, "wrong number of parameters (2 or more required, %d received)", argc);
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
		gu_snprintf(interp->result, TCL_RESULT_SIZE, "wrong number of parameters (3 required, %d received)", argc);
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

	/*
	 * Interactive mode startup file.  This is disabled for now.
	 */
	#if 0
	tcl_RcFileName = "~/.tclshrc";
	#endif
	
	return TCL_OK;
}
