/*
** mouse:~ppr/src/libppr/interfaces.c
** Copyright 1995--2001, Trinity College Computing Center
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 9 May 2001.
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
/*       Name           Jobbreak                Jobbreak PJL		Feedback	Codes			Codes TBCP	*/
	{"simple",  	JOBBREAK_CONTROL_D, 	JOBBREAK_PJL,		FALSE,		CODES_Clean8Bit,	CODES_TBCP},
	{"serial",	JOBBREAK_CONTROL_D,	JOBBREAK_PJL,		TRUE,		CODES_Clean8Bit,	CODES_TBCP},
	{"parallel",	JOBBREAK_CONTROL_D,	JOBBREAK_PJL, 		FALSE,		CODES_Clean8Bit,	CODES_TBCP},
	{"dummy",	JOBBREAK_NONE,		JOBBREAK_NONE,		FALSE,		CODES_Binary,		CODES_Binary},
/* PJL in PAP is not on by default because of problems with old HP printers. */
/*	{"atalk",	JOBBREAK_SIGNAL,	JOBBREAK_SIGNAL_PJL,	TRUE,		CODES_Binary,		CODES_Binary}, */
	{"atalk",	JOBBREAK_SIGNAL,	JOBBREAK_SIGNAL,	TRUE,		CODES_Binary,		CODES_Binary},
	{"tcpip",	JOBBREAK_CONTROL_D,	JOBBREAK_PJL,		TRUE,		CODES_Clean8Bit,	CODES_TBCP},
	{"lpr",		JOBBREAK_CONTROL_D,	JOBBREAK_PJL,		FALSE,		CODES_Clean8Bit,	CODES_TBCP},
	{"clispool",	JOBBREAK_CONTROL_D,	JOBBREAK_PJL,		FALSE,		CODES_Clean8Bit,	CODES_TBCP},
	{"smb",		JOBBREAK_CONTROL_D,	JOBBREAK_PJL,		FALSE,		CODES_Clean8Bit,	CODES_TBCP},
	{"gssimple",	JOBBREAK_NEWINTERFACE,	JOBBREAK_NEWINTERFACE,	TRUE,		CODES_Binary,		CODES_Binary},
	{"gsserial", 	JOBBREAK_NEWINTERFACE,	JOBBREAK_NEWINTERFACE,	TRUE,		CODES_Binary,		CODES_Binary},
	{"gsparallel",	JOBBREAK_NEWINTERFACE,	JOBBREAK_NEWINTERFACE,	TRUE,		CODES_Binary,		CODES_Binary},
	{"gsatalk",	JOBBREAK_NEWINTERFACE,	JOBBREAK_NEWINTERFACE,	TRUE,		CODES_Binary,		CODES_Binary},
	{"gstcpip",	JOBBREAK_NEWINTERFACE,	JOBBREAK_NEWINTERFACE,	TRUE,		CODES_Binary,		CODES_Binary},
	{"gslpr",	JOBBREAK_NEWINTERFACE,	JOBBREAK_NEWINTERFACE,	TRUE,		CODES_Binary,		CODES_Binary},
	{"gssmb",	JOBBREAK_NEWINTERFACE,	JOBBREAK_NEWINTERFACE,	TRUE,		CODES_Binary,		CODES_Binary},
	{(const char*)NULL,0,0,FALSE,CODES_UNKNOWN,CODES_UNKNOWN} };

static const struct INTERFACE_INFO *find_interface(const char name[])
    {
    int index;

    for(index = 0; interfaces[index].name; index++)
	{
	if(strcmp(name, interfaces[index].name) == 0)
	    {
	    return &interfaces[index];
	    break;
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
