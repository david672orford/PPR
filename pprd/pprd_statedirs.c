/*
** mouse:~ppr/src/pprd/pprd_statedirs.c
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
#include "pprd.h"
#include "pprd.auto_h"

void printer_spool_state_save(struct PRINTER_SPOOL_STATE *pstate, const char prnname[])
	{
	char fname[MAX_PPR_PATH];
	int fd;
	char temp[128];
	int len;
	ppr_fnamef(fname, "%s/%s/spool_state", PRINTERS_PERSISTENT_STATEDIR, prnname);
	if((fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, UNIX_644)) == -1)
		fatal(0, "can't create \"%s\" for write, errno=%d (%s)", fname, errno, strerror(errno));
	len = gu_snprintf(temp, sizeof(temp), "%d %d %d %d %d %d %d %d %d\n",
		pstate->accepting,
		pstate->previous_status,
		pstate->status,
		pstate->next_error_retry,
		pstate->next_engaged_retry,
		pstate->countdown,
		pstate->printer_state_change_time,
		pstate->protected,
		pstate->job_count
		);
	write(fd, temp, len);
	close(fd);
	} /* printer_spool_state_save() */

void group_spool_state_save(struct GROUP_SPOOL_STATE *gstate, const char grpname[])
	{
	char fname[MAX_PPR_PATH];
	int fd;
	char temp[128];
	int len;
	ppr_fnamef(fname, "%s/%s/spool_state", GROUPS_PERSISTENT_STATEDIR, grpname);
	if((fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, UNIX_644)) == -1)
		fatal(0, "can't create \"%s\" for write, errno=%d (%s)", fname, errno, strerror(errno));
	len = gu_snprintf(temp, sizeof(temp), "%d %d %d %d %d\n",
		gstate->accepting,
		gstate->held,
		gstate->printer_state_change_time,
		gstate->protected,
		gstate->job_count
		);
	write(fd, temp, len);
	close(fd);
	} /* group_spool_state_save() */

/* end of file */

