/*
** mouse:~ppr/src/libppr/gu_nonblock.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 9 May 2001.
*/

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "gu.h"

/*
** Set or clear the POSIX non-blocking flag.
*/
void gu_nonblock(int fd, gu_boolean on)
    {
    int flags;

    flags = fcntl(fd, F_GETFL);

    if(on)			/* set the flag */
    	flags |= O_NONBLOCK;
    else			/* clear the flag */
    	flags &= ~O_NONBLOCK;
    	
    fcntl(fd, F_SETFL, flags);
    }

/* end of file */

