/*
** mouse:~ppr/src/libuprint/uprint_sysv.c
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
** Last modified 18 February 2003.
*/

#include "before_system.h"
#include <string.h>
#include "gu.h"
#include "uprint.h"
#include "uprint_private.h"

/*
** Here we parse the printer options. and fill in the
** cooresponding portions of the UPRINT structure.
** Notice that we ignore ones we don't recognize.
*/
int uprint_parse_lp_interface_options(void *p)
	{
	struct UPRINT *upr = (struct UPRINT *)p;
	DODEBUG(("uprint_parse_lp_interface_options(p=%d)", p));

	if(upr->lp_interface_options)
		{
		char *s = upr->lp_interface_options;
		char *s2;
		while((s2=strtok(s, " \t")) != (char*)NULL)
			{
			if(strcmp(s2, "nobanner") == 0)
				uprint_set_nobanner(upr, TRUE);
			else if(strcmp(s2, "nofilebreak") == 0)
				uprint_set_filebreak(upr, FALSE);
			else if(strncmp(s2, "length=", 7) == 0)
				uprint_set_length(upr, &s2[7]);
			else if(strncmp(s2, "width=", 6) == 0)
				uprint_set_width(upr, &s2[6]);
			else if(strncmp(s2, "lpi=", 4) == 0)
				uprint_set_lpi(upr, &s2[4]);
			else if(strncmp(s2, "cpi=", 4) == 0)
				uprint_set_cpi(upr, &s2[4]);
			else if(strncmp(s2, "stty=", 5) == 0)
				{	 }

			s = (char*)NULL;
			}
		}

	return 0;
	} /* end of uprint_parse_lp_interface_options() */

/*
** And here we process a mode list if I can
** remember what a mode list is.
*/
int uprint_parse_lp_filter_modes(void *p)
	{
	struct UPRINT *upr = (struct UPRINT *)p;
	DODEBUG(("uprint_parse_lp_filter_modes(p=%d)", p));

	if(upr->lp_filter_modes)
		{
		char *s = upr->lp_filter_modes;
		char *s2;
		while((s2 = strtok(s, " \t")))
			{


			s = (char*)NULL;
			}
		}

	return 0;
	} /* end of uprint_parse_lp_filter_modes() */

/* end of file */

