/*
** mouse:~ppr/src/libppr/space.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 17 November 1999.
*/

/*
** Determine the amount of free disk space available to non-superusers.
*/

#include "before_system.h"
#include <sys/types.h>
#ifdef HAVE_STATVFS
#include <sys/statvfs.h>
#else
#ifdef HAVE_STATFS
#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#else
#include <sys/param.h>	/* for FreeBSD */
#include <sys/mount.h>
#endif
#endif
#endif
#include "gu.h"
#include "global_defines.h"


int disk_space(const char *path, unsigned int *free_blocks, unsigned int *free_files)
    {
    /* Make sure the spooler has enough disk space. */
    #ifdef HAVE_STATVFS		/* Use whichever function we have */
    struct statvfs filesys;
    if(statvfs(path, &filesys) == -1)
    #else
    #ifdef HAVE_STATFS
    struct statfs filesys;
    if(statfs(path, &filesys) == -1)
    #endif
    #endif

    #if HAVE_STATVFS | HAVE_STATFS
	{			/* If error, */
	return -1;
	}
    else			/* If no error, */
    	{
	*free_blocks = filesys.f_bavail;
	#ifdef HAVE_STATVFS
	*free_files = filesys.f_favail;
	#else
	*free_files = filesys.f_ffree;
	#endif
    	}

    #else			/* If this function is not supported, */
    *free_blocks = 100000;	/* guess and say there is lots of space. */
    *free_files = 5000;
    #endif

    return 0;
    } /* end of disk_space() */

/* end of file */
