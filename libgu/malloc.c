/*
** mouse:~ppr/src/libgu/malloc.c
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
** Last modified 25 February 2005.
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

*/

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include "gu.h"

static void gu_pool_register(void *block);
static void gu_pool_reregister(void *block, void *old_block);

#if 0
#define DODEBUG(a) debug a
#else
#define DODEBUG(a)
#endif

/* A memory allocation pool */
struct POOL {
	int size_allocated;
	int size_used;
	void *blocks[1];
	};

/* Stack of memory allocate pools */
#define POOLS_STACK_SIZE 10
static struct POOL *pools_stack[POOLS_STACK_SIZE];
static int pools_stack_pointer = -1;

/** Number of blocks currently allocated, used for debugging */
static int gu_alloc_blocks = 0;

/** Return the number of allocated memory blocks

  This function is used to check for memory leaks.  Before
  entering a block of code which is known not to allocate
  any memory that it does not free (or is known to allocate
  or deallocate a specific number of values before returning)
  one calls this function and saves the return value.  After
  the code returns one calls gu_alloc_assert() with the expected
  number of memory blocks.

*/
int gu_alloc_checkpoint(void)
	{
	return gu_alloc_blocks;
	} /* end of gu_alloc_checkpoint() */

/*
** This is called to assert that gu_alloc_blocks differs
** from the figure saved by gu_alloc_checkpoint by a
** certain amount.  This is called by a macro called
** gu_alloc_assert().  A positive amount is increase, a negative
** amount is decrease.
*/
void _gu_alloc_assert(const char *file, int line, int assertion)
	{
	const char function[] = "gu_alloc_assert";
	if(assertion != gu_alloc_blocks)
		{
		gu_Throw("%s(): assertion failed at %s line %d (is %d, expected %d)",
			function, file, line,
			gu_alloc_blocks, assertion);
		}
	} /* end of _gu_alloc_assert() */

/** Allocate a memory block to hold an array

The function gu_alloc() takes two arguments.  The first is the number of items
to allocate, the second is the size of each in bytes.  This function will return
a void pointer to the allocated memory.  The memory is not initialized.

*/
void *gu_alloc(size_t number, size_t size)
	{
	void *rval;

	DODEBUG(("gu_alloc(number=%ld, size=%ld)", (long)number, (long)size));

	if(!(rval = malloc(size*number)))
		gu_CodeThrow(errno, "gu_alloc(): malloc() failed, errno=%d (%s)", errno, gu_strerror(errno));

	gu_alloc_blocks++;
	gu_pool_register(rval);

	return rval;
	} /* end of gu_alloc() */

/** Duplicate a string

The function gu_strdup() takes a string pointer as its sole argument and
returns a pointer to a new copy of the string.

*/
char *gu_strdup(const char *string)
	{
	char *rval;

	DODEBUG(("gu_strdup(\"%s\")", string ? string : ""));
	
	if(!string)
		return NULL;

	if(!(rval = (char*)malloc(strlen(string)+1)))
		gu_CodeThrow(errno, "gu_strdup(): malloc() failed, errno=%d (%s)", errno, gu_strerror(errno));

	gu_alloc_blocks++;
	gu_pool_register(rval);

	strcpy(rval, string);

	return rval;
	} /* end of gu_strdup() */

/** Duplicate the initial segment of a string

The function gu_strndup() takes a string pointer and a maximum length as its
arguments.  It returns a pointer to a new string containing a copy of the
string truncated to the maximum length.

*/
char *gu_strndup(const char *string, size_t len)
	{
	char *rval;

	DODEBUG(("gu_strndup(string=\"%s\", len=%d)", string, len));

	if((rval = (char*)malloc(len+1)) == (char*)NULL)
		gu_CodeThrow(errno, "gu_strndup(): malloc() failed, errno=%d (%s)", errno, gu_strerror(errno));

	gu_alloc_blocks++;
	gu_pool_register(rval);

	strncpy(rval, string, len);
	rval[len] = '\0';

	return rval;
	} /* end of gu_strndup() */

