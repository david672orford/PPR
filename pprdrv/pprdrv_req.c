/*
** mouse:~ppr/src/pprdrv/pprdrv_req.c
** Copyright 1995, 1996, 1997, 1998, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 4 December 1998.
*/

/*
** This module contains functions for PostScript Requirements.  Additional
** requirements code is contained in the module pprdrv_capable.c.
*/

#include "before_system.h"
#include <stdio.h>
#include <sys/types.h>
#include "gu.h"
#include "global_defines.h"

#include "global_structs.h"
#include "pprdrv.h"

/*
** This is declared external here rather than in pprdrv.h because this
** is the only module besides pprdrv.c that uses it.
*/
extern int copies_auto;

/*
** Print all the requirements including those
** arising from copy counts.
*/
void write_requirement_comments(void)
    {
    int x;

    for(x=0;x<drvreq_count;x++)
    	{
	if(x)					/* not 1st requirement */
	    printer_printf("%%%%+ %s\n",drvreq[x]);
	else					/* 1st requirement */
	    printer_printf("%%%%Requirements: %s\n",drvreq[x]);
    	}

    /*
    ** Multiple copies is a requirement, but remember that we are
    ** specifying the requirements which the _printer_ must meet.
    ** If we are producing the copies, it is not a printer requirement.
    */
    if(copies_auto)
    	{
	if(x)				/* if %%Requirements: started, */
	    printer_puts("%%+");	/* just continue it */
	else				/* if not started, */
	    printer_puts("%%Requirements:");	/* start it now */

	printer_printf(" numcopies(%d)",job.opts.copies);

	if(job.opts.collate)		/* Since I know of no printer which can collate */
	    printer_puts(" collate");	/* auto copies, it seems likely that this will */
					/* never be TRUE. */
	printer_putc('\n');
	}

    } /* end of dump_document_requirements() */

/* end of file */
