/* A vector class.
 * - By Richard W.M. Jones <rich@annexia.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 */

#include "../nonppr_misc/c2lib/config.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif

#include <pstring.h>
#include <pool.h>
#include <vector.h>

#define INCREMENT 16

vector
_vector_new (pool pool, size_t size)
{
  vector v = pmalloc (pool, sizeof *v);

  v->pool = pool;
  v->size = size;
  v->data = 0;
  v->used = v->allocated = 0;

  return v;
}

inline vector
new_subvector (pool pool, vector v, int i, int j)
{
  vector new_v = pmalloc (pool, sizeof *v);

  assert (0 <= i && j <= v->used);

  new_v->pool = pool;
  new_v->size = v->size;

  if (i < j)
	{
	  new_v->data = pmemdup (pool, v->data + i * v->size, (j - i) * v->size);
	  new_v->used = new_v->allocated = j - i;
	}
  else
	{
	  new_v->data = 0;
	  new_v->used = new_v->allocated = 0;
	}

  return new_v;
}

vector
copy_vector (pool pool, vector v)
{
  return new_subvector (pool, v, 0, v->used);
}

inline void
_vector_push_back (vector v, const void *ptr)
{
  if (v->used >= v->allocated)
	{
	  int a = v->allocated + INCREMENT;
	  void *d = prealloc (v->pool, v->data, a * v->size);
	  v->allocated = a;
	  v->data = d;
	}

  if (ptr) memcpy (v->data + v->used * v->size, ptr, v->size);
  v->used++;
}

void
_vector_pop_back (vector v, void *ptr)
{
  assert (v->used > 0);
  v->used--;
  if (ptr) memcpy (ptr, v->data + v->used * v->size, v->size);
}

inline void
vector_insert_array (vector v, int i, const void *ptr, int n)
{
  int j;

  assert (0 <= i && i <= v->used);

  /* Insert n dummy elements at the end of the list. */
  for (j = 0; j < n; ++j) _vector_push_back (v, 0);

  /* Move the other elements up. */
  for (j = v->used-1; j > i; --j)
	memcpy (v->data + j * v->size, v->data + (j-n) * v->size, v->size);

  /* Insert these elements at position i. */
  if (ptr) memcpy (v->data + i * v->size, ptr, v->size * n);
}

void
_vector_insert (vector v, int i, const void *ptr)
{
  vector_insert_array (v, i, ptr, 1);
}

void
_vector_push_front (vector v, const void *ptr)
{
  _vector_insert (v, 0, ptr);
}

void
_vector_pop_front (vector v, void *ptr)
{
  _vector_get (v, 0, ptr);
  vector_erase (v, 0);
}

void
vector_push_back_vector (vector v, const vector w)
{
  int size = v->size;

  assert (size == w->size);

  if (v->used + w->used > v->allocated)
	{
	  int a = v->used + w->used;
	  void *d = prealloc (v->pool, v->data, a * size);
	  v->allocated = a;
	  v->data = d;
	}

  memcpy (v->data + v->used * size, w->data, size * w->used);
  v->used += w->used;
}

void
vector_push_front_vector (vector v, const vector w)
{
  int size = v->size;

  assert (size == w->size);

  if (v->used + w->used > v->allocated)
	{
	  int a = v->used + w->used;
	  void *d = prealloc (v->pool, v->data, a * size);
	  v->allocated = a;
	  v->data = d;
	}

  memmove (v->data + w->used * size, v->data, v->used * size);
  memcpy (v->data, w->data, size * w->used);
  v->used += w->used;
}

void
_vector_replace (vector v, int i, const void *ptr)
{
  assert (0 <= i && i < v->used);

  if (ptr) memcpy (v->data + i * v->size, ptr, v->size);
}

inline void
vector_erase_range (vector v, int i, int j)
{
  assert (0 <= i && i < v->used && 0 <= j && j <= v->used);

  if (i < j)
	{
	  int n = j - i, k;

	  for (k = i+n; k < v->used; ++k)
		memcpy (v->data + (k-n) * v->size, v->data + k * v->size, v->size);

	  v->used -= n;
	}
}

void
vector_erase (vector v, int i)
{
  vector_erase_range (v, i, i+1);
}

void
vector_clear (vector v)
{
  v->used = 0;
}

void
_vector_get (vector v, int i, void *ptr)
{
  assert (0 <= i && i < v->used);
  if (ptr) memcpy (ptr, v->data + i * v->size, v->size);
}

const void *
_vector_get_ptr (vector v, int i)
{
  assert (0 <= i && i < v->used);
  return v->data + i * v->size;
}

void
_vector_sort (vector v, int (*compare_fn) (const void *, const void *))
{
  qsort (v->data, v->used, v->size, compare_fn);
}

int
_vector_compare (vector v1, vector v2,
				 int (*compare_fn) (const void *, const void *))
{
  int i, r;
  void *p1, *p2;

  if (vector_size (v1) < vector_size (v2)) return -1;
  else if (vector_size (v1) > vector_size (v2)) return 1;

  for (i = 0; i < vector_size (v1); ++i)
	{
	  vector_get_ptr (v1, i, p1);
	  vector_get_ptr (v2, i, p2);

	  r = compare_fn (p1, p2);

	  if (r != 0) return r;
	}

  return 0;
}

void
_vector_fill (vector v, const void *ptr, int n)
{
  while (n--)
	_vector_push_back (v, ptr);
}

void
vector_swap (vector v, int i, int j)
{
  void *pi, *pj;
  char data[v->size];

  if (i == j) return;

  vector_get_ptr (v, i, pi);
  vector_get_ptr (v, j, pj);

  memcpy (data, pi, v->size);
  memcpy (pi, pj, v->size);
  memcpy (pj, data, v->size);
}

void
vector_reallocate (vector v, int n)
{
  if (n > v->allocated)
	{
	  void *d = prealloc (v->pool, v->data, n * v->size);
	  v->allocated = n;
	  v->data = d;
	}
}

vector
vector_grep (pool p, vector v, int (*match_fn) (const void *))
{
  vector nv = _vector_new (p, v->size);
  int i;

  for (i = 0; i < v->used; ++i)
	if (match_fn (v->data + i * v->size))
	  _vector_push_back (nv, v->data + i * v->size);

  return nv;
}

vector
vector_grep_pool (pool p, vector v, int (*match_fn) (pool, const void *))
{
  vector nv = _vector_new (p, v->size);
  int i;

  for (i = 0; i < v->used; ++i)
	if (match_fn (p, v->data + i * v->size))
	  _vector_push_back (nv, v->data + i * v->size);

  return nv;
}

vector
_vector_map (pool p, vector v,
			 void (*map_fn) (const void *, void *),
			 size_t result_size)
{
  vector nv = _vector_new (p, result_size);
  int i;

  vector_reallocate (nv, v->used);
  nv->used = v->used;

  for (i = 0; i < v->used; ++i)
	map_fn (v->data + i * v->size, nv->data + i * nv->size);

  return nv;
}

vector
_vector_map_pool (pool p, vector v,
				  void (*map_fn) (pool, const void *, void *),
				  size_t result_size)
{
  vector nv = _vector_new (p, result_size);
  int i;

  vector_reallocate (nv, v->used);
  nv->used = v->used;

  for (i = 0; i < v->used; ++i)
	map_fn (p, v->data + i * v->size, nv->data + i * nv->size);

  return nv;
}
