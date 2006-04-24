/*
** mouse:~ppr/src/ipp/ipp_str_to.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 24 April 2006.
*/

/*! \file */

#include "config.h"
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "gu.h"
#include "global_defines.h"
#include "ipp_constants.h"

struct TABLE {
	const char *name;
	int code;
	};

struct TABLE operations[] = 
	{
	{"Print-Job", IPP_PRINT_JOB},
	{"Print-URI", IPP_PRINT_URI},
	{"Validate-Job", IPP_VALIDATE_JOB},
	{"Create-Job", IPP_CREATE_JOB},
	{"Send-Document", IPP_SEND_DOCUMENT},
	{"Send-URI", IPP_SEND_URI},
	{"Cancel-Job", IPP_CANCEL_JOB},
	{"Get-Job-Attributes", IPP_GET_JOB_ATTRIBUTES},
	{"Get-Jobs", IPP_GET_JOBS},
	{"Get-Printer-Attributes", IPP_GET_PRINTER_ATTRIBUTES},
	{"Hold-Job", IPP_HOLD_JOB},
	{"Release-Job", IPP_RELEASE_JOB},
	{"Restart-Job", IPP_RESTART_JOB},
	{"Pause-Printer", IPP_PAUSE_PRINTER},
	{"Resume-Printer", IPP_RESUME_PRINTER},
	{"Purge-Jobs", IPP_PURGE_JOBS},
	{"Set-Printer-Attributes", IPP_SET_PRINTER_ATTRIBUTES},
	{"Set-Job-Attributes", IPP_SET_JOB_ATTRIBUTES},
	{"Get-Printer-Supported-Values", IPP_GET_PRINTER_SUPPORTED_VALUES},
	{"CUPS-Get-Default", CUPS_GET_DEFAULT},
	{"CUPS-Get-Printers", CUPS_GET_PRINTERS},
	{"CUPS-Add-Printer", CUPS_ADD_PRINTER},
	{"CUPS-Delete-Printer", CUPS_DELETE_PRINTER},
	{"CUPS-Get-Classes", CUPS_GET_CLASSES},
	{"CUPS-Add-Class", CUPS_ADD_CLASS},
	{"CUPS-Delete-Class", CUPS_DELETE_CLASS},
	{"CUPS-Accept-Jobs", CUPS_ACCEPT_JOBS},
	{"CUPS-Reject-Jobs", CUPS_REJECT_JOBS},
	{"CUPS-Set-Default", CUPS_SET_DEFAULT},
	{"CUPS-Get-Devices", CUPS_GET_DEVICES},
	{"CUPS-Get-PPDS", CUPS_GET_PPDS},
	{"CUPS-Move-Job", CUPS_MOVE_JOB},
	{NULL, 0}
	};

struct TABLE tags[] =
	{
	{"integer", IPP_TAG_INTEGER},
	{"boolean", IPP_TAG_BOOLEAN},
	{"enum", IPP_TAG_ENUM},
	{"octetString", IPP_TAG_STRING},
	{"dateTime", IPP_TAG_DATE},
	{"resolution", IPP_TAG_RESOLUTION},
	{"rangeOfInteger", IPP_TAG_RANGE},
	{"collection", IPP_TAG_COLLECTION},
	{"textWithLanguage", IPP_TAG_TEXTLANG},
	{"nameWithLanguage", IPP_TAG_NAMELANG},
	{"textWithoutLanguage", IPP_TAG_TEXT},
	{"nameWithoutLanguage", IPP_TAG_NAME},
	{"keyword", IPP_TAG_KEYWORD},
	{"uri", IPP_TAG_URI},
	{"urischeme", IPP_TAG_URISCHEME},
	{"charset", IPP_TAG_CHARSET},
	{"naturalLanguage", IPP_TAG_LANGUAGE},
	{"mimeMediaType", IPP_TAG_MIMETYPE},
	{NULL, 0}
	};

/** Convert a string to an operation code
*/
int ipp_str_to_operation_id(const char str[])
	{
	int iii;
	for(iii=0; operations[iii].name; iii++)
		{
		if(strcmp(operations[iii].name, str) == 0)
			return operations[iii].code;
		}
	return -1;
	}

/** Convert a string to a tag
 * The names printed are from the RFC 2565.
*/
int ipp_str_to_tag(const char str[])
	{
	int iii;
	for(iii=0; tags[iii].name; iii++)
		{
		if(strcmp(tags[iii].name, str) == 0)
			return tags[iii].code;
		}
	return IPP_TAG_UNSUPPORTED_VALUE;
	}

/* end of file */
