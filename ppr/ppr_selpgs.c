/*
** mouse:~ppr/src/ppr/ppr_main.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last revised 4 June 1999.
*/

#include "before_system.h"
#include <stdlib.h>
#include "gu.h"
#include "global_defines.h"

#include "global_structs.h"
#include "ppr.h"
#include "respond.h"
#include "util_exits.h"

void select_pages(struct QFileEntry *qentry, const char *page_list)
    {
    if(! qentry->attr.script)
    	{
    	respond(RESP_CANCELED_NOPAGES, "");
    	exit(EXIT_NOTPOSSIBLE);
    	}



    } /* end of select_pages */

/* end of file */

