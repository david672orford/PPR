/*
** mouse:~ppr/src/libppr_int/int_pe_tcpip.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 24 June 1999.
*/

#include "before_system.h"
#include <errno.h>
#include <string.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

#include "libppr_int.h"
#include "interface.h"

/*
** Explain why reading from or writing to the printer port failed.
*/
void int_printer_error_tcpip(int error_number)
    {
    switch(error_number)
    	{
	case EIO:
	    alert(int_cmdline.printer, TRUE, _("Connection to printer lost."));
	    break;
    	default:
	    alert(int_cmdline.printer, TRUE, _("TCP/IP communication failed, errno=%d (%s)."), error_number, gu_strerror(error_number));
	    break;
	}
    int_exit(EXIT_PRNERR);
    }

/* end of file */
