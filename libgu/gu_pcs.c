/*
** mouse:~ppr/src/libgu/gu_pcs.c
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
** Last modified 4 November 2003.
*/

/*! \file
	\brief Perl Compatible Strings

This module implements a string library.  This library is designed to make it
easier to port Perl code to C.  The strings are stored in objects known
as PCS (Perl Compatible String).

PCS objects can contain strings with embedded NULLs, but such string cannot
be converted to C strings because C strings can't contain embedded NULLs.

*/

#include "config.h"
#include <stdio.h>
#include <string.h>
#include "gu.h"

struct PCS {
	char *storage;			/* the actual storage for the strings chars */
	int storage_size;		/* number of allocated byte at storage */
	int length;				/* number currently used */
	int refcount;			/* how many have pointers to this PCS or to its storage? */
	int lurkers;			/* how many of those have pointers only to the storage? */
	};

/** create a PCS object

This function creates a new PCS (Perl compatible string) object and returns
a void pointer which should be passed to other gu_pcs_*() functions in order
to use it.

*/
void *gu_pcs_new(void)
	{
	struct PCS *p = gu_alloc(1, sizeof(struct PCS));
	p->storage = NULL;
	p->storage_size = 0;
	p->length = 0;
	p->refcount = 1;
	p->lurkers = 0;
	return (void *)p;
	}

/** create new PCS and initialize from a PCS

This function creates a new PCS and copies the string value from the a
pre-existing PCS supplied as an argument.

*/
void *gu_pcs_new_pcs(void **pcs)
	{
	void *p = gu_pcs_new();
	gu_pcs_set_pcs(&p, pcs);
	return p;
	}

/** create new PCS and initialize from a char[]

This function creates a new PCS and initializes it from the C character
array (string) provided.

*/
void *gu_pcs_new_cstr(const char cstr[])
	{
	void *p = gu_pcs_new();
	gu_pcs_set_cstr(&p, cstr);
	return p;
	}

/** destroy a PCS object

This function decrements the reference count of a PCS object and sets the
pointer pointed to by pcs to NULL.  If the reference counter reaches zero,
then the object is freed.

*/
void gu_pcs_free(void **pcs)
	{
	struct PCS *p = (struct PCS *)*pcs;
	p->refcount--;
	if((p->refcount - p->lurkers) == 0)
		{
		if(p->storage && p->lurkers == 0)
			{
			gu_free(p->storage);
			p->storage = NULL;
			}
		gu_free(*pcs);
		}
	*pcs = (void*)NULL;
	} /* end of gu_pcs_free() */

/** Destroy a PCS object but keep the C string
*/
char *gu_pcs_free_keep_cstr(void **pcs)
	{
	struct PCS *p = (struct PCS *)*pcs;
	char *strptr = p->storage;
	p->lurkers++;
	if((p->refcount - p->lurkers) == 0)
		gu_free(*pcs);
	*pcs = (void*)NULL;
	return strptr;
	} /* end of gu_pcs_free_keep_cstr() */

/** print a description of a PCS object on stdout
*/
void gu_pcs_debug(void **pcs, const char name[])
	{
	struct PCS *p = (struct PCS *)*pcs;
	printf("%s (%p) = {storage=%p, storage[]=\"%s\", storage_size=%d, length=%d, refcount=%d, lurkers=%d}\n",
		name,
		p,
		p->storage,
		p->storage,
		p->storage_size,
		p->length,
		p->refcount,
		p->lurkers);
	}

/** obtain a copy of a PCS object that won't be unexpectedly changed

This function increments the reference count of a PCS object.  A function
should call this if it is going to keep a pointer to a PCS object that was
passed to it as an argument.  If an attempt is made to modify a PCS object
with a non-zero reference count, a copy is made and the caller gets a
modified copy, but the copy held by other code is unmodified.

*/
void *gu_pcs_snapshot(void **pcs)
	{
	struct PCS *p = (struct PCS *)*pcs;
	p->refcount++;
	return *pcs;
	}

/** expand the internal storage of a PCS in anticipation of future growth

This function enlarges the specified PCS so that it can hold a string of the
specified size (excluding final NULL).  If the requested size is smaller
than the current storage size, this has no effect.

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

/** copy a char[] into an existing PCS

This function copies the contents of a C string (a NULL terminated character
array into the PCS object.  The function may have to allocate a new object
and change the pointer pointed to by I<pcs> to point to the new object.  A new
object will be allocated if the value has a reference count greater than one
(which means it should be copied on write).

*/
void gu_pcs_set_cstr(void **pcs, const char cstr[])
	{
	struct PCS *p = (struct PCS *)*pcs;

	if(p->refcount > 1)					/* if we share it, */
		{
		gu_pcs_free(pcs);				/* detach from it */
		*pcs = gu_pcs_new();			/* create a new object */
		p = (struct PCS *)*pcs;
		}

	p->length = strlen(cstr);
	gu_pcs_grow(pcs, p->length);					/* expand if necessary */
	memcpy(p->storage, cstr, p->length + 1);
	}

