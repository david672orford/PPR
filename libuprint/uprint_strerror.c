/*
** mouse:~ppr/src/libuprint/uprint_strerror.c
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
** Last modified 29 June 2000.
*/

#include "config.h"
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

#include "uprint.h"

/*
** Notice that these strings are marked for internationalization,
** however uprint_strerror() doesn't translate them.  It looks
** like that is up to the caller.
*/
static const char *uprint_errors[] =
		{
		N_("not an error"),
		N_("malloc() failure"),
		N_("bad argument"),
		N_("fork() failed"),
		N_("child dumped core"),
		N_("child killed"),
		N_("wait() failed"),
		N_("exec() failed"),
		N_("child failed"),
		N_("no print destination specified"),
		N_("non-existent print destination specified"),
		N_("too many files"),
		N_("unknown remote system"),
		N_("temporary failure"),
		N_("out of disk space"),
		N_("internal error"),
		N_("permission denied"),
		N_("file not found"),
		N_("bad function call order"),
		N_("error in configuration file")
		} ;

const char *uprint_strerror(int errnum)
	{
	if(errnum < 0 || (size_t)errnum > (sizeof(uprint_errors) / sizeof(uprint_errors[0])))
		return _("Unknown error code");

	return gettext(uprint_errors[errnum]);
	} /* end of uprint_strerror() */

/* end of file */

