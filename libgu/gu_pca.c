/*
** mouse:~ppr/src/libgu/gu_pca.c
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

Perl Compatible Array

This module implements an array of strings.  In addition to indexing, the 
shift, unshift, pop, and push operations familiar to Perl programers are 
implemented.  Slicing is not.

*/

#include "config.h"
#include <string.h>
#include "gu.h"

#define MAGIC 0x4202

struct PCA {
	int magic;
	int removed_at_start;		/* number of elements shifted off the start */
	int size_used;				/* number of slots used (including removed_at_start) */
	int size_allocated;			/* number of members in storage[] */
	int increment;				/* number of slots to expand by */
	char **storage;
	};

/** Create a Perl compatible array object
 * initial_size slots are initially allocated
 */
void *gu_pca_new(int initial_size, int increment)
	{
	struct PCA *p;
	if(initial_size < 0)
		gu_Throw("gu_pca_new(): initial_size must not be negative");
	if(increment < 0)
		gu_Throw("gu_pca_new(): increement must not be negative");
	p = gu_alloc(1, sizeof(struct PCA));
	p->magic = MAGIC;
	p->removed_at_start = 0;
	p->size_used = 0;
	p->size_allocated = initial_size;
	p->increment = increment;
	if(initial_size > 0)
		p->storage = gu_alloc(p->size_allocated, sizeof(char*));
	else
		p->storage = NULL;
	return (void*)p;
	}

/** Deallocate a PCA object.
 */
void gu_pca_free(void *pca)
	{
	struct PCA *p = pca;
	if(p->magic != MAGIC)
		gu_Throw("gu_pca_free(): bad magic");
	if(p->storage)
		gu_free(p->storage);
	gu_free(p);
	}

static void gu_pca_expand(void *pca)
	{
	struct PCA *p = (struct PCA *)pca;
	if(p->increment <= 0)
		gu_Throw("gu_pca_expand(): array is not growable");
	p->size_allocated += p->increment;
	p->storage = gu_realloc(p->storage, p->size_allocated, sizeof(char*));
	}

/** Return the number of elements in the array
 */
int gu_pca_size(void *pca)
	{
	struct PCA *p = (struct PCA *)pca;
	if(p->magic != MAGIC)
		gu_Throw("gu_pca_size(): bad magic");
	return (p->size_used - p->removed_at_start);
	}

/** Return the string at a given index
 */
void *gu_pca_index(void *pca, int index)
	{
	struct PCA *p = (struct PCA *)pca;
	if(p->magic != MAGIC)
		gu_Throw("gu_pca_index(): bad magic");
	index += p->removed_at_start;
	if(index >= 0 && index <= p->size_used)
		return p->storage[index];
	return NULL;
	}

/** Remove and return the last element
 */
void *gu_pca_pop(void *pca) 
	{
	struct PCA *p = (struct PCA *)pca;
	if(p->magic != MAGIC)
		gu_Throw("gu_pca_pop(): bad magic");
	if(p->size_used > p->removed_at_start)
		{
		p->size_used--;
		return p->storage[p->size_used];
		}
	return NULL;
	}

/** Add the given element to the end of the array 
 */
void gu_pca_push(void *pca, void *item)
	{
	struct PCA *p = (struct PCA *)pca;
	if(p->magic != MAGIC)
		gu_Throw("gu_pca_push(): bad magic");
	if(p->size_used == p->size_allocated)
		{
		if(p->removed_at_start > 0)		/* if we can slide the array down in storage, */
			{
			memmove(p->storage, &(p->storage[p->removed_at_start]), (p->size_used - p->removed_at_start) * sizeof(char*));
			p->size_used -= p->removed_at_start;
			p->removed_at_start = 0;
			}
		else
			{
			gu_pca_expand(pca);
			}
		}
	p->storage[p->size_used++] = item;
	}

/** Remove and return the first element
 */
void *gu_pca_shift(void *pca)
	{
	struct PCA *p = (struct PCA *)pca;
	if(p->magic != MAGIC)
		gu_Throw("gu_pca_shift(): bad magic");
	if(p->size_used > p->removed_at_start)
		{
		return p->storage[p->removed_at_start++];
		}
	return NULL;	
	}

/** Add the provided element to the begining of the array
 */
void gu_pca_unshift(void *pca, void *item)
	{
	struct PCA *p = (struct PCA *)pca;
	if(p->magic != MAGIC)
		gu_Throw("gu_pca_unshift(): bad magic");
	if(p->removed_at_start > 0)
		{
		p->removed_at_start--;
		p->storage[p->removed_at_start] = item;
		}
	else
		{
		if(p->size_used == p->size_allocated)
			gu_pca_expand(pca);
		memmove(&p->storage[1], p->storage, p->size_used * sizeof(char*));
		p->size_used++;
		p->storage[0] = item;
		}
	}

/* gcc -Wall -I../include -DTEST -o gu_pca gu_pca.c ../libgu.a */
#ifdef TEST
#include <stdio.h>
void print_array(void *a)
	{
	int iii;
	printf("a[] = ");
	for(iii=0; iii < gu_pca_size(a); iii++)
		{
		if(iii > 0)
			printf(", ");
		printf("\"%s\"", gu_pca_index(a, iii));
		}
	printf("\n");
	}
int main(int argc, char *argv[])
	{
	int i;
	void *array = gu_pca_new(10, 10);
	gu_pca_push(array, "hello");
	gu_pca_push(array, "world");
	gu_pca_unshift(array, "well");
	print_array(array);
	gu_pca_pop(array);
	print_array(array);
	gu_pca_shift(array);
	print_array(array);
	for(i=0; i < 20; i++)
		printf("\"%s\"\n", gu_pca_shift(array));
	for(i=0; i < 20; i++)
		gu_pca_unshift(array, "x");
	print_array(array);
	gu_pca_free(array);
	printf("memory blocks = %d\n", gu_alloc_checkpoint());
	return 0;
	}
#endif

/* end of file */
