/* A hash class.
 * By Richard W.M. Jones <rich@annexia.org>
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

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <pstring.h>
#include <vector.h>
#include <hash.h>

struct hash
{
  pool pool;
  size_t key_size;
  size_t value_size;
  vector buckets;
};

struct sash
{
  pool pool;
  vector buckets;
};

struct shash
{
  pool pool;
  size_t value_size;
  vector buckets;
};

#define HASH_NR_BUCKETS 32

/* This is the hashing function -- the same one as used by Perl. */
static inline unsigned
HASH (const void *key, size_t key_size, int nr_buckets)
{
  unsigned h = 0;
  const char *s = (const char *) key;

  while (key_size--)
    h = h * 33 + *s++;

  return h & (nr_buckets - 1);
}

/*----- HASHes -----*/

struct hash_bucket_entry
{
  void *key;
  void *value;
};

hash
_hash_new (pool pool, size_t key_size, size_t value_size)
{
  hash h;
  vector null = 0;

  h = pmalloc (pool, sizeof *h);
  h->pool = pool;
  h->key_size = key_size;
  h->value_size = value_size;
  h->buckets = new_vector (pool, sizeof (vector));
  vector_fill (h->buckets, null, HASH_NR_BUCKETS);

  return h;
}

/* NB. This only copies the hash, NOT the structures or whatever
 * which might be pointed to by keys/values in the hash. Beware.
 */
hash
copy_hash (pool pool, hash h)
{
  hash new_h;
  int b, i;

  new_h = pmalloc (pool, sizeof *new_h);
  new_h->pool = pool;
  new_h->key_size = h->key_size;
  new_h->value_size = h->value_size;
  new_h->buckets = copy_vector (pool, h->buckets);

  /* Copy the buckets. */
  for (b = 0; b < vector_size (new_h->buckets); ++b)
    {
      vector v;

      vector_get (new_h->buckets, b, v);

      if (v)
	{
	  v = copy_vector (pool, v);
	  vector_replace (new_h->buckets, b, v);

	  /* Copy the keys/values in this vector. */
	  for (i = 0; i < vector_size (v); ++i)
	    {
	      struct hash_bucket_entry entry;

	      vector_get (v, i, entry);
	      entry.key = pmemdup (pool, entry.key, h->key_size);
	      entry.value = pmemdup (pool, entry.value, h->value_size);
	      vector_replace (v, i, entry);
	    }
	}
    }

  return new_h;
}

int
_hash_get (hash h, const void *key, void *value)
{
  const void *ptr;

  ptr = _hash_get_ptr (h, key);
  if (ptr == 0) return 0;

  if (value) memcpy (value, ptr, h->value_size);
  return 1;
}

const void *
_hash_get_ptr (hash h, const void *key)
{
  int b, i;
  vector bucket;

  /* Find the appropriate bucket. */
  b = HASH (key, h->key_size, vector_size (h->buckets));
  vector_get (h->buckets, b, bucket);

  if (bucket == 0)
    return 0;

  /* Search this bucket linearly. */
  for (i = 0; i < vector_size (bucket); ++i)
    {
      struct hash_bucket_entry entry;

      vector_get (bucket, i, entry);
      if (memcmp (entry.key, key, h->key_size) == 0)
	return entry.value;
    }

  return 0;
}

int
_hash_insert (hash h, const void *key, const void *value)
{
  int b, i;
  vector bucket;
  struct hash_bucket_entry entry;

  /* Find the appropriate bucket. */
  b = HASH (key, h->key_size, vector_size (h->buckets));
  vector_get (h->buckets, b, bucket);

  /* If there is no bucket there, we have to allocate a fresh vector. */
  if (bucket == 0)
    {
      bucket = new_vector (h->pool, struct hash_bucket_entry);
      vector_replace (h->buckets, b, bucket);
    }

  /* Search this bucket linearly. */
  for (i = 0; i < vector_size (bucket); ++i)
    {
      vector_get (bucket, i, entry);
      if (memcmp (entry.key, key, h->key_size) == 0)
	{
	  memcpy (entry.value, value, h->value_size);
	  /*entry.value = pmemdup (h->pool, value, h->value_size);*/

	  /* Replace this entry. */
	  vector_replace (bucket, i, entry);

	  return 1;
	}
    }

  /* Append to this bucket. */
  entry.key = pmemdup (h->pool, key, h->key_size);
  entry.value = pmemdup (h->pool, value, h->value_size);

  vector_push_back (bucket, entry);
  return 0;
}

