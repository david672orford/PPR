/*
** mouse:~ppr/src/pprd/pprd_state.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 9 May 2001.
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
#include "pprd.h"
#include "pprd.auto_h"

/*
** Send a line to the file which is read by programs which display
** some aspect of the spooler state.
**
** Probably no longer true:
** This routine is sometimes called from within a signal handler, so
** it should not call non-reentrant routines such as malloc().
*/
void state_update(const char *string, ... )
	{
	static int countdown = 0;	/* countdown til start of next file */
	static int handle = -1;		/* handle of open file */
	static int serial = 1;		/* file serial number */
	va_list va;
	char line[128];

	while(handle == -1 || countdown <= 0)
		{
		if(handle == -1)
			{
			if((handle = open(STATE_UPDATE_FILE, O_WRONLY | O_CREAT | O_APPEND, UNIX_644)) == -1)
				fatal(0, "Failed to open \"%s\" for append, errno=%d (%s)", STATE_UPDATE_FILE, errno, gu_strerror(errno));
			}

		if(countdown <= 0)				/* If this file is full, */
			{
			fchmod(handle, UNIX_644 | S_IXOTH);
			close(handle);
			handle = -1;
			unlink(STATE_UPDATE_FILE);
			countdown = STATE_UPDATE_MAXLINES;
			continue;
			}

		snprintf(line, sizeof(line), "SERIAL %d\n", serial++);
		write(handle, line, strlen(line));

		gu_set_cloexec(handle);
		}

	va_start(va, string);
	vsnprintf(line, sizeof(line), string, va);
	strcat(line, "\n");
	va_end(va);

	write(handle, line, strlen(line) );

	countdown--;
	} /* end of state_update() */

/* end of file */

