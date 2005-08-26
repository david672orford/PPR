/*
** mouse:~ppr/src/lprsrv/uprint_argv_ppr.c
** Copyright 1995--2005, Trinity College Computing Center.
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
** Last modified 26 August 2005.
*/

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "rfc1179.h"
#include "lprsrv.h"

/*
**
** Given a filled in UPRINT structure, this function 
** will return an argument for ppr's -T switch.
*/
static const char *uprint_get_content_type_ppr(void *p)
	{
	struct UPRINT *upr = (struct UPRINT *)p;

	DODEBUG_UPRINT(("uprint_get_content_type_ppr(p=%p)", p));

	if(upr->content_type_lpr != '\0')
		{
		switch(upr->content_type_lpr)
			{
			case 'c':					/* cifplot(1) output */
				return "cif";
			case 'd':					/* TeX DVI */
				return "dvi";
			case 'f':					/* formatted */
				return (const char *)NULL;
			case 'g':					/* plot(1) data */
				return "plot";
			case 'l':					/* leave control codes */
				return (const char *)NULL;
			case 'n':					/* ditroff output */
				return "troff";
			case 'p':					/* pass thru pr(1) */
				return "pr";
			case 't':					/* old troff output */
				return "cat4";
			case 'v':					/* sun raster format */
				return "sunras";
			case 'o':					/* PostScript */
				return "postscript";
			}
		}

	else if(upr->content_type_lp != (char*)NULL)
		{
		if(strcmp(upr->content_type_lp, "simple") == 0)
			return (const char *)NULL;

		if(strcmp(upr->content_type_lp, "postscript") == 0)
			return "postscript";
		}

	return (const char *)NULL;
	} /* end of uprint_get_content_type_ppr() */