int
_hash_erase (hash h, const void *key)
{
  int b, i;
  vector bucket;
  struct hash_bucket_entry entry;

  /* Find the appropriate bucket. */
  b = HASH (key, h->key_size, vector_size (h->buckets));
  vector_get (h->buckets, b, bucket);

  if (bucket == 0) return 0;

  /* Search this bucket linearly. */
  for (i = 0; i < vector_size (bucket); ++i)
    {
      vector_get (bucket, i, entry);
      if (memcmp (entry.key, key, h->key_size) == 0)
	{
	  /* Remove this entry. */
	  vector_erase (bucket, i);

	  return 1;
	}
    }

  return 0;
}

inline vector
hash_keys_in_pool (hash h, pool p)
{
  int i, j;
  vector bucket, keys;
  struct hash_bucket_entry entry;

  keys = _vector_new (p, h->key_size);

  for (i = 0; i < vector_size (h->buckets); ++i)
    {
      vector_get (h->buckets, i, bucket);

      if (bucket)
	for (j = 0; j < vector_size (bucket); ++j)
	  {
	    vector_get (bucket, j, entry);
	    _vector_push_back (keys, entry.key);
	  }
    }

  return keys;
}

vector
hash_keys (hash h)
{
  return hash_keys_in_pool (h, h->pool);
}

inline vector
hash_values_in_pool (hash h, pool p)
{
  int i, j;
  vector bucket, values;
  struct hash_bucket_entry entry;

  values = _vector_new (p, h->value_size);

  for (i = 0; i < vector_size (h->buckets); ++i)
    {
      vector_get (h->buckets, i, bucket);

      if (bucket)
	for (j = 0; j < vector_size (bucket); ++j)
	  {
	    vector_get (bucket, j, entry);
	    _vector_push_back (values, entry.value);
	  }
    }

  return values;
}

vector
hash_values (hash h)
{
  return hash_values_in_pool (h, h->pool);
}

int
hash_size (hash h)
{
  vector bucket;
  int n = 0, i;

  for (i = 0; i < vector_size (h->buckets); ++i)
    {
      vector_get (h->buckets, i, bucket);
      n += bucket ? vector_size (bucket) : 0;
    }

  return n;
}

int
hash_get_buckets_used (hash h)
{
  vector bucket;
  int n = 0, i;

  for (i = 0; i < vector_size (h->buckets); ++i)
    {
      vector_get (h->buckets, i, bucket);
      n += bucket ? 1 : 0;
    }

  return n;
}

int
hash_get_buckets_allocated (hash h)
{
  return vector_size (h->buckets);
}

void
hash_set_buckets_allocated (hash h, int new_size)
{
  vector null = 0;

  /* The user has been warned not to call this after elements have been
   * inserted into the hash, and to make NEW_SIZE a power of 2.
   */
  if (vector_size (h->buckets) > new_size)
    vector_erase_range (h->buckets, new_size, vector_size (h->buckets));
  else if (vector_size (h->buckets) < new_size)
    vector_fill (h->buckets, null, new_size - vector_size (h->buckets));
}

/*----- SASHes -----*/

struct sash_bucket_entry
{
  char *key;
  char *value;
  int value_allocated;
};

sash
new_sash (pool pool)
{
  sash h;
  vector null = 0;

  h = pmalloc (pool, sizeof *h);
  h->pool = pool;
  h->buckets = new_vector (pool, sizeof (vector));
  vector_fill (h->buckets, null, HASH_NR_BUCKETS);

  return h;
}

/* NB. Unlike copy_hash, this does copy the strings into the new
 * pool. So a copy_sash is really a deep copy of the string hash and the
 * strings.
 */
