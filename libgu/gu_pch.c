/*
** mouse:~ppr/src/libgu/gu_pch.c
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
** Last modified 19 July 2002.
*/

#include "before_system.h"
#include <stdio.h>
#include <string.h>
#include "gu.h"

/*
=head1 gu_pch

This module implements a hash table library.  This library is designed to make it
easier to port Perl code to C.  The hash tables are stored in objects known
as PCH (Perl Compatible Hash).

PCH objects can contain PCS (Perl Compatible String) objects.

=cut
*/

struct PCH_BUCKET {
    void *key;
    void *value;
    struct PCH_BUCKET *next;
    };

struct PCH {
    struct PCH_BUCKET **buckets;
    int buckets_count;
    int current_bucket;
    int current_bucket_index;
    };

/*
=head2 void *gu_pch_new(int buckets_count)

This function creates a new PCH (Perl compatible hash) object and returns a
void pointer which should be passed to other gu_pch_*() functions in order
to use it.  It takes a single integer argument which is the number of
hash that the new hash should have.

=cut
*/
void *gu_pch_new(int buckets_count)
    {
    struct PCH *p = gu_alloc(1, sizeof(struct PCH));
    int x;
    p->buckets_count = buckets_count;
    p->buckets = gu_alloc(sizeof(struct PCH_BUCKET *), p->buckets_count);
    for(x=0; x < p->buckets_count; x++)
	{
	p->buckets[x] = (struct PCH_BUCKET *)NULL;
	}
    p->current_bucket = 0;
    p->current_bucket_index = 0;
    return (void *)p;
    }

/*
=head2 void gu_pch_free(void **I<pch>)

This function frees a PCH object.  This includes freeing all of the memory
it uses and decrementing the reference counts on any objects it holds
references to (which may result in their being freed too).

=cut
*/
void gu_pch_free(void **pch)
    {
    struct PCH *p = (struct PCH *)*pch;
    int x;

    /* Decrement the reference count on each PCS which is a key or a value. */
    for(x=0; x < p->buckets_count; x++)
	{

	}

    gu_free(p->buckets);	/* free the buckets within the object */
    gu_free(*pch);		/* free the object itself */
    *pch = (void*)NULL;		/* voiding the pointer will prevent accidental reuse */
    }

/*
=head2 void gu_pch_debug(void **I<pcs>)

This function prints a dump of a hash object.

=cut
*/
void gu_pch_debug(void **pch, const char name[])
    {
    struct PCH *p = (struct PCH *)*pch;
    int x;

    printf("%p ->\n", pch);

    for(x=0; x < p->buckets_count; x++)
	{
	struct PCH_BUCKET *bucket = p->buckets[x];
	printf("  buckets[%d]\n", x);
	while(bucket)
	    {
	    printf("    {%s} = \"%s\"\n", gu_pcs_get_cstr(&bucket->key), gu_pcs_get_cstr(&bucket->key));
	    bucket = bucket->next;
	    }
	}
    }

/*
** This internal function returns a pointer to the pointer which should be set
** to point to a given key.
*/
static struct PCH_BUCKET **gu_pch_find(void **pch, void **pcs_key)
    {
    struct PCH *p = (struct PCH *)*pch;
    int hash;
    hash = gu_pcs_hash(pcs_key) % p->buckets_count;
    return &p->buckets[hash];
    }

/*
=head2 void gu_pch_set(void **pch, void **pcs_key, void **pcs_value)

This function sets a given key of a given hash object to a given value.

=cut
*/
void gu_pch_set(void **pch, void **pcs_key, void **pcs_value)
    {
    struct PCH *p = (struct PCH *)*pch;

    }

/*
=head2 void gu_pch_get(void **pch, void **pcs_key)

This function looks up the indicated key and returns the value as a PCS (or
NULL if the key is not found).

=cut
*/
void *gu_pch_get(void **pch, void **pcs_key)
    {

    }

/*
=head2 void gu_pch_delete(void **pch, void **pcs_key)

This function deletes a key from the hash table.

=cut
*/
void gu_pch_delete(void **pch, void **pcs_key)
    {
    struct PCH *p = (struct PCH *)*pch;

    }

/*
=head2 void gu_pch_rewind(void **pch)

This rewinds the hash to the first key (for hash traversal with
gu_pch_nextkey()).

=cut
*/
void gu_pch_rewind(void **pch)
    {
    struct PCH *p = (struct PCH *)*pch;
    p->current_bucket = 0;
    p->current_bucket_index = 0;
    }

/*
=head2 void gu_pch_nextkey(void **pch)

This function returns the next key as a Perl Compatible String (PCS).
Use this for constructs like:

	foreach my $key (keys %myhash)

Do it like this:

	void *key;
	for(gu_pch_rewind(&myhash); (key = gu_pcsh_nextkey(&myhash)); gu_pcs_free(&key))
	    {



	    }

=cut
*/
void *gu_pch_nextkey(void **pch)
    {


    }

/*
** Test program
** gcc -Wall -DTEST -I../include gu_pch.c ../libgu.a
*/
#ifdef TEST
int main(int argc, char *argv[])
    {
    void *thash = gu_pch_new(10);

    gu_pch_debug(&thash, "thash");

    gu_pch_free(&thash);

    return 0;
    }
#endif

/* end of file */
