/*
** mouse:~ppr/src/pprdrv/pprdrv_snmp_messages.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 4 May 2001.
*/

#include "before_system.h"
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "pprdrv.h"

int translate_snmp_error(int bit, const char **description, int *severity)
    {
    switch(bit)
        {
        case 0:				/* 0x00000001 */
	    *description = "paper low";
	    *severity = 4;
	    break;
        case 1:				/* 0x00000002 */
            *description = "out of paper";
	    *severity = 6;
            break;
        case 2:				/* 0x00000004 */
            *description = "toner/ink is low";
	    *severity = 4;
            break;
        case 3:				/* 0x00000008 */
	    *description = "out of toner/ink";
	    *severity = 6;
            break;
        case 4:				/* 0x00000010 */
            *description = "cover open";
	    *severity = 6;
            break;
        case 5:				/* 0x00000020 */
            *description = "paper jam";
	    *severity = 6;
            break;
        case 6:				/* 0x00000040 */
            *description = "off line";
	    *severity = 6;
            break;
        case 7:				/* 0x00000080 */
            *description = "service requested";
	    *severity = 10;
            break;
	case 8:				/* 0x00000100 */
	    *description = "no paper tray";
	    *severity = 6;
            break;
	case 9:				/* 0x00000200 */
	    *description = "no output tray";
	    *severity = 6;
            break;
	case 10:			/* 0x00000400 */
	    *description = "toner/ink cartridge missing";
	    *severity = 6;
            break;
	case 11:			/* 0x00000800 */
	    *description = "output tray near full";
	    *severity = 3;
            break;
	case 12:			/* 0x00001000 */
	    *description = "output tray full";
	    *severity = 6;
            break;
	case 13:			/* 0x00002000 */
	    *description = "input tray empty";
	    *severity = 6;
            break;
	case 14:			/* 0x00004000 */
	    *description = "overdue preventative maintainance";
	    *severity = 2;
            break;
        default:
            *description = "[undefined SNMP error code]";
	    *severity = 1;
            break;
	}

    return 0;
    } /* end of translate_snmp_message() */

int translate_snmp_status(int device_status, int printer_status, const char **message, const char **raw1, int *severity)
    {
    switch(printer_status)
	{
	case 1:
	    *message = "other";
	    *raw1 = "other(1)";
	    *severity = 3;
	    break;
	case 2:
	    *message = "unknown";
	    *raw1 = "unknown(2)";
	    *severity = 3;
	    break;
	case 3:
	    *message = "idle";
	    *raw1 = "idle(3)";
	    *severity = 1;
	    break;
	case 4:
	    *message = "busy";
	    *raw1 = "printing(4)";
	    *severity = 2;
	    break;
	case 5:
	    *message = "warming up";
	    *raw1 = "warmup(5)";
	    *severity = 2;
	    break;
	default:
	    *message = "[undefined]";
	    *raw1 = "";
	    *severity = 1;
	    break;
        }
    return 0;
    } /* end of translate_snmp_status() */

/* end of file */
