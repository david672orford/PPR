/*
** mouse:~ppr/src/libuprint/uprint_uid.c
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
** Last modified 14 February 2000.
*/

#include "before_system.h"
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

#include "uprint.h"

int uprint_re_uid_setup(uid_t *uid, uid_t *safe_uid)
    {
    const char *myname = "uprint_id_setup()";
    struct stat statbuf;

    /* Save the real user id: */
    if(uid != NULL)
    	*uid = getuid();

    /* Set all to 0.  (We only really want
       to change the real uid but that isn't easy.) */
    if(setuid(0) == -1)
    	{
	uprint_errno = UPE_SETUID;
	uprint_error_callback("%s: setuid(0) failed, errno=%d (%s)", myname, errno, gu_strerror(errno));
	return -1;
    	}

    /* We will run under the effective id of the owner of
       uprint.conf.  Stat uprint.conf to find out which ID that is. */
    if(stat(UPRINTCONF, &statbuf) == -1)
    	{
    	uprint_errno = UPE_INTERNAL;
    	uprint_error_callback("%s: can't stat() \"%s\", errno=%d (%s)", myname, UPRINTCONF, errno, gu_strerror(errno));
     	return -1;
    	}

    /* We refuse to run with an effective id of "root": */
    if(statbuf.st_uid == 0)
    	{
	uprint_errno = UPE_INTERNAL;
	uprint_error_callback(_("The file \"%s\" must not be owned by root."), UPRINTCONF);
	return -1;
	}

    /* Save this for those who want to know. */
    if(safe_uid != NULL) *safe_uid = statbuf.st_uid;

    /* Set the effective id to "ppr" but leave the
       real user id as "root" so that we can return to
       root whenever we want to. */
    if(seteuid(statbuf.st_uid) == -1)
    	{
	uprint_errno = UPE_INTERNAL;
	uprint_error_callback("%s: seteuid(%ld) failed, errno=%d (%s)", myname, (long)statbuf.st_uid, errno, gu_strerror(errno));
	return -1;
	}

    return 0;
    }

/* end of file */

