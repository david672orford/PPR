/*
** mouse:~ppr/src/libgu/gu_pcs.c
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

#include "before_system.h"
#include <stdio.h>
#include <string.h>
#include "gu.h"

/*
=head1 gu_pcs

This module implements a string library.  This library is designed to make it
easier to port Perl code to C.  The strings are stored in objects known
as PCS (Perl Compatible String).

PCS objects can contain strings with embedded NULLs, but such string cannot
be converted to C strings because C strings can't contain embedded NULLs.

=cut
*/

struct PCS {
    char *storage;
    int storage_size;
    int length;
    int refcount;
};

/*
=head2 void *gu_pcs_new(void)

This function creates a new PCS (Perl compatible string) object and returns a
void pointer which should be passed to other gu_pcs_*() functions in order
to use it.

=cut
*/
void *gu_pcs_new(void)
    {
    struct PCS *p = gu_alloc(1, sizeof(struct PCS));
    p->storage = NULL;
    p->storage_size = 0;
    p->length = 0;
    p->refcount = 1;
    return (void *)p;
    }

/*
=head2 void gu_pcs_free(void **I<pcs>)

This function decrements the reference count of a PCS object and sets the
pointer I<pcs> to NULL.  If the reference counter reaches zero, then the
object is freed.

=cut
*/
void gu_pcs_free(void **pcs)
    {
    struct PCS *p = (struct PCS *)*pcs;
    if(p->refcount-- == 1)
    	{
	if(p->storage)
	    {
	    gu_free(p->storage);
	    p->storage = NULL;
	    }
	gu_free(*pcs);
    	}
    *pcs = (void*)NULL;
    }

/*
=head2 void gu_pcs_debug(void **I<pcs>)
=cut
*/
void gu_pcs_debug(void **pcs, const char name[])
    {
    struct PCS *p = (struct PCS *)*pcs;
    printf("%s (%p) = {storage=%p, storage[]=\"%s\", storage_size=%d, length=%d, refcount=%d}\n",
	name,
	p,
	p->storage,
	p->storage,
	p->storage_size,
	p->length,
	p->refcount);
    }

/*
=head2 void gu_pcs_snapshot(void **I<pcs>)

This function increments the reference count of a PCS object.  A function
should call this if it is going to keep a pointer to a PCS object that was
passed to it as an argument.

=cut
*/
void *gu_pcs_snapshot(void **pcs)
    {
    struct PCS *p = (struct PCS *)*pcs;
    p->refcount++;
    return *pcs;
    }

/*
=head2 void gu_pcs_grow(void **I<pcs>, int I<size>)

This function enlarges the specified PCS so that it can hold a string of the
specified size (excluding final NULL).

=cut
*/
void gu_pcs_grow(void **pcs, int new_size)
    {
    struct PCS *p = (struct PCS *)*pcs;

    if((new_size + 1) > p->storage_size)
    	{
	p->storage = gu_realloc(p->storage, (new_size + 1), sizeof(char));
	p->storage_size = (new_size + 1);
    	}
    }

/*
=head2 void *gu_pcs_new_pcs(void **I<pcs>)

This function creates a new PCS which is a duplicate of the one supplied.

=cut
*/
void *gu_pcs_new_pcs(void **pcs)
    {
    void *p = gu_pcs_new();
    gu_pcs_set_pcs(&p, pcs);
    return p;
    }


/*
=head2 void *gu_pcs_new_cstr(const char cstr[])

This function creates a new PCS and initializes it rom the C string provided.

=cut
*/
void *gu_pcs_new_cstr(const char cstr[])
    {
    void *p = gu_pcs_new();
    gu_pcs_set_cstr(&p, cstr);
    return p;
    }

