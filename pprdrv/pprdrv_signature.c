/*
** mouse:~ppr/src/pprdrv/pprdrv_signature.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 21 November 2000.
*/

/*
** Compute the the page number to print at the current signature position.
** If this position should be skipt on this pass, return -1.
*/

#include "config.h"
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "pprdrv.h"
#include "interface.h"

int signature(int sheetnumber, int sheetpage)
	{
	int leftcentre;				/* cardinal page number of center fold left side */
	int signumber;				/* cardinal signature number */
	int sigpage;				/* cardinal offset of page into signature (zero for GNU-CC) */
	int sigpages;				/* number of virtual pages per signature */
	int sigsheet;				/* cardinal sheet number in this signature */

	/*
	** If not in duplex mode, then fudge things.
	** It is unclear if this code is every invoked
	** since I think signiture mode forces duplex
	** mode.
	*/
	if( job.N_Up.N == job.attr.pagefactor )
		{
		if(sheetnumber & 1)						/* if odd sheet number, */
			sheetpage += job.N_Up.N;			/* bump up page index */
		sheetnumber /= 2;						/* reduce sheetnumber to duplex value. */
		}

	/*
	** If we are only doing one side of the signiture sheets and
	** this is not the side we should be doing, get out now.
	*/
	if( ( sheetpage < (job.attr.pagefactor/2) && !(job.N_Up.sigpart & SIG_FRONTS) )
		|| ( sheetpage >= (job.attr.pagefactor/2) && !(job.N_Up.sigpart & SIG_BACKS) ) )
		{
		return -1;
		}

	/* Compute the number of logical pages in a signiture. */
	sigpages = (job.N_Up.N * job.N_Up.sigsheets * 2);

	/* Which signiture in the book are we now working on? */
	signumber = sheetnumber / job.N_Up.sigsheets;

	/* Which sheet in the current signiture are we working on? */
	sigsheet = sheetnumber % job.N_Up.sigsheets;

	/*
	** Compute the sigpage number of the page on the
	** left hand side of the centre fold.
	**
	** The value of sigpage, which will be computed in
	** subsequent blocks of code is the index into the
	** page list for this signiture.
	*/
	leftcentre = (sigpages/2) - 1;

	/* The plan for 2-Up signitures. */
	if(job.N_Up.N == 2)
		{
		switch(sheetpage)
			{
			case 0:
				sigpage = leftcentre - (sigsheet*2);
				break;
			case 1:
				sigpage = leftcentre + (sigsheet*2) + 1;
				break;
			case 2:
				sigpage = leftcentre + (sigsheet*2) + 2;
				break;
			case 3:
				sigpage = leftcentre - (sigsheet*2) - 1;
				break;
			default:
				fatal(EXIT_PRNERR_NORETRY, "pprdrv_signature.c: this can't happend");
			}
		}

#if 0
	/* The plan for 4-Up signitures, not implemented yet. */
	else if(job.N_Up.N == 4)
		{

		}
#endif

	else						/* if unknown, in effect, */
		{						/* don't do signature */
		sigpage = sheetpage;
		}

	return (signumber * sigpages) + sigpage;
	} /* end of signature() */

/* end of file */