/** copy a PCS into an existing PCS

This function copies the contents of a PCS into the PCS object.  The function
may have to allocate a new object and change the pointer pointed to by I<pcs>
to point to the new object.  A new object will be allocated if the value has a
reference count greater than one (which means it should be copied on write).

*/
void gu_pcs_set_pcs(void **pcs, void **pcs2)
	{
	struct PCS *p = (struct PCS *)*pcs;
	
	#if 1
	printf("gu_pcs_set_pcs(pcs=%p{refcount=%d,length=%d}, pcs2=%p{refcount=%d,length=%d})\n",
			pcs,
				p->refcount,
				p->length,
			pcs2,
				((struct PCS *)*pcs2)->refcount,
				((struct PCS *)*pcs2)->length
			);
	#endif

	if(p->refcount > 1)					/* if we share target, */
		{
		gu_pcs_free(pcs);				/* detach from it */
		*pcs = gu_pcs_new_pcs(pcs2);	/* replace it with a copy of pcs2 */
		}
	else
		{
		struct PCS *p2 = (struct PCS *)*pcs2;
		p->length = p2->length;
		gu_pcs_grow(pcs, p->length);
		memcpy(p->storage, p2->storage, p->length + 1);
		}
	}

/** get pointer to const char[] within PCS

This function returns a pointer to a NULL terminated C string which contains
the value of the PCS object.  This pointer may cease to be valid if the PCS
object is modified or freed, so if you won't be using the value imediately,
you should call gu_strdup() on the result.  Also, the string should not be
modified by using this pointer.

*/
const char *gu_pcs_get_cstr(void **pcs)
	{
	struct PCS *p = (struct PCS *)*pcs;
	return p->storage;
	}

/** Get pointer to an editable char[] within PCS

This function should be called if you intend to edit the string in place. 
If anyone else has a reference to it, a new copy will be made just for you.
If you will change the length of the string, call gu_pcs_length() to
determine the initial length.  If you are enlarging the string, you need to
call gu_pcs_grow() first.  If you are making the string smaller, you should
call gu_pcs_truncate() when you are done.

*/
char *gu_pcs_get_editable_cstr(void **pcs)
	{
	struct PCS *p = (struct PCS *)*pcs;

	/*gu_pcs_debug(pcs, "gu_pcs_get_editable_cstr");*/

	if(p->refcount > 1)
		{
		void *new_pcs = gu_pcs_new_pcs(pcs);
		gu_pcs_free(pcs);
		*pcs = new_pcs;
		p = (struct PCS *)*pcs;
		}

	return p->storage;
	}

/** get length of PCS in bytes

This function returns the length in bytes of the PCS in bytes.

*/
int gu_pcs_length(void **pcs)
	{
	struct PCS *p = (struct PCS *)*pcs;
	return p->length;
	}

/** Truncate a PCS to a specified length in bytes.

*/
int gu_pcs_truncate(void **pcs, size_t newlen)
	{
	struct PCS *p = (struct PCS *)*pcs;

	if(newlen < p->length)
		{
		if(p->refcount > 1)
			{
			void *new_pcs = gu_pcs_new_pcs(pcs);
			gu_pcs_free(pcs);
			*pcs = new_pcs;
			p = (struct PCS *)*pcs;
			}
		p->length = newlen;
		p->storage[newlen] = '\0';
		}
		
	return p->length;
	}

/** append char to PCS

This function appends a C char to the the PCS object.

*/
void gu_pcs_append_char(void **pcs, int c)
	{
	struct PCS *p = (struct PCS *)*pcs;
	int new_length;

	if(p->refcount > 1)
		{
		void *new_pcs = gu_pcs_new_pcs(pcs);
		gu_pcs_free(pcs);
		*pcs = new_pcs;
		p = (struct PCS *)*pcs;
		}

	new_length = p->length + 1;
	gu_pcs_grow(pcs, new_length);
	p->storage[p->length] = c;
	p->storage[new_length] = '\0';		/* keep it null terminated */
	p->length = new_length;
	} /* end of gu_pcs_append_char() */