sash
copy_sash (pool pool, sash h)
{
  sash new_h;
  int b, i;

  new_h = pmalloc (pool, sizeof *new_h);
  new_h->pool = pool;
  new_h->buckets = copy_vector (pool, h->buckets);

  /* Copy the buckets. */
  for (b = 0; b < vector_size (new_h->buckets); ++b)
    {
      vector v;

      vector_get (new_h->buckets, b, v);

      if (v)
	{
	  v = copy_vector (pool, v);
	  vector_replace (new_h->buckets, b, v);

	  /* Copy the string keys/values in this vector. */
	  for (i = 0; i < vector_size (v); ++i)
	    {
	      struct sash_bucket_entry entry;

	      vector_get (v, i, entry);
	      entry.key = pstrdup (pool, entry.key);
	      entry.value = pstrdup (pool, entry.value);
	      vector_replace (v, i, entry);
	    }
	}
    }

  return new_h;
}

int
_sash_get (sash h, const char *key, const char **ptr)
{
  int b, i;
  vector bucket;

  /* Find the appropriate bucket. */
  b = HASH (key, strlen (key), vector_size (h->buckets));
  vector_get (h->buckets, b, bucket);

  if (bucket == 0)
    {
      if (ptr) *ptr = 0;
      return 0;
    }

  /* Search this bucket linearly. */
  for (i = 0; i < vector_size (bucket); ++i)
    {
      struct sash_bucket_entry entry;

      vector_get (bucket, i, entry);
      if (strcmp (entry.key, key) == 0)
	{
	  if (ptr) *ptr = entry.value;
	  return 1;
	}
    }

  if (ptr) *ptr = 0;
  return 0;
}

int
sash_insert (sash h, const char *key, const char *value)
{
  int b, i, len = strlen (value);
  vector bucket;
  struct sash_bucket_entry entry;

  /* Find the appropriate bucket. */
  b = HASH (key, strlen (key), vector_size (h->buckets));
  vector_get (h->buckets, b, bucket);

  /* If there is no bucket there, we have to allocate a fresh vector. */
  if (bucket == 0)
    {
      bucket = new_vector (h->pool, struct sash_bucket_entry);
      vector_replace (h->buckets, b, bucket);
    }

  /* Search this bucket linearly. */
  for (i = 0; i < vector_size (bucket); ++i)
    {
      vector_get (bucket, i, entry);
      if (strcmp (entry.key, key) == 0)
	{
	  /* To avoid unnecessarily allocating more memory, we try to
	   * be clever here. If the existing allocation is large enough
	   * to store the new string, use it. Otherwise reallocate it
	   * to make it bigger.
	   */
	  if (len < entry.value_allocated)
	    memcpy (entry.value, value, len + 1);
	  else
	    {
	      entry.value = prealloc (h->pool, entry.value, len + 1);
	      memcpy (entry.value, value, len + 1);
	      entry.value_allocated = len + 1;
	    }

	  /* Replace this entry. */
	  vector_replace (bucket, i, entry);

	  return 1;
	}
    }

  /* Append to this bucket. */
  entry.key = pstrdup (h->pool, key);
  entry.value = pstrdup (h->pool, value);
  entry.value_allocated = len + 1;

  vector_push_back (bucket, entry);
  return 0;
}

int
sash_erase (sash h, const char *key)
{
  int b, i;
  vector bucket;
  struct sash_bucket_entry entry;

  /* Find the appropriate bucket. */
  b = HASH (key, strlen (key), vector_size (h->buckets));
  vector_get (h->buckets, b, bucket);

  if (bucket == 0) return 0;

  /* Search this bucket linearly. */
  for (i = 0; i < vector_size (bucket); ++i)
    {
      vector_get (bucket, i, entry);
      if (strcmp (entry.key, key) == 0)
	{
	  /* Remove this entry. */
	  vector_erase (bucket, i);

	  return 1;
	}
    }

  return 0;
}

inline vector
sash_keys_in_pool (sash h, pool p)
{
  int i, j;
  vector bucket, keys;
  struct sash_bucket_entry entry;

  keys = new_vector (p, char *);

  for (i = 0; i < vector_size (h->buckets); ++i)
    {
      vector_get (h->buckets, i, bucket);

      if (bucket)
	for (j = 0; j < vector_size (bucket); ++j)
	  {
	    char *key;

	    vector_get (bucket, j, entry);
	    key = pstrdup (p, entry.key);
	    vector_push_back (keys, key);
	  }
    }

  return keys;
}

vector
sash_keys (sash h)
{
  return sash_keys_in_pool (h, h->pool);
}

