/*
** mouse:~ppr/src/libuprint/uprint_argv_ppr.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 5 June 2001.
*/

#include "before_system.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "uprint.h"
#include "uprint_private.h"

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
       and set appropriate UPRINT members because we can't
       parse them yet in this routine. */
    uprint_parse_lp_interface_options(p);
    uprint_parse_lp_filter_modes(p);

    /* Start to build the command: */
    ppr_argv[i++] = "ppr";

    /* Select the desired queue: */
    ppr_argv[i++] = "-d";
    ppr_argv[i++] = upr->dest;

    /* Use the switchset macro.  We use this early on
       so that explicit options can override options
       set in the switchset. */
    /* ppr_argv[i++] = "-I"; */		/* -I is obsolete */

    /* Identify users by username.  (This will make the queue listings
       look more like lpr's.  The -u switch requires no special
       privledge.  The -f below will do the job only if the invoking
       user has the necessary privledge.) */
    ppr_argv[i++] = "-u";
    ppr_argv[i++] = "yes";

    /* Whom is the job for? */
    if(upr->user)
    	{
	const char *p = upr->user;	/* default is just the username */

    	if(upr->from_format)
    	    {
	    if(strcmp(upr->from_format, "$user@$host") == 0)
		{
		snprintf(upr->str_for, sizeof(upr->str_for), "%s@%s", upr->user, upr->fromhost ? upr->fromhost : "<missing>");
		p = upr->str_for;
		}
	    else if(strcmp(upr->from_format, "$user@$proxyclass") == 0)
		{
		snprintf(upr->str_for, sizeof(upr->str_for), "%s@%s", upr->user, upr->proxy_class ? upr->proxy_class : "<missing>");
		p = upr->str_for;
		}
    	    }

	ppr_argv[i++] = "-f";
	ppr_argv[i++] = p;
	}

    /* Whom should we notify?  Build an email address.  The field upr->lpr_mailto
       will have been filled in only if the job came from lprsrv and the user chose
       to be notified.  (I.E., a M line in the queue file.)  Since there are times
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
    if(upr->ppr_responder)
	{
	ppr_argv[i++] = "--responder";
	ppr_argv[i++] = upr->ppr_responder;
	if(upr->ppr_responder_address)
	    {
	    ppr_argv[i++] = "--responder-address";
	    ppr_argv[i++] = upr->ppr_responder_address;
	    }
	if(upr->ppr_responder_options)
	    {
	    ppr_argv[i++] = "--responder-options";
	    ppr_argv[i++] = upr->ppr_responder_options;
	    }
	}
    else if(upr->notify_email)		/* explicit request for email */
    	{
    	ppr_argv[i++] = "-m";
    	ppr_argv[i++] = "mail";
    	ppr_argv[i++] = "-r";
    	ppr_argv[i++] = upr->str_mailaddr;
    	}
    else if(upr->notify_write)		/* explicit request for write */
    	{
	ppr_argv[i++] = "-m";  		/* These options are probably */
	ppr_argv[i++] = "write";	/* the ppr default, but if the job */
	ppr_argv[i++] = "-r";		/* is from lprsrv they won't be */
	ppr_argv[i++] = upr->user;	/* (though write is not useful then). */
    	}
    else
    	{
    	ppr_argv[i++] = "-m";
    	ppr_argv[i++] = "mail";
    	ppr_argv[i++] = "-r";
    	ppr_argv[i++] = upr->str_mailaddr;
	ppr_argv[i++] = "--responder-options";
	ppr_argv[i++] = "printed=no";
    	}

    /* Error messages should be sent thru the responder. */
    ppr_argv[i++] = "-e";
    ppr_argv[i++] = "responder";

    /* Is this a proxy job?  If so, use the -X switch to
       identify the party we are acting for. */
    if(upr->proxy_class)
	{
	snprintf(upr->str_principal, sizeof(upr->str_principal), "%s@%s", upr->user, upr->proxy_class);
	ppr_argv[i++] = "-X";
	ppr_argv[i++] = upr->str_principal;
	}

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
    if(upr->banner)			/* asked for it? */
	{
	ppr_argv[i++] = "-b";
	ppr_argv[i++] = "yes";
	}
    if(upr->nobanner)			/* please no? */
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
	char temp[LPR_MAX_T * 2 + 1];
	int si, di;
	int c;

	for(si=di=0;si < LPR_MAX_T && (c = upr->pr_title[si]); si++)
	    {
	    if(c == '"' || c == '\\')
	    	temp[di++] = '\\';
	    temp[di++] = c;
	    }

	ppr_argv[i++] = "-o";
	snprintf(upr->str_pr_title, sizeof(upr->str_pr_title), "title=\"%s\"", temp);
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
    ** gutters.  (Gutters would look silly on N-Up printed pages.)
    */
    if(upr->nup > 0)
	{
	if(upr->nup > 999)	/* what we have room for */
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

