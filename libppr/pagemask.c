/*
** mouse:~ppr/src/libppr/pagemask.c
** Copyright 1995--2001, Trinity College Computing Center.
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
** Last modified 5 September 2001.
*/

#include "before_system.h"
#include <memory.h>
#include <ctype.h>
#include <stdlib.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"

/* Read an ASCII string and store it in job as the mask of pages
   to be printed. */
int pagemask_encode(struct QFileEntry *job, const char pages[])
    {
    int bytes;
    int page, lastsep;
    const char *p;

    /* If there is an old page mask, deallocate it. */
    if(job->PageMask)
    	gu_free(job->PageMask);

    /* Allocate enough memory and set it to all zeros. */
    bytes = (job->attr.pages + 5) / 6;
    job->PageMask = gu_alloc(bytes+1, 1);
    memset(job->PageMask, 0, bytes+1);

    /* This is the parser loop. */
    for(p=pages, lastsep=0, page=1; *p; )
    	{
	/* Spaces have no value except to terminate numbers. */
	if(isspace(*p))
	    {
	    p++;
	    continue;
	    }

	/* If a number, interpretation depends on context. */
	if(isdigit(*p))
	    {
	    /* Parse the digits and move the read pointer past them. */
	    int value = atoi(p);
	    p += strspn(p, "0123456789");

	    /* If it is a hyphen and nothing proceded it and it
	       wasn't at the start of the string, it is an error. */
	    if(lastsep == '-')
	    	{
	    	if(page == 0)
		    return -1;
		}

	    /* If the last separator wasn't a hyphen, this is
	       a starting value or a single value. */
	    else
		page = value;

	    /* Step from the starting value to the ending value
	       or the end of the document, whichever comes first,
	       setting bits as we go. */
            for( ; page <= value && page <= job->attr.pages; page++)
            	{
		int offset = (page - 1);
		int bytenum = offset / 6;
		int bitnum = offset % 6;
		job->PageMask[bytenum] |= (1 << bitnum);
            	}

	    /* If that was the end of a range, close it. */
	    if(lastsep == '-')
	    	{
	    	lastsep = 0;
	    	page = 0;
	    	}

	    continue;
	    }

	/* Commas and hyphens change the interpretation of numbers
	   that follow them, so remember them. */
	if(*p == ',' || *p == '-')
	    {
	    lastsep = *p;
	    p++;
	    continue;
	    }

	/* Other characters are invalid. */
	return -1;
    	}

    {
    int x;
    for(x=0; x<bytes; x++)
	{
    	job->PageMask[x] += 33;
    	}
    }

    return 0;
    }

/* Print a human readable representation of the list of the
   page mask. */
void pagemask_print(const struct QFileEntry *job)
    {
    if(job->PageMask)
	{
	int x;
	for(x=1; x<job->attr.pages; x++)
	    {
	    int offset = (x - 1);
	    int bytenum = offset / 6;
	    int bitnum = offset % 6;
	    if((job->PageMask[bytenum] - 33) & (1 << bitnum))
	    	printf("%d ", x);
	    }

	}
    } /* end of pagemask_print() */

/* Return the value of a bit in the array of bits that represents the pages.
   The value must be in the range 1-job->attr.pages.
   */
int pagemask_get_bit(const struct QFileEntry *job, int page)
    {
    /* If there is no page mask, we print all pages. */
    if(!job->PageMask)
    	return 1;

    /* We don't want to handle this sort of error here. */
    if(page <= 0 || page > job->attr.pages)
    	return 1;

    /* Find the right bit and return one if it is set, zero if it isn't. */
    {
    int offset = (page - 1);
    int bytenum = offset / 6;
    int bitnum = offset % 6;
    if((job->PageMask[bytenum] - 33) & (1 << bitnum))
    	return 1;
    else
	return 0;
    }
    } /* end of pagemask_get_bit() */

/* Return the number of bits that are set.  This will be the
   number of pages that will be printed (per copy of course).
   */
int pagemask_count(const struct QFileEntry *job)
    {
    /* If there is no page mask, we print all pages. */
    if(!job->PageMask)
    	return job->attr.pages;
    } /* end of pagemask_count() */

/* end of file */
