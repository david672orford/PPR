/*
** mouse:~ppr/src/libgu/lock_exclusive.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 22 November 2000.
*/

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "gu.h"


/*
** An operating system independent function for obtaining an
** exclusive lock on a file.
**
** Obtain an exclusive lock on an entire file.
** Return -1 if we fail.
** If waitmode is TRUE, block until lock is obtained.
*/
int gu_lock_exclusive(int filenum, gu_boolean waitmode)
    {
    struct flock lock;
    int retval;

    lock.l_type = F_WRLCK;		/* exclusive lock */
    lock.l_whence = SEEK_SET;		/* absolute offset */
    lock.l_start = (off_t)0;		/* from begining */
    lock.l_len = (off_t)0;		/* to future end */

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
