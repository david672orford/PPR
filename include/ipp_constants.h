/*
** mouse:~ppr/src/ipp_constants.h
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 21 April 2006.
*/

/*! \file

These IPP constant names are from the CUPS documentation.  The values are from
RFC 2565.  The RFC 2565 names are noted in the comments.

*/

/*=========================== ipp_to_str.c ==============================*/

const char *ipp_operation_id_to_str(int op);
const char *ipp_tag_to_str(int tag);
const char *ipp_status_code_to_str(int status_code);
int ipp_tag_simplify(int value_tag);

/*=========================== ipp_str_to.c =============================*/

int ipp_str_to_operation_id(const char str[]);
int ipp_str_to_tag(const char str[]);

/* RFC 2565 section 3.7.1 Delimiter Tags */
#define IPP_TAG_ZERO 0x00				/** reserved */
#define IPP_TAG_OPERATION 0x01			/** operation-attributes-tag */
#define IPP_TAG_JOB 0x02				/** job-attributes-tag */
#define IPP_TAG_END 0x03				/** end-of-attributes-tag */
#define IPP_TAG_PRINTER 0x04			/** printer-attributes-tag */
#define IPP_TAG_UNSUPPORTED 0x05		/** unsupported-attributes-tag (missing from CUPS) */

/* RFC 2565 section 3.7.2 Value Tags */
#define IPP_TAG_UNSUPPORTED_VALUE 0x10	/** unsupported */
#define IPP_TAG_DEFAULT 0x11			/** reserved for future 'default' */
#define IPP_TAG_UNKNOWN 0x12			/** unknown */
#define IPP_TAG_NOVALUE 0x13			/** no-value */
#define IPP_TAG_INTEGER 0x21			/** integer */
#define IPP_TAG_BOOLEAN 0x22			/** boolean */
#define IPP_TAG_ENUM 0x23				/** enum */
#define IPP_TAG_STRING 0x30				/** octetString with an unspecified format */
#define IPP_TAG_DATE 0x31				/** dateTime */
#define IPP_TAG_RESOLUTION 0x32			/** resolution */
#define IPP_TAG_RANGE 0x33				/** rangeOfInteger */
#define IPP_TAG_COLLECTION 0x34			/** reserved for a collection (in the future) */
#define IPP_TAG_TEXTLANG 0x35			/** textWithLangauge */
#define IPP_TAG_NAMELANG 0x36			/** nameWithLanguage */
#define IPP_TAG_TEXT 0x41				/** textWithoutLanguage */
#define IPP_TAG_NAME 0x42				/** nameWithoutLanguage */
#define IPP_TAG_KEYWORD 0x44			/** keyword */
#define IPP_TAG_URI 0x45				/** uri */
#define IPP_TAG_URISCHEME 0x46			/** urischeme */
#define IPP_TAG_CHARSET	0x47			/** charset */
#define IPP_TAG_LANGUAGE 0x48			/** naturalLanguage */
#define IPP_TAG_MIMETYPE 0x49			/** mimeMediaType */

/* IPP Operations */
#define IPP_PRINT_JOB 0x0002
#define IPP_PRINT_URI 0x0003
#define IPP_VALIDATE_JOB 0x0004
#define IPP_CREATE_JOB 0x0005
#define IPP_SEND_DOCUMENT 0x0006
#define IPP_SEND_URI 0x0007
#define IPP_CANCEL_JOB 0x0008
#define IPP_GET_JOB_ATTRIBUTES 0x0009
#define IPP_GET_JOBS 0x000a
#define IPP_GET_PRINTER_ATTRIBUTES 0x000b
#define IPP_HOLD_JOB 0x000c
#define IPP_RELEASE_JOB 0x000d
#define IPP_RESTART_JOB 0x000e
#define IPP_PAUSE_PRINTER 0x0010
#define IPP_RESUME_PRINTER 0x0011
#define IPP_PURGE_JOBS 0x0012
#define IPP_SET_PRINTER_ATTRIBUTES 0x0013
#define IPP_SET_JOB_ATTRIBUTES 0x0014
#define IPP_GET_PRINTER_SUPPORTED_VALUES 0x0015
#define IPP_CREATE_PRINTER_SUBSCRIPTION 0x0016
#define IPP_CREATE_JOB_SUBSCRIPTION 0x0017
#define IPP_GET_SUBSCRIPTION_ATTRIBUTES 0x0018
#define IPP_GET_SUBSCRIPTIONS 0x0019
#define IPP_RENEW_SUBSCRIPTION 0x001a
#define IPP_CANCEL_SUBSCRIPTION 0x001b
#define IPP_GET_NOTIFICATIONS 0x001c
#define IPP_SEND_NOTIFICATIONS 0x001d
#define IPP_GET_PRINT_SUPPORT_FILES 0x0021
#define IPP_ENABLE_PRINTER 0x0022
#define IPP_DISABLE_PRINTER 0x0023
#define IPP_PAUSE_PRINTER_AFTER_CURRENT_JOB 0x0024
#define IPP_HOLD_NEW_JOBS 0x0025
#define IPP_RELEASE_HELD_NEW_JOBS 0x0026
#define IPP_DEACTIVATE_PRINTER 0x0027
#define IPP_ACTIVATE_PRINTER 0x0028
#define IPP_RESTART_PRINTER 0x0029
#define IPP_SHUTDOWN_PRINTER 0x002a
#define IPP_STARTUP_PRINTER 0x002b
#define IPP_REPROCESS_JOB 0x002c
#define IPP_CANCEL_CURRENT_JOB 0x002d
#define IPP_SUSPEND_CURRENT_JOB 0x002e
#define IPP_RESUME_JOB 0x002f
#define IPP_PROMOTE_JOB 0x0030
#define IPP_SCHEDULE_JOB_AFTER 0x0031

