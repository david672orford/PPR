/*
** mouse:~ppr/src/libuprint/uprint_argv_lpr.c
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
** Last modified 14 February 2000.
*/

#include "before_system.h"
#include "gu.h"
#include "global_defines.h"

#include "uprint.h"
#include "uprint_private.h"

/*
** Build the argument list for lpr.  The list of file names
** is not included.  The return value is the number of
** argument list entries used.
*/
#ifdef HAVE_LPR
int uprint_print_argv_lpr(void *p, const char **lpr_argv, int argv_size)
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
	if(upr->nup > 999)	/* what we have room for */
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
    } /* end of uprint_print_argv_lpr() */
#endif

/* end of file */
