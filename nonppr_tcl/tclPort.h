/*
 * tclPort.h --
 *
 *	This header file handles porting issues that occur because
 *	of differences between systems.  It reads in UNIX-related
 *	header files and sets up UNIX-related macros for Tcl's UNIX
 *	core.  It should be the only file that contains #ifdefs to
 *	handle different flavors of UNIX.  This file sets up the
 *	union of all UNIX-related things needed by any of the Tcl
 *	core files.  This file depends on configuration #defines such
 *	as NO_DIRENT_H that are set up by the "configure" script.
 *
 *	Much of the material in this file was originally contributed
 *	by Karl Lehenbauer, Mark Diekhans and Peter da Silva.
 *
 * Copyright (c) 1991-1994 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 * Copyright (c) 2002 Trinity College Computing Center.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * Last modified 18 January 2002.
 */

#ifndef _TCLPORT
#define _TCLPORT

#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * The stuff below is needed by the "time" command.  If this
 * system has no gettimeofday call, then must use times and the
 * CLK_TCK #define (from sys/param.h) to compute elapsed time.
 * Unfortunately, some systems only have HZ and no CLK_TCK, and
 * some might not even have HZ.
 */

#ifdef NO_GETTOD
#   include <sys/times.h>
#   include <sys/param.h>
#   ifndef CLK_TCK
#       ifdef HZ
#           define CLK_TCK HZ
#       else
#           define CLK_TCK 60
#       endif
#   endif
#else
#   ifdef HAVE_BSDGETTIMEOFDAY
#	define gettimeofday BSDgettimeofday
#   endif
#endif

/*
 * Make sure that MAXPATHLEN is defined.
 */

#ifndef MAXPATHLEN
#   ifdef PATH_MAX
#       define MAXPATHLEN PATH_MAX
#   else
#       define MAXPATHLEN 2048
#   endif
#endif

/*
 * Substitute Tcl's own versions for several system calls.  The
 * Tcl versions retry automatically if interrupted by signals.
 * (see tclUnixUtil.c).
 */

#define open(a,b,c) TclOpen(a,b,c)
#define read(a,b,c) TclRead(a,b,c)
#define waitpid(a,b,c) TclWaitpid(a,b,c)
#define write(a,b,c) TclWrite(a,b,c)
int TclOpen(char *path, int oflag, int mode);
int TclRead(int fd, VOID *buf, size_t numBytes);
int TclWaitpid(pid_t pid, int *statPtr, int options);
int TclWrite(int fd, VOID *buf, size_t numBytes);

/*
 * Variables provided by the C library:
 */

#if defined(_sgi) || defined(__sgi)
#define environ _environ
#endif
extern char **environ;

#endif /* _TCLPORT */
