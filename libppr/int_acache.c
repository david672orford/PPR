/*
** mouse:~ppr/src/libppr/int_acache.c
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
** Last modified 3 April 2006.
*/

#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "libppr_int.h"

/*
** This code is used by printer interface programs for caching expensive 
** address lookups.
**
** For some reason we are trying not to use stdio.
*/
void int_addrcache_save(const char printer[], const char interface[], const char address[], const char resolution[])
	{
	int fd;
	char fname[MAX_PPR_PATH];
	char *temp;
	int towrite, written;

	/* The name "-" indicates a printer with no queue yet.  We don't support
	   caching under such circumstances. */
	if(strcmp(printer, "-") == 0)
		return;

	ppr_fnamef(fname, "%s/%s/resolved_device_address", PRINTERS_PURGABLE_STATEDIR, printer);
	if((fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, UNIX_644)) == -1)
		{
		alert(printer, TRUE, "%s interface: can't open \"%s\" for write, errno=%d (%s)", interface, fname, errno, gu_strerror(errno));
		return;
		}

	/* Allocate a block to hold the data we want to write
	   and contruct that data in the block. */
	{
	int space_needed = (strlen(interface) + strlen(address) + strlen(resolution) + 4);			/* 3 newlines and a NULL */
	temp = (char*)gu_alloc(space_needed, sizeof(char));
	snprintf(temp, space_needed, "%s\n%s\n%s\n", interface, address, resolution);				/* remove snprintf() bloat!!! */
	}

	if((written = write(fd, temp, towrite=strlen(temp))) == -1)
		alert(printer, TRUE, "%s interface: write() to address cache file failed, errno=%d (%s)", interface, errno, gu_strerror(errno));
	if(written != towrite)
		alert(printer, TRUE, "%s interface: towrite=%d, written=%d", interface, towrite, written);

	gu_free(temp);

	close(fd);
	} /* end of int_addrcache_save() */

char *int_addrcache_load(const char printer[], const char interface[], const char address[], int *age)
	{
	char fname[MAX_PPR_PATH];
	int fd;
	char *answer = NULL;

	if(strcmp(printer, "-") == 0)
		return NULL;

	/* Try to open the address cache file.  If we fail because it is not there,
	   that is ok, we will create it later so we fail silently.  For other
	   errors we put a warning in the printer's alert log. */
	ppr_fnamef(fname, "%s/%s/resolved_device_address", PRINTERS_PURGABLE_STATEDIR, printer);
	if((fd = open(fname, O_RDONLY)) == -1)
		{
		if(errno != ENOENT)
			alert(printer, TRUE, "%s interface: can't open \"%s\" for read, errno=%d (%s)", interface, fname, errno, gu_strerror(errno));
		return NULL;
		}

	/* This is not a loop, it is a crude exception handling block. */
	while(1)
		{
		#define MAX_READ 4095
		char temp[MAX_READ + 1];
		size_t len;
		char *p;

		if((len = read(fd, temp, MAX_READ)) < 0)
			{
			alert(printer, TRUE, "%s interface: read() of address cache file failed, errno=%d (%s)", interface, errno, gu_strerror(errno));
			break;
			}

		/* NULL terminate the data from the file */
		temp[len] = '\0';

		/* Make sure the cache file was left by this interface program. */
		len = strlen(interface);
		if(strcspn(temp, "\n") != len || strncmp(temp, interface, len) != 0)
			break;

		/* Make sure the printer address parameter isn't different from
		   last time. */
		p = temp + 6;
		len = strlen(address);
		if(strcspn(p, "\n") == len && strncmp(p, address, len) != 0)
			break;

		p += len;
		p++;
		p[strcspn(p, "\n")] = '\0';

		/* Get the cache file modification date
		   and compute its age in seconds. */
		{
		struct stat statbuf;
		time_t time_now;

		if(fstat(fd, &statbuf) == -1)
			{
			alert(printer, TRUE, "%s interface: can't fstat address cache file, errno=%d (%s)", interface, errno, gu_strerror(errno));
			break;
			}

		time(&time_now);
		*age = (time_now - statbuf.st_mtime);
		}

		/* Make a duplicate of the resolved address
		   to return to the caller. */
		answer = gu_strdup(p);

		break;
		}

	/* close the cache file */
	close(fd);

	/* remove file since we will create a new one if we print sucessfully */
	unlink(fname);

	return answer;
	} /* end of int_addrcache_load() */

/* end of file */
