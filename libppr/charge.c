/*
** mouse:~ppr/src/libppr/charge.c
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
** Last modified 28 March 2005.
*/

/*! \file */

#include "config.h"
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"

#if 0
#define DEBUG 1
#define debug printf
#endif

/** compute charge for printing
 *
 * Given the required information, this function will compute the price to be
 * charged for printing the job.  This computation is complicated and is 
 * needed in more than one place, that is why it is now a function.  It is 
 * called from pprdrv when printing the banner page and when debiting the 
 * account and from ppr-respond when invoking the responder.
 *
 * The result is not a single number but rather a receipt outlining how the
 * total was arrived out.  The receipt is stored in the struct 
 * COMPUTED_CHARGE charge.
 */
void compute_charge(
		struct COMPUTED_CHARGE *charge,		/** results stored here */
		int per_duplex_sheet,				/** price in cents of double-sided sheet */
		int per_simplex_sheet,				/** price in cents of single-sided sheet */
		int vpages,							/** number of unique page descriptions printed */
		int n_up_n,							/** number of virtual pages per side */
		int vpages_per_sheet,				/** number of page descriptions per sheet */
		int sigsheets,						/** sheets in pseudo-signature, 0 if signature mode is off */
		int sigpart,						/** fronts, backs, or both */
		int copies							/** number of copies of document, -1 if unknown */
		)
	{
	#ifdef DEBUG
	debug("compute_charge(charge=?, per_duplex_sheet=%d, per_simplex_sheet=%d, vpages=%d, n_up_n=%d, vpages_per_sheet=%d, sigsheets=%d, sigpart=%d, copies=%d",
		per_duplex_sheet, per_simplex_sheet,
		vpages, n_up_n, vpages_per_sheet,
		sigsheets, sigpart, copies);
	#endif

	/* store prices so that we can say things like "X simplex sheets @ $0.10 per" */
	charge->per_duplex = per_duplex_sheet;
	charge->per_simplex = per_simplex_sheet;

	/* If doing signiture printing, */
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

	/* If not doing signiture printing, */
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
