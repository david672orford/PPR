/*
** mouse:~ppr/src/libppr/dimens.c
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
** Get a dimension and convert it to points.
**
** Last modified 3 May 2004.
*/

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include "gu.h"
#include "global_defines.h"

/** Parse a linear dimension
 *
 * This function parses a statement of length in any of the many forms accepted
 * by PPR and returns the dimension in PostScript units.  (1 PSU = 1/72 inch, 
 * slightly smaller than a printer's point.)  If the dimension is
 * not parsable, then -1 is returned.
 */
double convert_dimension(const char *string)
	{
	const char *unitptr;		/* pointer to the units specifier */
	double number, multiplier;

	/* Skip leading space. */
	string += strspn(string, " \t");

	/* First we give the C library a wack at it.  This annoying function
	 * will try to parse the string as a number with decimal point according
	 * to the notation of the current locale.  That's fine of course, but
	 * it is a problem when we run a script written with numbers according
	 * to the notation of the "C" locale.
	 */
	number = strtod(string, (char**)&unitptr);

	/* If strtod() didn't consume all of the ASCII digits 
	 * or stopt at a decimal point, then use gu_sscanf()
	 * to try again with ASCII digits and '.' for a decimal
	 * separator.
	 */
	if(gu_isdigit(*unitptr) || *unitptr == '.')
		{
		float temp;
		if(gu_sscanf(string, "%f", &temp) != 1)
			return -1;
		number = temp;
		unitptr = string + strspn(string, "0123456789.");
		}

	unitptr += strspn(unitptr, " \t");

	if(gu_strcasecmp(unitptr, "points") == 0
				|| gu_strcasecmp(unitptr, "pt") == 0
				|| gu_strcasecmp(unitptr, "psu") == 0)
		multiplier = 1.0;

	else if(strlen(unitptr) == 0
			|| gu_strcasecmp(unitptr, "inches") == 0
			|| gu_strcasecmp(unitptr, "inch") == 0
			|| gu_strcasecmp(unitptr, "in") == 0)
		multiplier = 72.0;

	else if(gu_strcasecmp(unitptr, "centimeters") == 0
			|| gu_strcasecmp(unitptr, "cm") == 0
			|| gu_strcasecmp(unitptr, "centimetres") == 0)
		multiplier = 72.0 / 2.54;

	else if(gu_strcasecmp(unitptr, "millimeters") == 0
			|| gu_strcasecmp(unitptr, "mm") == 0
			|| gu_strcasecmp(unitptr, "millimetres") == 0)
		multiplier = 72.0 / 25.4;

	else
		return -1;

	return number * multiplier;
	} /* end of convert_dimension() */

/* end of file */
