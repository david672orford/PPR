/*
** mouse:~ppr/src/include/sysdep.h
** Copyright 1995--2003, Trinity College Computing Center.
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
** Last modified 7 March 2003.
*/

/*
** Try to define some things properly for the
** system we are compiling on.
**
** Note that this file is not named in any of the make files.  That
** means that if you modify it you will have to do a "make clean".
**
** This file is included twice, one before any system header files
** are included, once after.  The first time it is include, the
** symbols PASS1 is defined, the second time the symbol PASS2
** is defined.
**
** =====================================================================
** When PASS1 is defined, this file should define any of the following
** symbols which may be necessary:
** =====================================================================
**
** #define HAVE_STRSIGNAL 1
**			strsignal() is in library.  If this is not set,
**			PPR will provide one.
**
** #define HAVE_SNPRINTF 1
**			snprintf() is in the library.  If this is not set,
**			PPR will provide one.
**
** #define HAVE_VSNPRINTF 1
**			vsnprintf() is in the library.  If this is not set,
**			PPR will provide one.
**
** #define HAVE_STATVFS	1
**			Use statvfs() to find disk free space.
**			This implies that sys/statvfs.h should be
**			included.
**
** #define HAVE_STATFS 1
**			Use statfs() to find disk free space.
**
** #define HAVE_SYS_VFS_H 1
**			Use sys/vfs.h instead of sys/mount.h for statfs().
**
** #define HAVE_TERMIOS_H 1
**			The system has termios.h and its functions.
**			(Currently, all of the systems define this.)
**
** #define HAVE_PUTENV 1
**			putenv() is available.
**
** #define HAVE_UNSETENV
**			unsetenv() is available.  This is used by prune_env().
**
** #define HAVE_SPAWN 1	OK to spawn*() instead of fork() and exec*().
**
** #define HAVE_FORK 1	C library has a working fork() function.
**			(PPR will not be cripled if this is not defined.)
**
** #define HAVE_MKFIFO 1
**			Have a working mkfifo() function.
**
** #define HAVE_SYS_MODEM_H 1
**			Define this if TIOCM_CTS, TIOCM_DSR, and TIOCM_CAR
**			(values for the TIOCMGET ioctl) are defined in
**			sys/modem.h.
**
** #define HAVE_H_ERRNO 1
**			Resolver library functions such as gethostbyname()
**			use h_errno to return error code.
**
** #define HAVE_MKSTEMP 1
**			The BSD/GNU function mkstemp() is available.  This
**			is a replacement for POSIX mktemp() which avoids
**			symbolic link exploits.
**
** #define HAVE_NETGROUP 1
**			Do we have netdb.h and innetgrp()?
**
** #define HAVE_INITGROUPS 1
**			Do we have initgroups()?
**
** =====================================================================
** When PASS2 is defined, this file should define the paths to various
** programs and correct any mistakes the system include files may
** have made.
** =====================================================================
**
** #define SENDMAIL_PATH "/usr/lib/sendmail"
**			The path to sendmail or compatible program
**			If you wish to change this, you must #undef
**			it first since it is already defined as
**			"/usr/lib/sendmail".
**
** #define LPR_EXTENSIONS_OSF 1
**			Define this if your lpr has the DEC OSF extensions
**			such as the -I, -j, -K, -N, -o, -O, and -x switches.
**
** #define LP_LIST_PRINTERS "/etc/lp/printers"
**			This can be defined as
**			the name of a directory which will contain
**			one entry (either a file or a subdirectory)
**			named after each printer.  There is no
**			default value.
**
** #define LP_LIST_CLASSES "/etc/lp/classes"
**			This can be defined as the
**			name of a directory which will contain one entry
**			(either a file or a subdirectory) named after
**			each class (group).  There is no default
**			value.
**
** #define LP_LPSTAT_BROKEN 1
**			This is defined if lpstat is so old that it cannot
**			parse the line "lpstat -o myprinter".
**
** #define SAFE_PATH "/bin:/usr/bin"
**			Defines a nice, secure path for filters, interfaces,
**			responders, and other children of ppr and pprd.
**			Other reasonable values are "/bin"
**			and "/bin:/usr/bin:/usr/local/bin".
**
** #define LATE_UNLINK 1
**			This should be defined if unlink()ing an open file
**			is not allowed or causes bad things to happen.  Though
**			common in Unix implementations, the ability to remove
**			all names for an open file is not a Posix requirement.
**			(This option is not fully implemented.)
**
** #define SET_BACKSPACE 1
**			Define this if ppop and ppad should do a stty to set
**			the backspace to control-h when entering interactive
**			mode.
**
** #define BIND_ACCESS_BUG 1
**			Define this if bind() can't assign a reserved port
** 			number unless the socket was opened by root.
**
** #define BROKEN_SETUID_BIT 1
**			Set this if the setuid and setgid bits are ignored.
**			At the moment, this causes ppr and pprd to ommit
**			some startup tests.
**
** #define COLON_FILENAME_BUG 1
**			Set this if colons in filenames cause problems.
**			Defining this causes fname_sprintf() to change
**			colons into excalmation points.
*/