/*
** This function fills an array of character pointers with the list of
** argument for invoking ppr to print the job.
*/
int uprint_print_argv_ppr(void *p, const char **ppr_argv, int argv_size)
	{
	const char function[] = "uprint_print_argv_ppr";
	struct UPRINT *upr = (struct UPRINT *)p;
	const char *ccptr;

	int i = 0;

	/* Make sure the printer to use has been indicated: */
	if(upr->dest == (const char *)NULL)
		{
		uprint_errno = UPE_NODEST;
		uprint_error_callback("%s(): dest not set", function);
		return -1;
		}

	/* Make sure user is set: */
	if(upr->dest == (const char *)NULL)
		{
		uprint_errno = UPE_BADARG;
		uprint_error_callback("%s(): user not set", function);
		return -1;
		}

	/* Call the routine to parse System V -o options
	   and set apropriate UPRINT members because we can't
	   parse them yet in this routine. */
	uprint_parse_lp_interface_options(p);
	uprint_parse_lp_filter_modes(p);

	/* Start to build the command: */
	ppr_argv[i++] = "ppr";

	/* Select the desired queue: */
	ppr_argv[i++] = "-d";
	ppr_argv[i++] = upr->dest;

	/* Whom is the job for? */
	if(upr->user)
		{
		snprintf(upr->str_for, sizeof(upr->str_for), "%s@%s", upr->user, upr->user_domain ? upr->user_domain : "<missing>");
		ppr_argv[i++] = "-u";
		ppr_argv[i++] = upr->str_for;
		}

	/* Whom should we notify?  Build an email address.  The field upr->lpr_mailto
	   will have been filled in only if the job came from lprsrv and the user chose
	   to be notified.	(I.E., a M line in the queue file.)	 Since there are times
	   when we notify the user even when he hasn't asked to be, we will fall back on
	   the the user name if the mailto name is absent. */
	{
	const char *mailhost = upr->lpr_mailto_host ? upr->lpr_mailto_host : upr->fromhost;

	/* Some email systems may be not able to handle
	   user@localhost, so we won't use it. */
	if(mailhost && strcmp(mailhost, "localhost") == 0)
		mailhost = NULL;

	snprintf(upr->str_mailaddr, sizeof(upr->str_mailaddr), "%s%s%s",
				upr->lpr_mailto ? upr->lpr_mailto : upr->user,
				mailhost ? "@" : "",
				mailhost ? mailhost : "");
	}

	/*
	** How shall the user be notified?  This is difficult to answer since some
	** of the methods don't make sense in a network environment.
	*/
	if(upr->notify_email)			/* explicit request for e-mail */
		{
		ppr_argv[i++] = "-m";
		ppr_argv[i++] = "mail";
		ppr_argv[i++] = "-r";
		ppr_argv[i++] = upr->str_mailaddr;
		}
	else if(upr->notify_write)			/* explicit request for write */
		{
		ppr_argv[i++] = "-m";			/* These options are probably */
		ppr_argv[i++] = "write";		/* the ppr default, but if the job */
		ppr_argv[i++] = "-r";			/* is from lprsrv they won't be */
		ppr_argv[i++] = upr->user;		/* (though write is not useful then). */
		}
	else								 /* let it be followme */
		{
		ppr_argv[i++] = "-r";
		ppr_argv[i++] = upr->str_mailaddr;
		ppr_argv[i++] = "--responder-options";
		ppr_argv[i++] = "printed=no";
		}

	/* Error messages should be sent thru the responder. */
	ppr_argv[i++] = "-e";
	ppr_argv[i++] = "responder";

	snprintf(upr->str_user, sizeof(upr->str_user), "%s@%s", upr->user, upr->user_domain);
	ppr_argv[i++] = "--user";
	ppr_argv[i++] = upr->str_user;

	/* Job name?  (Such as specified by the lpr -J switch.) */
	if(upr->jobname)
		{
		ppr_argv[i++] = "--title";
		ppr_argv[i++] = upr->jobname;
		}

	/* System V lp normaly shows the request id, we might
	   be expected to do so too: */
	if(upr->show_jobid)
		ppr_argv[i++] = "--show-jobid";

	/* Number of copies: */
	if(upr->copies >= 0)
		{
		ppr_argv[i++] = "-n";
		snprintf(upr->str_numcopies, sizeof(upr->str_numcopies), "%d", upr->copies);
		ppr_argv[i++] = upr->str_numcopies;
		}

	/* Banner page? */
	if(upr->banner)						/* asked for it? */
		{
		ppr_argv[i++] = "-b";
		ppr_argv[i++] = "yes";
		}
	if(upr->nobanner)					/* please no? */
		{
		ppr_argv[i++] = "-b";
		ppr_argv[i++] = "no";
		}

	/* Width option: */
	if(upr->width != (const char *)NULL)
		{
		int width = atoi(upr->width);
		if(width <= 0 || width > 999)
			{
			uprint_errno = UPE_BADARG;
			uprint_error_callback("uprint_print_argv_ppr(): invalid width");
			return -1;
			}

		ppr_argv[i++] = "-o";
		snprintf(upr->str_width, sizeof(upr->str_width), "width=%d", width);
		ppr_argv[i++] = upr->str_width;
		}

	/* Title for pr: */
	if(upr->pr_title)
		{
		char temp[LPR_MAX_T * 2 + 1];			/* <-- room for every character backslashed and one NUL */
		int si, di;
		int c;

		/* Copy the title while backslashing quotes and backslashes. */
		for(si=di=0;si < LPR_MAX_T && (c = upr->pr_title[si]); si++)
			{
			if(c == '"' || c == '\\')
				temp[di++] = '\\';
			temp[di++] = c;
			}
		temp[di] = '\0';

		/* Pass the title as a filter option.  We don't want to pass it using
		   --title because that is how we pass "lpr -J".
		   */
		ppr_argv[i++] = "-o";
		snprintf(upr->str_pr_title, sizeof(upr->str_pr_title), "pr-title=\"%s\"", temp);
		ppr_argv[i++] = upr->str_pr_title;
		}

	/* DEC OSF Input tray: */
	if(upr->osf_LT_inputtray)
		{
		ppr_argv[i++] = "-F";
		snprintf(upr->str_inputtray, sizeof(upr->str_inputtray), "*InputSlot %s", upr->osf_LT_inputtray);
		upr->str_inputtray[11] = toupper(upr->str_inputtray[11]);
		ppr_argv[i++] = upr->str_inputtray;
		}

	/* DEC OSF Output tray: */
	if(upr->osf_GT_outputtray)
		{
		ppr_argv[i++] = "-F";
		snprintf(upr->str_outputtray, sizeof(upr->str_outputtray), "*OutputBin %s", upr->osf_GT_outputtray);
		upr->str_outputtray[11] = toupper(upr->str_outputtray[11]);
		ppr_argv[i++] = upr->str_outputtray;
		}

	/* DEC OSF Orientation.  Many PPR filters will ignore this. */
	if(upr->osf_O_orientation)
		{
		char *ptr;
		ppr_argv[i++] = "-o";
		snprintf(upr->str_orientation, sizeof(upr->str_orientation), "orientation=%s", upr->osf_O_orientation);
		for(ptr = upr->str_orientation + 12; *ptr; ptr++) *ptr = tolower(*ptr);
		ppr_argv[i++] = upr->str_orientation;
		}

	/*
	** DEC OSF Duplex settings.  Not all filters implement
	** all of these modes.  We insert a filter option so that
	** the filter may know how to format the job.  We insert a
	** -F switch, in the first three instances, so that the
	** desired mode will be selected even if the filter does
	** not implement the duplex= option; in the last three
	** instances, in order to override the duplex mode selected
	** by the filter.  This is because the last three options
	** call for margins and gutters appropriate for duplex or simplex
	** but actual printing in the opposite mode.
	*/
	if(upr->osf_K_duplex)
		{
		/* Simplex */
		if(strcmp(upr->osf_K_duplex, "one") == 0)
			{
			ppr_argv[i++] = "-o";
			ppr_argv[i++] = "duplex=none";
			ppr_argv[i++] = "-F";
			ppr_argv[i++] = "*Duplex None";
			}
		/* Duplex */
		if(strcmp(upr->osf_K_duplex, "two") == 0)
			{
			ppr_argv[i++] = "-o";
			ppr_argv[i++] = "duplex=notumble";
			ppr_argv[i++] = "-F";
			ppr_argv[i++] = "*Duplex DuplexNoTumble";
			}
		/* Duplex Tumble */
		if(strcmp(upr->osf_K_duplex, "tumble") == 0)
			{
			ppr_argv[i++] = "-o";
			ppr_argv[i++] = "duplex=tumble";
			ppr_argv[i++] = "-F";
			ppr_argv[i++] = "*Duplex DuplexTumble";
			}
		/* Format for duplex, force simplex */
		if(strcmp(upr->osf_K_duplex, "one_sided_duplex") == 0)
			{
			ppr_argv[i++] = "-o";
			ppr_argv[i++] = "duplex=notumble";
			ppr_argv[i++] = "-F";
			ppr_argv[i++] = "*Duplex None";
			}
		/* Format for duplex tumble, force simplex */
		if(strcmp(upr->osf_K_duplex, "one_sided_tumble") == 0)
			{
			ppr_argv[i++] = "-o";
			ppr_argv[i++] = "duplex=tumble";
			ppr_argv[i++] = "-F";
			ppr_argv[i++] = "*Duplex None";
			}
		/* Format for simplex, force duplex */
		if(strcmp(upr->osf_K_duplex, "two_sided_simplex") == 0)
			{
			ppr_argv[i++] = "-o";
			ppr_argv[i++] = "duplex=none";
			ppr_argv[i++] = "-F";
			ppr_argv[i++] = "*Duplex Duplex";
			}
		}

	/*
	** DEC OSF N-Up:
	**
	** This _MUST_ come after the duplex option code.  This is
	** because if N-Up is invoked we want to override the
	** duplex= filter option so that the filters will not insert
	** gutters.	 (Gutters would look silly on N-Up printed pages.)
	*/
	if(upr->nup > 0)
		{
		if(upr->nup > 999)		/* what we have room for */
			{
			uprint_errno = UPE_BADARG;
			uprint_error_callback("uprint_print_argv_ppr(): nup specifier too long");
			return -1;
			}
		ppr_argv[i++] = "-N";
		snprintf(upr->str_nup, sizeof(upr->str_nup), "%d", upr->nup);
		ppr_argv[i++] = upr->str_nup;
		ppr_argv[i++] = "-o";
		ppr_argv[i++] = "duplex=undef";
		}

	/* Filter modes (as from uprint-lp's -o switch): */
	if(upr->lp_interface_options)
		{
		}

	/* Filter modes (as from uprint-lp's -y switch): */
	if(upr->lp_filter_modes)
		{
		}

	/* Page list: */
	if(upr->lp_pagelist)
		{
		ppr_argv[i++] = "--page-list";
		ppr_argv[i++] = upr->lp_pagelist;
		}

	/* Handling: */
	if(upr->lp_handling && strcmp(upr->lp_handling, "hold") == 0)
		{
		ppr_argv[i++] = "--hold";
		}

	/* File type: */
	if((ccptr = uprint_get_content_type_ppr(p)) != (const char *)NULL)
		{
		ppr_argv[i++] = "-T";
		ppr_argv[i++] = ccptr;
		}

	return i;
	} /* end of uprint_print_argv_ppr() */

/* end of file */

