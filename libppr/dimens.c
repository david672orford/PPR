/*
** mouse:~ppr/src/libppr/dimens.c
** Copyright 1995, 1996, 1997, 1998, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
** Get a dimension and convert it to points.
**
** Last modified 7 September 1998.
*/

#include "before_system.h"
#include <string.h>
#include <stdlib.h>
#include "gu.h"
#include "global_defines.h"


double convert_dimension(const char *string)
    {
    const char *nptr;		/* pointer to the number */
    const char *unitptr;	/* pointer to the units specifier */
    int unitlen;		/* length of unit specifier in characters */

    nptr=&string[strspn(string, " \t")];	/* eat blanks */
    unitptr=nptr+strspn(string, "0123456789.");	/* eat number */
    unitptr+=strspn(unitptr, " \t");		/* eat trailing space */
    unitlen=strlen(unitptr);			/* get unit spec length */

    if( (unitlen==6 && gu_strcasecmp(unitptr, "points") == 0)
		|| (unitlen==2 && gu_strcasecmp(unitptr, "pt") == 0)
		|| (unitlen==3 && gu_strcasecmp(unitptr, "psu") == 0) )
	return atoi(nptr);

    else if( (unitlen==0)
	    || (unitlen==6 && gu_strcasecmp(unitptr, "inches") == 0)
	    || (unitlen==4 && gu_strcasecmp(unitptr, "inch") == 0)
	    || (unitlen==2 && gu_strcasecmp(unitptr, "in") == 0) )
	return ( atof(nptr) * 72.0 );

    else if( (unitlen==10 && gu_strcasecmp(unitptr, "centimeters") == 0)
	    || (unitlen==2 && gu_strcasecmp(unitptr, "cm") == 0)
	    || (unitlen==10 && gu_strcasecmp(unitptr, "centimetres") == 0) )
	return ( atof(nptr) / 2.54 * 72.0 );

    else if( (unitlen==11 && gu_strcasecmp(unitptr, "millimeters") == 0)
	    || (unitlen==2 && gu_strcasecmp(unitptr, "mm") == 0)
	    || (unitlen==11 && gu_strcasecmp(unitptr, "millimetres") == 0) )
	return ( atof(nptr) / 25.4 * 72.0 );

    else
	{
	return -1;
	}

    } /* end of convert_dimension() */

/* end of file */