/** Copy a string into a preexisting block, resizing if necessary

This function has the semantics of realloc().  That is to say, 
the returned value will be a pointer either to the original
block resized or to a replacement block.

*/
char *gu_restrdup(char *ptr, size_t *number, const char *string)
	{
	size_t len = strlen(string);

	DODEBUG(("gu_restrdup(ptr=%p, number=%d, string=\"%s\")", ptr, number, string));

	if(!ptr || *number <= len)			/* must be at least one greater */
		{
		void *rval;
		*number = (len + 1);
		if(!(rval = (char*)realloc(ptr, *number)))
			gu_CodeThrow(errno, "gu_restrdup(): realloc() failed, errno=%d (%s)", errno, gu_strerror(errno));
		if(rval != ptr)
			{
			gu_pool_reregister(rval, ptr);
			ptr = rval;
			}
		}
	strcpy(ptr, string);
	return ptr;
	}

/** Change the size of an already allocated array

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
		gu_CodeThrow(errno, "gu_realloc(): realloc(%p, %d) failed, errno=%d (%s)", ptr, (int)(number*size), errno, gu_strerror(errno));

	if(rval != ptr)
		gu_pool_reregister(rval, ptr);
	
	return rval;
	} /* end of ppr_realloc() */

/** Free memory

The function gu_free() is used to free any memory allocated by the other functions.

If the pools stack is non-empty, this function is a noop.

*/
void gu_free(void *ptr)
	{
	DODEBUG(("gu_free(%p)", ptr));

	if(!ptr)
		gu_CodeThrow(errno, "gu_free(): attempt to free NULL pointer");

	if(pools_stack_pointer < 0)		/* if no pools exist, */
		{
		free(ptr);
		gu_alloc_blocks--;
		}
	} /* end of gu_free() */

/** Create memory pool

Create a pool into which all memory allocated with the gu_ routines
will be placed.

*/
void *gu_pool_new(void)
	{
	struct POOL *pool;
	if(!(pool = malloc(sizeof(struct POOL) - sizeof(void*) + 10 * sizeof(void*))))
		gu_CodeThrow(errno, "gu_pool_new(): malloc() failed, errno=%d (%s)", errno, gu_strerror(errno));
	gu_alloc_blocks++;
	pool->size_allocated = 10;
	pool->size_used = 0;
	return (void*)pool;
	}

/** Deallocate a memory pool

The indicated memory pool and all of the blocks which it contains
will be deallocated.

*/
void gu_pool_destroy(void *p)
	{
	struct POOL *pool = (struct POOL*)p;	
	int iii;
	for(iii=0; iii < pool->size_used; iii++)
		{
		free(pool->blocks[iii]);
		gu_alloc_blocks--;
		}
	free(p);
	gu_alloc_blocks--;
	}

/* Internal function to all a newly allocated block to the 
 * current pool (if any).
 */
static void gu_pool_register(void *block)
	{
	if(pools_stack_pointer >= 0)
		{
		struct POOL *pool = pools_stack[pools_stack_pointer];
		if(pool->size_used == pool->size_allocated)
			{
			pool->size_allocated += 10;
			if(!(pool = realloc(pool, sizeof(struct POOL) - sizeof(void*) + pool->size_allocated * sizeof(void*))))
				gu_CodeThrow(errno, "gu_pool_register(): realloc() failed, errno=%d (%s)", errno, gu_strerror(errno));
			pools_stack[pools_stack_pointer] = pool;
			}
		pool->blocks[pool->size_used++] = block;
		}
	}

/* Internal function to reregister a block moved by realloc(). */
static void gu_pool_reregister(void *block, void *old_block)
	{
	if(pools_stack_pointer >= 0)
		{
		struct POOL *pool = pools_stack[pools_stack_pointer];
		int iii;
		for(iii = (pool->size_used - 1); iii >= 0; iii--)
			{
			if(pool->blocks[iii] == old_block)
				{
				pool->blocks[iii] = block;
				break;
				}
			}
		if(iii < 0)
			gu_Throw("gu_pool_reregister(): block not allocated in current pool");
		}
	}

