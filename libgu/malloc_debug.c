/*
** mouse:~ppr/src/libppr/gu_alloc_debug.c
** Copyright 1995--2004, Trinity College Computing Center.
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
** Last modified 23 January 2004.
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
	const char function[] = "gu_alloc_assert";
	if((gu_alloc_blocks_saved + assertion) != gu_alloc_blocks)
		{
		gu_Throw("%s(): assertion failed at %s line %d (is %d, expected %d)",
				function, file, line,
				gu_alloc_blocks, (gu_alloc_blocks_saved + assertion) );
		}
	} /* end of _gu_alloc_assert() */

/* end of gu_alloc_debug.c */