/* This doesn't work yet. */
#ifdef PPR_AUTOCONF
#include "../config.h"
#else

/*
** These will be the most normal values.
** They serve as defaults.
*/
#ifdef PASS1
#define HAVE_STRSIGNAL 1
#define HAVE_SNPRINTF 1
#define HAVE_VSNPRINTF 1
#define HAVE_FORK 1
#define HAVE_PUTENV 1
#define HAVE_TERMIOS_H 1
#define HAVE_MKFIFO 1
#define HAVE_NETGROUP 1
#define HAVE_SETREUID 1
#define HAVE_SETREGID 1
#endif

#ifdef PASS2
#define SENDMAIL_PATH "/usr/lib/sendmail"	/* mail program for alerting */
#define SAFE_PATH "/bin:/usr/bin"		/* Secure path */
#endif

/*========================================================================
** AT&T System V Release 4.0 for the AT&T WGS series
** of 386/486 microcomputers.  This is the origional
** platform for PPR.
**
** This port has not been tested recently.
**
** Sample machine: alice
========================================================================*/
#ifdef PPR_ATTWGS
#ifdef PASS1

#undef HAVE_STRSIGNAL
#undef HAVE_SNPRINTF
#undef HAVE_VSNPRINTF

#define BACKSPACE 1
#define HAVE_STATVFS 1

#endif
#ifdef PASS2

/* /bin is a link to /usr/bin */
#undef SAFE_PATH
#define SAFE_PATH "/usr/bin"

#undef SENDMAIL_PATH
#define SENDMAIL_PATH "/usr/ucblib/sendmail"

/* The LP paths are just a guess!!! */
#define LP_LIST_PRINTERS "/var/spool/lp/admins/lp/interfaces"
#define LP_LIST_CLASSES "/var/spool/lp/admins/lp/classes"

#ifndef S_ISLNK
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#endif

/*
** This Unix has a lot of errors in its include files.
** We will provide prototypes for a number of functions whose
** protypes are missing from the include files.
** For the socket functions, we must include sys/socket.h now
** in order for the definitions to make sense.
*/
int fchmod(int fildes, mode_t mode);
#include <sys/socket.h>
int getpeername(int s, struct sockaddr *name, int *namelen);
int socket(int domain, int type, int protocol);
int connect(int s, struct sockaddr *name, int namelen);
int setsockopt(int s, int level, int optname, char *optval, int optlen);
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *execptfds, struct timeval *timeout);

#endif /* PASS2 */
#endif /* PPR_ATTWGS */

/*========================================================================
** SunOS 5.0 thru 5.5.1 (Solaris 2.0 thru 5.5.1)
** See below for SunOS 5.6 and later.
**
** Sample machine: Lor?
========================================================================*/
#ifdef PPR_SUNOS_5
#ifdef PASS1

/* These don't appear until 5.6 */
#undef HAVE_SNPRINTF
#undef HAVE_VSNPRINTF

#define HAVE_STATVFS 1

#endif
#ifdef PASS2

/* /bin is a link to /usr/bin */
#undef SAFE_PATH
#define SAFE_PATH "/usr/bin"

#define LP_LIST_PRINTERS "/etc/lp/printers"
#define LP_LIST_CLASSES "/etc/lp/classes"

/* Work around difference in when access
   to reserved ports is permitted. */
#define BIND_ACCESS_BUG 1

#endif /* PASS2 */
#endif /* PPR_SUNOS_5 */

/*========================================================================
** NetBSD 1.0
**
** Sample machine:
========================================================================*/
#ifdef PPR_NETBSD
#ifdef PASS1

#define HAVE_STATFS 1
#define HAVE_INITGROUPS 1

#endif
#ifdef PASS2

#define setsid() setpgrp(0, getpid()) /* setsid() (Posix?) is missing */
#undef SENDMAIL_PATH
#define SENDMAIL_PATH "/usr/sbin/sendmail"

