/* Generalized hash and string hash (sash) classes.
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

#ifndef HASH_H
#define HASH_H

#include <vector.h>

/* Note, hash and sash are identical but for the fact that
 * hash maps fixed sized keys to values (eg. int -> int or
 * int -> struct) and sash maps nul-terminated ASCII strings
 * to nul-terminated ASCII strings (ie. string -> string ONLY).
 * shash maps nul-terminated ASCII strings to anything else.
 */

struct hash;
typedef struct hash *hash;

struct sash;
typedef struct sash *sash;

struct shash;
typedef struct shash *shash;

/* Function: new_hash - allocate a new hash
 * Function: _hash_new
 *
 * Allocate a new hash in @code{pool} mapping
 * @code{key_type} to @code{value_type}. You can map both
 * simple types like @code{int} and also aggregate types
 * like structures and unions. However, beware of aggregate
 * types that might contain 'holes' because of alignment --
 * such types will probably not work as you expect, particularly
 * if used as keys.
 *
 * If you wish to have
 * a hash which maps strings to something, then calling
 * @code{new_hash(pool, char *, char *)} (for example) will not do what
 * you expect. You are better to use either a sash (string to string hash)
 * or a shash (string to anything hash) instead (see
 * @ref{new_sash(3)} and @ref{new_shash(3)}).
 */
#define new_hash(pool,key_type,value_type) _hash_new ((pool), sizeof (key_type), sizeof (value_type))
extern hash _hash_new (pool, size_t key_size, size_t value_size);

/* Function: copy_hash - copy a hash
 *
 * Copy a hash into a new pool. This function copies the keys
 * and values, but if keys and values are pointers, then it does
 * not perform a 'deep' copy.
 */
extern hash copy_hash (pool, hash);

/* Function: hash_get - look up in a hash
 * Function: _hash_get
 * Function: hash_get_ptr
 * Function: _hash_get_ptr
 * Function: hash_exists
 *
 * Get the @code{value} associated with key @code{key} and return true.
 * If there is no @code{value} associated with @code{key}, this returns
 * false and @code{value} is left unchanged.
 *
 * The @code{*_ptr} variants return a pointer rather than copying
 * out the entire value object. The pointer is typically only
 * valid for a short period of time. Particularly if you insert
 * or remove elements from the hash, this pointer may become
 * invalid.
 *
 * @code{hash_exists} simply tests whether or not @code{key} exists
 * in the hash. If so, it returns true, otherwise false.
 */
#define hash_get(h,key,value) _hash_get ((h), &(key), &(value))
extern int _hash_get (hash, const void *key, void *value);
#define hash_get_ptr(h,key,ptr) ((ptr) = ((typeof (ptr))_hash_get_ptr ((h), &(key))))
extern const void *_hash_get_ptr (hash, const void *key);
#define hash_exists(h,key) (_hash_get_ptr ((h), &(key)) ? 1 : 0)

/* Function: hash_insert - insert a (key, value) pair into a hash
 * Function: _hash_insert
 *
 * Insert an element (@code{key}, @code{value}) into the hash.
 * If @code{key} already
 * exists in the hash, then the existing value is replaced by
 * @code{value}
 * and the function returns true. If there was no previous @code{key}
 * in the hash then this function returns false.
 */
#define hash_insert(h,key,value) _hash_insert((h),&(key),&(value))
extern int _hash_insert (hash, const void *key, const void *value);

/* Function: hash_erase - erase a key from a hash
 * Function: _hash_erase
 *
 * Erase @code{key} from the hash. If an element was erased,
 * this returns true, else this returns false.
 */
#define hash_erase(h,key) _hash_erase((h),&(key))
extern int _hash_erase (hash, const void *key);

/* Function: hash_keys - return a vector of the keys or values in a hash
 * Function: hash_keys_in_pool
 * Function: hash_values
 * Function: hash_values_in_pool
 * 
 * Return a vector containing all the keys or values of hash. The
 * @code{*_in_pool} variants allow you to allocate the vector in
 * another pool (the default is to allocate the vector in the same
 * pool as the hash).
 */
