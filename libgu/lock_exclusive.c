/*
** mouse:~ppr/src/libgu/lock_exclusive.c
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
** Last modified 25 February 2005.
*/

/*! \file */

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "gu.h"


/** Lock a file

An operating system independent function for obtaining an
exclusive lock on a file.

Obtain an exclusive lock on an entire file.
Return -1 if we fail.

If waitmode is TRUE, block until lock is obtained.

*/
int gu_lock_exclusive(int filenum, gu_boolean waitmode)
	{
	struct flock lock;
	int retval;

	lock.l_type = F_WRLCK;				/* exclusive lock */
	lock.l_whence = SEEK_SET;			/* absolute offset */
	lock.l_start = (off_t)0;			/* from begining */
	lock.l_len = (off_t)0;				/* to future end */

	if(waitmode)
		retval = fcntl(filenum, F_SETLKW, &lock);
	else
		retval = fcntl(filenum, F_SETLK, &lock);

	if(retval == -1)
		return -1;
	else
		return 0;
	} /* end of gu_lock_exclusive() */

/* end of file */