/* CUPS IPP Extension Operations */
#define CUPS_GET_DEFAULT 0x4001
#define CUPS_GET_PRINTERS 0x4002
#define CUPS_ADD_PRINTER 0x4003
#define CUPS_DELETE_PRINTER 0x4004
#define CUPS_GET_CLASSES 0x4005
#define CUPS_ADD_CLASS 0x4006
#define CUPS_DELETE_CLASS 0x4007
#define CUPS_ACCEPT_JOBS 0x4008
#define CUPS_REJECT_JOBS 0x4009
#define CUPS_SET_DEFAULT 0x400a
#define CUPS_GET_DEVICES 0x400b
#define CUPS_GET_PPDS 0x400c
#define CUPS_MOVE_JOB 0x400d
#define CUPS_AUTHENTICATE_JOB 0x400e

/*
 * IPP request result codes
 * See RFC 2566 section 13.1 and RFC 2911 section 13.1.
 */ 
#define IPP_OK                      0x0000
#define IPP_OK_SUBST                0x0001
#define IPP_OK_CONFLICT             0x0002
#define IPP_OK_IGNORED_SUBSCRIPTIONS 0x0003
#define IPP_OK_IGNORED_NOTIFICATIONS 0x0004
#define IPP_OK_TOO_MANY_EVENTS		0x0005
#define IPP_OK_BUT_CANCEL_SUBSCRIPTION 0x0006

#define IPP_REDIRECTION_OTHER_SITE	0x0300

#define IPP_BAD_REQUEST             0x0400
#define IPP_FORBIDDEN               0x0401
#define IPP_NOT_AUTHENTICATED       0x0402
#define IPP_NOT_AUTHORIZED          0x0403
#define IPP_NOT_POSSIBLE            0x0402
#define IPP_TIMEOUT                 0x0405
#define IPP_NOT_FOUND               0x0406
#define IPP_GONE					0x0407
#define IPP_REQUEST_ENTITY			0x0408
#define IPP_REQUEST_VALUE			0x0409
#define IPP_DOCUMENT_FORMAT			0x040a
#define IPP_ATTRIBUTES				0x040b
#define IPP_URI_SCHEME				0x040c
#define IPP_CHARSET                 0x040d
#define IPP_CONFLICT				0x040e
#define IPP_COMPRESSION_NOT_SUPPORTED 0x040f
#define IPP_COMPRESSION_ERROR		0x0410
#define IPP_DOCUMENT_FORMAT_ERROR	0x0411
#define IPP_DOCUMENT_ACCESS_ERROR	0x0412
#define IPP_ATTRIBUTES_NOT_SETTABLE	0x0413
#define IPP_IGNORED_ALL_SUBSCRIPTIONS 0x0414
#define IPP_TOO_MANY_SUBSCRIPTIONS	0x0415
#define IPP_IGNORED_ALL_NOTIFICATIONS 0x0416
#define IPP_PRINT_SUPPORT_FILE_NOT_FOUND 0x0417

#define IPP_INTERNAL_ERROR			0x0500
#define IPP_OPERATION_NOT_SUPPORTED 0x0501
#define IPP_SERVICE_UNAVAILABLE		0x0502
#define IPP_VERSION_NOT_SUPPORTED	0x0503
#define IPP_DEVICE_ERROR			0x0504
#define IPP_TEMPORARY_ERROR			0x0505
#define IPP_NOT_ACCEPTING			0x0506
#define IPP_PRINTER_BUSY			0x0507
#define IPP_ERROR_JOB_CANCELLED		0x0508
#define IPP_MULTIPLE_JOBS_NOT_SUPPORTED 0x0509
#define IPP_PRINTER_IS_DEACTIVATED	0x050a

/* IPP printer-state codes */
#define IPP_PRINTER_IDLE 3
#define IPP_PRINTER_PROCESSING 4
#define IPP_PRINTER_STOPPED 5

/* IPP job-state codes */
#define IPP_JOB_PENDING 3
#define IPP_JOB_HELD 4
#define IPP_JOB_PROCESSING 5
#define IPP_JOB_STOPPED 6
#define IPP_JOB_CANCELLED 7
#define IPP_JOB_ABORTED 8
#define IPP_JOB_COMPLETED 9

/* end of file */
