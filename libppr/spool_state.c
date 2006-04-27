/*
** mouse:~ppr/src/libppr/spool_state.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 27 April 2006.
*/

#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"

int printer_spool_state_load(struct PRINTER_SPOOL_STATE *pstate, const char prnname[])
	{
	char fname[MAX_PPR_PATH];
	int fd = -1;
	char spool_state_data[128];
	int len;
	int retval = 0;

	pstate->protected = FALSE;
	pstate->previous_status = PRNSTATUS_IDLE;
	pstate->status = PRNSTATUS_IDLE;
	pstate->next_error_retry = 0;
	pstate->next_engaged_retry = 0;
	pstate->countdown = 0;
	pstate->printer_state_change_time = 0;
	pstate->accepting = TRUE;
	pstate->job_count = 0;

	ppr_fnamef(fname, "%s/%s/spool_state", PRINTERS_PERSISTENT_STATEDIR, prnname);
	do	{
		if((fd = open(fname, O_RDONLY)) == -1)
			break;
		if((len = read(fd, spool_state_data, sizeof(spool_state_data))) == -1)
			{
			retval = -1;
			break;
			}
		if(len == sizeof(spool_state_data))
			{
			retval = -1;
			break;
			}
		spool_state_data[len] = '\0';
		if(gu_sscanf(spool_state_data, "%d %d %d %d %d %d %d %d %d",
				&(pstate->accepting),
				&(pstate->previous_status),
				&(pstate->status),
				&(pstate->next_error_retry),
				&(pstate->next_engaged_retry),
				&(pstate->countdown),
				&(pstate->printer_state_change_time),
				&(pstate->protected),
				&(pstate->job_count)
				) != 9)
			{
			retval = -1;
			break;
			}
		} while(FALSE);

	if(fd != -1)
		close(fd);

	return retval;
	} /* printer_spool_state_load() */

int group_spool_state_load(struct GROUP_SPOOL_STATE *gstate, const char grpname[])
	{
	char fname[MAX_PPR_PATH];
	int fd = -1;
	char spool_state_data[128];
	int len;
	int retval = 0;

	gstate->accepting = TRUE;
	gstate->held = FALSE;
	gstate->printer_state_change_time = 0;
	gstate->protected = FALSE;
	gstate->job_count = 0;

	ppr_fnamef(fname, "%s/%s/spool_state", GROUPS_PERSISTENT_STATEDIR, grpname);
	do	{
		if((fd = open(fname, O_RDONLY)) == -1)
			break;
		if((len = read(fd, spool_state_data, sizeof(spool_state_data))) == -1)
			{
			retval = -1;
			break;
			}
		if(len == sizeof(spool_state_data))
			{
			retval = -1;
			break;
			}
		spool_state_data[len] = '\0';
		if(gu_sscanf(spool_state_data, "%d %d %d %d %d",
				&(gstate->accepting),
				&(gstate->held),
				&(gstate->printer_state_change_time),
				&(gstate->protected),
				&(gstate->job_count)
				) != 5)
			{
			retval = -1;
			break;
			}
		} while(FALSE);

	if(fd != -1)
		close(fd);

	return retval;
	} /* group_spool_state_load() */

/* end of file */

