/*
** mouse:~ppr/src/ppr/ppr_req.c
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
** Last modified 23 May 2001.
*/

/*
** Requirement comment handling routines.
*/

#include "before_system.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "ppr.h"

/*
** This is called for each "%%Requirements:"
** or "%%PageRequirements:" comment.
**
** The parameter "reftype" is either REQ_DOC or REQ_PAGE.
*/
void requirement(int reftype, const char req_name[])
    {
    int x;
    char *sptr;
    char scratch[9];

    #ifdef DEBUG_REQUIREMENTS
    printf("Requirement: %s, reftype=%d\n", req_name, reftype);
    #endif

    /* Convert the old style punch options to the new ones. */
    if(gu_sscanf(req_name, "punch%d", &x) == 1 && x < 10)
    	{
    	if(qentry.attr.DSClevel >= 3.0)
    	    warning(WARNING_PEEVE, "Requirement punch%d should be punch(%d) in DSC >= 3.0", x, x);
	snprintf(scratch, sizeof(scratch), "punch(%d)", x);
	req_name = scratch;
	}

    if(strcmp(req_name, "collate") == 0)	/* If it is collated copies */
	{					/* and we should pay attention */
	if(read_copies)				/* to copies data, */
	    qentry.opts.collate = TRUE;		/* Then, set collate to true */
	return;
	}

    if(strncmp(req_name, "numcopies(", 10) == 0)
	{
	if(read_copies)
	    {
	    int x;
	    if((x = atoi(&req_name[10])) < 1)
		warning(WARNING_SEVERE, "Unreasonable requirement \"%s\" ignored", req_name);
	    else
	    	qentry.opts.copies = x;
	    }
	return;
	}

    /* Look for this requirement in the list of known ones. */
    for(x=0; x<thing_count; x++)
	{
	if(things[x].th_type != TH_REQUIREMENT)
	    continue;

	sptr = (char*)things[x].th_ptr;
	if(strcmp(sptr, req_name) == 0)
	    break;
	}

    /* If it is not found, */
    if(x == thing_count)
	{
	/* A page requirement should have been mentioned in the header. */
	if(reftype == REQ_PAGE)
	    warning(WARNING_PEEVE, "Requirement \"%s\" missing from \"%%%%Requirements:\" comment", req_name);

	things_space_check();		/* make room in the array */

	things[x].th_type = TH_REQUIREMENT;
	things[x].R_Flags = reftype;
	things[x].th_ptr = (void*)gu_strdup(req_name);

	thing_count++;
	}

    else                            /* if already present, */
	{                           /* add this reference to ref flags */
	things[x].R_Flags |= reftype;
	}

    if(reftype == REQ_PAGE)	    /* If it was in a %%PageRequirement: comment, */
	set_thing_bit(x);	    /* set the bit for this page. */

    } /* end of requirement() */

/*
** Delete a requirement we don't need after all.
** This deletes all requirements of the specified type.
**
** This function is called when the -F switch is used
** to specify a Duplex requirement.  It is used to delete
** conflicting requirements.
*/
void delete_requirement(const char req_name[])
    {
    int x;
    const char *sptr;
    const char *nptr;

    for(x=0; x<thing_count; x++)	/* look for this one */
	{
	if(things[x].th_type != TH_REQUIREMENT)
	    continue;

	sptr = (char*)things[x].th_ptr;	/* pointer to requirement */
	nptr = req_name;		/* name of what deleteing */

	while(*sptr)                    /* compare */
	    {
	    if(*(sptr++) != *(nptr++))  /* is mis-match, stop here */
		break;
	    }

	if(*sptr=='\0' && (*nptr=='(' || *nptr=='\0') )
	    things[x].R_Flags|=REQ_DELETED; /* if all matched, delete */
	}

    } /* end of delete_requirement() */

/*
** Dump PostScript requirements for the current page.
*/
void dump_page_requirements(void)
    {
    int x;
    int started=FALSE;

    for(x=0;x<thing_count;x++)                      /* try each thing */
	{
	if(things[x].th_type != TH_REQUIREMENT)     /* only interested in */
	    continue;                               /* requirements */

	if( things[x].R_Flags & REQ_DELETED )       /* ignore those reqs */
	    continue;                               /* we have deleted */

	if( ! (things[x].R_Flags & REQ_PAGE) )      /* only if in this page */
	    continue;

	if(started)                                 /* if 2nd or subsequent */
	    {                                       /* then use cont line */
	    fprintf(page_comments,"%%%%+ %s\n",(char*)things[x].th_ptr);
	    }
	else                                        /* otherwise, use 1st */
	    {                                       /* line format */
	    fprintf(page_comments,"%%%%PageRequirements: %s\n",
		(char*)things[x].th_ptr);
	    started=TRUE;
	    }

	things[x].R_Flags &= ~ REQ_PAGE;            /* clear for next page */
	}
    } /* end of dump_page_requirements() */

/*
** Write one "Req:" lines for each requirement.
*/
void write_requirement_lines(FILE *out, int fragment)
    {
    int x;

    for(x=0;x<thing_count;x++)
	{
	if( things[x].th_type != TH_REQUIREMENT )
	    continue;

	if( ! is_thing_in_current_fragment(x, fragment) )
	    continue;

	fprintf(out,"Req: %s\n",(char*)things[x].th_ptr);
	}

    } /* end of write_requirement_lines() */

/* end of file */

