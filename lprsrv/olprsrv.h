/*
** mouse:~ppr/src/include/olprsrv.h
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
** Last modified 9 August 1999.
*/

/*
** Header file for the old Berkeley LPR/LPD compatible print server.
*/

/* Which operations do we want debugging information generated for? */
#if 0
#define DEBUG_MAIN 1			/* main loop */
#define DEBUG_STANDALONE 1		/* standalone daemon operation */
#define DEBUG_PRINT 1			/* take job command */
#define DEBUG_CONTROL_FILE 1		/* read control file */
/* #define DEBUG_GRITTY 1 */		/* details of many things */
/* #define DEBUG_DISKSPACE 1 */		/* disk space checks */
#define DEBUG_LPQ 1			/* queue listing */
#define DEBUG_LPRM 1			/* job removal */
#endif

/* Where do we want debugging and error messages? */
#define LPRSRV_LOGFILE LOGDIR"/olprsrv"

/* This locked file is created when running in standalone mode: */
#define LPRSRV_LOCKFILE RUNDIR"/olprsrv.pid"

/*
** The size for our data structures which contain a list of
** of the files received.
*/
#define MAX_FILES_PER_JOB 100

/* end of file */
