/* A vector class.
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

#ifndef VECTOR_H
#define VECTOR_H

#include <pool.h>

struct vector
{
  pool pool;
  size_t size;
  void *data;
  int used, allocated;
};

typedef struct vector *vector;

/* Function: new_vector - allocate a new vector
 * Function: _vector_new
 *
 * Allocate a new vector in @code{pool} of type @code{type}. The
 * first form is just a macro which evaluates the size of @code{type}.
 * The second form creates a vector with elements of the given
 * @code{size} directly.
 */
#define new_vector(pool,type) _vector_new ((pool), sizeof (type))
extern vector _vector_new (pool, size_t size);

/* Function: copy_vector - copy a vector
 * Function: new_subvector
 *
 * Copy a vector @code{v} into pool @code{pool}. If the vector
 * contains pointers, then this function will not copy the pointed-to
 * data as well: you will need to copy this yourself if appropriate.
 *
 * @code{new_subvector} creates a copy of part of an existing vector.
 * The new vector contains the @code{j-i} elements of the old vector
 * starting at element number @code{i} and finishing at element number
 * @code{j-1}.
 */
extern vector copy_vector (pool, vector v);
extern vector new_subvector (pool, vector v, int i, int j);

/* Function: vector_push_back - push and pop objects into and out of vectors
 * Function: _vector_push_back
 * Function: vector_pop_back
 * Function: _vector_pop_back
 * Function: vector_push_front
 * Function: _vector_push_front
 * Function: vector_pop_front
 * Function: _vector_pop_front
 *
 * The @code{*_push_*} functions push objects onto vectors.
 *
 * The @code{*_pop_*} functions pop objects off vectors into local variables.
 *
 * The @code{*_front} functions push and pop objects off the front
 * of a vector. This type of operation is not very efficient, because
 * it involves moving all other elements of the vector up or down
 * by one place.
 *
 * The @code{*_back} functions push and pop elements
 * off the end of the vector, which is efficient.
 *
 * Each function has two forms: a macro version and an underlying
 * function.
 *
 * Array indexes are checked. You cannot pop an empty vector. If
 * @code{ptr} is @code{NULL} then the popped object is discarded.
 */
#define vector_push_back(v,obj) _vector_push_back((v),&(obj))
extern void _vector_push_back (vector, const void *ptr);
#define vector_pop_back(v,obj) _vector_pop_back((v),&(obj))
extern void _vector_pop_back (vector, void *ptr);
#define vector_push_front(v,obj) _vector_push_front((v),&(obj))
extern void _vector_push_front (vector, const void *ptr);
#define vector_pop_front(v,obj) _vector_pop_front((v),&(obj))
extern void _vector_pop_front (vector, void *ptr);

/* Function: vector_push_back_vector - push list of objects on to vector
 * Function: vector_push_front_vector
 *
 * @code{vector_push_back_vector} appends the elements of vector
 * @code{w} on to the end of vector @code{v}.
 *
 * @code{vector_push_front_vector} prepends the elements of vector
 * @code{w} on to the front of vector @code{v}.
 *
 * In both cases, vector @code{w} is left unchanged.
 *
 * See also: @ref{vector_push_back(3)}, @ref{vector_push_front(3)}.
 */
extern void vector_push_back_vector (vector v, const vector w);
extern void vector_push_front_vector (vector v, const vector w);

/* Function: vector_get - array-style indexing for vectors
 * Function: _vector_get
 * Function: vector_get_ptr
 * Function: _vector_get_ptr
 *
 * @code{vector_get} copies the element at index @code{v[i]} into
 * local variable @code{obj}. The vector is unaffected.
 *
 * @code{_vector_get} copies the element into the memory pointed
 * to by @code{ptr}. If @code{ptr} is @code{NULL} then the effect
 * is simply to check that element @code{v[i]} exists.
 *
 * @code{vector_get_ptr} and @code{_vector_get_ptr} are used to
 * get a pointer to an element in the vector. This pointer actually
 * points to the vector's internal data array. It is only valid
 * as long as the user does not cause the internal data array to
 * be reallocated or moved - typically this can happen when the
 * user performs some operation which inserts more elements into
 * the array.
 *
 * Array indexes are checked. An attempt to access an out-of-bounds
 * element will result in a call to @ref{abort(3)}.
 */
#define vector_get(v,i,obj) _vector_get((v),(i),&(obj))
extern void _vector_get (vector, int i, void *ptr);
#define vector_get_ptr(v,i,ptr) (ptr) =((typeof (ptr))_vector_get_ptr((v),(i)))
extern const void *_vector_get_ptr (vector, int i);

/* Function: vector_insert - insert elements into a vector
 * Function: _vector_insert
 * Function: vector_insert_array
 *
 * @code{vector_insert} inserts a single object @code{obj} before element
 * @code{i}. All other elements are moved up to make space.
 *
 * @code{vector_insert_array} inserts an array of @code{n} objects
 * starting at address @code{ptr} into the vector before index
 * @code{i}.
 *
 * Array indexes are checked.
 */
#define vector_insert(v,i,obj) _vector_insert((v),(i),&(obj))
extern void _vector_insert (vector, int i, const void *ptr);
extern void vector_insert_array (vector v, int i, const void *ptr, int n);

/* Function: vector_replace - replace elements of a vector
 * Function: _vector_replace
 * Function: vector_replace_array
 *
 * @code{vector_replace} replaces the single element at
 * @code{v[i]} with object @code{obj}.
 *
 * @code{vector_replace_array} replaces the @code{n} elements
 * in the vector starting at index @code{i} with the @code{n}
 * elements from the array @code{ptr}.
 * 
 * Array indexes are checked.
 */
