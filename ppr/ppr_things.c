/*
** mouse:~ppr/src/ppr/ppr_things.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 19 July 2001.
*/

#include "config.h"
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "ppr.h"

static int things_space = 0;

/*
** Make sure there is space to add at least one more entry to things[].
*/
void things_space_check(void)
	{
	if( (things_space - thing_count) < 1 )
		{
		things_space += 100;
		things = (struct Thing *)gu_realloc(things, things_space, sizeof(struct Thing));
		}
	} /* end of things_space_check() */

/* end of file */