extern vector hash_keys (hash);
extern vector hash_keys_in_pool (hash, pool);
extern vector hash_values (hash);
extern vector hash_values_in_pool (hash, pool);

/* Function: hash_size - return the number of (key, value) pairs in a hash
 *
 * Count the number of (key, value) pairs in the hash.
 */
extern int hash_size (hash);

/* Function: hash_get_buckets_used - return the number of buckets in a hash
 * Function: hash_get_buckets_allocated
 *
 * Return the number of hash buckets used and allocated. The number of
 * buckets allocated is always a power of 2. See also
 * @ref{hash_set_buckets_allocated} to change the number used
 * in the hash.
 */
extern int hash_get_buckets_used (hash);
extern int hash_get_buckets_allocated (hash);

/* Function: hash_set_buckets_allocated - set the number of buckets
 *
 * Set the number of buckets allocated. You may ONLY do this when you
 * have just created the hash and before you have inserted any elements.
 * Otherwise the results are undefined (and probably bad). The number
 * of buckets MUST be a power of 2.
 */
extern void hash_set_buckets_allocated (hash, int);

/* Function: new_sash - allocate a new sash (string hash)
 *
 * Allocate a new sash in @code{pool} mapping
 * strings to strings.
 *
 * Use a string hash in preference to a hash of
 * @code{char *} -> @code{char *} which will probably not
 * quite work as you expect.
 */
extern sash new_sash (pool);

/* Function: copy_sash - copy a sash
 *
 * Copy a sash into a new pool. This function copies the keys
 * and values, but if keys and values are pointers, then it does
 * not perform a 'deep' copy.
 */
extern sash copy_sash (pool, sash);

/* Function: sash_get - look up in a sash
 * Function: _sash_get
 * Function: sash_exists
 *
 * Get the @code{value} associated with key @code{key} and return true.
 * If there is no @code{value} associated with @code{key}, this returns
 * false and @code{value} is left unchanged.
 *
 * @code{sash_exists} simply tests whether or not @code{key} exists
 * in the sash. If so, it returns true, otherwise false.
 */
#define sash_get(sash,key,value) _sash_get((sash),(key),&(value))
extern int _sash_get (sash, const char *key, const char **ptr);
#define sash_exists(sash,key) _sash_get ((sash), (key), 0)

/* Function: sash_insert - insert a (key, value) pair into a sash
 *
 * Insert an element (@code{key}, @code{value}) into the sash.
 * If @code{key} already
 * exists in the sash, then the existing value is replaced by
 * @code{value}
 * and the function returns true. If there was no previous @code{key}
 * in the sash then this function returns false.
 */
extern int sash_insert (sash, const char *key, const char *value);

/* Function: sash_erase - erase a key from a sash
 *
 * Erase @code{key} from the sash. If an element was erased,
 * this returns true, else this returns false.
 */
extern int sash_erase (sash, const char *key);

/* Function: sash_keys - return a vector of the keys or values in a sash
 * Function: sash_keys_in_pool
 * Function: sash_values
 * Function: sash_values_in_pool
 * 
 * Return a vector containing all the keys or values of sash. The
 * @code{*_in_pool} variants allow you to allocate the vector in
 * another pool (the default is to allocate the vector in the same
 * pool as the sash).
 */
extern vector sash_keys (sash);
extern vector sash_keys_in_pool (sash, pool);
extern vector sash_values (sash);
extern vector sash_values_in_pool (sash, pool);

/* Function: sash_size - return the number of (key, value) pairs in a sash
 *
 * Count the number of (key, value) pairs in the sash.
 */
extern int sash_size (sash);

/* Function: sash_get_buckets_used - return the number of buckets in a sash
 * Function: sash_get_buckets_allocated
 *
 * Return the number of sash buckets used and allocated. The number of
 * buckets allocated is always a power of 2. See also
 * @ref{sash_set_buckets_allocated} to change the number used
 * in the sash.
 */
extern int sash_get_buckets_used (sash);
extern int sash_get_buckets_allocated (sash);

/* Function: sash_set_buckets_allocated - set the number of buckets
 *
 * Set the number of buckets allocated. You may ONLY do this when you
 * have just created the sash and before you have inserted any elements.
 * Otherwise the results are undefined (and probably bad). The number
 * of buckets MUST be a power of 2.
 */
