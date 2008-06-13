/*
** mouse:~ppr/src/pprd/pprd_destid.c
** Copyright 1995--2008, Trinity College Computing Center.
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
** Last modified 13 June 2008.
*/

/*
** This module contains routines for converting printer and group names to
** id numbers and back again and for finding out certain information about
** printers and groups as identified by destid.
**
** Those functions which don't take a node_id argument will work only
** for local printers, groups, and destids.
*/

#include "before_system.h"
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "pprd.h"
#include "pprd.auto_h"

/*===========================================================================
** Functions which work only with local destination IDs.
===========================================================================*/

/*
** Convert a local destination (group or printer) id to a destination name.
*/
const char *destid_local_to_name(int destid)
	{
	if(destid_local_is_group(destid))
		{
		int group_index = destid_local_to_gindex(destid);
		if(group_index < group_count && !groups[group_index].deleted)
			return groups[group_index].name;
		else
			return "<invalid>";
		}
	else
		{
		if(destid >= 0 && destid < printer_count && printers[destid].status != PRNSTATUS_DELETED)
			return printers[destid].name;
		else
			return "<invalid>";
		}
	} /* end of destid_local_to_name() */

/*
** Return the destid which matches the specified local printer name.  If the
** printer does not exist, return -1.
*/
int destid_local_by_printer(const char name[])
	{
	int x;

	for(x=0; x < printer_count; x++)
		{
		if(strcmp(printers[x].name, name) == 0 && printers[x].status != PRNSTATUS_DELETED)
			return x;
		}

	return -1;
	} /* end of destid_local_by_printer() */

/*
** Return the destid which matches the specified local group name.  If the
** group does not exist, return -1.
*/
static int destid_local_by_group(const char name[])
	{
	int x;

	for(x=0; x < group_count; x++)				/* try all groups */
		{
		if(strcmp(groups[x].name, name) == 0 && !groups[x].deleted)
			return destid_local_by_gindex(x);
		}

	return -1;
	} /* end of destid_local_by_group() */

/*
** This is the one we normally use.  If there is both a group and a printer
** with the name we are looking for, this one returns the ID of the group.
** If neither a group nor a printer is found, return -1.
*/
int destid_local_by_name(const char name[])
	{
	int ret;

	if((ret = destid_local_by_group(name)) != -1)
		return ret;

	if((ret = destid_local_by_printer(name)) != -1)
		return ret;

	return -1;
	} /* end of destid_local_by_name() */

/*
** This one is rarely used.  If there are a printer and a group
** with the same name, it will return the printer's destid.
*/
int destid_local_by_name_reversed(char *name)
	{
	int ret;

	if((ret = destid_local_by_printer(name)) != -1)
		return ret;

	if((ret = destid_local_by_group(name)) != -1)
		return ret;

	return -1;				/* we didn't find it */
	} /* end of destid_local_by_name_reversed() */

/*
** Return TRUE if the destination id in question is a group id.
*/
int destid_local_is_group(int id)
	{
	if(id >= printer_count)		/* printers run 0 - (printer_count - 1) */
		return -1;
	else
		return 0;
	} /* end of destid_local_is_group() */

/*
** Get the offset of a certain printer into a certain group array.
** Return -1 if the printer is not a member.
*/
int destid_local_get_member_offset(int destid, int prnid)
	{
	struct Group *cl;
	int x;

	if(!destid_local_is_group(destid))
		fatal(1, "destid_local_get_member_offset(): assertion failed, destid=%d, prnid=%d", destid, prnid);

	cl = &groups[destid_local_to_gindex(destid)];

	for(x=0; x < cl->members; x++)
		{
		if(cl->printers[x] == prnid)
			return x;
		}

	return -1;
	} /* end of destid_local_get_member_offset() */

/*
** Get the bitmask which identifies a particular printer in the "never" and
** "notnow" bit fields of jobs with a certain destination id.
**
** If the printer is not included in the specified destination (i.e. the
** destination id is not the printer id and is not that of a group containing
** the printer) then, return 0.
**
** If the destination is the printer, return 1.
*/
int destid_printer_bit(int destid, int prnid)
	{
	int off;

	/* If the destination is the printer, set bit one. */
	if(prnid == destid)
		return 1;

	/* If not but the destination is a printer, set no bits. */
	if(!destid_local_is_group(destid))
		return 0;

	/* Since it is a group, get the bit number. */
	off = destid_local_get_member_offset(destid , prnid);

	/* If the printer is not a member, set no bits. */
	if(off == -1)
		return 0;

	/* It is a member, set the correct bit. */
	return 1 << off;
	} /* end of destid_printer_bit() */

/*
** Convert a local node destination id to a group array index.
*/
int destid_local_to_gindex(int destid)
	{
	int answer = (destid - MAX_PRINTERS);
	if(answer < 0 || answer >= MAX_GROUPS)
		fatal(1, "destid_local_to_gindex(): assertion failed, destid=%d, answer=%d", destid, answer);
	return answer;
	} /* end of destid_local_to_gindex() */

/*
** Convert a group array index to a local node destination id.
*/
int destid_local_by_gindex(int gindex)
	{
	if(gindex < 0 || gindex >= MAX_GROUPS)
		fatal(1, "destid_local_by_gindex(): assertion failed, gindex=%d", gindex);
	return (gindex + MAX_PRINTERS);
	} /* end of destid_local_by_gindex() */

/*===========================================================================
** Functions which work on both local and remote destination IDs.
===========================================================================*/

/*
** Assign a destination id for the distination of a new job.  For local
** jobs, this simply returns the pre-assigned destination id, but for
** remote jobs it may dynamically allocate a destination id.
*/
int destid_assign(int destnode_id, const char name[])
	{
	if(nodeid_is_local_node(destnode_id))
		return destid_local_by_name(name);
	else
		return nodeid_assign(name);		/* nasty hack!!! */
	}

/*
** Free a dynamically allocated destination id.  Local destination IDs are
** not dynamically allocated, so in such a case there is nothing to do.
*/
void destid_free(int destnode_id, int destid)
	{
	if(!nodeid_is_local_node(destnode_id))
		{
		nodeid_free(destid);			/* nasty hack!!! */
		}
	} /* end of destid_free() */

/*
** Convert a destination (group or printer) id to a destination name.
*/
const char *destid_to_name(int destnode_id, int destid)
	{
	if(nodeid_is_local_node(destnode_id))
		{
		return destid_local_to_name(destid);
		}
	else
		{
		return nodeid_to_name(destid);	/* nasty hack!!! */
		}
	} /* end of destid_to_name() */

/*
** Given a (possibly remote) destination name, return the
** id number.
*/
int destid_by_name(int destnode_id, const char name[])
	{
	if(nodeid_is_local_node(destnode_id))
		return destid_local_by_name(name);
	else
		return nodeid_by_name(name);	/* nasty hack!!! */
	}

/*
** Return true if the indicated destination is accepting requests.
*/
gu_boolean destid_accepting(int destnode_id, int destid)
	{
	if(nodeid_is_local_node(destnode_id))
		{
		if(destid_local_is_group(destid))
			return groups[destid_local_to_gindex(destid)].accepting;
		else
			return printers[destid].accepting;
		}
	else
		{
		return TRUE;			/* !!! */
		}
	} /* end of destid_accepting() */

/* end of file */

