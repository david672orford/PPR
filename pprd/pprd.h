/*
** mouse:~ppr/src/include/pprd.h
** Copyright 1995--2007, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 30 March 2007.
*/

/*
** Values and structures used only by pprd and ppop.  Both pprd.c
** and ppop.c include this file after they include global_defines.h.
** Ppop actually only uses a few of them.  Hopefully, some day it
** will not use any.
*/

/* This you might want to change */
#define TICK_INTERVAL 5					/* seconds between calls to tick() */
#define RETRY_MULTIPLIER 30				/* extra seconds per retry */
#define MIN_RETRY 600					/* retry at least this often (seconds) */
#define ENGAGED_RETRY 60				/* interval to retry ``otherwise engaged'' printers */
#define MAX_ACTIVE 15					/* maximum simultainiously active printers */
#define STARVING_RETRY_INTERVAL 5		/* how often to retry starving printers */
#define ENGAGED_NAG_TIME 20				/* Engaged time to qualify as "remaining printer problem" */

#define QUEUE_SIZE_INITIAL 200			/* entries allocated at startup */
#define QUEUE_SIZE_GROWBY 50			/* additional entries allocated at each overflow */
#define QUEUE_SIZE_MAX 10000			/* absolute maximum size we will attempt to allocate */

/*
** These are the pprd debugging options.  Change "#if 0" to "#if 1" to turn 
** debugging on.
**
** Note that we use C++ comments here to disable the ones we don't want.  This
** OK only because it is inside a block that is normaly excluded by the #if 0.
*/
#if 0
#define DEBUG 1							/* define function[] strings */
//#define DEBUG_STARTUP 1				/* initialization routines */
//#define DEBUG_MAINLOOP 1				/* main loop */
//#define DEBUG_RECOVER 1				/* reloading jobs and mounted media on restart */
//#define DEBUG_NEWJOBS 1				/* receipt of new jobs */
//#define DEBUG_PRNSTART 1				/* starting of printers */
//#define DEBUG_PRNSTART_GRITTY 1		/* details of starting printers */
//#define DEBUG_PRNSTOP 1				/* analysis of pprdrv exit */
//#define DEBUG_DEQUEUE 1				/* removal from the queue */
//#define DEBUG_MEDIA 1					/* media operations */
//#define DEBUG_TICK 1					/* debug timer tick routine */
#define DEBUG_RESPOND 1				/* launching of responders */
//#define DEBUG_PPOPINT 1				/* interface to ppop */
//#define DEBUG_ALERTS 1				/* sending of operator alerts */
//#define DEBUG_NODEID 1				/* allocating and deallocating node id numbers */
//#define DEBUG_QUESTIONS 1				/* sending questions to job submitters */
#define DEBUG_IPP 1						/* Internet Printing Protocol operations */
#define DEBUG_LISTENER 1				/* TCP socket listeners */
#endif

/*
** The name of the debugging output file.  Even if all of the above
** debugging options are turned off, some output will still be sent
** to this file.
*/
#define PPRD_LOGFILE LOGDIR"/pprd"

/*============ User: don't change anything below this line. ============*/

/* A few global variables: */
extern const char myname[];
extern gu_boolean option_foreground;
extern gu_boolean option_debug;
extern time_t daemon_start_time;
extern gu_boolean lockfile_created;

/* A few critical declarations: */
void fatal(int exval, const char string[], ...)
#ifdef __GNUC__
__attribute__ (( noreturn, format (printf, 2, 3) ))
#endif
;
void debug(const char format[],...)
#ifdef __GNUC__
__attribute__ (( format (printf, 1, 2) ))
#endif
;
void error(const char *string, ... )
#ifdef __GNUC__
__attribute__ (( format (printf, 1, 2) ))
#endif
;

/*
** Values which pprd uses as the first argument to fatal().
** Ordinarily it uses ERROR_DIE, but if the value is
** ERROR_DUMPCORE, then fatal() tries to prevoke a core
** dump.  This feature is available for some debugging
** purpose which I cannot now remember.
*/
#define ERROR_DIE 0
#define ERROR_DUMPCORE 100

