/*
** mouse:~ppr/src/libgu/gu_pch.c
** Copyright 1995--2008, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 3 September 2008.
*/

/*! \file

This module implements a hash table library.  This library is designed to make it
easier to port Perl code to C.  The hash tables are stored in objects known
as PCH (Perl Compatible Hash).

The hash keys are C strings (passed by pointer).  The values are C string pointers
too.  The caller must take care of allocating and deallocting both keys and values.

*/

#include "config.h"
#include <stdio.h>
#include <string.h>
#include "gu.h"

#define MAGIC 0x4201

struct PCH_ENTRY {
	char *key;					/* key for this item */
	char *value;				/* value for this item */
	struct PCH_ENTRY *next;		/* next item in bucket */
	};

struct PCH {
	int magic;
	struct PCH_ENTRY **buckets;
	int bucket_count;				/* how many buckets are in the hash? */
	int key_count;					/* how many items in the hash? */
	int current_bucket;				/* bucket which iteration has reached */
	struct PCH_ENTRY *next_item;	/* bucket item which iternation has reached */
	};

/** Create Perl Compatible Hash object

This function creates a new PCH (Perl compatible hash) object and returns a
void pointer which should be passed to other gu_pch_*() functions in order
to use it.  It takes a single integer argument which is the number of
hash that the new hash should have.

*/
void *gu_pch_new(int bucket_count)
	{
	struct PCH *p;
	int x;

	if(bucket_count < 1)
		gu_Throw("gu_pch_new(): bucket count must be at least one");
	
	p = gu_alloc(1, sizeof(struct PCH));
	p->magic = MAGIC;
	p->bucket_count = bucket_count;
	p->buckets = gu_alloc(p->bucket_count, sizeof(struct PCH_ENTRY *));
	for(x=0; x < p->bucket_count; x++)
		{
		p->buckets[x] = (struct PCH_ENTRY *)NULL;
		}
	p->key_count = 0;
	p->current_bucket = -1;
	p->next_item = NULL;
	return (void *)p;
	}

/** Destroy a PCH object

This function frees a PCH object.  This includes freeing all of the memory
it uses and decrementing the reference counts on any objects it holds
references to (which may result in their being freed too).

*/
void gu_pch_free(void *pch)
	{
	struct PCH *p = (struct PCH *)pch;
	int x;
	struct PCH_ENTRY *bucket, *bucket_next;;

	if(p->magic != MAGIC)
		gu_Throw("gu_pch_free(): bad magic");

	/* Decrement the reference count on each PCS which is a key or a value. */
	for(x=0; x < p->bucket_count; x++)
		{
		bucket = p->buckets[x];
		while(bucket)
			{
			bucket_next = bucket->next;
			gu_free(bucket);
			bucket = bucket_next;
			}
		}

	gu_free(p->buckets);		/* free the buckets within the object */
	gu_free(pch);				/* free the object itself */
	}

/** Print description of PCH object

*/
void gu_pch_debug(void *pch, const char name[])
	{
	struct PCH *p = (struct PCH *)pch;
	int x;

	if(p->magic != MAGIC)
		gu_Throw("gu_pch_debug(): bad magic: 0x%08x", p->magic);

	printf("%s (%p) {\n", name, pch);

	for(x=0; x < p->bucket_count; x++)
		{
		struct PCH_ENTRY *entry = p->buckets[x];
		printf("  buckets[%d]\n", x);
		while(entry)
			{
			printf("    {%s} = \"%s\"\n", entry->key, entry->value);
			entry = entry->next;
			}
		}

	printf("  } (%d items)\n", p->key_count);
	}

/** Set a given key of a given hash object to a given value.
*/
void gu_pch_set(void *pch, char key[], void *value)
	{
	struct PCH *p = (struct PCH *)pch;
	struct PCH_ENTRY *entry, **entry_pp;

	if(p->magic != MAGIC)
		gu_Throw("gu_pch_set(): bad magic");

	entry_pp = &p->buckets[gu_hash(key,p->bucket_count)];
	while((entry = *entry_pp) && strcmp(entry->key, key) != 0)
		{
		entry_pp = &(entry->next);
		}
	if(entry)
		{
		entry->value = value;
		}
	else
		{
		*entry_pp = entry = gu_alloc(1, sizeof(struct PCH_ENTRY));
		entry->key = key;
		entry->value = value;
		entry->next = NULL;
		p->key_count++;
		}
	}

/**

This function looks up the indicated key and returns the value as a PCS (or
NULL if the key is not found).

*/
void *gu_pch_get(void *pch, const char key[])
	{
	struct PCH *p = (struct PCH *)pch;
	struct PCH_ENTRY *entry;

	if(p->magic != MAGIC)
		gu_Throw("gu_pch_get(): bad magic");

	entry = p->buckets[gu_hash(key,p->bucket_count)];
	while(entry && strcmp(entry->key, key) != 0)
		entry = entry->next;
	if(entry)
		return entry->value;
	else
		return NULL;
	}

