/* A tree class.
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

#include <stdlib.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <pool.h>
#include <vector.h>
#include <pstring.h>

#include "tree.h"

tree
_tree_new (pool pool, size_t size)
{
  tree t = (tree) new_vector (pool, tree);

  t = c2_prealloc (pool, t, sizeof (*t) + size);
  t->size = size;

  return t;
}

extern tree
copy_tree (pool pool, tree t)
{
  tree nt = _tree_new (pool, t->size);
  int i;

  /* Copy the node data. */
  memcpy (nt->data, t->data, t->size);

  /* Copy each subnode, recursively. */
  for (i = 0; i < tree_size (t); ++i)
	{
	  tree st, nst;

	  tree_get (t, i, st);
	  nst = copy_tree (pool, st);
	  tree_push_back (nt, nst);
	}

  return nt;
}

void
_tree_get_data (tree t, void *ptr)
{
  if (ptr) memcpy (ptr, t->data, t->size);
}

void
_tree_set_data (tree t, const void *ptr)
{
  if (ptr) memcpy (t->data, ptr, t->size);
}
