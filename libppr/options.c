/*
** mouse:~ppr/src/libppr/options.c
** Copyright 1995--2003, Trinity College Computing Center.
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
** Last modified 31 October 2003.
*/

/*
** This is a generic options parser for name=value pair options.  An
** additional routine for reporting option parsing errors in filters
** is in the file foptions.c.
*/

#include "before_system.h"
#include <string.h>
#include <ctype.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

/*
** Reset these routines, giving them a new set of options
** to parse.
*/
void options_start(const char *s, struct OPTIONS_STATE *o)
	{
	o->magic = 689;
	o->options = s;						/* set ptr to start of options */
	o->error = "<NO ERROR>";			/* set error message to dummy value */
	o->index = strspn(s, " \t");		/* skip leading space */
	o->index_of_name = o->index;
	o->index_of_prev_name = o->index;
	o->next_time_skip = 0;
	} /* end of options_start() */

/*
** Get the next name and value.  Return 1 if we are sucessfull,
** zero if we are at the end of the string, and -1 if there is an
** error.
**
** On each return, o->error points to the error message (if applicable),
** o->options points to the string which was passed to options_start(),
** o->index points to the point at which the error was found
** and o->index_of_prev_word points a little way back from the error.
*/
int options_get_one(struct OPTIONS_STATE *o, char *name, int maxnamelen, char *value, int maxvaluelen)
	{
	int namelen, valuelen;
	const char *ptr;

	if(o->magic != 689)
		{
		o->error = "options_get_one(): options_start() must be called first";
		return -1;
		}

	o->index += o->next_time_skip;
	o->next_time_skip = 0;
	o->index_of_prev_name = o->index_of_name;
	o->index_of_name = o->index;
	ptr = o->options + o->index;

	if(*ptr == '\0')					/* If at end of options, */
		return 0;						/* so indicate. */

	/* determine length til equals sign, space, or tab */
	namelen = strcspn(ptr, "= \t");

	if(namelen > (maxnamelen - 1))
		{
		o->error = N_("Option name is illegaly long");
		return -1;
		}

	if(namelen == 0)
		{
		o->error = N_("Missing option name");
		return -1;
		}

	gu_strlcpy(name, ptr, maxnamelen);	/* copy the name, */
	name[namelen] = '\0';				/* and NULL terminate it */

	ptr += namelen;						/* move beyond name */
	o->index += namelen;

	if(*ptr != '=')						/* if no equals sign, */
		{								/* that is a problem */
		if(ptr[strspn(ptr, " \t")] == '=')
			o->error = N_("No spaces allowed before equals sign");
		else
			o->error = N_("Equals sign expected");

		return -1;
		}

	ptr++;								/* move beyond equals sign */
	o->index++;

	if(isspace(*ptr))
		{
		o->error = N_("No spaces allowed after equals sign");
		return -1;
		}

	if(*ptr != '"')
		{
		/* Length of value is length until next space or tab. */
		valuelen = strcspn(ptr, " \t");

		if(valuelen > (maxvaluelen - 1))
			{
			o->error = N_("Option value is too long");
			return -1;
			}

		if(valuelen == 0)
			{
			o->error = N_("Empty value is not allowed");
			return -1;
			}

		/* Copy the value and NULL terminate it. */
		gu_strlcpy(value, ptr, maxvaluelen);
		}
	else
		{
		int x;
		for(x=0,valuelen=1,ptr++; *ptr != '"'; valuelen++)
			{
			if(!*ptr)
				{
				o->error = N_("Unclosed quote");
				return -1;
				}
			if(*ptr == '\\' && (ptr[1] == '"' || ptr[1] == '\\'))
				{
				ptr++;
				valuelen++;
				}
			if(x >= maxvaluelen)
				{
				o->error = N_("Option value is too long");
				return -1;
				}
			value[x++] = *ptr++;
			}
		value[x] = '\0';
		ptr++;
		valuelen++;
		}	 

	/* Determine the distance to next name=value pair.
	   This will be used at the start of the next call. */
	o->next_time_skip = valuelen;
	o->next_time_skip += strspn(&ptr[o->next_time_skip], " \t");

	return 1;
	} /* end of options_get_one() */

/* end of file */

