/* An Apache-like pool allocator.
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

#ifndef POOL_H
#define POOL_H

#include <stdlib.h>

struct pool;
typedef struct pool *pool;

/* Function: new_pool - allocate a new pool
 *
 * Allocate a new pool. Pools must eventually be deleted explicitly
 * by calling @ref{delete_pool(3)}.
 *
 * Note that @code{new_pool} is now deprecated. It is almost always
 * better to create a subpool of the global pool, ie:
 *
 * @code{pool = new_subpool (global_pool);}
 *
 * This has the distinct advantage that your new pool will be cleaned
 * up properly if the process calls @code{exit}.
 *
 * See also: @ref{new_subpool(3)}, @ref{global_pool(3)},
 * @ref{delete_pool(3)}.
 */
extern pool new_pool (void);

/* Function: new_subpool - allocate a subpool of an existing pool
 *
 * Allocate a new subpool. The pool may either be deleted explicitly, or
 * else is deleted implicitly when the parent pool is deleted.
 */
extern pool new_subpool (pool);

/* Function: delete_pool - delete a pool
 *
 * Delete a pool or subpool. This also deletes any subpools that the
 * pool may own (and recursively subpools of those subpools, etc.)
 */
extern void delete_pool (pool);

/* Function: pmalloc - allocate memory in a pool
 * Function: pcalloc
 * Function: prealloc
 *
 * Allocate memory in a pool or, if pool is null, on the main heap
 * (equivalent to plain @code{malloc}). If memory is allocated in a real
 * pool, then it is automatically freed when the pool is deleted.
 *
 * Memory returned is word-aligned.
 *
 * If a memory allocation fails, the @code{bad_malloc_handler} function is
 * called (which defaults to just calling @ref{abort(3)}).
 *
 * @code{pcalloc} is identical to @code{pmalloc} but also sets the memory
 * to zero before returning it.
 *
 * @code{prealloc} increases the size of an existing memory allocation.
 * @code{prealloc} might move the memory in the process of reallocating it.
 *
 * Bugs: @code{prealloc} cannot reduce the size of an existing memory
 * allocation.
 */
extern void *pmalloc (pool, size_t n);
extern void *pcalloc (pool, size_t nmemb, size_t size);
extern void *prealloc (pool, void *ptr, size_t n);

/* Function: pool_register_malloc - allow pool to own malloc'd memory
 *
 * Register an anonymous area of malloc-allocated memory which
 * will be freed (with @ref{free(3)}) when the pool is deleted.
 */
extern void pool_register_malloc (pool, void *ptr);

/* Function: pool_register_fd - allow pool to own file descriptor
 *
 * Register a file descriptor to be closed when the pool is deleted.
 * There is no way to unregister a file descriptor. If you wish to
 * do that, then you probably want to register the fd in a subpool.
 */
extern void pool_register_fd (pool, int fd);

/* Function: pool_register_cleanup_fn - call function when pool is deleted
 *
 * Register a function to be called when the pool is deleted. There
 * is no way to unregister this function. If you wish to do that, then
 * you probably want to register it in a subpool.
 */
extern void pool_register_cleanup_fn (pool, void (*fn) (void *), void *data);

/* Function: pool_set_bad_malloc_handler - set handler for when malloc fails
 *
 * Set the function which is called when an underlying malloc or realloc
 * operation fails. The default is that @ref{abort(3)} is called.
 *
 * This function returns the previous handler.
 */
extern void (*pool_set_bad_malloc_handler (void (*fn) (void))) (void);

struct pool_stats
{
  int nr_subpools;
  int struct_size;
};

/* Function: pool_get_stats - get statistics from the pool
 *
 * Return various statistics collected for the pool. This function
 * fills in the @code{stats} argument which should point to a
 * structure of type @code{struct pool_stats}. @code{n} should be
 * set to the size of this structure.
 *
 * @code{struct pool_stats} currently contains the following fields:
 *
 * @code{nr_subpools}: The number of subpools (including the current pool).
 *
 * @code{struct_size}: The memory overhead used by the pool allocator
 * itself to store structures. This includes subpools.
 */
extern void pool_get_stats (const pool, struct pool_stats *stats, size_t n);

/* Obsolete calls for backwards compatibility. These will be removed soon. */
#define pool_get_size(p) (-1)
#define pool_get_areas(p) (-1)
#define pool_get_allocations(p) (-1)
#define pool_nr_subpools(p) (-1)

#ifndef NO_GLOBAL_POOL
/* Variable: global_pool - the global pool for global allocations
 *
 * This is the global pool which is allocated before @code{main()} is called
 * and deleted automatically upon program exit. Items allocated on this
 * pool cannot be deleted until the program ends, and so it is a good
 * idea not to allocate short-lived items here.
 */
extern pool global_pool;
#endif /* !NO_GLOBAL_POOL */

#endif /* POOL_H */