inline vector
sash_values_in_pool (sash h, pool p)
{
  int i, j;
  vector bucket, values;
  struct sash_bucket_entry entry;

  values = new_vector (p, char *);

  for (i = 0; i < vector_size (h->buckets); ++i)
    {
      vector_get (h->buckets, i, bucket);

      if (bucket)
	for (j = 0; j < vector_size (bucket); ++j)
	  {
	    char *value;

	    vector_get (bucket, j, entry);
	    value = pstrdup (p, entry.value);
	    vector_push_back (values, value);
	  }
    }

  return values;
}

vector
sash_values (sash h)
{
  return sash_values_in_pool (h, h->pool);
}

int
sash_size (sash h)
{
  vector bucket;
  int n = 0, i;

  for (i = 0; i < vector_size (h->buckets); ++i)
    {
      vector_get (h->buckets, i, bucket);
      n += bucket ? vector_size (bucket) : 0;
    }

  return n;
}

int
sash_get_buckets_used (sash h)
{
  vector bucket;
  int n = 0, i;

  for (i = 0; i < vector_size (h->buckets); ++i)
    {
      vector_get (h->buckets, i, bucket);
      n += bucket ? 1 : 0;
    }

  return n;
}

int
sash_get_buckets_allocated (sash h)
{
  return vector_size (h->buckets);
}

void
sash_set_buckets_allocated (sash h, int new_size)
{
  vector null = 0;

  /* The user has been warned not to call this after elements have been
   * inserted into the sash, and to make NEW_SIZE a power of 2.
   */
  if (vector_size (h->buckets) > new_size)
    vector_erase_range (h->buckets, new_size, vector_size (h->buckets));
  else if (vector_size (h->buckets) < new_size)
    vector_fill (h->buckets, null, new_size - vector_size (h->buckets));
}

/*----- SHASHes -----*/

struct shash_bucket_entry
{
  char *key;
  void *value;
};

shash
_shash_new (pool pool, size_t value_size)
{
  shash h;
  vector null = 0;

  h = pmalloc (pool, sizeof *h);
  h->pool = pool;
  h->value_size = value_size;
  h->buckets = new_vector (pool, sizeof (vector));
  vector_fill (h->buckets, null, HASH_NR_BUCKETS);

  return h;
}

/* NB. Unlike copy_hash, this does copy the strings into the new
 * pool. So a copy_shash is really a deep copy of the shash and the
 * strings.
 */
shash
copy_shash (pool pool, shash h)
{
  shash new_h;
  int b, i;

  new_h = pmalloc (pool, sizeof *new_h);
  new_h->pool = pool;
  new_h->value_size = h->value_size;
  new_h->buckets = copy_vector (pool, h->buckets);

  /* Copy the buckets. */
  for (b = 0; b < vector_size (new_h->buckets); ++b)
    {
      vector v;

      vector_get (new_h->buckets, b, v);

      if (v)
	{
	  v = copy_vector (pool, v);
	  vector_replace (new_h->buckets, b, v);

	  /* Copy the string keys in this vector. */
	  for (i = 0; i < vector_size (v); ++i)
	    {
	      struct shash_bucket_entry entry;

	      vector_get (v, i, entry);
	      entry.key = pstrdup (pool, entry.key);
	      entry.value = pmemdup (pool, entry.value, h->value_size);
	      vector_replace (v, i, entry);
	    }
	}
    }

  return new_h;
}

int
_shash_get (shash h, const char *key, void *value)
{
  const void *ptr;

  ptr = _shash_get_ptr (h, key);
  if (ptr == 0) return 0;

  if (value) memcpy (value, ptr, h->value_size);
  return 1;
}

const void *
_shash_get_ptr (shash h, const char *key)
{
  int b, i;
  vector bucket;

  /* Find the appropriate bucket. */
  b = HASH (key, strlen (key), vector_size (h->buckets));
  vector_get (h->buckets, b, bucket);

  if (bucket == 0)
    return 0;

  /* Search this bucket linearly. */
  for (i = 0; i < vector_size (bucket); ++i)
    {
      struct shash_bucket_entry entry;

      vector_get (bucket, i, entry);
      if (strcmp (entry.key, key) == 0)
	return entry.value;
    }

  return 0;
}

