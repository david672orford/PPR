/*
** mouse:~ppr/src/include/pprd.h
** Copyright 1995--2005, Trinity College Computing Center.
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
** Last modified 23 February 2005.
*/

/*
** Values and structures used only by pprd and ppop.  Both pprd.c
** and ppop.c include this file after they include global_defines.h.
** Ppop actually only uses a few of them.  Hopefully, some day it
** will not use any.
*/

/* This you might want to change */
#define TICK_INTERVAL 5					/* seconds between calls to tick() */
#define UPGRADE_INTERVAL 20				/* number of ticks to gain one pri point */
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
#if 1
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
//#define DEBUG_RESPOND 1				/* launching of responders */
//#define DEBUG_PPOPINT 1				/* interface to ppop */
//#define DEBUG_ALERTS 1				/* sending of operator alerts */
//#define DEBUG_NODEID 1				/* allocating and deallocating node id numbers */
#define DEBUG_QUESTIONS 1				/* sending questions to job submitters */
#define DEBUG_IPP 1						/* Internet Printing Protocol operations */
#endif

/*
** The name of the debugging output file.  Even if all of the above
** debugging options are turned off, some output will still be sent
** to this file.
*/
#define PPRD_LOGFILE LOGDIR"/pprd"

/*============ User: don't change anything below this line. ============*/

/* A few global variables: */
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
	char name[MAX_DESTNAME+1];			/* name of the printer */
	int alert_interval;					/* every this many retries */
	char *alert_method;					/* means of communicating with operator */
	char *alert_address;				/* address of operator */
	gu_boolean protect;					/* TRUE if "Charge:" line in conf file */
	int charge_per_duplex;				/* per-sheet charge */
	int charge_per_simplex;				/* half-sheet charge */
	gu_boolean accepting;				/* TRUE if is accepting as destination */
	int nbins;							/* number of bins */
	int bins[MAX_BINS];					/* binname id of each bin */
	gu_boolean AutoSelect_exists;		/* TRUE if any bin is named "AutoSelect" */
	int media[MAX_BINS];				/* media id of media in each bin */
	int previous_status;				/* saved previous status */
	int status;							/* idle, disabled, etc */
	gu_boolean cancel_job;				/* cancel the job at pprdrv exit */
	gu_boolean hold_job;				/* hold the job at pprdrv exit */
	int next_error_retry;				/* number of next retry */
	int next_engaged_retry;				/* number of times otherwise engaged or off-line */
	int countdown;						/* seconds till next retry */
	pid_t pid;							/* pid of process driving the printer */
	int jobdestid;						/* dest id of the job we are printing */
	int id;								/* queue id of job being printed */
	int subid;							/* queue subid of job being printed */
	int homenode_id;					/* id number of node which submitted the job */
	pid_t ppop_pid;						/* send SIGUSR1 to this process when stopt */
	} ;

/* a group */
struct Group
	{
	char name[MAX_DESTNAME+1];			/* name of group */
	int printers[MAX_GROUPSIZE];		/* printer id's of members */
	int members;						/* number of members */
	int last;							/* member offset of member last used */
	gu_boolean accepting;				/* TRUE if accepting new jobs */
	gu_boolean held;					/* TRUE if jobs for group held */
	gu_boolean rotate;					/* TRUE if we should use in rotation */
	gu_boolean protect;					/* TRUE if we should restrict use */
	gu_boolean deleted;					/* TRUE if group has been deleted */
	} ;

/* A queue entry as stored by pprd and passed back to ppop.
   Notice that this is shorter than struct QFileEntry. */
struct QEntry
	{
	SHORT_INT destnode_id;				/* destination node by key number */
	SHORT_INT destid;					/* destination key number */
	SHORT_INT id;						/* queue id */
	SHORT_INT subid;					/* fractional queue id */
	SHORT_INT homenode_id;				/* id of node job come from */

	SHORT_INT status;					/* printer id if printing, < 0 for other status */
	unsigned short int flags;			/* --keep, responding, etc. */
	time_t resend_message_at;			/* time at which to retry responder to questioner */

	SHORT_INT priority;					/* priority number (0=highest, 39=lowest) */
	unsigned char never;				/* bitmap of group member which can't print */
	unsigned char notnow;				/* bitmap of group members without media mntd */
	SHORT_INT media[MAX_DOCMEDIA];		/* list of id numbers of media types req. */
	SHORT_INT pass;						/* number of current pass thru printers in group */
	} ;

/*
** Printer status values
*/
#define PRNSTATUS_IDLE 0				/* idle but ready to print */
#define PRNSTATUS_PRINTING 1			/* printing right now */
#define PRNSTATUS_CANCELING 2			/* canceling a job */
#define PRNSTATUS_SEIZING 3				/* stopping printing current job and holding it */
#define PRNSTATUS_FAULT 4				/* waiting for auto retry */
#define PRNSTATUS_ENGAGED 5				/* printer is printing for another computer */
#define PRNSTATUS_STARVED 6				/* starved for system resources */
#define PRNSTATUS_STOPT 7				/* stopt by user */
#define PRNSTATUS_STOPPING 8			/* will go to PRNSTATUS_STOPT at job end */
#define PRNSTATUS_HALTING 9				/* pprdrv being killed */
#define PRNSTATUS_DELETED 10			/* printer has been deleted */
#define PRNSTATUS_DELIBERATELY_DOWN 7	/* 1st non-printing value (stopt) */

/*
** Structures for response to the "f" command.
** Used by pprd and ppop.  These will have to go
** because the format is not network portable.
*/
struct fcommand1
	{
	int nbins;						/* number of bins in this printer */
	char prnname[MAX_DESTNAME+1];	/* name of the printer */
	} ;
struct fcommand2
	{
	char bin[MAX_BINNAME+1];		/* name of this bin */
	char media[MAX_MEDIANAME+1];	/* name of mounted media */
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

/* end of file */
