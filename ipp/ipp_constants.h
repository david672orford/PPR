/*
** mouse:~ppr/src/ipp_constants.h
**
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
** Last modified 3 April 2003.
*/

/*! \file

These IPP constant names are from the CUPS documentation.  The values are from RFC 2565.
The RFC 2565 names are noted in the comments.

*/

/* RFC 2565 section 3.7.1 Delimiter Tags */
#define IPP_TAG_ZERO 0x00				/* reserved */
#define IPP_TAG_OPERATION 0x01			/* operation-attributes-tag */
#define IPP_TAG_JOB 0x02				/* job-attributes-tag */
#define IPP_TAG_END 0x03				/* end-of-attributes-tag */
#define IPP_TAG_PRINTER 0x04			/* printer-attributes-tag */
#define IPP_TAG_UNSUPPORTED 0x05		/* unsupported-attributes-tag (missing from CUPS) */

/* RFC 2565 section 3.7.2 Value Tags */
#define IPP_TAG_UNSUPPORTED_VALUE 0x10	/* unsupported */
#define IPP_TAG_DEFAULT 0x11			/* reserved for future 'default' */
#define IPP_TAG_UNKNOWN 0x12			/* unknown */
#define IPP_TAG_NOVALUE 0x13			/* no-value */
#define IPP_TAG_INTEGER 0x21			/* integer */
#define IPP_TAG_BOOLEAN 0x22			/* boolean */
#define IPP_TAG_ENUM 0x23				/* enum */
#define IPP_TAG_STRING 0x30				/* octetString with an unspecified format */
#define IPP_TAG_DATE 0x31				/* dateTime */
#define IPP_TAG_RESOLUTION 0x32			/* resolution */
#define IPP_TAG_RANGE 0x33				/* rangeOfInteger */
#define IPP_TAG_COLLECTION 0x34			/* reserved for a collection (in the future) */
#define IPP_TAG_TEXTLANG 0x35			/* textWithLangauge */
#define IPP_TAG_NAMELANG 0x36			/* nameWithLanguage */
#define IPP_TAG_TEXT 0x41				/* textWithoutLanguage */
#define IPP_TAG_NAME 0x42				/* nameWithoutLanguage */
#define IPP_TAG_KEYWORD 0x44			/* keyword */
#define IPP_TAG_URI 0x45				/* uri */
#define IPP_TAG_URISCHEME 0x46			/* urischeme */
#define IPP_TAG_CHARSET	0x47			/* charset */
#define IPP_TAG_LANGUAGE 0x48			/* naturalLanguage */
#define IPP_TAG_MIMETYPE 0x49			/* mimeMediaType */

/* IPP Operations */
#define IPP_PRINT_JOB
#define IPP_PRINT_URI
#define IPP_VALIDATE_JOB
#define IPP_CREATE_JOB
#define IPP_SEND_DOCUMENT
#define IPP_SEND_URI
#define IPP_CANCEL_JOB
#define IPP_GET_JOB_ATTRIBUTES
#define IPP_GET_JOBS
#define IPP_GET_PRINTER_ATTRIBUTES
#define IPP_HOLD_JOB
#define IPP_RELEASE_JOB
#define IPP_RESTART_JOB
#define IPP_PAUSE_PRINTER
#define IPP_RESUME_PRINTER
#define IPP_PURGE_JOBS
#define IPP_SET_PRINTER_ATTRIBUTES
#define IPP_SET_JOB_ATTRIBUTES
#define IPP_GET_PRINTER_SUPPORTED_VALUES

/* CUPS IPP Extension Operations */
#define CUPS_GET_DEFAULT
#define CUPS_GET_PRINTERS
#define CUPS_ADD_PRINTER
#define CUPS_DELETE_PRINTER
#define CUPS_GET_CLASSES
#define CUPS_ADD_CLASS
#define CUPS_DELETE_CLASS
#define CUPS_ACCEPT_JOBS
#define CUPS_REJECT_JOBS
#define CUPS_SET_DEFAULT
#define CUPS_GET_DEVICES
#define CUPS_GET_PPDS
#define CUPS_MOVE_JOB

/* end of file */
