/*
** mouse:~ppr/src/pprdrv/pprdrv_req.c
** Copyright 1995--2002, Trinity College Computing Center.
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
** Last modified 27 September 2002.
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
** Print all the requirements including those
** arising from copy counts.
*/
void write_requirement_comments(void)
	{
	int i;

	/*
	** Copy requirements from the original PostScript file.  Note that "collate"
	** and "numcopies(X)" have already been filtered out.
	*/
	for(i=0; i < drvreq_count; i++)
		{
		if(i)									/* not 1st requirement */
			printer_printf("%%%%+ %s\n", drvreq[i]);
		else									/* 1st requirement */
			printer_printf("%%%%Requirements: %s\n", drvreq[i]);
		}

	/*
	** Multiple copies is a requirement, but remember that we are
	** specifying the requirements which the _printer_ must meet.
	** If we are producing the copies, it is not a printer requirement.
	*/
	if(copies_auto > 1)					/* note that copies_auto==1 is not a requirement */
		{
		if(i)							/* if %%Requirements: started, */
			printer_puts("%%+");		/* just continue it */
		else							/* if not started, */
			printer_puts("%%Requirements:");	/* start it now */

		printer_printf(" numcopies(%d)", job.opts.copies);

		if(copies_auto_collate)
			printer_puts(" collate");

		printer_putc('\n');
		}

	} /* end of write_requirement_comments() */

/* end of file */
