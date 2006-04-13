/*
** mouse:~ppr/src/libppr/spool_state.c
** Copyright 1995--2006, Trinity College Computing Center.
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
** Last modified 10 April 2006.
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

	pstate->accepting = TRUE;
	pstate->protected = FALSE;
	pstate->previous_status = PRNSTATUS_IDLE;
	pstate->status = PRNSTATUS_IDLE;
	pstate->next_error_retry = 0;
	pstate->next_engaged_retry = 0;
	pstate->countdown = 0;
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
		if(gu_sscanf(spool_state_data, "%d %d %d %d %d %d %d",
				&(pstate->accepting),
				&(pstate->protected),
				&(pstate->status),
				&(pstate->next_error_retry),
				&(pstate->next_engaged_retry),
				&(pstate->countdown),
				&(pstate->job_count)
				) != 7)
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
	gstate->protected = FALSE;
	gstate->held = FALSE;
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
		if(gu_sscanf(spool_state_data, "%d %d %d %d",
				&(gstate->accepting),
				&(gstate->protected),
				&(gstate->held),
				&(gstate->job_count)
				) != 4)
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

