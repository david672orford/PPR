/*
** mouse:~ppr/src/pprd/pprd_pprdrv.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 19 July 2001.
*/

/*
** This module contains routines for launching pprdrv and interpreting
** the result when it exits.
*/

#include "before_system.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "pprd.h"
#include "pprd.auto_h"
#include "interface.h"
#include "respond.h"

/*
** This routine starts pprdrv for a specific printer to print a specific job.
** It is called only from pprd_printer.c:printer_start().
**
** Note that this routine should only be called _after_ it has been determined
** that the indicated job should be started on the indicated printer at the
** present time.  This determination is made by printer_start() before it
** calls this routine.
**
** If this routine does start the job it returns 0, if there is a temporary
** failure (such as insufficient resources) it returns -1.
**
** Always bracket calls to this routine with calls to lock() and unlock().
*/
int pprdrv_start(int prnid, struct QEntry *job)
    {
    const char function[] = "pprdrv_start";
    pid_t pid;                  /* process id of pprdrv */

    DODEBUG_PRNSTART(("%s(prnid=%d, job={%d,%d,%d})", function, prnid, job->destid, job->id, job->subid));

    if(!nodeid_is_local_node(job->destnode_id))
    	fatal(0, "%s(): assertion failed: destnode is not local", function);

    if(job->status != STATUS_WAITING)
    	fatal(0, "%s(): assertion failed: job->status != STATUS_WAITING", function);

    if(printers[prnid].status != PRNSTATUS_IDLE)
    	fatal(0, "%s(): assertion failed: printer not idle", function);

    if(lock_level == 0)
    	fatal(0, "%s(): tables not locked", function);

    /*
    ** Make sure we have not hit MAX_ACTIVE printers.
    ** If we have, this printer must starve for now.
    */
    if(active_printers == MAX_ACTIVE)
    	{
	DODEBUG_PRNSTART(("%s(): Starting printer \"%s\" would exceed MAX_ACTIVE", function, destid_local_to_name(prnid)));
	printer_new_status(&printers[prnid], PRNSTATUS_STARVED);
	starving_printers++;
	return -1;
    	}

    /*
    ** Make sure we are not required to yield to a printer which
    ** has been waiting for rations.
    ** If we yield, we become a starving printer.
    */
    if(printers[prnid].previous_status != PRNSTATUS_STARVED && (active_printers+starving_printers) >= MAX_ACTIVE)
    	{
    	DODEBUG_PRNSTART(("%s(): \"%s\" yielding to a starving printer", function, destid_local_to_name(prnid)));
	printer_new_status(&printers[prnid], PRNSTATUS_STARVED);
	starving_printers++;
	return -1;
    	}

    /* start pprdrv */
    if((pid = fork()) == -1)		/* if error */
	{
	error("%s(): Couldn't fork, printer \"%s\" not started", function, destid_local_to_name(prnid));
	printer_new_status(&printers[prnid], PRNSTATUS_STARVED);
	starving_printers++;
	return -1;
	}

    if(pid)				/* parent */
	{
	DODEBUG_PRNSTART(("%s(): Starting printer \"%s\", pid=%d", function, destid_local_to_name(prnid), pid));
	active_printers++;				/* add to count of printers printing */
	printers[prnid].pid = pid;			/* remember which process is printing it */
	printers[prnid].jobdestid = job->destid;	/* remember what job is being printed */
	printers[prnid].id = job->id;
	printers[prnid].subid = job->subid;
	printers[prnid].homenode_id = job->homenode_id;
	printer_new_status(&printers[prnid], PRNSTATUS_PRINTING);

	job_new_status(job->id, job->subid, job->homenode_id, prnid);

	/* If is a group job, mark last printer in group that was used. */
	if(destid_local_is_group(job->destid))
	    groups[destid_local_to_gindex(job->destid)].last = destid_local_get_member_offset(job->destid, prnid);
	}
    else				/* child */
	{
	char jobname[MAX_PPR_PATH];
	char pass_str[10];

	/*
	** Our child should not start out with
	** signals blocked, even if we have some blocked.
	*/
	{
	sigset_t sigset;
	sigemptyset(&sigset);
	sigprocmask(SIG_SETMASK, &sigset, (sigset_t*)NULL);
	}

	/*
	** Reconstruct the queue file name.
	** We can not use the library routine "local_jobid()" here
	** because it tends to ommit parts which conform
	** to default values.
	*/
	snprintf(jobname, sizeof(jobname), "%s:%s-%d.%d(%s)",
		nodeid_to_name(job->destnode_id),
		destid_to_name(job->destnode_id, job->destid),
		job->id,job->subid,
		nodeid_to_name(job->homenode_id));

	/*
	** Convert the pass number to a string so that
	** we may use it as a argument to execl().
	*/
	snprintf(pass_str, sizeof(pass_str), "%d", job->pass);

	/* Overlay this child process with pprdrv. */
	execl(PPRDRV_PATH, "pprdrv",			/* execute the driver program */
	    destid_local_to_name(prnid),		/* printer name */
	    jobname,					/* full job id string */
	    pass_str,					/* pass number as a string */
	    (char*)NULL);

	/*
	** Give parent time to record PID in printers[].  If we don't allow
	** it enough time to do that, the code in reapchild() will not
	** understand the import of the death of this process.
	*/
	sleep(1);

	/*
	** We mustn't call fatal() here as it would remove the
	** pprd lock file.  I am not even sure it is ok to
	** call error() and exit().
	*/
	error("%s(): Can't execute pprdrv, execl() failed, errno = %d (%s)", function, errno, gu_strerror(errno));
	alert(destid_local_to_name(prnid), TRUE, "Can't execute \"%s\", errno=%d (%s)", PPRDRV_PATH, errno, gu_strerror(errno));
	exit(EXIT_PRNERR);
	}

    return 0;
    } /* end of pprdrv_start() */

