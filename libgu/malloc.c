/*
** mouse:~ppr/src/libgu/malloc.c
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
** Last modified 10 March 2003.
*/

/*! \file
	\brief memory allocator for PPR programs

This file contains the routines which PPR uses to allocate and free memory.

The normal Unix memory allocation routines return a NULL pointer if they
fail to allocate the requested memory.  Since it is very rare that a Unix
system will refuse to allocate any reasonable amount of memory, programers
are often emboldened to ignore the possiblity of malloc() and friends
returning NULL.  We don't ignore such a possibility, but since a memory
allocation failure is highly unlikely, PPR programs treats it as a fatal
error, but, rather than test the return value of malloc() after each call,
the calls are encapsulated in special functions which test the return value
and abort the program if the allocation fails.

A program calling these functions must provide a function called
libppr_throw(). This function should print an error message and exit.  The
first argument will be EXCEPTION_STARVED, the second will be the name of the
function that failed to allocate memory, the third is a printf() style
format string for the error message.  Any remaining arguments are the values
specified by the format string.

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

/** allocate a memory block to hold an array

The function gu_alloc() takes two arguments.  The first is the number of items
to allocate, the second is the size of each in bytes.  This function will return
a void pointer to the allocated memory.  The memory is not initialized.

*/
void *gu_alloc(size_t number, size_t size)
	{
	void *rval;

	DODEBUG(("gu_alloc(number=%ld, size=%ld)", (long)number, (long)size));

	if((rval = malloc(size*number)) == (void*)NULL)
		libppr_throw(EXCEPTION_STARVED, "gu_alloc", "malloc() failed, errno=%d (%s)", errno, gu_strerror(errno));

	gu_alloc_blocks++;

	return rval;
	} /* end of gu_alloc() */

/** duplicate a string

The function gu_strdup() takes a string pointer as its sole argument
and returns a pointer to a new copy of the string.

*/
char *gu_strdup(const char *string)
	{
	char *rval;

	DODEBUG(("gu_strdup(\"%s\")", string));

	if((rval = (char*)malloc(strlen(string)+1)) == (char*)NULL)
		libppr_throw(EXCEPTION_STARVED, "gu_strdup", "malloc() failed, errno=%d (%s)", errno, gu_strerror(errno));

	gu_alloc_blocks++;

	strcpy(rval, string);

	return rval;
	} /* end of gu_strdup() */

/** duplicate the initial segment of a string

The function gu_strndup() takes a string pointer and a maximum length as its
arguments.  It returns a pointer to a new string containing a copy of the
string truncated to the maximum length.

*/
char *gu_strndup(const char *string, size_t len)
	{
	char *rval;

	DODEBUG(("gu_strndup(string=\"%s\", len=%d)", string, len));

	if((rval = (char*)malloc(len+1)) == (char*)NULL)
		libppr_throw(EXCEPTION_STARVED, "gu_strndup", "malloc() failed, errno=%d (%s)", errno, gu_strerror(errno));

	gu_alloc_blocks++;

	strncpy(rval, string, len);
	rval[len] = '\0';

	return rval;
	} /* end of gu_strndup() */

/** copy a string into a preexisting block, resizing if necessary

*/
char *gu_restrdup(char *ptr, size_t *number, const char *string)
	{
	size_t len = strlen(string);

	DODEBUG(("gu_restrdup(ptr=%p, number=%d, string=\"%s\")", ptr, number, string));

	if(!ptr || *number <= len)			/* must be at least one greater */
		{
		*number = (len + 1);
		if((ptr = (char*)realloc(ptr, *number)) == (char*)NULL)
			libppr_throw(EXCEPTION_STARVED, "gu_restrdup", "realloc() failed, errno=%d (%s)", errno, gu_strerror(errno));
		}
	strcpy(ptr, string);
	return ptr;
	}

/** change the size of an already allocated array

The function gu_realloc() changes the size of a memory block.  The
first argument is a pointer to the old block, the second is the desired new
number of members, the third argument is the size of each member in bytes.  This
function returns a pointer to a resized block, possibly at a different
location.

*/
void *gu_realloc(void *ptr, size_t number, size_t size)
	{
	void *rval;

	DODEBUG(("gu_realloc(ptr=%p, number=%d, size=%d)", ptr, number, size));

	if((rval = realloc(ptr, number*size)) == (void*)NULL)
		libppr_throw(EXCEPTION_STARVED, "gu_realloc", "realloc(%p, %d) failed, errno=%d (%s)", ptr, (int)(number*size), errno, gu_strerror(errno));

	return rval;
	} /* end of ppr_realloc() */

/** free memory

The function gu_free() is used to free any memory allocated by the other
functions.

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

