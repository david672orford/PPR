/*
** mouse:~ppr/src/ppr/ppr_req.c
** Copyright 1995--2002, Trinity College Computing Center.
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
** Last modified 15 November 2002.
*/

/*
** DSC Requirement comment handling routines.
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

	if(strcmp(req_name, "collate") == 0)		/* If it is collated copies */
		{										/* and we should pay attention */
		if(read_copies)							/* to copies data, */
			qentry.opts.collate = TRUE;			/* Then, set collate to true */
		return;									/* <-- and filter out */
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
		return;									/* <-- filter out */
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

		things_space_check();			/* make room in the array */

		things[x].th_type = TH_REQUIREMENT;
		things[x].R_Flags = reftype;
		things[x].th_ptr = (void*)gu_strdup(req_name);

		thing_count++;
		}

	else							/* if already present, */
		{							/* add this reference to ref flags */
		things[x].R_Flags |= reftype;
		}

	if(reftype == REQ_PAGE)			/* If it was in a %%PageRequirement: comment, */
		set_thing_bit(x);			/* set the bit for this page. */

	} /* end of requirement() */

/*
** Delete a requirement we don't need after all.  This deletes all 
** requirements of the specified type.  In other words, calling
**
** delete_requirement("duplex");
**
** will delete both "duplex" and "duplex(tumble)".
**
** One possible use of this function is to delete old duplex requirements when
** we change the duplex mode of a job.
*/
void delete_requirement(const char req_name[])
	{
	int x;
	const char *sptr;
	const char *nptr;

	for(x=0; x<thing_count; x++)		/* look for this one */
		{
		if(things[x].th_type != TH_REQUIREMENT)
			continue;

		sptr = (char*)things[x].th_ptr; /* pointer to requirement */
		nptr = req_name;				/* name of what deleteing */

		while(*sptr)					/* compare */
			{
			if(*(sptr++) != *(nptr++))	/* is mis-match, stop here */
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

	for(x=0;x<thing_count;x++)						/* try each thing */
		{
		if(things[x].th_type != TH_REQUIREMENT)		/* only interested in */
			continue;								/* requirements */

		if( things[x].R_Flags & REQ_DELETED )		/* ignore those reqs */
			continue;								/* we have deleted */

		if( ! (things[x].R_Flags & REQ_PAGE) )		/* only if in this page */
			continue;

		if(started)									/* if 2nd or subsequent */
			{										/* then use cont line */
			fprintf(page_comments,"%%%%+ %s\n",(char*)things[x].th_ptr);
			}
		else										/* otherwise, use 1st */
			{										/* line format */
			fprintf(page_comments,"%%%%PageRequirements: %s\n",
				(char*)things[x].th_ptr);
			started=TRUE;
			}

		things[x].R_Flags &= ~ REQ_PAGE;			/* clear for next page */
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

