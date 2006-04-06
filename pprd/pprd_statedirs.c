/*
** mouse:~ppr/src/pprd/pprd_statedirs.c
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
** Last modified 6 April 2006.
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

void set_file_boolean(const char statedir[], const char destname[], const char variable[], gu_boolean value)
	{
	char fname[MAX_PPR_PATH];
	ppr_fnamef(fname, "%s/%s/%s", statedir, destname, variable);
	if(value)
		{
		int fd;
		if((fd = open(fname, O_WRONLY | O_CREAT, UNIX_660)) == -1)
			error("can't create \"%s\", errno=%d (%s)", fname, errno, strerror(errno));
		else
			close(fd);
		}
	else
		{
		if(unlink(fname) == -1)
			error("%s(\"%s\") failed, errno=%d (%s)", "unlink", fname, errno, strerror(errno));
		}
	}

gu_boolean get_file_boolean(const char statedir[], const char destname[], const char variable[])
	{
	char fname[MAX_PPR_PATH];
	struct stat statbuf;
	ppr_fnamef(fname, "%s/%s/%s", statedir, destname, variable);
	if(stat(fname, &statbuf) == 0)
		return TRUE;
	else
		return FALSE;
	}

void spool_state_load(struct Printer *printer)
	{
	char fname[MAX_PPR_PATH];
	int fd;
	char spool_state_data[128];
	int len;

	printer->previous_status = PRNSTATUS_IDLE;
	printer->status = PRNSTATUS_IDLE;
	printer->next_error_retry = 0;
	printer->next_engaged_retry = 0;
	printer->countdown = 0;

	ppr_fnamef(fname, "%s/%s/spool_state", PRINTERS_PERSISTENT_STATEDIR, printer->name);
	do	{
		if((fd = open(fname, O_RDONLY)) == -1)
			break;
		if((len = read(fd, spool_state_data, sizeof(spool_state_data))) == -1)
			{
			error("failed to read \"%s\", errno=%d (%s)", fname, errno, strerror(errno));
			break;
			}
		if(len == sizeof(spool_state_data))
			{
			error("to much data in \"%s\"", fname);
			break;
			}
		spool_state_data[len] = '\0';
		if(gu_sscanf(spool_state_data, "%d %d %d %d",	/* fifth field */
				&(printer->status),
				&(printer->next_error_retry),
				&(printer->next_engaged_retry),
				&(printer->countdown)
				/* don't load job_count, administrator might have manually delete jobs */
			) != 4)
			{
			error("corrupt \"%s\"", spool_state_data);
			}
		} while(FALSE);

	}

void spool_state_save(struct Printer *printer)
	{
	char fname[MAX_PPR_PATH];
	int fd;
	char temp[128];
	int len;
	ppr_fnamef(fname, "%s/%s/spool_state", PRINTERS_PERSISTENT_STATEDIR, printer->name);
	if((fd = open(fname, O_WRONLY | O_CREAT, UNIX_644)) == -1)
		fatal(0, "can't create \"%s\" for write, errno=%d (%s)", fname, errno, strerror(errno));
	len = gu_snprintf(temp, sizeof(temp), "%d %d %d %d %d\n",
		printer->status,
		printer->next_error_retry,
		printer->next_engaged_retry,
		printer->countdown,
		printer->job_count
		);
	write(fd, temp, len);
	close(fd);
	}

void group_spool_state_save(struct Group *group)
	{
	char fname[MAX_PPR_PATH];
	int fd;
	char temp[128];
	int len;
	ppr_fnamef(fname, "%s/%s/spool_state", GROUPS_PERSISTENT_STATEDIR, group->name);
	if((fd = open(fname, O_WRONLY | O_CREAT, UNIX_644)) == -1)
		fatal(0, "can't create \"%s\" for write, errno=%d (%s)", fname, errno, strerror(errno));
	len = gu_snprintf(temp, sizeof(temp), "%d\n",
		group->job_count
		);
	write(fd, temp, len);
	close(fd);
	}

/* end of file */