/*
=head2 void gu_pcs_set_cstr(void **I<pcs>, const char I<cstr>[])

This function copies the contents of a C string (a NULL terminated character
array into the PCS object.  The function may have to allocate a new object
and change the pointer pointed to by I<pcs> to point to the new object.  A new
object will be allocated if the value has a reference count greater than one
(which means it should be copied on write).

=cut
*/
void gu_pcs_set_cstr(void **pcs, const char cstr[])
    {
    struct PCS *p = (struct PCS *)*pcs;

    if(p->refcount > 1)			/* if we share it, */
    	{
	gu_pcs_free(pcs);
	*pcs = gu_pcs_new();
	p = (struct PCS *)*pcs;
    	}

    p->length = strlen(cstr);
    gu_pcs_grow(pcs, p->length);
    strncpy(p->storage, cstr, p->storage_size);
    }

/*
=head2 void gu_pcs_set_pcs(void **I<pcs>, void *I<pcs2>)

This function copies the contents of a PCS into the PCS object.  The function
may have to allocate a new object and change the pointer pointed to by I<pcs>
to point to the new object.  A new object will be allocated if the value has a
reference count greater than one (which means it should be copied on write).

=cut
*/
void gu_pcs_set_pcs(void **pcs, void **pcs2)
    {
    struct PCS *p = (struct PCS *)*pcs;

    if(p->refcount > 1)			/* if we share it, */
	{
	gu_pcs_free(pcs);
	*pcs = gu_pcs_new_pcs(pcs2);
	}
    else
	{
	struct PCS *p2 = (struct PCS *)*pcs2;
	p->length = p2->length;
	gu_pcs_grow(pcs, p->length);
	memcpy(p->storage, p2->storage, p->length + 1);
	}
    }

/*
=head2 const char *gu_pcs_get_cstr(void **I<pcs>)

This function returns a pointer to a NULL terminated C string which contains
the value of the PCS object.  This pointer may cease to be valid if the PCS
object is modified or freed, so if you won't be using the value imediately,
you should call B<gu_strdup()> on the result.

=cut
*/
const char *gu_pcs_get_cstr(void **pcs)
    {
    struct PCS *p = (struct PCS *)*pcs;
    return p->storage;
    }

/*
=head2 int gu_pcs_size(void **pcs)

This function returns the length of the PCS in bytes.

=cut
*/
int gu_pcs_bytes(void **pcs)
    {
    struct PCS *p = (struct PCS *)*pcs;
    return p->length;
    }

/*
=head2 void gu_pcs_append_cchar(void **I<pcs>, char I<c>[])

This function appends a C character to the the PCS object.

=cut
*/
void gu_pcs_append_byte(void **pcs, int c)
    {
    struct PCS *p = (struct PCS *)*pcs;
    int new_length;

    if(p->refcount > 1)
    	{
	void *new_pcs = gu_pcs_new_pcs(pcs);
	gu_pcs_free(pcs);
	*pcs = new_pcs;
    	}

    p = (struct PCS *)*pcs;
    new_length = p->length + 1;
    gu_pcs_grow(pcs, new_length);
    p->storage[p->length] = c;
    p->storage[new_length] = '\0';	/* keep it null terminated */
    p->length = new_length;
    }

/*
=head2 void gu_pcs_append_cstr(void **I<pcs>, const char I<cstr>[])

This function appends a C string the the PCS object.

=cut
*/
void gu_pcs_append_cstr(void **pcs, const char cstr[])
    {
    struct PCS *p = (struct PCS *)*pcs;
    int new_length;

    if(p->refcount > 1)
    	{
	void *new_pcs = gu_pcs_new_pcs(pcs);
	gu_pcs_free(pcs);
	*pcs = new_pcs;
    	}

    p = (struct PCS *)*pcs;
    new_length = p->length + strlen(cstr);
    gu_pcs_grow(pcs, new_length);
    strcpy(p->storage + p->length, cstr);
    p->length = new_length;
    }

