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

#include "../nonppr_misc/c2lib/config.h"

#if SIZEOF_VOID_P != SIZEOF_LONG
#error "This library currently assumes that sizeof (void *) == sizeof (long), which is not the case on your platform. This may cause the library to crash at runtime."
#endif

/* If set, then calls to c2_pmalloc will initialise the memory to 0xefefef...,
 * helping to catch uninitialised memory problems. This is very useful for
 * debugging new code, but should be turned off on production systems.
 */
#define DEBUG_UNINITIALISED_MEMORY 1

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <pool.h>

struct _pool_allocs
{
  struct _pool_allocs *next;

  /* The flags field contains:
   * bit  31	== if set, this structure shouldn't be freed
   * bits 30-16 == number of slots in the structure
   * bits 15- 0 == number of slots used in the structure.
   */
  unsigned flags;

#define _PA_NO_FREE(pa) ((pa)->flags & 0x80000000U)
#define _PA_SLOTS(pa) (((pa)->flags & 0x7fff0000U) >> 16)
#define _PA_SLOTS_USED(pa) ((pa)->flags & 0xffffU)

  void *slot[0];
} __attribute__((packed));

struct _pool_cleanup_slot
{
  void (*fn) (void *);
  void *data;
};

struct _pool_cleanups
{
  struct _pool_cleanups *next;

  /* The flags field contains:
   * bit  31	== if set, this structure shouldn't be freed
   * bits 30-16 == number of slots in the structure
   * bits 15- 0 == number of slots used in the structure.
   */
  unsigned flags;

#define _PC_NO_FREE(pc) ((pc)->flags & 0x80000000U)
#define _PC_SLOTS(pc) (((pc)->flags & 0x7fff0000U) >> 16)
#define _PC_SLOTS_USED(pc) ((pc)->flags & 0xffffU)

  struct _pool_cleanup_slot slot[0];
} __attribute__((packed));

#define INITIAL_PA_SLOTS 16U
#define MAX_PA_SLOTS	 16384U /* Must be <= 16384 */
#define INITIAL_PC_SLOTS 2U
#define MAX_PC_SLOTS	 16384U /* Must be <= 16384 */

struct pool
{
  /* If this is a subpool, then this points to the parent. */
  struct pool *parent_pool;

  /* When subpools are stored on a list, this is used to link the list. */
  struct pool *next;

  /* Sub-pools. */
  struct pool *subpool_list;

  /* Pointer to head block of memory allocations. */
  struct _pool_allocs *allocs;

  /* Pointer to head block of clean-up functions. */
  struct _pool_cleanups *cleanups;
};

#ifndef NO_GLOBAL_POOL
pool global_pool;
#endif

static int trace_fd = -1;
static const char *trace_filename = 0;

static void (*bad_malloc_handler) (void) = abort;
#ifndef NO_GLOBAL_POOL
static void alloc_global_pool (void) __attribute__((constructor));
static void free_global_pool (void) __attribute__((destructor));
#endif
static void open_trace_file (void) __attribute__((constructor));
static void trace (const char *fn, void *caller, struct pool *ptr1, void *ptr2, void *ptr3, int i1);

#define TRACE(ptr1, ptr2, ptr3, i1) do { if (trace_filename) trace (__PRETTY_FUNCTION__, __builtin_return_address (0), (ptr1), (ptr2), (ptr3), (i1)); } while (0)

pool
new_pool ()
{
  /* The amount of space required for pool + allocs + cleanups. */
  const int size
	= sizeof (struct pool) +
	sizeof (struct _pool_allocs) +
	INITIAL_PA_SLOTS * sizeof (void *) +
	sizeof (struct _pool_cleanups) +
	INITIAL_PC_SLOTS * sizeof (struct _pool_cleanup_slot);

  pool p = malloc (size);
  if (p == 0) bad_malloc_handler ();

  memset (p, 0, size);

  p->allocs = (struct _pool_allocs *) ((char *)p + sizeof (struct pool));
  p->cleanups = (struct _pool_cleanups *)
	((char *)p + sizeof (struct pool) + sizeof (struct _pool_allocs)
	 + INITIAL_PA_SLOTS * sizeof (void *));

  p->allocs->flags = 0x80000000U | INITIAL_PA_SLOTS << 16;
  p->cleanups->flags = 0x80000000U | INITIAL_PC_SLOTS << 16;

  TRACE (p, 0, 0, 0);

  return p;
}

