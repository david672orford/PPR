/*
** mouse:~ppr/src/libgu/malloc.c
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
** Last modified 22 November 2000.
*/

/*
** Memory allocator for PPR programs.
*/

#include "before_system.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include "gu.h"

#if 0
#define DODEBUG(a) debug a
#else
#define DODEBUG(a)
#endif

/* number of blocks currently allocated */
int gu_alloc_blocks = 0;

/*
** PPR memory allocator.
*/
void *gu_alloc(size_t number, size_t size)
    {
    void *rval;

    DODEBUG(("gu_alloc(number=%ld, size=%ld)", (long)number, (long)size));

    if((rval = malloc(size*number)) == (void*)NULL)
	libppr_throw(EXCEPTION_STARVED, "gu_alloc", "malloc() failed, errno=%d", errno);

    gu_alloc_blocks++;

    return rval;
    } /* end of gu_alloc() */

/*
** PPR memory allocator, duplicate a string.
*/
char *gu_strdup(const char *string)
    {
    char *rval;

    DODEBUG(("gu_strdup(\"%s\")", string));

    if((rval = (char*)malloc(strlen(string)+1)) == (char*)NULL)
	libppr_throw(EXCEPTION_STARVED, "gu_strdup", "malloc() failed, errno=%d", errno);

    gu_alloc_blocks++;

    strcpy(rval, string);

    return rval;
    } /* end of gu_strdup() */

/*
** PPR memory allocator, duplicate a part of a string.
*/
char *gu_strndup(const char *string, size_t len)
    {
    char *rval;

    DODEBUG(("gu_strndup(string=\"%s\", len=%d)", string, len));

    if((rval = (char*)malloc(len+1)) == (char*)NULL)
	libppr_throw(EXCEPTION_STARVED, "gu_strndup", "malloc() failed, errno=%d", errno);

    gu_alloc_blocks++;

    strncpy(rval, string, len);
    rval[len] = '\0';

    return rval;
    } /* end of gu_strndup() */

/*
** PPR memory allocator, duplicate a string into the indicated block,
** resizing it if necessary.
*/
char *gu_restrdup(char *ptr, size_t *number, const char *string)
    {
    size_t len = strlen(string);
    if(!ptr || *number <= len)		/* must be at least one greater */
    	{
	*number = (len + 1);
	if((ptr = (char*)realloc(ptr, *number)) == (char*)NULL)
	    libppr_throw(EXCEPTION_STARVED, "gu_restrdup", "realloc() failed, errno=%d", errno);
    	}
    strcpy(ptr, string);
    return ptr;
    }

/*
** PPR memory allocator, change the size of a block.
*/
void *ppr_realloc(void *ptr, size_t number, size_t size)
    {
    void *rval;

    DODEBUG(("ppr_realloc(ptr=%p, number=%d, size=%d)", ptr, number, size));

    if((rval = realloc(ptr, number*size)) == (void*)NULL)
	libppr_throw(EXCEPTION_STARVED, "ppr_realloc", "realloc() failed, errno=%d", errno);

    return rval;
    } /* end of ppr_realloc() */

/*
** PPR memory allocator, free a block.
*/
void gu_free(void *ptr)
    {
    DODEBUG(("gu_free(%p)", ptr));

    if(!ptr)
    	libppr_throw(EXCEPTION_BADUSAGE, "gu_free", "attempt to free NULL pointer");

    free(ptr);

    gu_alloc_blocks--;
    } /* end of gu_free() */

/* end of file */