#endif /* PASS2 */
#endif /* PPR_NETBSD */

/*========================================================================
** Linux
** Linux for i386 is the principal development platform.
** Linux for DEC Alpha has received some testing at Trinity College.
**
** Sample machine: Mouse
========================================================================*/
#ifdef PPR_LINUX
#ifdef PASS1

#define HAVE_STATFS 1
#define HAVE_SYS_VFS_H 1
#define HAVE_UNSETENV 1
#define HAVE_H_ERRNO 1
#define HAVE_MKSTEMP 1
#define HAVE_INITGROUPS 1

/* This is to test the substitute code which normally isn't
   used on Linux systems. */
#if 0
#undef HAVE_STRSIGNAL
#undef HAVE_SNPRINTF
#undef HAVE_VSNPRINTF
#undef HAVE_MKSTEMP
#endif

#endif
#ifdef PASS2

#undef SENDMAIL_PATH
#define SENDMAIL_PATH "/usr/sbin/sendmail"

#endif /* PASS2 */
#endif /* PPR_LINUX */

/*========================================================================
** SunOS 4.1.3_U1
**
** Sample machine:
========================================================================*/
#ifdef PPR_SUNOS4
#ifdef PASS1

#undef HAVE_STRSIGNAL
#undef HAVE_SNPRINTF
#undef HAVE_VSNPRINTF
#define HAVE_STATFS 1
#define HAVE_SYS_VFS_H 1

#endif
#ifdef PASS2

/* /bin is a link to /usr/bin */
#undef SAFE_PATH
#define SAFE_PATH "/usr/bin"

#ifndef WCOREDUMP
#define WCOREDUMP(stat) ((stat)&0200)
#endif

#define difftime(t1,t0) ((double)((t1)-(t0)))

#define memmove(ARG1,ARG2,LENGTH) bcopy(ARG1,ARG2,LENGTH)

#endif /* PASS2 */
#endif /* PPR_SUNOS4 */

/*========================================================================
** OSF/1 3.2 or Digital Unix 4.0 for DEC Alpha
**
** Sample machine: Heart
========================================================================*/
#ifdef PPR_OSF1
#ifdef PASS1

#undef HAVE_STRSIGNAL
#undef HAVE_SNPRINTF
#undef HAVE_VSNPRINTF
#define HAVE_STATVFS 1

#endif
#ifdef PASS2

/* /bin is a link to /usr/bin */
#undef SAFE_PATH
#define SAFE_PATH "/usr/bin"

/* It looks like lp is just a front end to lpr: */
#define LPR_EXTENSIONS_OSF 1

#endif /* PASS2 */
#endif /* PPR_OSF1 */

/*========================================================================
** SGI IRIX 6.3
**
** Sample machine: Ermac
========================================================================*/
#ifdef PPR_IRIX
#ifdef PASS1

#undef HAVE_STRSIGNAL
#undef HAVE_SNPRINTF
#undef HAVE_VSNPRINTF
#define HAVE_STATVFS 1		/* we have statvfs() */

#endif
#ifdef PASS2

/* /bin is a link to /usr/bin */
#undef SAFE_PATH
#define SAFE_PATH "/usr/bin"

#define LP_LIST_PRINTERS "/var/spool/lp/interface"
#define LP_LIST_CLASSES "/var/spool/lp/class"

#endif
#endif /* PPR_IRIX */

/*========================================================================
** MS-Windows 95 or NT with Cygwin 1.1
** Sample machine: Jane
========================================================================*/
#ifdef PPR_CYGWIN
#ifdef PASS1

#undef HAVE_SNPRINTF
#undef HAVE_VSNPRINTF
#undef HAVE_MKFIFO
#undef HAVE_NETGROUP

#endif
#ifdef PASS2

#define WCOREDUMP(a) 0		/* not supported? */
#define BROKEN_SETUID_BIT 1
#define COLON_FILENAME_BUG 1
#undef SAFE_PATH
#define SAFE_PATH "/bin:/usr/bin:/winnt/system32:/winnt"

int seteuid(uid_t);		/* not defined in header files */

#define setreuid(a,b) setuid(a)
#define setregid(a,b) setgid(a)

#endif /* PASS2 */
#endif /* PPR_CYGWIN */

/*========================================================================
** MS-Windows 95/NT with AT&T UWIN 2.9
** Sample machine: Jane
========================================================================*/
#ifdef PPR_UWIN
#ifdef PASS1

/* As of 2.25: */
#undef HAVE_STRSIGNAL

