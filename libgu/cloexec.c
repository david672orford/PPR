/*
** mouse:~ppr/src/libppr/cloexec.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 22 November 2000.
*/

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "gu.h"

void gu_set_cloexec(int fd)
	{
	int flags;
	flags = fcntl(fd, F_GETFD);
	flags |= FD_CLOEXEC;
	fcntl(fd, F_SETFD, flags);
	}

/* end of file */