#define vector_replace(v,i,obj) _vector_replace((v),(i),&(obj))
extern void _vector_replace (vector, int i, const void *ptr);
extern void vector_replace_array (vector v, int i, const void *ptr, int n);

/* Function: vector_erase - erase elements of a vector
 * Function: vector_erase_range
 * Function: vector_clear
 *
 * @code{vector_erase} removes the element @code{v[i]}, shuffling
 * the later elements down by one place to fill the space.
 *
 * @code{vector_erase_range} removes a range of elements @code{v[i]}
 * to @code{v[j-1]} (@code{i <= j}), shuffling later elements down
 * to fill the space.
 *
 * @code{vector_clear} removes all elements from the vector, setting
 * its size to @code{0}.
 *
 * Array indexes are checked.
 */
extern void vector_erase (vector v, int i);
extern void vector_erase_range (vector v, int i, int j);
extern void vector_clear (vector v);

/* Function: vector_fill - fill a vector with identical elements
 * Function: _vector_fill
 *
 * @code{vector_fill} appends @code{n} identical copies of
 * @code{obj} to the vector. It is equivalent to calling
 * @ref{vector_push_back(3)} in a loop @code{n} times.
 */
#define vector_fill(v,obj,n) _vector_fill((v),&(obj),(n))
extern void _vector_fill (vector, const void *ptr, int n);

/* Function: vector_size - return the size of a vector
 *
 * Return the size (ie. number of elements) in the vector.
 */
#define vector_size(v) ((v)->used)

/* Function: vector_allocated - return the space allocated to a vector
 *
 * Return the amount of space which has been allocated for storing
 * elements of the vector. This is different from the number of
 * elements actually stored by the vector (see @ref{vector_size(3)})
 * and also different from the size of each element in bytes
 * (see @ref{vector_element_size(3)}).
 */
#define vector_allocated(v) ((v)->allocated)

/* Function: vector_element_size - return the size of each element of a vector
 *
 * Return the size in bytes of each element in vector.
 */
#define vector_element_size(v) ((v)->size)

/* Function: vector_reallocate - change allocation for a vector
 *
 * Increase the amount of space allocated to a vector. See also
 * @ref{vector_allocated(3)}. This function can be used to avoid
 * the vector itself making too many calls to the underlying
 * @ref{c2_prealloc(3)}, particularly if you know in advance exactly
 * how many elements the vector will contain.
 */
extern void vector_reallocate (vector v, int n);

/* Function: vector_grep - produce a new vector containing elements of the old vector which match a boolean function
 *
 * This macro calls @code{match_fn(&t)} for each element @code{t} of
 * vector @code{v}. It constructs and returns a new vector containing
 * all those elements where the function returns true.
 */
extern vector vector_grep (pool, vector v, int (*match_fn) (const void *));

/* Function: vector_grep_pool - produce a new vector containing elements of the old vector which match a boolean function
 *
 * This macro calls @code{match_fn(pool, &t)} for each element @code{t} of
 * vector @code{v}. It constructs and returns a new vector containing
 * all those elements where the function returns true.
 */
extern vector vector_grep_pool (pool, vector v, int (*match_fn) (pool, const void *));

/* Function: vector_map - apply function to each element of a vector
 * Function: _vector_map
 *
 * Call @code{map_fn(&t, &r)} for each element @code{t} of vector @code{v}.
 * The result (@code{r}) should have type @code{result_type}
 * and these are concatenated to form a new vector which is returned.
 */
#define vector_map(pool,v,map_fn,result_type) _vector_map ((pool), (v), (map_fn), sizeof (result_type))
extern vector _vector_map (pool, vector v, void (*map_fn) (const void *, void *), size_t result_size);

/* Function: vector_map_pool - apply function to each element of a vector
 * Function: _vector_map_pool
 *
 * Call @code{map_fn(pool, &t, &r)} for each element @code{t} of vector
 * @code{v}. The result (@code{r}) should have type @code{result_type}
 * and these are concatenated to form a new vector which is returned.
 */
#define vector_map_pool(pool,v,map_fn,result_type) _vector_map_pool ((pool), (v), (map_fn), sizeof (result_type))
extern vector _vector_map_pool (pool, vector v, void (*map_fn_pool) (pool, const void *, void *), size_t result_size);

/* Function: vector_sort - sort a vector in-place
 * Function: _vector_sort
 *
 * Sort a vector in-place, comparing elements using @code{compare_fn}.
 */
#define vector_sort(v,compare_fn) _vector_sort ((v), (int (*)(const void *,const void *)) (compare_fn))
extern void _vector_sort (vector v, int (*compare_fn) (const void *, const void *));

/* Function: vector_compare - compare two vectors
 * Function: _vector_compare
 *
 * Compare two vectors. This returns 0 if the two vectors are
 * identical. It returns > 0 if @code{v1} > @code{v2}. This
 * returns < 0 if @code{v1} < @code{v2}.
 */
#define vector_compare(v1,v2,compare_fn) _vector_compare ((v1), (v2), (int (*)(const void *,const void *)) (compare_fn))
extern int _vector_compare (vector, vector, int (*compare_fn) (const void *, const void *));

/* Function: vector_reverse - reverse the elements of a vector in-place
 *
 * Reverse the elements of a vector in-place.
 */
extern void vector_reverse (vector v);

/* Function: vector_swap - swap two elements of a vector
 *
 * Swap elements @code{i} and @code{j} of vector @code{v}.
 */
extern void vector_swap (vector v, int i, int j);

#endif /* VECTOR_H */