extern void sash_set_buckets_allocated (sash, int);

/* Function: new_shash - allocate a new shash (string -> something hash)
 *
 * Allocate a new shash in @code{pool} mapping
 * strings to strings.
 *
 * Use a shash in preference to a hash of
 * @code{char *} -> something which will probably not
 * quite work as you expect.
 */
#define new_shash(pool,value_type) _shash_new ((pool), sizeof (value_type))
extern shash _shash_new (pool, size_t value_size);

/* Function: copy_shash - copy a shash
 *
 * Copy a shash into a new pool. This function copies the keys
 * and values, but if keys and values are pointers, then it does
 * not perform a 'deep' copy.
 */
extern shash copy_shash (pool, shash);

/* Function: shash_get - look up in a shash
 * Function: _shash_get
 * Function: shash_get_ptr
 * Function: _shash_get_ptr
 * Function: shash_exists
 *
 * Get the @code{value} associated with key @code{key} and return true.
 * If there is no @code{value} associated with @code{key}, this returns
 * false and @code{value} is left unchanged.
 *
 * The @code{*_ptr} variants return a pointer rather than copying
 * out the entire value object. The pointer is typically only
 * valid for a short period of time. Particularly if you insert
 * or remove elements from the shash, this pointer may become
 * invalid.
 *
 * @code{shash_exists} simply tests whether or not @code{key} exists
 * in the shash. If so, it returns true, otherwise false.
 */
#define shash_get(shash,key,value) _shash_get((shash),(key),&(value))
extern int _shash_get (shash, const char *key, void *value);
#define shash_get_ptr(h,key,ptr) ((ptr) = ((typeof (ptr))_shash_get_ptr ((h),(key))))
extern const void *_shash_get_ptr (shash, const char *key);
#define shash_exists(shash,key) (_shash_get_ptr ((shash), (key)) ? 1 : 0)

/* Function: shash_insert - insert a (key, value) pair into a shash
 * Function: _shash_insert
 *
 * Insert an element (@code{key}, @code{value}) into the shash.
 * If @code{key} already
 * exists in the shash, then the existing value is replaced by
 * @code{value}
 * and the function returns true. If there was no previous @code{key}
 * in the shash then this function returns false.
 */
#define shash_insert(h,key,value) _shash_insert((h),(key),&(value))
extern int _shash_insert (shash, const char *key, const void *value);

/* Function: shash_erase - erase a key from a shash
 *
 * Erase @code{key} from the shash. If an element was erased,
 * this returns true, else this returns false.
 */
extern int shash_erase (shash, const char *key);

/* Function: shash_keys - return a vector of the keys or values in a shash
 * Function: shash_keys_in_pool
 * Function: shash_values
 * Function: shash_values_in_pool
 * 
 * Return a vector containing all the keys or values of shash. The
 * @code{*_in_pool} variants allow you to allocate the vector in
 * another pool (the default is to allocate the vector in the same
 * pool as the shash).
 */
extern vector shash_keys (shash);
extern vector shash_keys_in_pool (shash, pool);
extern vector shash_values (shash);
extern vector shash_values_in_pool (shash, pool);

/* Function: shash_size - return the number of (key, value) pairs in a shash
 *
 * Count the number of (key, value) pairs in the shash.
 */
extern int shash_size (shash);

/* Function: shash_get_buckets_used - return the number of buckets in a shash
 * Function: shash_get_buckets_allocated
 *
 * Return the number of shash buckets used and allocated. The number of
 * buckets allocated is always a power of 2. See also
 * @ref{shash_set_buckets_allocated} to change the number used
 * in the shash.
 */
extern int shash_get_buckets_used (shash);
extern int shash_get_buckets_allocated (shash);

/* Function: shash_set_buckets_allocated - set the number of buckets
 *
 * Set the number of buckets allocated. You may ONLY do this when you
 * have just created the shash and before you have inserted any elements.
 * Otherwise the results are undefined (and probably bad). The number
 * of buckets MUST be a power of 2.
 */
extern void shash_set_buckets_allocated (shash, int);

#endif /* HASH_H */
