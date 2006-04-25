/*
** mouse:~ppr/src/libppr/get_default.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 24 April 2006.
*/

/*! \file
	\brief Get default destination


*/

#include "config.h"
#include "gu.h"
#include "global_defines.h"

/** Get default destination
 * The name of the default destination is returned in a block which
 * the caller must free.  If there is no default destination, then
 * NULL is returned.
*/
char *ppr_get_default(void)
	{
	FILE *f;
	char *default_destination = NULL;

	if((f = fopen(DEFAULT_CONFFILE, "r")))
		{
		char *line = NULL;
		int line_space = 16;
		if((line = gu_getline(line, &line_space, f)))
			{
			default_destination = line;
			}
		fclose(f);
		}

	return default_destination;	
	}

/* end of file */