/** This function deletes a key from the hash table.

  This function is analogous to Perl's delete operator.
*/
void *gu_pch_delete(void *pch, char key[])
	{
	struct PCH *p = (struct PCH *)pch;
	struct PCH_ENTRY *entry, **entry_pp = &p->buckets[gu_hash(key,p->bucket_count)];
	while((entry = *entry_pp))
		{
		if(strcmp(entry->key, key) == 0)
			{
			char *value = entry->value;
			*entry_pp = entry->next;
			gu_free(entry);
			p->key_count--;
			return value;
			}
		entry_pp = &entry->next;
		}
	return NULL;
	}

/** rewind key iteration to first key

This rewinds the hash to the first key (for hash traversal with
gu_pch_nextkey()).

*/
void gu_pch_rewind(void *pch)
	{
	struct PCH *p = (struct PCH *)pch;
	p->current_bucket = -1;
	p->next_item = NULL;
	}

/** get next hash key in iteration

This function returns the next key as a C string.
Use this for constructs like:

	foreach my $key (keys %myhash)

Do it like this:

	const char *key;
	for(gu_pch_rewind(&myhash); (key = gu_pch_nextkey(&myhash)); )
		{

		}

*/
char *gu_pch_nextkey(void *pch, void **value)
	{
	struct PCH *p = (struct PCH *)pch;
	if(!p->next_item)
		{
		/* move to next non-empty bucket */
		while(++p->current_bucket < p->bucket_count)
			{
			if((p->next_item = p->buckets[p->current_bucket]))
				break;
			}
		}
	if(p->next_item)
		{
		char *key;
		if(value)
			*value = p->next_item->value;
		key = p->next_item->key;
		p->next_item = p->next_item->next;
		return key;
		}
	return NULL;
	}

/** return number of members in hash */
int gu_pch_size(void *pch)
	{
	struct PCH *p = (struct PCH *)pch;
	return p->key_count;
	}

/** create a hash value from a string

This function hashes a string.  The range of the hash value is limited
to 0 thru (modus - 1) inclusive.  The hash function is attibuted to 
P. J Weinberger.  It is assume that an int is at least 32 bits long.

*/
int gu_hash(const char string[], int modus)
	{
	unsigned int total;
	const char *p;
	int temp;

	p = string;
	for(total = 0, p = string; *p; p++)
		{
		total = (total << 4) + *p;
		if((temp = (total & 0xf0000000)))		/* if we are about to lose something off the top, */
			{
			total ^= (temp >> 24);				/* mix it into the bottom */
			total ^= temp;						/* and remove it from the top */
			}
		}

	return total % modus;
	}

/* gcc -Wall -DTEST -I../include -o gu_pch gu_pch.c ../libgu.a */
#ifdef TEST
int main(int argc, char *argv[])
	{
	void *thash = gu_pch_new(10);
	gu_pch_debug(thash, "thash");

	gu_pch_set(thash, "x", "100");
	gu_pch_set(thash, "y", "110");
	gu_pch_set(thash, "z", "120");
	gu_pch_set(thash, "john", "president");
	gu_pch_set(thash, "sally", "treasurer");
	gu_pch_set(thash, "sue", "secretary");
	gu_pch_set(thash, "1", "one");
	gu_pch_set(thash, "2", "two");
	gu_pch_set(thash, "3", "three");
	gu_pch_set(thash, "4", "four");
	gu_pch_set(thash, "5", "five");
	gu_pch_set(thash, "6", "six");
	gu_pch_set(thash, "7", "seven");
	gu_pch_set(thash, "8", "eight");
	gu_pch_set(thash, "9", "nine");
	gu_pch_set(thash, "10", "ten");
	gu_pch_debug(thash, "thash");

	printf("sue=\"%s\"\n", gu_pch_delete(thash, "sue"));
	printf("math=\"%s\"\n", gu_pch_delete(thash, "math"));
	gu_pch_set(thash, "john", "fired");
	gu_pch_debug(thash, "thash");

	gu_pch_set(thash, "*Option1", "False");

	{
	const char *key;
	for(gu_pch_rewind(thash); (key = gu_pch_nextkey(thash, NULL)); )
		{
		printf("\"%s\" -> \"%s\"\n", key, gu_pch_get(thash, key));
		}
	}

	gu_pch_free(thash);

	printf("memory blocks = %d\n", gu_alloc_checkpoint());
	
	return 0;
	}
#endif

/* end of file */
