/*
** mouse.trincoll.edu:~ppr/src/libppr/charge.c
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
** Last modified 16 July 1998.
*/

#include "config.h"
#include "gu.h"
#include "global_defines.h"

#include "global_structs.h"

#if 0
#define DEBUG 1
#endif

/*
** Given the required information, this function will compute the cost
** of printing the job.  This function is complicated, that is why it
** is now a function.  It is called from pprdrv when printing the banner
** page and when debiting the account and from pprd when invoking the
** responder.
*/
void compute_charge(struct COMPUTED_CHARGE *charge,
		int per_duplex_sheet, int per_simplex_sheet,
		int vpages, int n_up_n, int vpages_per_sheet,
		int sigsheets, int sigpart, int copies)
	{
	#ifdef DEBUG
	debug("compute_charge(charge=?, per_duplex_sheet=%d, per_simplex_sheet=%d, vpages=%d, n_up_n=%d, vpages_per_sheet=%d, sigsheets=%d, sigpart=%d, copies=%d",
		per_duplex_sheet, per_simplex_sheet,
		vpages, n_up_n, vpages_per_sheet,
		sigsheets, sigpart, copies);
	#endif

	charge->per_duplex = per_duplex_sheet;
	charge->per_simplex = per_simplex_sheet;

	/*
	** If doing signiture printing,
	*/
	if(sigsheets != 0)
		{
		int pages_per_signiture = n_up_n * 2 * sigsheets;

		int signature_count =
				(vpages + pages_per_signiture - 1)
						/
				pages_per_signiture;

		/* If printing both fronts and backs, */
		if((sigpart & SIG_BOTH) == SIG_BOTH)
			{
			/* The expression (n_up_n * 2 / vpages_per_sheet) doubles the
			   number of sheets if we are not doing duplex.	 (We might
			   print both fronts and backs in simplex mode if we will
			   later photocopy them to duplexed output.) */
			charge->duplex_sheets = signature_count * sigsheets * ((n_up_n * 2)/vpages_per_sheet);
			charge->simplex_sheets = 0;
			}
		/* If only fronts or only backs in simplex mode. */
		else if(vpages_per_sheet == n_up_n)
			{
			charge->duplex_sheets = 0;
			charge->simplex_sheets = signature_count * sigsheets;
			}
		/* If only fronts or only backs in duplex mode.
		  (This seems like a rather silly thing to do, but
		  we want to be able to charge correctly.) */
		else
			{
			charge->duplex_sheets = (signature_count * sigsheets + 1) / 2;
			charge->simplex_sheets = 0;
			}
		}

	/*
	** If not doing signiture printing,
	*/
	else
		{
		int sides = (vpages + n_up_n - 1) / n_up_n;

		/* Simplex mode: */
		if(vpages_per_sheet == n_up_n)
			{
			charge->duplex_sheets = 0;
			charge->simplex_sheets = sides;
			}
		/* Duplex mode: */
		else
			{
			charge->duplex_sheets = sides / 2;
			charge->simplex_sheets = sides % 2;
			}
		}

	/*
	** copies == -1 means # copies unknown, for all other
	** values we multiple by # copies.
	*/
	if(copies >= 0)
		{
		charge->duplex_sheets *= copies;
		charge->simplex_sheets *= copies;
		}

	/*
	** The total charge is easy to arrive at.
	*/
	charge->total = (charge->duplex_sheets * charge->per_duplex)
				+ (charge->simplex_sheets * charge->per_simplex);

	#ifdef DEBUG
	debug("compute_charge(): duplex_sheets=%d, simplex_sheets=%d, total=%d",
		charge->duplex_sheets, charge->simplex_sheets, charge->total);
	#endif

	} /* end of compute_charge() */

/* end of file */