pool
new_subpool (pool parent)
{
  pool p = new_pool ();
  p->parent_pool = parent;

  p->next = parent->subpool_list;
  parent->subpool_list = p;

  TRACE (p, parent, 0, 0);

  return p;
}

static inline void
_do_cleanups (pool p)
{
  struct _pool_cleanups *pc, *pc_next;
  int i;

  for (pc = p->cleanups; pc; pc = pc_next)
	{
	  pc_next = pc->next;

	  for (i = 0; i < _PC_SLOTS_USED (pc); ++i)
		pc->slot[i].fn (pc->slot[i].data);
	  if (!_PC_NO_FREE (pc))
		free (pc);
	}
}

static inline void
_do_frees (pool p)
{
  struct _pool_allocs *pa, *pa_next;
  int i;

  for (pa = p->allocs; pa; pa = pa_next)
	{
	  pa_next = pa->next;

	  for (i = 0; i < _PA_SLOTS_USED (pa); ++i)
		free (pa->slot[i]);
	  if (!_PA_NO_FREE (pa))
		free (pa);
	}
}

void
delete_pool (pool p)
{
  _do_cleanups (p);

  /* Clean up any sub-pools. */
  while (p->subpool_list) delete_pool (p->subpool_list);

  _do_frees (p);

  /* Do I have a parent? If so, remove myself from my parent's subpool
   * list.
   */
  if (p->parent_pool)
	{
	  pool parent = p->parent_pool, this, last = 0;

	  for (this = parent->subpool_list; this; last = this, this = this->next)
		{
		  if (this == p)
			{
			  /* Remove this one. */
			  if (last != 0)
				last->next = this->next;
			  else
				parent->subpool_list = this->next;

			  goto found_me;
			}
		}

	  abort ();					/* Oops - self not found on subpool list. */
	found_me:;
	}

  free (p);

  TRACE (p, 0, 0, 0);
}

void *
c2_pmalloc (pool p, size_t n)
{
  void *ptr;

  ptr = malloc (n);
  if (ptr == 0) bad_malloc_handler ();

#if DEBUG_UNINITIALISED_MEMORY
  memset (ptr, 0xef, n);
#endif

  pool_register_malloc (p, ptr);

  TRACE (p, ptr, 0, n);

  return ptr;
}

void *
c2_pcalloc (pool p, size_t nmemb, size_t size)
{
  void *ptr = c2_pmalloc (p, nmemb * size);
  if (ptr) memset (ptr, 0, nmemb * size);
  return ptr;
}

void *
c2_prealloc (pool p, void *ptr, size_t n)
{
  struct _pool_allocs *pa;
  int i;
  void *new_ptr;

  if (ptr == 0)
	return c2_pmalloc (p, n);

  new_ptr = realloc (ptr, n);
  if (new_ptr == 0) bad_malloc_handler ();

  /* XXX This is inefficient. We need to search through the
   * allocations to find this one and update the pointer.
   */
  if (ptr != new_ptr)
	{
	  for (pa = p->allocs; pa; pa = pa->next)
		{
		  for (i = 0; i < _PA_SLOTS_USED (pa); ++i)
			if (pa->slot[i] == ptr)
			  {
				pa->slot[i] = new_ptr;
				goto found;
			  }
		}
	  abort ();

	found:;
	}

  TRACE (p, ptr, new_ptr, n);

  return new_ptr;
}

void
pool_register_cleanup_fn (pool p, void (*fn) (void *), void *data)
{
  unsigned nr_slots;
  struct _pool_cleanups *pc;

  if (_PC_SLOTS_USED (p->cleanups) < _PC_SLOTS (p->cleanups))
	{
	again:
	  p->cleanups->slot[_PC_SLOTS_USED(p->cleanups)].fn = fn;
	  p->cleanups->slot[_PC_SLOTS_USED(p->cleanups)].data = data;
	  p->cleanups->flags++;
	  return;
	}

  /* Allocate a new block of cleanup slots. */
  nr_slots = _PC_SLOTS (p->cleanups);
  if (nr_slots < MAX_PC_SLOTS)
	nr_slots *= 2;

  pc = malloc (sizeof (struct _pool_cleanups) +
			   nr_slots * sizeof (struct _pool_cleanup_slot));
  if (pc == 0) bad_malloc_handler ();
  pc->next = p->cleanups;
  pc->flags = nr_slots << 16;
  p->cleanups = pc;

  goto again;
}

