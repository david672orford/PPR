/*
** mouse:~ppr/src/libuprint/uprint_argv_bsd.c
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
** Last modified 14 May 2003.
*/

#include "config.h"
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "uprint.h"
#include "uprint_private.h"

/*
** This function returns the switch which should be used
** to indicate the content type when invoking lpr.
** It will return a NULL character if none should be
** used.
*/
char uprint_get_content_type_lpr(void *p)
	{
	struct UPRINT *upr = (struct UPRINT *)p;

	/* If it was set directly, */
	if(upr->content_type_lpr != '\0')
		return upr->content_type_lpr;

	/* If we can convert an lp content type spec, */
	if(upr->content_type_lp != (char*)NULL)
		{
		struct LP_LPR_TYPE_XLATE *p;

		for(p = lp_lpr_type_xlate; p->lpname != (const char *)NULL || p->lprcode != '\0'; p++)
			{
			if(strcmp(p->lpname, upr->content_type_lp) == 0)
				return p->lprcode;
			}
		}

	return '\0';
	} /* end of uprint_get_content_type_lpr() */

/* end of file */
