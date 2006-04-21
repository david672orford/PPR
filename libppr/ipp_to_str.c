/*
** mouse:~ppr/src/ipp/ipp_to_str.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 20 April 2006.
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

/** Convert an operation id code to a string
*/
const char *ipp_operation_id_to_str(int op)
	{
	switch(op)
		{
		case IPP_PRINT_JOB:
			return "Print-Job";
		case IPP_PRINT_URI:
			return "Print-URI";
		case IPP_VALIDATE_JOB:
			return "Validate-Job";
		case IPP_CREATE_JOB:
			return "Create-Job";
		case IPP_SEND_DOCUMENT:
			return "Send-Document";
		case IPP_SEND_URI:
			return "Send-URI";
		case IPP_CANCEL_JOB:
			return "Cancel-Job";
		case IPP_GET_JOB_ATTRIBUTES:
			return "Get-Job-Attributes";
		case IPP_GET_JOBS:
			return "Get-Jobs";
		case IPP_GET_PRINTER_ATTRIBUTES:
			return "Get-Printer-Attributes";
		case IPP_HOLD_JOB:
			return "Get-Jobs";
		case IPP_RELEASE_JOB:
			return "Release-Job";
		case IPP_RESTART_JOB:
			return "Restart-Job";
		case IPP_PAUSE_PRINTER:
			return "Pause-Printer";
		case IPP_RESUME_PRINTER:
			return "Resume-Printer";
		case IPP_PURGE_JOBS:
			return "Purge-Jobs";
		case IPP_SET_PRINTER_ATTRIBUTES:
			return "Set-Printer-Attributes";
		case IPP_SET_JOB_ATTRIBUTES:
			return "Set-Job-Attributes";
		case IPP_GET_PRINTER_SUPPORTED_VALUES:
			return "Get-Printer-Supported-Values";
		case CUPS_GET_DEFAULT:
			return "CUPS-Get-Default";
		case CUPS_GET_PRINTERS:
			return "CUPS-Get-Printers";
		case CUPS_ADD_PRINTER:
			return "CUPS-Add-Printer";
		case CUPS_DELETE_PRINTER:
			return "CUPS-Delete-Printer";
		case CUPS_GET_CLASSES:
			return "CUPS-Get-Classes";
		case CUPS_ADD_CLASS:
			return "CUPS-Add-Class";
		case CUPS_DELETE_CLASS:
			return "CUPS-Delete-Class";
		case CUPS_ACCEPT_JOBS:
			return "CUPS-Accept-Jobs";
		case CUPS_REJECT_JOBS:
			return "CUPS-Reject-Jobs";
		case CUPS_SET_DEFAULT:
			return "CUPS-Set-Default";
		case CUPS_GET_DEVICES:
			return "CUPS-Get-Devices";
		case CUPS_GET_PPDS:
			return "CUPS-Get-PPDS";
		case CUPS_MOVE_JOB:
			return "CUPS-Move-Job";
		default:
			return "unknown";
		}
	} /* end of ipp_operation_to_str() */

/** Convert a tag to a string (for debugging purposes).
 * The names printed are from the RFC 2565.
*/
const char *ipp_tag_to_str(int tag)
	{
	switch(tag)
		{
		case IPP_TAG_OPERATION:
			return "operation-attributes";
		case IPP_TAG_JOB:
			return "job-attributes";
		case IPP_TAG_END:
			return "end-of-attributes";
		case IPP_TAG_PRINTER:
			return "printer-attributes";
		case IPP_TAG_UNSUPPORTED:
			return "unsupported-attributes";

		case IPP_TAG_UNSUPPORTED_VALUE:
			return "unsupported";
		case IPP_TAG_UNKNOWN:
			return "unknown";
		case IPP_TAG_NOVALUE:
			return "no-value";

		case IPP_TAG_INTEGER:
			return "integer";
		case IPP_TAG_BOOLEAN:
			return "boolean";
		case IPP_TAG_ENUM:
			return "enum";
		case IPP_TAG_STRING:
			return "octetString";
		case IPP_TAG_DATE:
			return "dateTime";
		case IPP_TAG_RESOLUTION:
			return "resolution";
		case IPP_TAG_RANGE:
			return "rangeOfInteger";
		case IPP_TAG_COLLECTION:
			return "collection";
		case IPP_TAG_TEXTLANG:
			return "textWithLanguage";
		case IPP_TAG_NAMELANG:
			return "nameWithLanguage";
		case IPP_TAG_TEXT:
			return "textWithoutLanguage";
		case IPP_TAG_NAME:
			return "nameWithoutLanguage";
		case IPP_TAG_KEYWORD:
			return "keyword";
		case IPP_TAG_URI:
			return "uri";
		case IPP_TAG_URISCHEME:
			return "urischeme";
		case IPP_TAG_CHARSET:
			return "charset";
		case IPP_TAG_LANGUAGE:
			return "naturalLanguage";
		case IPP_TAG_MIMETYPE:
			return "mimeMediaType";

		default:
			return "unrecognized tag";
		}
	} /* ipp_tag_to_str() */

/** Convert an IPP status code to a string
 */
const char *ipp_status_code_to_str(int status_code)
	{
	switch(status_code)
		{
		case IPP_OK:
			return "success-ok";
		case IPP_BAD_REQUEST:
			return "client-error-bad-request";
		case IPP_NOT_FOUND:
			return "client-error-not-found";
		default:
			return NULL;
		}
	}

/** Group IPP tags by encoding
 * Many tag types have the same encoding.  This function lumps them together
 * for easier handling by the data processing routines.
 */
int ipp_tag_simplify(int value_tag)
	{
	switch(value_tag)
		{
		case IPP_TAG_INTEGER:
		case IPP_TAG_ENUM:
			return IPP_TAG_INTEGER;
		case IPP_TAG_STRING:
		case IPP_TAG_TEXT:
		case IPP_TAG_NAME:
		case IPP_TAG_KEYWORD:
		case IPP_TAG_URI:
		case IPP_TAG_CHARSET:
		case IPP_TAG_LANGUAGE:
		case IPP_TAG_MIMETYPE:
			return IPP_TAG_STRING;
		case IPP_TAG_BOOLEAN:
			return IPP_TAG_BOOLEAN;
		default:
			return value_tag;
		}
	}

/* end of file */
