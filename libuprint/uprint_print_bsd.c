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

#include "before_system.h"
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
**
** This is not static because it is used by uprint_print_rfc1179.c.
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

/*
** Build the argument list for lpr.  The list of file names
** is not included.  The return value is the number of
** argument list entries used.
*/
int uprint_print_argv_bsd(void *p, const char **lpr_argv, int argv_size)
	{
	struct UPRINT *upr = (struct UPRINT *)p;

	int i = 0;
	int temp;

	if(upr->dest == (const char *)NULL)
		{
		uprint_error_callback("uprint_print_argv_lpr(): dest not set");
		uprint_errno = UPE_NODEST;
		return -1;
		}

	/* Get lp -o and -y converted. */
	uprint_parse_lp_interface_options(p);
	uprint_parse_lp_filter_modes(p);

	/* Start to build a command line. */
	lpr_argv[i++] = "lpr";

	/* Specify the desired printer: */
	lpr_argv[i++] = "-P";
	lpr_argv[i++] = upr->dest;

	/*
	** Pass on the user name information, even though
	** LPR may not think we have sufficent priviledge
	** to be allowed to declare ourself to be representing
	** somebody else.  (See the lpr(1) man page.)  Also,
	**
	** I have been unable to get this feature to work,
	** even when the user running lpr is root.
	*/
	#if 0
	if(upr->user != (const char *)NULL)
		{
		lpr_argv[i++] = "-U";
		lpr_argv[i++] = upr->user;
		}
	#endif

	/* lpr -J */
	if(upr->jobname)
		{
		lpr_argv[i++] = "-J";
		lpr_argv[i++] = upr->jobname;
		}

	/* lpr -C */
	if(upr->lpr_class)
		{
		lpr_argv[i++] = "-C";
		lpr_argv[i++] = upr->lpr_class;
		}

	/* Possibly specify the content type: */
	if((temp = uprint_get_content_type_lpr(p)))
		{
		snprintf(upr->str_typeswitch, sizeof(upr->str_typeswitch), "-%c", temp);
		lpr_argv[i++] = upr->str_typeswitch;
		}

	/* lpr -T */
	if(upr->pr_title)
		{
		lpr_argv[i++] = "-T";
		lpr_argv[i++] = upr->pr_title;
		}

	/* lpr -w (width for pr) */
	if(upr->width)
		{
		lpr_argv[i++] = "-w";
		lpr_argv[i++] = upr->width;
		}

	/* lpr -i (indent) */
	if(upr->indent)
		{
		lpr_argv[i++] = "-i";
		lpr_argv[i++] = upr->indent;
		}

	/* Troff fonts: */
	if(upr->troff_1)
		{
		lpr_argv[i++] = "-1";
		lpr_argv[i++] = upr->troff_1;
		}
	if(upr->troff_2)
		{
		lpr_argv[i++] = "-2";
		lpr_argv[i++] = upr->troff_2;
		}
	if(upr->troff_3)
		{
		lpr_argv[i++] = "-3";
		lpr_argv[i++] = upr->troff_3;
		}
	if(upr->troff_4)
		{
		lpr_argv[i++] = "-4";
		lpr_argv[i++] = upr->troff_4;
		}

	/*
	** If the number of copies is set to a value other
	** than the default (-1), then pass a -# switch
	** to lpr.
	*/
	if(upr->copies > 0)
		{
		lpr_argv[i++] = "-#";
		snprintf(upr->str_numcopies, sizeof(upr->str_numcopies), "%d", upr->copies);
		lpr_argv[i++] = upr->str_numcopies;
		}

	/* If banner suppression requested, use the -h switch: */
	if(upr->nobanner)
		lpr_argv[i++] = "-h";

	/*
	** If email should be sent on job completion,
	** use the -m switch.
	*/
	if(upr->notify_email)
		lpr_argv[i++] = "-m";

	/*
	** Use the -r switch if the files should
	** be unlinked after reading them.
	*/
	if(upr->unlink)
		lpr_argv[i++] = "-r";

	/*
	** If the files needn't be copied into the queue
	** immediately, then use the -s (symbolic link) switch.
	*/
	if(! upr->immediate_copy)
		lpr_argv[i++] = "-s";

	/*
	** Extensions present in DEC OSF/1 3.2 lpr.
	*/
	#ifdef LPR_EXTENSIONS_OSF
	if(upr->osf_LT_inputtray)
		{
		lpr_argv[i++] = "-<";
		lpr_argv[i++] = upr->osf_LT_inputtray;
		}

	if(upr->osf_GT_outputtray)
		{
		lpr_argv[i++] = "->";
		lpr_argv[i++] = upr->osf_GT_outputtray;
		}

	if(upr->osf_O_orientation)
		{
		lpr_argv[i++] = "-O";
		lpr_argv[i++] = upr->osf_O_orientation;
		}

	if(upr->osf_K_duplex)
		{
		lpr_argv[i++] = "-K";
		lpr_argv[i++] = upr->osf_K_duplex;
		}

	if(upr->nup > 0)
		{
		if(upr->nup > 999)		/* what we have room for */
			{
			uprint_errno = UPE_BADARG;
			uprint_error_callback("uprint_print_argv_lpr(): nup specifier too long");
			return -1;
			}
		lpr_argv[i++] = "-N";
		snprintf(upr->str_nup, sizeof(upr->str_nup), "%d", upr->nup);
		lpr_argv[i++] = upr->str_nup;
		}
	#endif

	return i;
	} /* end of uprint_print_argv_bsd() */

/* end of file */
