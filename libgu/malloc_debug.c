/*
** mouse:~ppr/src/libppr/gu_alloc_debug.c
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

#include "before_system.h"
#include "gu.h"

/* number of blocks at last gu_alloc_checkpoint() */
static int gu_alloc_blocks_saved;

/* number of blocks currently allocated */
extern int gu_alloc_blocks;

/*
** Save the value of gu_alloc_blocks.
*/
void gu_alloc_checkpoint(void)
	{
	gu_alloc_blocks_saved = gu_alloc_blocks;
	} /* end of gu_alloc_checkpoint() */

/*
** Routines to get and put the difference between gu_alloc_blocks
** and gu_alloc_blocks_saved.
**
** This is used by library routines and signal handler
** so they don't mess up the gu_alloc_checkpoint()/gu_alloc_assert()
** mechanisms of routines they are called from or routines
** they interupt.
*/
int gu_alloc_checkpoint_get(void) { return gu_alloc_blocks - gu_alloc_blocks_saved; }
void gu_alloc_checkpoint_put(int n) { gu_alloc_blocks_saved = gu_alloc_blocks - n; }

/*
** This is called to assert that gu_alloc_blocks differs
** from the figure saved by gu_alloc_checkpoint by a
** certain amount.  This is called by a macro called
** gu_alloc_assert().  A positive amount is increase, a negative
** amount is decrease.
*/
void _gu_alloc_assert(const char *file, int line, int assertion)
	{
	if((gu_alloc_blocks_saved + assertion) != gu_alloc_blocks)
		{
		libppr_throw(EXCEPTION_BADUSAGE, "gu_alloc_assert", "assertion failed at %s line %d (is %d, expected %d)",
				file, line, gu_alloc_blocks, (gu_alloc_blocks_saved + assertion) );
		}
	} /* end of _gu_alloc_assert() */

/* end of gu_alloc_debug.c */