/*
** This routine is called whenever a pprdrv process exits.
**
** Notice that this routine makes a lot of calls to printer_new_status() which
** attempt to set the printer's status to PRNSTATUS_IDLE without regard to
** whether that is the correct new state.  This is ok since printer_new_status()
** sets the printer to the stopt state if an attempt is made to move it from
** stopping or halting to idle.
*/
static void pprdrv_exited(int prnid, int wstat)
    {
    const char function[] = "pprdrv_exited";
    int estat;				/* for pprdrv exit status */
    int job_status = STATUS_WAITING;	/* job status will be set to this (as modified) */
    int prn_status = PRNSTATUS_IDLE;	/* printer status will be set to this (as modified) */

    DODEBUG_PRNSTOP(("%s(prnid=%d, wstat=0x%04x)", prnid, wstat));

    printers[prnid].pid = 0;		/* prevent future false match */
    active_printers--;                  /* a printer is no longer active */

    /*
    ** This is good.
    */
    if(WIFEXITED(wstat))
    	{
	estat = WEXITSTATUS(wstat);
    	}

    /*
    ** If it caught a signal and core dumped, that is a major error.  If it
    ** caught a signal and didn't core dump it may be that we sent it that
    ** signal to cancel a job that was already printing or to halt a printer.
    ** We will handle the latter case below.
    */
    else if(WIFSIGNALED(wstat))
        {
        if(WCOREDUMP(wstat))
            {
            alert(printers[prnid].name, FALSE, _("Internal error, pprdrv core dumped after receiving signal %d (%s)."),
            	WSTOPSIG(wstat), gu_strsignal(WSTOPSIG(wstat)));
            estat = EXIT_PRNERR_NORETRY;
            }
	else
	    {
	    estat = EXIT_SIGNAL;
	    }
        }

    /*
    ** Stopped by signal!  Someone is playing games!
    */
    else if(WIFSTOPPED(wstat))
        {
        alert(printers[prnid].name, TRUE, _("Mischief afoot, pprdrv stopt by signal %d (%s)."), WSTOPSIG(wstat), gu_strsignal(WSTOPSIG(wstat)));
        estat = EXIT_PRNERR_NORETRY;
        }

    /*
    ** Deep mystery!
    */
    else
        {
        alert(printers[prnid].name, TRUE, X_("Bizaar pprdrv malfunction, exit status not understood."));
        estat = EXIT_PRNERR_NORETRY;
        }

    /*
    ** Act on pprdrv exit code.  Note that we wait until the next switch to
    ** handle EXIT_PRNERR and EXIT_PRNERR_NORETRY.  Also note that in this
    ** switch we replace certain more specific exit codes with those codes.
    */
    switch(estat)
        {
        case EXIT_PRINTED:              /* no error */
            DODEBUG_PRNSTOP(("(printed normally)"));

	    /* If we were trying to cancel the job, it is too late now. */
	    printers[prnid].cancel_job = FALSE;

	    /* Tell the user that the job has been printed. */
            respond(nodeid_local(), printers[prnid].jobdestid, printers[prnid].id, printers[prnid].subid, printers[prnid].homenode_id, prnid, RESP_FINISHED);

	    /* If the operator was informed that the printer was in a fault state, this call
	       will inform the operator that it has recovered. */
            alert_printer_working(printers[prnid].name,
                    printers[prnid].alert_interval,
                    printers[prnid].alert_method,
                    printers[prnid].alert_address,
                    printers[prnid].next_error_retry);

	    /* Since the printer suceeded, reset the error retry and
	       engaged retry counts. */
            printers[prnid].next_error_retry = 0;
            printers[prnid].next_engaged_retry = 0;

            break;

        case EXIT_JOBERR:               /* PostScript error in job */
            DODEBUG_PRNSTOP(("(job error)"));

            /* Place the job in the special held state called "arrested". */
            job_status = STATUS_ARRESTED;

            /* Tell the user that his job has been arrested. */
            respond(nodeid_local(), printers[prnid].jobdestid, printers[prnid].id, printers[prnid].subid, printers[prnid].homenode_id, prnid, RESP_ARRESTED);

	    /* If the printer was in a fault state, this function will inform the
	       operator that it has recovered. */
            alert_printer_working(printers[prnid].name,
                    printers[prnid].alert_interval,
                    printers[prnid].alert_method,
                    printers[prnid].alert_address,
                    printers[prnid].next_error_retry);

	    /* Reset error counters. */
            printers[prnid].next_error_retry = 0;
            printers[prnid].next_engaged_retry = 0;

            break;

        case EXIT_INCAPABLE:            /* capabilities exceeded */
            DODEBUG_PRNSTOP(("(printer is incapable)"));
            {
            int y;
            int id,subid;

            id = printers[prnid].id;        /* x is the printer id */
            subid = printers[prnid].subid;

            lock();

            for(y=0; ; y++)             /* find the queue entry */
                {
                if(y == queue_entries)
                    fatal(0, "%s(): job missing from array", function);
                if(queue[y].id == id && queue[y].subid == subid)
                    break;
                }

            /*
            ** If a single printer, set the never mask to 1,
            ** arrest the job, and say the printer is incapable.
            */
            if( ! destid_local_is_group(printers[prnid].jobdestid) )
                {
                queue[y].never |= 1;
                job_status = STATUS_STRANDED;
                respond(nodeid_local(), printers[prnid].jobdestid,
                        printers[prnid].id, printers[prnid].subid,
                        printers[prnid].homenode_id,
                        prnid, RESP_STRANDED_PRINTER_INCAPABLE);
                }

            /*
            ** If a group of printers, set the appropriate
            ** bit in the never mask.  If the bit has been
            ** set for every printer in the group, arrest
            ** the job and inform the user.
            */
            else
                {
                /*
                ** Get a number with the bit corresponding to this
                ** printer's possition in this destination group
                ** and add that bit to the bits set in the never
                ** mask.  If the printer has been removed from
                ** the group since the job was started, then
                ** media_prn_bitmask() will return 0 which will
                ** be ok.
                */
                queue[y].never |= destid_printer_bit(printers[prnid].jobdestid, prnid);

                /*
                ** If every never bit has been set, then arrest the job and
                ** inform the user.  But, if that was the 1st pass, we give
                ** the job a second chance.
                */
                if(queue[y].never == ((1 << groups[destid_local_to_gindex(printers[prnid].jobdestid)].members) - 1))
                    {
                    if( ++(queue[y].pass) > 2 ) /* if beyond the second pass */
                        {
                        job_status = STATUS_STRANDED;
                        respond(nodeid_local(), printers[prnid].jobdestid,
                                printers[prnid].id,printers[prnid].subid,
                                printers[prnid].homenode_id,
                                prnid, RESP_STRANDED_GROUP_INCAPABLE);
                        }
                    else
                        {
                        queue[y].never = 0;
                        }
                    }
                } /* end of else (group of printers) */

            unlock();

            }
            break;

        case EXIT_ENGAGED:
            DODEBUG_PRNSTOP(("(otherwise engaged or off-line)"));
	    printers[prnid].next_engaged_retry++;		/* increment retry count */
	    printers[prnid].countdown = ENGAGED_RETRY;		/* and set time for retry */
	    prn_status = PRNSTATUS_ENGAGED;
	    break;

        case EXIT_STARVED:
            DODEBUG_PRNSTOP(("(starved for system resources)"));
	    starving_printers++;
	    prn_status = PRNSTATUS_STARVED;
	    break;

        case EXIT_SIGNAL:           /* clean shutdown after receiving a signal */
            DODEBUG_PRNSTOP(("(aborted due to signal)"));

            if(printers[prnid].status == PRNSTATUS_HALTING)
		break;

            if(printers[prnid].status == PRNSTATUS_SEIZING)
                break;

            if(printers[prnid].status == PRNSTATUS_CANCELING)
                break;

	    if(WIFSIGNALED(wstat))	/* if it was a real signal, */
		{			/* (Note: we know that there was no core dump) */
		alert(printers[prnid].name, TRUE,
			_("Printing aborted because pprdrv was killed by signal %d (%s).\n"
			"This was not expected and the cause is unknown."), WSTOPSIG(wstat), gu_strsignal(WSTOPSIG(wstat)));
		}
	    else			/* Otherwise, pprdrv caught the signal and exited gracefully, */
	    	{
	    	alert(printers[prnid].name, TRUE, _("Unexpected pprdrv shutdown.  It claims to have received a signal to terminate."));
	    	}
	    estat = EXIT_PRNERR;
	    break;

	case EXIT_PRNERR:
	case EXIT_PRNERR_NOT_RESPONDING:
	case EXIT_PRNERR_NO_SUCH_ADDRESS:
	    estat = EXIT_PRNERR;
	    break;

	case EXIT_PRNERR_NORETRY:
	case EXIT_PRNERR_NORETRY_ACCESS_DENIED:
	case EXIT_PRNERR_NORETRY_BAD_SETTINGS:
	case EXIT_PRNERR_NORETRY_NO_SUCH_ADDRESS:
	    estat = EXIT_PRNERR_NORETRY;
	    break;

	default:
            DODEBUG_PRNSTOP(("(unknown exit code)"));
	    alert(printers[prnid].name, FALSE, _("Unrecognized exit code %d returned by pprdrv."), estat);
	    estat = EXIT_PRNERR;
	    break;
	}

    /*
    ** Here we go again.  This time we only pay attention to two generic
    ** error codes.  More specific error codes have been converted by
    ** now.
    */
    switch(estat)
	{
        case EXIT_PRNERR_NORETRY:
            DODEBUG_PRNSTOP(("(fault, no retry)"));
            alert(printers[prnid].name, FALSE, _("Printer placed in fault mode, no auto-retry."));
	    printers[prnid].next_error_retry = 0;
	    printers[prnid].countdown = 0;
	    prn_status = PRNSTATUS_FAULT;
	    break;

        case EXIT_PRNERR:
            DODEBUG_PRNSTOP(("(fault)"));
            alert(printers[prnid].name, FALSE, _("Printer placed in auto-retry mode."));
	    printers[prnid].next_error_retry++;
	    printers[prnid].countdown = printers[prnid].next_error_retry * RETRY_MULTIPLIER;
	    if(printers[prnid].countdown > MIN_RETRY)
		printers[prnid].countdown = MIN_RETRY;
	    prn_status = PRNSTATUS_FAULT;
            break;
        }

    /* If the printer is in fault mode (either with or without retries,
       give the alert system a chance to inform the operator if it
       wants to. */
    if(prn_status == PRNSTATUS_FAULT)
	{
	alert_printer_failed(printers[prnid].name,
		printers[prnid].alert_interval, printers[prnid].alert_method, printers[prnid].alert_address,
		printers[prnid].next_error_retry);
	}

    /* If there is an outstanding hold request, prepare to set job
       state to "held". */
    if(printers[prnid].hold_job)
	{
	job_status = STATUS_HELD;
	printers[prnid].hold_job = FALSE;
	}

    /* If there is an outstanding cancel request, inform the user.
       But we leave the flag set because the next if() tests it. */
    if(printers[prnid].cancel_job)
        {
        respond(nodeid_local(), printers[prnid].jobdestid,
                        printers[prnid].id, printers[prnid].subid,
                        printers[prnid].homenode_id,
                        prnid, RESP_CANCELED_PRINTING);
	}

    /* If we should remove the job because it was printed or canceled, do it now. */
    if(estat == EXIT_PRINTED || printers[prnid].cancel_job)
	{
        queue_dequeue_job(nodeid_local(), printers[prnid].jobdestid, printers[prnid].id, printers[prnid].subid, printers[prnid].homenode_id);
        queue_unlink_job(nodeid_local(), printers[prnid].jobdestid, printers[prnid].id, printers[prnid].subid, printers[prnid].homenode_id);
	destid_free(nodeid_local(), printers[prnid].jobdestid);
	nodeid_free(nodeid_local());
        nodeid_free(printers[prnid].homenode_id);
	printers[prnid].cancel_job = FALSE;
        }

    /* If we will not remove the job, set it to its new state. */
    else
    	{
	struct QEntry *p = job_new_status(printers[prnid].id, printers[prnid].subid, printers[prnid].homenode_id, job_status);
	if(job_status == STATUS_WAITING)
            printer_try_start_suitable_4_this_job(p);
    	}

    /* Stopping printers can only become stopt. */
    if(printers[prnid].status == PRNSTATUS_STOPPING || printers[prnid].status == PRNSTATUS_HALTING)
	prn_status = PRNSTATUS_STOPT;

    /* Actualy set the printer to its new state. */
    printer_new_status(&printers[prnid], prn_status);

    /* If the printer is now idle, look for something for it to do. */
    if(printers[prnid].status == PRNSTATUS_IDLE)
	printer_look_for_work(prnid);

    } /* end of pprdrv_exited() */

/*
** This routine is called from reapchild() at every process termination.
** It will return TRUE if the process was a pprdrv.
*/
gu_boolean pprdrv_child_hook(pid_t pid, int wstat)
    {
    int x;

    /* Search the printers to see if any of them has launched
       a pprdrv with this PID.
       */
    for(x=0; x < printer_count; x++)
        {
        if(printers[x].pid == pid)
            {
            pprdrv_exited(x, wstat);
            return TRUE;
            }
        }

    /* No match, tell reapchild() to keep looking. */
    return FALSE;
    } /* end of pprdrv_child_hook() */

/* end of file */