/* structure to describe a printer */
struct Printer
	{
	char *name;							/* name of the printer */
	struct PRINTER_SPOOL_STATE spool_state;
	struct ALERT alert;					/* printer alert settings */
	int charge_per_duplex;				/* per-sheet charge */
	int charge_per_simplex;				/* half-sheet charge */
	int nbins;							/* number of bins */
	const char *bins[MAX_BINS];			/* binname of each bin */
	gu_boolean AutoSelect_exists;		/* TRUE if any bin is named "AutoSelect" */
	int media[MAX_BINS];				/* media id of media in each bin */
	gu_boolean cancel_job;				/* cancel the job at pprdrv exit */
	gu_boolean hold_job;				/* hold the job at pprdrv exit */
	pid_t job_pid;						/* pid of process driving the printer */
	int job_destid;						/* dest id of the job we are printing */
	int job_id;							/* queue id of job being printed */
	int job_subid;						/* queue subid of job being printed */
	pid_t ppop_pid;						/* send SIGUSR1 to this process when stopt */
	} ;

/* a group */
struct Group
	{
	char *name;							/* name of group */
	struct GROUP_SPOOL_STATE spool_state;
	int printers[MAX_GROUPSIZE];		/* printer id's of members */
	int members;						/* number of members */
	int last;							/* member offset of member last used */
	gu_boolean rotate;					/* TRUE if we should use in rotation */
	gu_boolean deleted;					/* TRUE if group has been deleted */
	} ;

/*
** Debugging macros.
*/
#ifdef DEBUG
#define FUNCTION4DEBUG(a) const char function[] = a ;
#else
#define FUNCTION4DEBUG(a)
#endif

#ifdef DEBUG_STARTUP
#define DODEBUG_STARTUP(a) debug a
#else
#define DODEBUG_STARTUP(a)
#endif

#ifdef DEBUG_MAINLOOP
#define DODEBUG_MAINLOOP(a) debug a
#else
#define DODEBUG_MAINLOOP(a)
#endif

#ifdef DEBUG_RECOVER
#define DODEBUG_RECOVER(a) debug a
#else
#define DODEBUG_RECOVER(a)
#endif

#ifdef DEBUG_NEWJOB
#define DODEBUG_NEWJOB(a) debug a
#else
#define DODEBUG_NEWJOB(a)
#endif

#ifdef DEBUG_DEQUEUE
#define DODEBUG_DEQUEUE(a) debug a
#else
#define DODEBUG_DEQUEUE(a)
#endif

#ifdef DEBUG_MEDIA
#define DODEBUG_MEDIA(a) debug a
#else
#define DODEBUG_MEDIA(a)
#endif

#ifdef DEBUG_PROGINIT
#define DODEBUG_PROGINIT(a) debug a
#else
#define DODEBUG_PROGINIT(a)
#endif

#ifdef DEBUG_RESPOND
#define DODEBUG_RESPOND(a) debug a
#else
#define DODEBUG_RESPOND(a)
#endif

#ifdef DEBUG_ALERTS
#define DODEBUG_ALERTS(a) debug a
#else
#define DODEBUG_ALERTS(a)
#endif

#ifdef DEBUG_PPOPINT
#define DODEBUG_PPOPINT(a) debug a
#else
#define DODEBUG_PPOPINT(a)
#endif

#ifdef DEBUG_PRNSTART
#define DODEBUG_PRNSTART(a) debug a
#else
#define DODEBUG_PRNSTART(a)
#endif

#ifdef DEBUG_PRNSTOP
#define DODEBUG_PRNSTOP(a) debug a
#else
#define DODEBUG_PRNSTOP(a)
#endif

#ifdef DEBUG_NOTNOW
#define DODEBUG_NOTNOW(a) debug a
#else
#define DODEBUG_NOTNOW(a)
#endif

#ifdef DEBUG_NEWJOBS
#define DODEBUG_NEWJOBS(a) debug a
#else
#define DODEBUG_NEWJOBS(a)
#endif

#ifdef DEBUG_TICK
#define DODEBUG_TICK(a) debug a
#else
#define DODEBUG_TICK(a)
#endif

#ifdef DEBUG_NODEID
#define DODEBUG_NODEID(a) debug a
#else
#define DODEBUG_NODEID(a)
#endif

#ifdef DEBUG_QUESTIONS
#define DODEBUG_QUESTIONS(a) debug a
#else
#define DODEBUG_QUESTIONS(a)
#endif

#ifdef DEBUG_IPP
#define DODEBUG_IPP(a) debug a
#else
#define DODEBUG_IPP(a)
#endif

#ifdef DEBUG_LISTENER
#define DODEBUG_LISTENER(a) debug a
#else
#define DODEBUG_LISTENER(a)
#endif

/* end of file */