/*
=head2 void gu_pcs_append_pcs(void **I<pcs>, void *I<pcs2>)

This function appends a PCS object to the the PCS object.

=cut
*/
void gu_pcs_append_pcs(void **pcs, void **pcs2)
    {
    struct PCS *p, *p2;
    int new_length;

    p = (struct PCS *)*pcs;

    if(p->refcount > 1)
    	{
	void *new_pcs = gu_pcs_new_pcs(pcs);
	gu_pcs_free(pcs);
	*pcs = new_pcs;
	p = (struct PCS *)*pcs;
    	}

    p2 = (struct PCS *)*pcs2;
    new_length = p->length + p2->length;
    gu_pcs_grow(pcs, new_length);
    memcpy(p->storage + p->length, p2->storage,p2->length + 1);
    p->length = new_length;
    }

/*
=head2 int gu_pcs_hash(void **pcs_key)

This function hashes a PCS.
This function is attibuted to P. J Weinberger.

=cut
*/
int gu_pcs_hash(void **pcs_key)
    {
    int total;
    const char *p;
    int temp, count;

    p = gu_pcs_get_cstr(pcs_key);
    for(total = 0, count = gu_pcs_bytes(pcs_key); count-- > 0; )
	{
	total = (total << 4) + *p++;
	if((temp = (total & 0xf0000000)))	/* if we are about to lose something off the top, */
	    {
	    total ^= (temp >> 24);		/* mix it into the bottom */
	    total ^= temp;			/* and remove it from the top */
	    }
	}
    }

/*
=head int gu_pcs_cmp(void *pcs1, void *pcs2)

This function does for PCSs what strcmp() does for C strings.

=cut
*/
int gu_pcs_cmp(void **pcs1, void **pcs2)
    {
    struct PCS *p1 = (struct PCS *)*pcs1;
    struct PCS *p2 = (struct PCS *)*pcs2;
    int remaining1 = p1->length + 1;
    int remaining2 = p2->length + 1;
    char *cp1 = p1->storage;
    char *cp2 = p2->storage;
    int cmp;
    while(remaining1-- && remaining2--)
	{
	if((cmp = (*cp1++ - *cp2++)) != 0)
	    break;
	}
    return cmp;
    }

/*
** Test program
** gcc -Wall -DTEST -I../include gu_pcs.c ../libgu.a
*/
#ifdef TEST
int main(int argc, char *argv[])
    {
    void *pcs_a, *pcs_b, *pcs_c;

    pcs_a = gu_pcs_new_cstr("Hello, World!");
    gu_pcs_debug(&pcs_a, "pcs_a");
    pcs_b = gu_pcs_snapshot(&pcs_a);
    gu_pcs_debug(&pcs_a, "pcs_a");
    gu_pcs_debug(&pcs_b, "pcs_b");
    printf("\n");

    gu_pcs_append_cstr(&pcs_a, "  What do you want to do today?");
    gu_pcs_debug(&pcs_a, "pcs_a");
    gu_pcs_debug(&pcs_b, "pcs_b");
    printf("\n");

    pcs_c = gu_pcs_new();
    gu_pcs_set_cstr(&pcs_c, "The sky is blue.");
    gu_pcs_append_pcs(&pcs_a, &pcs_c);
    gu_pcs_append_pcs(&pcs_b, &pcs_c);
    gu_pcs_debug(&pcs_a, "pcs_a");
    gu_pcs_debug(&pcs_b, "pcs_b");
    printf("\n");

    gu_pcs_set_pcs(&pcs_a, &pcs_c);
    gu_pcs_set_pcs(&pcs_b, &pcs_c);
    gu_pcs_debug(&pcs_a, "pcs_a");
    gu_pcs_debug(&pcs_b, "pcs_b");
    printf("\n");

    printf("pcs_a=%p, pcs_b=%p, pcs_c=%p\n", pcs_a, pcs_b, pcs_c);
    gu_pcs_free(&pcs_a);
    gu_pcs_debug(&pcs_b, "pcs_b");
    gu_pcs_free(&pcs_b);

    return 0;
    }
#endif


/* end of file */
