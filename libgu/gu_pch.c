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
** Last modified 2 July 2002.
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
to use it.

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

=cut
*/
void gu_pch_free(void **pch)
    {
    struct PCH *p = (struct PCH *)*pch;
    int x;

    for(x=0; x < p->buckets_count; x++)
	{
	}

    gu_free(*pch);
    *pch = (void*)NULL;
    }

/*
=head2 void gu_pch_debug(void **I<pcs>)

=cut
*/
void gu_pch_debug(void **pch, const char name[])
    {
    struct PCH *p = (struct PCH *)*pch;
    int x;

    for(x=0; x < p->buckets_count; x++)
	{
	}


    }

/*
** This internal function returns a pointer to the pointer which should be set
** to point to a given key.
*/
static struct PCH_BUCKET *gu_pch_find(void **pch, void **pcs_key)
    {
    struct PCH *p = (struct PCH *)*pch;
    int hash;
    struct PCH_BUCKET *bp;

    hash = gu_pch_hash_pcs(pcs_key) % p->buckets_count;
    bp = p->buckets[hash];

    }

/*
=head2 void gu_pch_set(void **pch, void **pcs_key, void **pcs_value)

=cut
*/
void gu_pch_set(void **pch, void **pcs_key, void **pcs_value)
    {
    struct PCH *p = (struct PCH *)*pch;

    }

/*
=head2 void gu_pch_get(void **pch, void **pcs_key)

This function looks up the indicated key and returns a PCS (or NULL if the key
is not found).

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

    return 0;
    }
#endif

/* end of file */