/** append C char[] to PCS

This function appends a C string the the PCS object.

=cut
*/
void gu_pcs_append_cstr(void **pcs, const char cstr[])
	{
	struct PCS *p = (struct PCS *)*pcs;
	int cstr_len;

	if(p->refcount > 1)
		{
		void *new_pcs = gu_pcs_new_pcs(pcs);
		gu_pcs_free(pcs);
		*pcs = new_pcs;
		p = (struct PCS *)*pcs;
		}

	cstr_len = strlen(cstr);
	gu_pcs_grow(pcs, p->length + cstr_len);
	memcpy(p->storage + p->length, cstr, cstr_len + 1);
	p->length += cstr_len;
	} /* end of gu_pcs_append_cstr() */

/** append PCS to existing PCS

This function appends a PCS object to the the PCS object.

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
	memcpy(p->storage + p->length, p2->storage, p2->length + 1);
	p->length = new_length;
	}

/** create a hash value from a PCS

This function hashes a PCS.  The hash function is attibuted to P. J Weinberger.

*/
int gu_pcs_hash(void **pcs_key)
	{
	int total;
	const char *p;
	int temp, count;

	p = gu_pcs_get_cstr(pcs_key);
	for(total = 0, count = gu_pcs_length(pcs_key); count-- > 0; )
		{
		total = (total << 4) + *p++;
		if((temp = (total & 0xf0000000)))		/* if we are about to lose something off the top, */
			{
			total ^= (temp >> 24);				/* mix it into the bottom */
			total ^= temp;						/* and remove it from the top */
			}
		}

	return total;
	}

/** compare PCSs

This function does for PCSs what strcmp() does for C strings.

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

	printf("pcs_a initialized to string \"Hello, World!\".\n");
	pcs_a = gu_pcs_new_cstr("Hello, World!");
	gu_pcs_debug(&pcs_a, "pcs_a");
	printf("length: %d\n", gu_pcs_length(&pcs_a));
	printf("\n");

	printf("pcs_b is a snapshot of pcs_a\n");
	pcs_b = gu_pcs_snapshot(&pcs_a);
	gu_pcs_debug(&pcs_a, "pcs_a");
	gu_pcs_debug(&pcs_b, "pcs_b");
	printf("\n");

	printf("append to pcs_a shouldn't change pcs_b.\n");
	gu_pcs_append_cstr(&pcs_a, "  What do you want to do today?");
	gu_pcs_debug(&pcs_a, "pcs_a");
	gu_pcs_debug(&pcs_b, "pcs_b");
	printf("\n");

	printf("create empty pcs_c\n");
	pcs_c = gu_pcs_new();
	gu_pcs_debug(&pcs_c, "pcs_c");
	printf("Set it to a string...\n");
	gu_pcs_set_cstr(&pcs_c, "The sky is blue.");
	gu_pcs_debug(&pcs_c, "pcs_c");
	printf("\n");

	printf("Append a character to pcs_a...\n");
	gu_pcs_append_char(&pcs_a, ' ');
	gu_pcs_debug(&pcs_a, "pcs_a");
	printf("Append a C string to pcs_a...\n");
	gu_pcs_append_cstr(&pcs_a, "--");
	gu_pcs_debug(&pcs_a, "pcs_a");

	printf("Append pcs_c to pcs_a and pcs_b...\n");
	gu_pcs_append_pcs(&pcs_a, &pcs_c);
	gu_pcs_append_pcs(&pcs_b, &pcs_c);
	gu_pcs_debug(&pcs_a, "pcs_a");
	gu_pcs_debug(&pcs_b, "pcs_b");
	printf("\n");

	printf("Set pcs_a and pcs_b to pcs_c...\n");
	gu_pcs_set_pcs(&pcs_a, &pcs_c);
	gu_pcs_set_pcs(&pcs_b, &pcs_c);
	gu_pcs_debug(&pcs_a, "pcs_a");
	gu_pcs_debug(&pcs_b, "pcs_b");
	printf("\n");

	printf("pcs_a=%p, pcs_b=%p, pcs_c=%p\n", pcs_a, pcs_b, pcs_c);
	gu_pcs_free(&pcs_a);
	gu_pcs_debug(&pcs_b, "pcs_b");
	gu_pcs_free(&pcs_b);
	printf("pcs_a=%p, pcs_b=%p, pcs_c=%p\n", pcs_a, pcs_b, pcs_c);

	return 0;
	}
#endif

/* end of file */
