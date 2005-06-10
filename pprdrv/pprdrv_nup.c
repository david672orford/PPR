/*
** mouse:~ppr/src/pprdrv/pprdrv_nup.c
** Copyright 1995--2005, Trinity College Computing Center.
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
** Last modified 2 June 2005.
*/

/*
** This module is called to arange for the N-Up procedure set to be included
** in the output stream and to turn N-Up on and off.
*/

#include "config.h"
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "pprdrv.h"
#include "interface.h"

/* This font is used to print a 'draft notice' if the user wishes. */
static const char draft_notice_font[] = "Helvetica-Bold";

/* Change these to use a different version of PPR's N-Up procedure set. */
static const double procset_version = 3.0;
static const int	procset_revision = 7;		/* 6 or 7 */

/*
** This routine in the N-Up module is called before any output is
** generated in order to give the N-Up machinery an oportunity to
** request the downloading of any resources it will need.
*/
void prestart_N_Up_hook(void)
	{
	if(!job.N_Up.job_does_n_up)
		{
		/*
		** If N-Up or a "Draft" notice will be required, add the N-Up
		** proceedure set to the list of resources.
		*/
		if(job.N_Up.N != 1 || job.draft_notice)
			{
			if(add_resource("procset", "TrinColl-PPR-dmm-nup", procset_version, procset_revision) == -1)
				fatal(EXIT_JOBERR, "The procset \"TrinColl-PPR-dmm-nup\" is not available");
			}
	
		/*
		** If we are printing a draft notice, add Helvetica-Bold if it
		** is not already in the resource list.
		*/
		if(job.draft_notice)
			{
			if(add_resource("font", draft_notice_font, 0.0, 0) == -1)
				fatal(EXIT_JOBERR, _("The font \"%s\", which is required for the draft notice, is not available."), draft_notice_font);
			}
		}
	} /* end of prestart_N_Up_hook() */

/*
** Invoke the N-Up dictionary if N-Up or a draft notice
** is desired.
*/
void invoke_N_Up(void)
	{
	if(!job.N_Up.job_does_n_up)
		{
		if(job.N_Up.N != 1 || job.draft_notice)		/* if N-Up is needed */
			{
			/*
			** Call the N-Up dictionary initialization routine.
			**
			** We pass it parameters which indicate how many logical pages
			** we want on a physical page, and whether the document is printed
			** in landscape.  If it is printed in landscape, it lays the pages
			** out in a different order.
			*/
			printer_printf("%d true %s false false %d (",
					job.N_Up.N,
					job.attr.orientation == ORIENTATION_LANDSCAPE ? "true" : "false",
					job.N_Up.borders ? 0 : -1 );			/* do borders or not */
	
			if(job.draft_notice)							/* print the draft notice */
				printer_puts_escaped(job.draft_notice);		/* if there is one */
	
			printer_printf(") /%s DMM-nup-pre %%PPR\n", draft_notice_font);
	
			/*
			** Set the job title.
			** (We do not have to worry about the possible absence of statusdict
			** because the N-Up dictionary creates statusdict if it does not
			** already exist.)
			*/
			if(job.Title)
				{
				printer_puts("statusdict /jobname (");
				printer_puts_escaped(job.Title);
				printer_puts(") put %PPR\n");
				}
	
			/*
			** Tell the N-Up dictionary how many sides it will be printing
			** on and whether it will be backwards, i.e. set pp-total to 8
			** for eight sides sent in ascending order, set to -8 for eight
			** sides send in descending order.
			**
			** The code emmited here manipulates the internal state of
			** the N-Up code.
			*/
			if(job.N_Up.N != 1)
				{
				printer_printf("DMM-nup-state /pp-total %d put %%PPR\n",
					((job.attr.pages+job.attr.pagefactor-1) / job.attr.pagefactor)
							* (job.attr.pagefactor/job.N_Up.N)
							* print_direction );
				}
	
			/*
			** Tell the N-Up dictionary if we will be printing duplex.
			** If we are, it will need to adjust the page numbering
			** when printing backwards.
			**
			** The code emmited here manipulates the internal state of
			** the N-Up code.
			*/
			if( job.attr.pagefactor == (job.N_Up.N * 2) )
				printer_putline("DMM-nup-state /pp-duplex true put %PPR");
	
			} /* end of if N-Up needed */
		}
   } /* end of invoke_N_Up() */

/*
** Clean up N-Up if we invoked it.
*/
void close_N_Up(void)
	{
	if(!job.N_Up.job_does_n_up)
		{
		if(job.N_Up.N != 1 || job.draft_notice)
			{
			printer_putline("% do N-Up package end of job processing");
			printer_putline("DMM-nup-post");
			}
		}
	} /* end of close_N_Up() */

/* end of file */
