/*
** mouse:~ppr/src/pprd/pprd_nodeid.c
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
** Last modified 15 October 2003.
*/

/*
** This module implements a set of functions which allow a PPR node name
** to be represented as a small integer.
*/

#include "config.h"
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "pprd.h"
#include "pprd.auto_h"

/*
** This structure describes one entry in the node name table.
** If refcount is 0, the entry is not in use.
*/
struct NODEID
	{
	int refcount;
	char *name;
	} ;

/*
** This is the array of node names.
*/
static struct NODEID nodes[MAX_NODES];
static int table_size = 0;

/*
** When a node name is fed to this function, it searches nodes[]
** for a matching entry.  If none is found, one is created with a
** reference count of one.  If one is found, the reference count
** is incremented.  The index of the table entry, new or existing,
** is returned.
*/
int nodeid_assign(const char nodename[])
	{
	const char function[] = "nodeid_assign";
	int x;								/* roving index into nodename array */
	int empty_spot = -1;				/* first empty spot we see */

	DODEBUG_NODEID(("%s(\"%s\")", function, nodename));

	if(strcmp(nodename, "*") == 0)		/* special value reserved for the */
		return NODEID_WILDCARD;			/* wildcard nodeid */

	if(strcmp(nodename, "localhost") == 0)
		return NODEID_LOCALHOST;

	/* Search the table to find if this node already has
	   an ID assigned. */
	for(x=0; x < table_size; x++)
		{
		if(nodes[x].refcount == 0)		/* If this is an unused node, */
			{
			if(empty_spot == -1)		/* if we haven't seen one yet, */
				empty_spot = x;			/* make a note of it; */
			continue;					/* nothing else of interest in this entry. */
			}

		if(strcmp(nodes[x].name, nodename) == 0)		/* If the names match, */
			{
			nodes[x].refcount++;				/* increment reference count, */
			DODEBUG_NODEID(("%s(): refcount for %d increased to %d", function, x, nodes[x].refcount));
			return x;							/* return the name */
			}
		}

	/* If we didn't pass an empty spot, use the slot
	   at the end of the table if there is still room. */
	if(empty_spot == -1)
		{
		empty_spot = table_size;

		if(++table_size > MAX_NODES)
			fatal(1, "%s(): table overflow", function);
		}

	/* Carfully allocate space for the new name and set
	   the node reference count to 1. */
	{
	int gu_alloc_save = gu_alloc_checkpoint_get();
	nodes[empty_spot].name = gu_strdup(nodename);
	gu_alloc_checkpoint_put(gu_alloc_save);
	}
	nodes[empty_spot].refcount = 1;

	DODEBUG_NODEID(("%s(): new node, refcount=1", function));

	return empty_spot;
	} /* end of nodeid_assign() */

/*
** The reference count of the indicated table entry
** is decremented.  If the decrement operation reduces
** the reference count to zero, the storage occupied
** by the name is freed.
*/
void nodeid_free(int nodeid)
	{
	const char function[] = "nodeid_free";

	DODEBUG_NODEID(("%s(%d)", function, nodeid));

	if(nodeid == NODEID_WILDCARD)
		return;

	if(nodeid < 0 || nodeid >= table_size)
		fatal(1, "%s(): %d is an invalid nodeid", function, nodeid);

	if(nodes[nodeid].refcount == 0)
		fatal(1, "%s(): attempt to free invalid nodeid %d", function, nodeid);

	DODEBUG_NODEID(("%s(): refcount for \"%s\" reduced to %d", function, nodes[nodeid].name, nodes[nodeid].refcount - 1));

	if( --nodes[nodeid].refcount == 0 )			/* if decrementing the reference */
		{										/* count yields zero, */
		int gu_alloc_save = gu_alloc_checkpoint_get();
		gu_free(nodes[nodeid].name);
		gu_alloc_checkpoint_put(gu_alloc_save);

		if( (nodeid + 1) == table_size )		/* shrink the table if possible */
			table_size--;
		}
	} /* end of nodeid_free() */

/*
** This function is similiar to nodeid_assign(), but if the
** node name is not found in the list this function
** returns NODEID_NOTFOUND rather than adding the node to
** the list.  This function also returns NODEID_WILDCARD
** for "*" which nodeid_assign() will cease to do at some
** point in the future.
*/
int nodeid_by_name(const char name[])
	{
	FUNCTION4DEBUG("nodeid_by_name")
	int x;

	DODEBUG_NODEID(("%s(\"%s\")", function, name));

	if(strcmp(name, "*") == 0)			/* special value reserved for the */
		return NODEID_WILDCARD;			/* wildcard nodeid */

	if(strcmp(name, "localhost") == 0)
		return NODEID_LOCALHOST;

	for(x=0; x < table_size; x++)		/* Move thru the part of the table which */
		{								/* has been used so far. */
		if(nodes[x].refcount == 0)		/* If this is an unused node, */
			continue;					/* nothing else of interest in this entry. */

		if(strcmp(nodes[x].name, name) == 0)
			return x;
		}

	return NODEID_NOTFOUND;
	} /* end of nodeid_by_name() */

/*
** This function returns the node name associated with a certain table
** entry.  In previous version of this function, the two calls to
** error() were calls to fatal().  This was changed because it was
** probably an extreme response and because it didn't provide enough
** information to fix the bug.
*/
const char *nodeid_to_name(int nodeid)
	{
	const char function[] = "nodeid_to_name";

	DODEBUG_NODEID(("%s(%d)", function, nodeid));

	if(nodeid == NODEID_WILDCARD)		/* should never happen */
		return "*";

	if(nodeid == NODEID_NOTFOUND)		/* shouldn't happen either */
		return "<NODEID_NOTFOUND>";

	if(nodeid < 0 || nodeid >= table_size)
		{
		error("%s(): %d is an invalid nodeid", function, nodeid);
		return "<bug_invalid>";
		}

	if(nodes[nodeid].refcount < 1)
		{
		error("%s(): node %d is already freed", function, nodeid);
		return "<bug_freed>";
		}

	return nodes[nodeid].name;
	} /* end of nodeid_to_name() */

/*
** Return the node id of the local machine.
*/
int nodeid_local(void)
	{
	return NODEID_LOCALHOST;
	}

/*
** Return TRUE if the node id given is that of
** the local machine.
*/
gu_boolean nodeid_is_local_node(int nodeid)
	{
	if(nodeid == NODEID_LOCALHOST)
		return TRUE;
	else
		return FALSE;
	}

/* end of file */

