/*
** mouse:~ppr/src/libppr/int_snmp.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 21 June 2001.
*/

#include "before_system.h"
#include <stdio.h>
#include <unistd.h>
#include "gu.h"
#include "global_defines.h"
#include "libppr_int.h"

/*
** This is called to print a LaserWriter style message when the printer refuses a
** TCP connexion.
*/
void int_snmp_status(void *p)
    {
    struct gu_snmp *snmp_obj = p;
    int error_code;
    int n1, n2;
    unsigned int n3;

    if(gu_snmp_get(snmp_obj, &error_code,
		"1.3.6.1.2.1.25.3.2.1.5.1", GU_SNMP_INT, &n1,
    		"1.3.6.1.2.1.25.3.5.1.1.1", GU_SNMP_INT, &n2,
                "1.3.6.1.2.1.25.3.5.1.2.1", GU_SNMP_BIT, &n3,
    		NULL) < 0)
	{
	alert(int_cmdline.printer, TRUE, "gu_snmp_get() failed, error_code=%d", error_code);
    	}

    else
	{
	/* This will be picked up by pprdrv. */
	printf("%%%%[ PPR SNMP: %d %d %08x ]%%%%\n", n1, n2, n3);
	fflush(stdout);
	}

    } /* end of int_snmp_status() */

/* end of file */