#endif
#ifdef PASS2

/* Missing from 1.51: */
#define WCOREDUMP(a) 0

/* This is missing from UWIN 1.51: */
#define setegid(x) 0

/* #define BROKEN_SETUID_BIT 1 */

#define COLON_FILENAME_BUG 1

#undef SAFE_PATH
#define SAFE_PATH "/usr/bin"

#endif /* PASS2 */
#endif /* __UWIN__ */

/*========================================================================
** HP-UX 10.x
**
** Sample machine: Micro1
========================================================================*/
#ifdef PPR_HPUX
#ifdef PASS1

#undef HAVE_STRSIGNAL
#undef HAVE_SNPRINTF
#undef HAVE_VSNPRINTF
#define HAVE_STATVFS 1
#define HAVE_SYS_MODEM_H 1

#endif
#ifdef PASS2

/* /bin is a link to /usr/bin */
#undef SAFE_PATH
#define SAFE_PATH "/usr/bin"

#define LP_LPSTAT_BROKEN 1
#define LP_LIST_PRINTERS "/etc/lp/interface"
#define LP_LIST_CLASSES "/etc/lp/class"

#define seteuid(x)     setresuid(-1,(x),-1)
#define setegid(x)     setresgid(-1,(x),-1)
#define setreuid(x,y)  setresuid((x),(y),-1)
#define setregid(x,y)  setresgid((x),(y),-1)

#endif /* PASS2 */
#endif /* PPR_HPUX */

/*========================================================================
** DEC ULTRIX 4.4 (RISC DECstation, similiar to OSF1, but older)
**
** Sample machine:
========================================================================*/
#ifdef PPR_ULTRIX
#ifdef PASS1

#undef HAVE_STRSIGNAL
#undef HAVE_SNPRINTF
#undef HAVE_VSNPRINTF

#endif
#ifdef PASS2

#define LPR_EXTENSIONS_OSF 1

#ifndef S_ISLNK
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#endif

#ifndef WCOREDUMP
#define WCOREDUMP(a) 0
#endif

#endif /* PASS2 */
#endif /* PPR_ULTRIX */

/*========================================================================
** SunOS 5.6 thru 5.7 (Solaris 2.6 thru 7)
**
** Sample machine: shakti
========================================================================*/
#ifdef PPR_SUNOS_5_6
#ifdef PASS1

#define HAVE_STATVFS 1

#endif
#ifdef PASS2

/* /bin is a link to /usr/bin */
#undef SAFE_PATH
#define SAFE_PATH "/usr/bin"

#define LP_LIST_PRINTERS "/etc/lp/printers"
#define LP_LIST_CLASSES "/etc/lp/classes"

/* Work around difference in when access
   to reserved ports is permitted. */
#define BIND_ACCESS_BUG 1

#endif /* PASS2 */
#endif /* PPR_SUNOS_5_6 */

/*========================================================================
** FreeBSD 3.1R
**
** Sample machine:
========================================================================*/
#ifdef PPR_FREEBSD
#ifdef PASS1

#undef HAVE_STRSIGNAL
#define HAVE_STATFS 1
#define HAVE_INITGROUPS 1

#endif
#ifdef PASS2

#undef SAFE_PATH
#define SAFE_PATH "/bin:/usr/bin:/usr/local/bin"
#undef SENDMAIL_PATH
#define SENDMAIL_PATH "/usr/sbin/sendmail"

#endif /* PASS2 */
#endif /* PPR_FREEBSD */

/*========================================================================
** Darwin (MacOS 10.x)
========================================================================*/
#ifdef PPR_DARWIN
#ifdef PASS1
#undef HAVE_STRSIGNAL
#endif

#ifdef PASS2
#undef SAFE_PATH
#define SAFE_PATH "/sw/bin:/usr/bin:/bin"

#endif
#endif /* PPR_DARWIN */

/*========================================================================
** Win32 testing on Linux
========================================================================*/
#ifdef PPR_WIN32_TESTING
#ifdef PASS1

#undef HAVE_MKFIFO
#undef HAVE_STATFS
#undef HAVE_SYS_VFS_H
#undef HAVE_UNSETENV
#undef SENDMAIL_PATH
#define SENDMAIL_PATH "/usr/sbin/sendmail"
#endif
#ifdef PASS2

#define COLON_FILENAME_BUG 1

#endif /* PASS2 */
#endif /* WIN32_TESTING */

/*======================================================================*/

#endif /* Not autoconf */

/* end of file */

