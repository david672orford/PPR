/*
** mouse:~ppr/src/libuprint/uprint_argv_lp.c
** Copyright 1995--2002, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 22 April 2002.
*/

#include "before_system.h"
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "uprint.h"
#include "uprint_private.h"

/*
** Generate an lp -o option:
*/
static int o_opt(const char *name, const char *value, char *scratch, size_t scratch_len, const char **argv, int i)
    {
    if(value != (const char *)NULL)
	{
	if(strlen(value) > (scratch_len - strlen(name)))
	    {
	    uprint_errno = UPE_BADARG;
	    uprint_error_callback("uprint_print_argv_lp(): %s specifier too long", name);
	    return -1;
	    }

	argv[i++] = "-o";
	snprintf(scratch, sizeof(scratch), "%s=%s", name, value);
	argv[i++] = scratch;
	}
    return i;
    }

/*
** Fill in the part of the lp argument list before the files
** names.  The non-error return value is the number of entries
** in the argument array that were used.
*/
int uprint_print_argv_lp(void *p, const char **lp_argv, int argv_size)
    {
    const char function[] = "uprint_print_argv_lp";
    struct UPRINT *upr = (struct UPRINT *)p;

    int i = 0;
    const char *temp;

    if(upr->dest == (const char *)NULL)
    	{
    	uprint_errno = UPE_NODEST;
    	uprint_error_callback("%s(): dest not set", function);
    	return -1;
    	}

    /* The argument list starts with the name of the program: */
    lp_argv[i++] = "lp";

    /* Specify the printer desired: */
    lp_argv[i++] = "-d";
    lp_argv[i++] = upr->dest;

    /* Possibly suppress messages: */
    if(! upr->show_jobid)
	lp_argv[i++] = "-s";

    /* Was the number of copies specified? */
    if(upr->copies >= 0)
    	{
    	lp_argv[i++] = "-n";
	snprintf(upr->str_numcopies, sizeof(upr->str_numcopies), "%d", upr->copies);
	lp_argv[i++] = upr->str_numcopies;
	}

    /* Jobname in the style of lpr -J: */
    if(upr->jobname != (const char *)NULL)
	{
	lp_argv[i++] = "-t";
	lp_argv[i++] = upr->jobname;
	}

    /* If the priority has been specified, specify it to lp: */
    if(upr->priority != -1)
	{
	if(upr->priority < 0 || upr->priority > 39)
	    {
	    uprint_errno = UPE_BADARG;
	    uprint_error_callback("%s(): priority %d is out of range 0 to 39", function, upr->priority);
	    return -1;
	    }
	snprintf(upr->str_priority, sizeof(upr->str_numcopies), "%d", upr->priority);
	lp_argv[i++] = "-q";
	lp_argv[i++] = upr->str_priority;
	}

    /* Should we write to users terminal or send
       email on job completion? */
    if(upr->notify_write)
    	lp_argv[i++] = "-w";
    if(upr->notify_email)
	lp_argv[i++] = "-m";

    /*
    ** If the user has asked for suppression of the
    ** banner page, try to suppress it now.
    */
    if(upr->nobanner)
    	{
    	lp_argv[i++] = "-o";
    	lp_argv[i++] = "nobanner";
    	}

    /* The nofilebreak option, whatever that does: */
    if(! upr->filebreak)
    	{
    	lp_argv[i++] = "-o";
    	lp_argv[i++] = "nofilebreak";
    	}

    /* Various -o options: */
    i = o_opt("width", upr->width, upr->str_width, sizeof(upr->str_width), lp_argv, i);
    i = o_opt("length", upr->length, upr->str_length, sizeof(upr->str_length), lp_argv, i);
    i = o_opt("cpi", upr->cpi, upr->str_cpi, sizeof(upr->str_cpi), lp_argv, i);
    i = o_opt("lpi", upr->lpi, upr->str_lpi, sizeof(upr->str_lpi), lp_argv, i);

    /* -o options supplied as such: */
    if(upr->lp_interface_options)
    	{
    	lp_argv[i++] = "-o";
    	lp_argv[i++] = upr->lp_interface_options;
    	}

    /* Filter options (-y switch)? */
    if(upr->lp_filter_modes)
    	{
    	lp_argv[i++] = "-y";
    	lp_argv[i++] = upr->lp_filter_modes;
    	}

    /* Character set specified? */
    if(upr->charset)
    	{
    	lp_argv[i++] = "-S";
    	lp_argv[i++] = upr->charset;
	}

    /* Specific form requested? */
    if(upr->form)
    	{
    	lp_argv[i++] = "-f";
    	lp_argv[i++] = upr->form;
	}

    /* Page ranges? */
    if(upr->lp_pagelist)
    	{
    	lp_argv[i++] = "-P";
    	lp_argv[i++] = upr->lp_pagelist;
	}

    /* Special handling? */
    if(upr->lp_handling)
    	{
    	lp_argv[i++] = "-H";
    	lp_argv[i++] = upr->lp_handling;
    	}

    /*
    ** If we know the correct option, select the
    ** content type.
    */
    if((temp = uprint_get_content_type_lp(p)) != (const char *)NULL)
    	{
	if(strcmp(temp, "-r") == 0)
	    {
	    lp_argv[i++] = "-r";
	    }
	else
	    {
    	    lp_argv[i++] = "-T";
    	    lp_argv[i++] = temp;
    	    }
    	}

    return i;
    } /* end of uprint_print_argv_lp() */

/* end of file */
