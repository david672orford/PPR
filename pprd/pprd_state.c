/*
** mouse:~ppr/src/pprd/pprd_state.c
** Copyright 1995--2005, Trinity College Computing Center.
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
** Last modified 23 September 2005.
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