/* Move memory block to parent pool

The indicated block is found in the current pool.  It is removed
from the current pool and registered in the parent pool (if there is one).

Functions which uses their own pools internally should call this function
on memory blocks pointers to which they will return to the caller.  This
should be called before destroying the internal pools.

*/ 
void *gu_pool_return(void *block)
	{
	if(pools_stack_pointer < 0)
		{
		gu_Throw("gu_pool_return(): pools stack is empty");
		}
	else
		{
		struct POOL *pool = gu_pool_pop();
		int iii;
		for(iii = (pool->size_used - 1); iii >= 0; iii--)	/* hunt downward */
			{
			if(pool->blocks[iii] == block)
				{
				int move_len;
				pool->size_used--;
				if((move_len = (pool->size_used - iii)) > 0)
					memmove(&pool->blocks[iii], &pool->blocks[iii+1], move_len * sizeof(void*));
				gu_pool_register(block);	/* register in parent pool */
				break;
				}
			}
		if(iii < 0)
			gu_Throw("gu_pool_return(): block not allocated in current pool");
		gu_pool_push(pool);
		}
	return block;
	}

/** Push a memory pool onto the top of the pools stack

Memory allocates are recorded in the top pool on the stack.
This function should generally be called like this:

gu_pool_push(gu_pool_new());

*/
void gu_pool_push(void *p)
	{
	if((pools_stack_pointer + 1) >= POOLS_STACK_SIZE)
		gu_Throw("gu_pool_push(): pools stack overflow");
	pools_stack[++pools_stack_pointer] = p;
	}

/** Pop the top memory pool off of the pools tack

This function should generally be called thus:

gu_pool_destroy(gu_pool_pop());

*/
void *gu_pool_pop(void)
	{
	if(pools_stack_pointer < 0)
		gu_Throw("gu_pool_pop(): pools stack underflow");
	return pools_stack[pools_stack_pointer--];
	}

/* gcc -I../include -Wall -DTEST -o malloc malloc.c ../libgu.a */
#ifdef TEST
#include <stdio.h>
int main(int argc, char *argv[])
	{
	char *p, *p2;
	printf("gu_alloc_blocks=%d\n", gu_alloc_blocks);

	gu_pool_push(gu_pool_new());
	printf("gu_alloc_blocks=%d (with pool)\n", gu_alloc_blocks);

	p = gu_strdup("smith");
	printf("gu_alloc_blocks=%d (with one item in pool)\n", gu_alloc_blocks);

	p = gu_strdup("jones");
	printf("gu_alloc_blocks=%d (with two items in pool)\n", gu_alloc_blocks);

	p2 = gu_pool_return(p);
	printf("gu_alloc_blocks=%d (with item moved to 'global pool'\n", gu_alloc_blocks);

	gu_pool_push(gu_pool_new());
	printf("gu_alloc_blocks=%d (with second-level pool)\n", gu_alloc_blocks);

	p = gu_strdup("smith");
	p = gu_strdup("smith");
	p = gu_strdup("smith");
	printf("gu_alloc_blocks=%d (with three items in second-level pool)\n", gu_alloc_blocks);

	{
	int number = strlen(p);
	p = gu_restrdup(p, &number, "now is the time");
	printf("gu_alloc_blocks=%d (after gu_restrdup())\n", gu_alloc_blocks);
	}

	gu_pool_destroy(gu_pool_pop());
	printf("gu_alloc_blocks=%d (with second-level pool destroyed)\n", gu_alloc_blocks);

	gu_pool_destroy(gu_pool_pop());
	printf("gu_alloc_blocks=%d (with pool destroyed)\n", gu_alloc_blocks);

	printf("p2[]=\"%s\"\n", p2);
	return 0;
	}
#endif

/* end of file */

