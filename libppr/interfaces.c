/*
** mouse:~ppr/src/libppr/interfaces.c
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
** Last modified 14 May 2003.
*/

#include "before_system.h"
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "interface.h"

/*
** This is a list of the supplied interfaces and their characteristics.
** This allows jobbreak and feedback to be set automatically for
** the listed interface programs.
*/

struct INTERFACE_INFO {
		const char *name;
		char jobbreak;
		char pjl_jobbreak;
		gu_boolean feedback;
		enum CODES codes;
		enum CODES tbcp_codes;
		};

struct INTERFACE_INFO interfaces[] = {
/*		 Name			Jobbreak				Jobbreak PJL			Feedback		Codes					Codes TBCP		*/
		{"simple",		JOBBREAK_CONTROL_D,		JOBBREAK_PJL,			FALSE,			CODES_Clean8Bit,		CODES_TBCP},
		{"serial",		JOBBREAK_CONTROL_D,		JOBBREAK_PJL,			TRUE,			CODES_Clean8Bit,		CODES_TBCP},
		{"parallel",	JOBBREAK_CONTROL_D,		JOBBREAK_PJL,			FALSE,			CODES_Clean8Bit,		CODES_TBCP},
		{"usblp",		JOBBREAK_CONTROL_D,		JOBBREAK_PJL,			FALSE,			CODES_Clean8Bit,		CODES_TBCP},
		{"dummy",		JOBBREAK_NONE,			JOBBREAK_NONE,			FALSE,			CODES_Binary,			CODES_Binary},

		{"tcpip",		JOBBREAK_CONTROL_D,		JOBBREAK_PJL,			TRUE,			CODES_Clean8Bit,		CODES_TBCP},
		{"socketapi",	JOBBREAK_CONTROL_D,		JOBBREAK_PJL,			TRUE,			CODES_Clean8Bit,		CODES_TBCP},
		{"appsocket",	JOBBREAK_NEWINTERFACE,	JOBBREAK_NEWINTERFACE,	TRUE,			CODES_Clean8Bit,		CODES_TBCP},
		{"jetdirect",	JOBBREAK_CONTROL_D,		JOBBREAK_PJL,			TRUE,			CODES_Clean8Bit,		CODES_TBCP},

/* PJL in PAP is not on by default because of problems with the HP 4M. */
/*		{"atalk",		JOBBREAK_SIGNAL,		JOBBREAK_SIGNAL_PJL,	TRUE,			CODES_Binary,			CODES_Binary}, */
		{"atalk",		JOBBREAK_SIGNAL,		JOBBREAK_SIGNAL,		TRUE,			CODES_Binary,			CODES_Binary},
		{"lpr",			JOBBREAK_CONTROL_D,		JOBBREAK_PJL,			FALSE,			CODES_Clean8Bit,		CODES_TBCP},
		{"clispool",	JOBBREAK_CONTROL_D,		JOBBREAK_PJL,			FALSE,			CODES_Clean8Bit,		CODES_TBCP},
		{"smb",			JOBBREAK_CONTROL_D,		JOBBREAK_PJL,			FALSE,			CODES_Clean8Bit,		CODES_TBCP},
		{"pros",		JOBBREAK_CONTROL_D,		JOBBREAK_PJL,			FALSE,			CODES_Clean8Bit,		CODES_TBCP},

		{"gssimple",	JOBBREAK_NEWINTERFACE,	JOBBREAK_NEWINTERFACE,	TRUE,			CODES_Binary,			CODES_Binary},
		{"gsserial",	JOBBREAK_NEWINTERFACE,	JOBBREAK_NEWINTERFACE,	TRUE,			CODES_Binary,			CODES_Binary},
		{"gsparallel",	JOBBREAK_NEWINTERFACE,	JOBBREAK_NEWINTERFACE,	TRUE,			CODES_Binary,			CODES_Binary},
		{"gsatalk",		JOBBREAK_NEWINTERFACE,	JOBBREAK_NEWINTERFACE,	TRUE,			CODES_Binary,			CODES_Binary},
		{"gstcpip",		JOBBREAK_NEWINTERFACE,	JOBBREAK_NEWINTERFACE,	TRUE,			CODES_Binary,			CODES_Binary},
		{"gslpr",		JOBBREAK_NEWINTERFACE,	JOBBREAK_NEWINTERFACE,	TRUE,			CODES_Binary,			CODES_Binary},
		{"gssmb",		JOBBREAK_NEWINTERFACE,	JOBBREAK_NEWINTERFACE,	TRUE,			CODES_Binary,			CODES_Binary},

		{(const char*)NULL,0,0,FALSE,CODES_UNKNOWN,CODES_UNKNOWN} };

static const struct INTERFACE_INFO *find_interface(const char name[])
	{
	int index;

	if(name)
		{
		for(index = 0; interfaces[index].name; index++)
			{
			if(strcmp(name, interfaces[index].name) == 0)
				{
				return &interfaces[index];
				break;
				}
			}
		}

	return NULL;
	}

gu_boolean interface_default_feedback(const char interface[], const struct PPD_PROTOCOLS *prot)
	{
	const struct INTERFACE_INFO *i;

	if((i = find_interface(interface)))
		return i->feedback;
	else
		return FALSE;
	}

int interface_default_jobbreak(const char interface[], const struct PPD_PROTOCOLS *prot)
	{
	const struct INTERFACE_INFO *i;

	if((i = find_interface(interface)))
		return prot->PJL ? i->pjl_jobbreak : i->jobbreak;
	else
		return JOBBREAK_CONTROL_D;
	}

enum CODES interface_default_codes(const char interface[], const struct PPD_PROTOCOLS *prot)
	{
	const struct INTERFACE_INFO *i;

	if((i = find_interface(interface)))
		return prot->TBCP ? i->tbcp_codes : i->codes;
	else
		return CODES_UNKNOWN;
	}

/* end of file */
