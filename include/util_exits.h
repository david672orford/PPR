/*
** mouse:~ppr/src/include/util_exits.h
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
** Last modified 18 January 2000.
*/

/*
** This file defines the exit codes to be used by the utilities
** such as ppop, ppad, and ppuser.
*/

#define EXIT_OK_DATA -1		/* for ppop/pprd communication */
#define EXIT_OK 0               /* normal exit */
#define EXIT_BADDEST 1		/* non-existent destination specified */
#define EXIT_BADJOB 2		/* non-existent job specified */
#define EXIT_BADBIN 3		/* non-existent input tray */
#define EXIT_NOTFOUND 3		/* non-existent parameter value */
#define EXIT_PRINTING 4		/* can't move or hold, already printing */
#define EXIT_DENIED 5           /* access denied */
#define EXIT_PRNONLY 6		/* command only applicable to printers */
#define EXIT_CANTWAIT 7		/* wait feature already in use */
#define EXIT_ALREADY 8		/* printer/job already in desired state */
#define EXIT_INTERNAL 10        /* internal error */
#define EXIT_NOSPOOLER 11       /* spooler not running */
#define EXIT_OVERFLOW 12	/* too many of something */
#define EXIT_SYNTAX 20          /* invokation syntax error */
#define EXIT_ERROR 21           /* user error */
#define EXIT_NOTPOSSIBLE 22	/* request not executable */
#define EXIT_USER_ABORT 23	/* user requested abort */

/* end of file */