void
pool_register_malloc (pool p, void *ptr)
{
  unsigned nr_slots;
  struct _pool_allocs *pa;

  if (_PA_SLOTS_USED (p->allocs) < _PA_SLOTS (p->allocs))
	{
	again:
	  p->allocs->slot[_PA_SLOTS_USED(p->allocs)] = ptr;
	  p->allocs->flags++;
	  return;
	}

  /* Allocate a new block of slots. */
  nr_slots = _PA_SLOTS (p->allocs);
  if (nr_slots < MAX_PA_SLOTS)
	nr_slots *= 2;

  pa = malloc (sizeof (struct _pool_allocs) + nr_slots * sizeof (void *));
  if (pa == 0) bad_malloc_handler ();
  pa->next = p->allocs;
  pa->flags = nr_slots << 16;
  p->allocs = pa;

  goto again;
}

static void
_pool_close (void *fdv)
{
  long fd = (long) fdv;
  close (fd);
}

void
pool_register_fd (pool p, int fd)
{
  pool_register_cleanup_fn (p, _pool_close, (void *) (long) fd);
}

#ifndef NO_GLOBAL_POOL
static void
alloc_global_pool ()
{
  global_pool = new_pool ();
}

static void
free_global_pool ()
{
  delete_pool (global_pool);
}
#endif /* !NO_GLOBAL_POOL */

void (*
pool_set_bad_malloc_handler (void (*fn) (void))) (void)
{
  void (*old_fn) (void) = bad_malloc_handler;
  bad_malloc_handler = fn;
  return old_fn;
}

static int
_get_struct_size (const pool p)
{
  pool subpool;
  struct _pool_allocs *pa;
  struct _pool_cleanups *pc;
  int size;

  size = sizeof (*p);

  for (pa = p->allocs; pa; pa = pa->next)
	size += sizeof (struct _pool_allocs)
			+ _PA_SLOTS (pa) * sizeof (void *);

  for (pc = p->cleanups; pc; pc = pc->next)
	size += sizeof (struct _pool_cleanups)
			+ _PC_SLOTS (pc) * sizeof (struct _pool_cleanup_slot);

  for (subpool = p->subpool_list; subpool; subpool = subpool->next)
	size += _get_struct_size (subpool);

  return size;
}

static int
_get_nr_subpools (const pool p)
{
  pool subpool;
  int count = 1;

  for (subpool = p->subpool_list; subpool; subpool = subpool->next)
	count += _get_nr_subpools (subpool);

  return count;
}

void
pool_get_stats (const pool p, struct pool_stats *stats, size_t n)
{
  struct pool_stats s;

  s.nr_subpools = _get_nr_subpools (p);
  s.struct_size = _get_struct_size (p);

  memcpy (stats, &s, n);
}

static void
open_trace_file ()
{
  char msg1[] =
	"\n"
	"Pool allocator running in trace mode.\n"
	"Trace is being saved to file ";
  char msg2[] = "\n\n";

  trace_filename = getenv ("POOL_TRACE");

  if (trace_filename)
	{
	  trace_fd = open (trace_filename, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	  if (trace_fd == -1)
		{
		  perror (trace_filename);
		  exit (1);
		}

	  write (2, msg1, sizeof msg1);
	  write (2, trace_filename, strlen (trace_filename));
	  write (2, msg2, sizeof msg2);
	}
}

static void
trace (const char *fn, void *caller, struct pool *ptr1, void *ptr2, void *ptr3, int i1)
{
  char buffer[128];

  sprintf (buffer,
		   "%s caller: %p ptr1: %p ptr2: %p ptr3: %p i1: %d\n",
		   fn, caller, ptr1, ptr2, ptr3, i1);
  write (trace_fd, buffer, strlen (buffer));
}
