/*
** mouse:~ppr/src/libppr/money.c
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

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include "gu.h"
#include "global_defines.h"


/*
** Return a string representing the amount of money indicated
** in the argument which is the basic unit times 100.
*/
char *money(int amount)
	{
	static char temp[32];
	static const char *pos_values = NULL;
	static const char *neg_values = NULL;
	int dollars, cents;

	if(!pos_values || !neg_values)
		{
		if(gu_ini_scan_list(PPR_CONF, "internationalization", "money",
				GU_INI_TYPE_NONEMPTY_STRING, &pos_values,
				GU_INI_TYPE_NONEMPTY_STRING, &neg_values,
				GU_INI_TYPE_END))
			{			/* if error message returned, */
			pos_values = "%d.%02d";
			neg_values = "-%d.%02d";
			}
		}

	dollars = abs(amount / 100);
	cents = abs(amount % 100);

	if(amount < 0)
		snprintf(temp, sizeof(temp), neg_values, dollars, cents);
	else
		snprintf(temp, sizeof(temp), pos_values, dollars, cents);

	return temp;
	} /* end of money() */

/* end of file */
