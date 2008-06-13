/*
** mouse:~ppr/src/pprd/pprd_destid.c
** Copyright 1995--2008, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 13 June 2008.
*/

/*
** This module contains routines for converting printer and group names to
** id numbers and back again and for finding out certain information about
** printers and groups as identified by destid.
*/

#include "config.h"
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "pprd.h"
#include "pprd.auto_h"

/*===========================================================================
** Functions for destination IDs.
===========================================================================*/

/*
** Convert a local destination (group or printer) id to a destination name.
*/
const char *destid_to_name(int destid)
	{
	if(destid_is_group(destid))
		{
		int group_index = destid_to_gindex(destid);
		if(group_index < group_count && !groups[group_index].deleted)
			return groups[group_index].name;
		else
			return "<invalid>";
		}
	else
		{
		if(destid >= 0 && destid < printer_count && printers[destid].spool_state.status != PRNSTATUS_DELETED)
			return printers[destid].name;
		else
			return "<invalid>";
		}
	} /* end of destid_to_name() */

/*
** Return the destid which matches the specified local printer name.  If the
** printer does not exist, return -1.
*/
int destid_by_printer(const char name[])
	{
	int x;

	for(x=0; x < printer_count; x++)
		{
		if(printers[x].spool_state.status != PRNSTATUS_DELETED && strcmp(printers[x].name, name) == 0)
			return x;
		}

	return -1;
	} /* end of destid_by_printer() */

/*
** Return the destid which matches the specified local group name.  If the
** group does not exist, return -1.
*/
int destid_by_group(const char name[])
	{
	int x;

	for(x=0; x < group_count; x++)				/* try all groups */
		{
		if(!groups[x].deleted && strcmp(groups[x].name, name) == 0)
			return destid_by_gindex(x);
		}

	return -1;
	} /* end of destid_by_group() */

/*
** This is the one we normally use.  If there is both a group and a printer
** with the name we are looking for, this one returns the ID of the group.
** If neither a group nor a printer is found, return -1.
*/
int destid_by_name(const char name[])
	{
	int ret;

	if((ret = destid_by_group(name)) != -1)
		return ret;

	if((ret = destid_by_printer(name)) != -1)
		return ret;

	return -1;
	} /* end of destid_by_name() */

/*
** This one is rarely used.  If there are a printer and a group
** with the same name, it will return the printer's destid.
*/
int destid_by_name_reversed(char *name)
	{
	int ret;

	if((ret = destid_by_printer(name)) != -1)
		return ret;

	if((ret = destid_by_group(name)) != -1)
		return ret;

	return -1;				/* we didn't find it */
	} /* end of destid_by_name_reversed() */

/*
** Return TRUE if the destination id in question is a group id.
*/
gu_boolean destid_is_group(int id)
	{
	if(id >= printer_count)		/* printers IDs run from 0 to (printer_count - 1) */
		return TRUE;
	else
		return FALSE;
	} /* end of destid_is_group() */

gu_boolean destid_is_printer(int id)
	{
	if(id < printer_count)
		return TRUE;
	else
		return FALSE;
	}

/*
** Get the offset of a certain printer into a certain group array.
** Return -1 if the printer is not a member.
*/
int destid_get_member_offset(int destid, int prnid)
	{
	struct Group *cl;
	int x;

	if(!destid_is_group(destid))
		fatal(1, "destid_get_member_offset(): assertion failed, destid=%d, prnid=%d", destid, prnid);

	cl = &groups[destid_to_gindex(destid)];

	for(x=0; x < cl->members; x++)
		{
		if(cl->printers[x] == prnid)
			return x;
		}

	return -1;
	} /* end of destid_get_member_offset() */

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
	if(!destid_is_group(destid))
		return 0;

	/* Since it is a group, get the bit number. */
	off = destid_get_member_offset(destid , prnid);

	/* If the printer is not a member, set no bits. */
	if(off == -1)
		return 0;

	/* It is a member, set the correct bit. */
	return 1 << off;
	} /* end of destid_printer_bit() */

/*
** Convert a destination id to a group array index.
*/
int destid_to_gindex(int destid)
	{
	int answer = (destid - MAX_PRINTERS);
	if(answer < 0 || answer >= MAX_GROUPS)
		fatal(1, "destid_to_gindex(): assertion failed, destid=%d, answer=%d", destid, answer);
	return answer;
	} /* end of destid_to_gindex() */

/*
** Convert a group array index to a destination id.
*/
int destid_by_gindex(int gindex)
	{
	if(gindex < 0 || gindex >= MAX_GROUPS)
		fatal(1, "destid_by_gindex(): assertion failed, gindex=%d", gindex);
	return (gindex + MAX_PRINTERS);
	} /* end of destid_by_gindex() */

/*===========================================================================
** Functions which work on both local and remote destination IDs.
===========================================================================*/

/*
** Return true if the indicated destination is accepting requests.
*/
gu_boolean destid_accepting(int destid)
	{
	if(destid_is_group(destid))
		return groups[destid_to_gindex(destid)].spool_state.accepting;
	else
		return printers[destid].spool_state.accepting;
	} /* end of destid_accepting() */

/* end of file */