int
_shash_insert (shash h, const char *key, const void *value)
{
  int b, i;
  vector bucket;
  struct shash_bucket_entry entry;

  /* Find the appropriate bucket. */
  b = HASH (key, strlen (key), vector_size (h->buckets));
  vector_get (h->buckets, b, bucket);

  /* If there is no bucket there, we have to allocate a fresh vector. */
  if (bucket == 0)
    {
      bucket = new_vector (h->pool, struct shash_bucket_entry);
      vector_replace (h->buckets, b, bucket);
    }

  /* Search this bucket linearly. */
  for (i = 0; i < vector_size (bucket); ++i)
    {
      vector_get (bucket, i, entry);
      if (strcmp (entry.key, key) == 0)
	{
	  memcpy (entry.value, value, h->value_size);
	  /*entry.value = pmemdup (h->pool, value, h->value_size);*/

	  /* Replace this entry. */
	  vector_replace (bucket, i, entry);

	  return 1;
	}
    }

  /* Append to this bucket. */
  entry.key = pstrdup (h->pool, key);
  entry.value = pmemdup (h->pool, value, h->value_size);

  vector_push_back (bucket, entry);
  return 0;
}

int
shash_erase (shash h, const char *key)
{
  int b, i;
  vector bucket;
  struct shash_bucket_entry entry;

  /* Find the appropriate bucket. */
  b = HASH (key, strlen (key), vector_size (h->buckets));
  vector_get (h->buckets, b, bucket);

  if (bucket == 0) return 0;

  /* Search this bucket linearly. */
  for (i = 0; i < vector_size (bucket); ++i)
    {
      vector_get (bucket, i, entry);
      if (strcmp (entry.key, key) == 0)
	{
	  /* Remove this entry. */
	  vector_erase (bucket, i);

	  return 1;
	}
    }

  return 0;
}

inline vector
shash_keys_in_pool (shash h, pool p)
{
  int i, j;
  vector bucket, keys;
  struct shash_bucket_entry entry;

  keys = new_vector (p, char *);

  for (i = 0; i < vector_size (h->buckets); ++i)
    {
      vector_get (h->buckets, i, bucket);

      if (bucket)
	for (j = 0; j < vector_size (bucket); ++j)
	  {
	    char *key;

	    vector_get (bucket, j, entry);
	    key = pstrdup (p, entry.key);
	    vector_push_back (keys, key);
	  }
    }

  return keys;
}

vector
shash_keys (shash h)
{
  return shash_keys_in_pool (h, h->pool);
}

inline vector
shash_values_in_pool (shash h, pool p)
{
  int i, j;
  vector bucket, values;
  struct shash_bucket_entry entry;

  values = _vector_new (p, h->value_size);

  for (i = 0; i < vector_size (h->buckets); ++i)
    {
      vector_get (h->buckets, i, bucket);

      if (bucket)
	for (j = 0; j < vector_size (bucket); ++j)
	  {
	    vector_get (bucket, j, entry);
	    _vector_push_back (values, entry.value);
	  }
    }

  return values;
}

vector
shash_values (shash h)
{
  return shash_values_in_pool (h, h->pool);
}

int
shash_size (shash h)
{
  vector bucket;
  int n = 0, i;

  for (i = 0; i < vector_size (h->buckets); ++i)
    {
      vector_get (h->buckets, i, bucket);
      n += bucket ? vector_size (bucket) : 0;
    }

  return n;
}

int
shash_get_buckets_used (shash h)
{
  vector bucket;
  int n = 0, i;

  for (i = 0; i < vector_size (h->buckets); ++i)
    {
      vector_get (h->buckets, i, bucket);
      n += bucket ? 1 : 0;
    }

  return n;
}

int
shash_get_buckets_allocated (shash h)
{
  return vector_size (h->buckets);
}

void
shash_set_buckets_allocated (shash h, int new_size)
{
  vector null = 0;

  /* The user has been warned not to call this after elements have been
   * inserted into the shash, and to make NEW_SIZE a power of 2.
   */
  if (vector_size (h->buckets) > new_size)
    vector_erase_range (h->buckets, new_size, vector_size (h->buckets));
  else if (vector_size (h->buckets) < new_size)
    vector_fill (h->buckets, null, new_size - vector_size (h->buckets));
}
