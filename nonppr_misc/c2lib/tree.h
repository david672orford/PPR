/* A tree class.
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

#ifndef TREE_H
#define TREE_H

#include <pool.h>
#include <vector.h>

struct tree
{
  struct vector v;		/* Vector of subnodes. */
  size_t size;			/* Size of the data. */
  char data[0];			/* Opaque data (used by the application). */
};

typedef struct tree *tree;

/* Function: new_tree - allocate a new tree, node or leaf
 * Function: _tree_new
 *
 * Allocate a new tree / node / leaf.
 *
 * A node in the tree is defined as a list of pointers to subnodes
 * and some data stored in the node itself. Because of the recursive
 * definition of trees, a tree is just a node, the special 'root'
 * node. A leaf is just a node which happens to have no subnodes. You
 * can add subnodes to a leaf later if you want.
 *
 * The @code{new_vector} macro is used to allocate a node in the
 * tree storing data @code{type} in the node. It just evaluates the
 * size of @code{type} and calls @code{_tree_new} which actually
 * allocates the node.
 *
 * Unless you are very careful with types, you should always make
 * sure that each node in your tree has the same @code{type}.
 *
 * Returns: The tree / node (of type @code{tree}).
 */
#define new_tree(pool,type) _tree_new ((pool), sizeof (type))
extern tree _tree_new (pool, size_t size);

/* Function: tree_get_data - access the data stored in a node
 * Function: _tree_get_data
 * Function: tree_set_data
 * Function: _tree_set_data
 *
 * These functions allow you to access the data stored in the node.
 *
 * @code{tree_get_data} copies the data out of the node into the
 * local variable.
 *
 * @code{tree_set_data} copies the data from the local variable into
 * the node.
 */
#define tree_get_data(t,obj) _tree_get_data ((t), &(obj))
extern void _tree_get_data (tree t, void *ptr);
#define tree_set_data(t,obj) _tree_set_data ((t), &(obj))
extern void _tree_set_data (tree t, const void *ptr);

/* Function: copy_tree - make a deep copy of a tree
 *
 * Make a deep copy of tree @code{t} into pool @code{pool}. This
 * copies all of the nodes and the data in those nodes recursively
 * Note however that if the data in a node is a / contains a pointer
 * then the pointed-to data will not be copied.
 *
 * Returns: The copied tree (of type @code{tree}).
 */
extern tree copy_tree (pool, tree t);

/* Function: tree_push_back - add or remove subnodes to or from a node
 * Function: tree_pop_back
 * Function: tree_push_front
 * Function: tree_pop_front
 *
 * The @code{*_push_*} functions push subnodes into nodes.
 *
 * The @code{*_pop_*} functions pop subnodes off nodes into local variables.
 *
 * The @code{*_front} functions push and pop subnodes off the front
 * of a node.
 *
 * The @code{*_back} functions push and pop subnodes
 * off the end of the node.
 *
 * Array indexes are checked. You cannot pop a node which has
 * no subnodes.
 */
#define tree_push_back(t,subnode) (vector_push_back ((vector)(t), (subnode)))
#define tree_pop_back(t,subnode) (vector_pop_back ((vector)(t), (subnode)))
#define tree_push_front(t,subnode) (vector_push_front ((vector)(t), (subnode)))
#define tree_pop_front(t,subnode) (vector_pop_front ((vector)(t), (subnode)))

/* Function: tree_get_subnode - get a particular subnode of a node
 * Function: tree_get
 *
 * @code{tree_get_subnode} returns the @code{i}th subnode of a node. The
 * node is unaffected.
 *
 * @code{tree_get} is identical.
 */
#define tree_get_subnode(t,i,subnode) tree_get((t), (i), (subnode))
#define tree_get(t,i,subnode) (vector_get ((vector)(t), (i), (subnode)))

/* Function: tree_insert - insert subnodes into a tree
 * Function: tree_insert_array
 *
 * @code{tree_insert} inserts a single subnode @code{subnode} before subnode
 * @code{i}. All other subnodes are moved up to make space.
 *
 * @code{tree_insert_array} inserts an array of @code{n} subnodes
 * starting at address @code{ptr} into the node before index
 * @code{i}.
 *
 * Array indexes are checked.
 */
#define tree_insert(t,i,subnode) (vector_insert ((vector)(t), (i), (subnode)))
#define tree_insert_array(t,i,ptr,n) (vector_insert_array ((vector)(t), (i), (ptr), (n)))

/* Function: tree_replace - replace subnodes of a node
 * Function: tree_replace_array
 *
 * @code{tree_replace} replaces the single subnode at
 * @code{v[i]} with @code{subnode}.
 *
 * @code{tree_replace_array} replaces the @code{n} subnodes
 * in the node starting at index @code{i} with the @code{n}
 * subnodes from the array @code{ptr}.
 * 
 * Array indexes are checked.
 */
#define tree_replace(t,i,subnode) (vector_replace ((vector)(t), (i), (subnode)))
#define tree_replace_array(t,i,ptr,n) (vector_replace_array ((vector)(t), (i), (ptr), (n)))

/* Function: tree_erase - erase subnodes of a node
 * Function: tree_erase_range
 * Function: tree_clear
 *
 * @code{tree_erase} removes the subnode index @code{i}, shuffling
 * the later subnodes down by one place to fill the space.
 *
 * @code{tree_erase_range} removes a range of subnodes @code{i}
 * to @code{j-1} (@code{i <= j}), shuffling later subnodes down
 * to fill the space.
 *
 * @code{tree_clear} removes all subnodes from the node, setting
 * its size to @code{0}, and effectively making the node into a leaf.
 *
 * Array indexes are checked.
 */
#define tree_erase(t,i) (vector_erase ((vector)(t), (i)))
#define tree_erase_range(t,i,j) (vector_erase_range ((vector)(t), (i), (j)))
#define tree_clear(t) (vector_clear ((vector)(t)))

/* Function: tree_size - return the number of subnodes of a node
 * Function: tree_nr_subnodes
 *
 * Return the size (ie. number of subnodes) in the node. The
 * @code{tree_nr_subnodes} macro is identical.
 */
#define tree_size(t) (vector_size ((vector)(t)))
#define tree_nr_subnodes(t) tree_size(t)

/* Function: tree_swap - swap the order of two subnodes of a node
 *
 * Swap subnodes @code{i} and @code{j} of tree @code{t}.
 */
#define tree_swap(t,i,j) (vector_swap ((vector)(t), (i), (j)))

#endif /* TREE_H */
