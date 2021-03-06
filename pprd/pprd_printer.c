/*
** mouse:~ppr/src/pprd/pprd_printer.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 27 April 2006.
*/

/*
** This module contains routines for managing printer status and arranging for
** jobs to be printed on the right printers.  To actually start printing,
** this module calls pprdrv_start().
*/

#include "config.h"
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "pprd.h"
#include "pprd.auto_h"

/*
** This routine makes sure certain conditions are met before calling
** pprdrv_start() to do the actual work.
**
** Return values:
**		0		job started
**		-1		printer not idle or start attempt failed (due to a presumably transient condition)
**		-2		job can't be printed on this printer (at least not right now)
*/
static int printer_start(int prnid, struct QEntry *job)
	{
	const char function[] = "printer_start";

	DODEBUG_PRNSTART(("%s(prinid=%d, job={%s-%d.%d})", function, prnid, destid_to_name(job->destid), job->id, job->subid));

	if(job->status != STATUS_WAITING)
		fatal(0, "%s(): assertion failed: job->status != STATUS_WAITING", function);

	if(lock_level == 0)
		fatal(0, "%s(): tables not locked", function);

	/*
	** Don't start try to start the printer if it is already printing.
	*/
	if(printers[prnid].spool_state.status != PRNSTATUS_IDLE)
		{
		DODEBUG_PRNSTART(("%s(): printer \"%s\" is not idle", function, destid_to_name(prnid)));
		return -1;
		}

	/*
	** Don't start it if the destination is a group and it is held.
	*/
	if(destid_is_group(job->destid) && groups[destid_to_gindex(job->destid)].spool_state.held)
		{
		DODEBUG_PRNSTART(("%s(): destination \"%s\" is held", function, destid_to_name(job->destid)));
		return -2;
		}

	/*
	** Don't start it the printer doesn't have the forms or pprdrv has already
	** ruled that it is incapable of printing this job.
	*/
	{
	int bitmask = destid_printer_bit(job->destid, prnid);
	DODEBUG_PRNSTART(("%s(): bitmask=%d, job->notnow=%d, job->never=%d", function, bitmask, job->notnow, job->never));
	if(job->notnow & bitmask)
		{
		DODEBUG_PRNSTART(("%s(): notnow bit set for this printer", function));
		return -2;
		}
	if(job->never & bitmask)
		{
		DODEBUG_PRNSTART(("%s(): never bit set for this printer", function));
		return -2;
		}
	}

	/*
	** All is clear, go ahead and try.
	*/
	{
	int retval = pprdrv_start(prnid, job);
	if(retval != 0 && retval != -1)
		fatal(0, "%s(): assertion_failed: pprdrv_start() return %d", function, retval);
	return retval;
	}
	} /* end of printer_start() */

/*
** If there are any jobs this printer may print, start it now.
** Nothing really bad will happen if this routine is called while the
** printer is not idle, but doing so will waste time.
**
** This routine checks `never' and `notnow' for itself.  Doing so is
** not necessary but the code was accidentally written and probably
** makes things minutely faster, so it was left in.
*/
void printer_look_for_work(int prnid)
	{
	const char function[] = "printer_look_for_work";
	int x;

	DODEBUG_PRNSTART(("%s(): Looking for work for printer %d (\"%s\")", function, prnid, destid_to_name(prnid)));

	lock();						/* lock out others while we modify */

	if(printers[prnid].spool_state.status != PRNSTATUS_IDLE)
		fatal(0, "%s(): assertion failed: printer is not idle", function);

	for(x=0; x < queue_entries; x++)
		{
		#ifdef DEBUG_PRNSTART_GRITTY
		debug("trying job: destid=%d, id=%d, subid=%d, status=%d",
			queue[x].destid,queue[x].id,queue[x].subid,queue[x].status);
		#endif

		/* This if() is true if all of these are true:
		   1) Job is ready to print
		   2) Printer is destination or member of destination group
		   */
		if(queue[x].status == STATUS_WAITING
				&& ( queue[x].destid == prnid
					|| (destid_is_group(queue[x].destid) && destid_get_member_offset(queue[x].destid, prnid) != -1) )
				)
			{
			/* Try to start the printer.  If the return value is 0 (success)
			   or -1 (failure), then stop.  If it is -2 (job unsuitable),
			   keep looking.
			   */
			if(printer_start(prnid, &queue[x]) != -2)
				break;
			}
		} /* loop thru jobs */

	#ifdef DEBUG_PRNSTART
	if(x == queue_entries)
		debug("no work for \"%s\"", destid_to_name(prnid));
	#endif

	unlock();					/* allow others to use tables now */
	} /* printer_look_for_work() */

void group_look_for_work(int gindex)
	{
	int y;
	for(y=0; y<groups[gindex].members; y++)
		{
		if(printers[groups[gindex].printers[y]].spool_state.status == PRNSTATUS_IDLE)
			printer_look_for_work(groups[gindex].printers[y]);
		}
	} /* group_look_for_work() */

/*
** Figure out what printers might be able to start this job and try each
** of them in turn.  We let printer_start() determine if the printers are
** idle and if `notnow' or `never' bits are set.  (Which would indicate that
** printers either don't have the required forms or are intrinsically
** unsuitable for the job.)
**
** It is quite normal for this routine to fail.
*/
void printer_try_start_suitable_4_this_job(struct QEntry *job)
	{
	const char function[] = "printer_try_start_suitable_4_this_job";

	DODEBUG_PRNSTART(("%s(job={%d,%d,%d})", function, job->destid, job->id, job->subid));

	lock();

	if(job->status != STATUS_WAITING)
		fatal(0, "%s(): assertion failed: job->status != STATUS_WAITING", function);

	if(destid_is_group(job->destid))		/* if group, we have many to try */
		{
		struct Group *cl;
		int x, y;

		cl = &groups[destid_to_gindex(job->destid)];

		if(cl->rotate)			/* if we should rotate */
			y = cl->last;		/* set just before next one */
		else					/* otherwise, set just before */
			y = -1;				/* first printer */

		for(x=0; x < cl->members; x++)
			{
			#ifdef DEBUG_PRNSTART_GRITTY
			debug("last printer in group was %d", y);
			#endif

			/* rotate to next printer */
			y = (y+1) % cl->members;

			#ifdef DEBUG_PRNSTART_GRITTY
			debug("trying member %d",y);
			#endif

			if(printer_start(cl->printers[y], job) == 0)
				break;
			}
		}
	else							/* if a single printer, */
		{							/* we can try only one */
		printer_start(job->destid, job);
		}

	unlock();
	} /* end of printer_try_start_suitable_4_this_job() */

/*
** Change the status of a printer.  This is done in a function so that we
** can keep track of the previous status, inform queue display programs 
** of the change, and create the files from which we load the printer's
** status after a restart.
*/
void printer_new_status(struct Printer *printer, int newstatus)
	{
	const char function[] = "printer_new_status";

	lock();										/* lock the queue and printer list */

	/* Deleted printers can't change state. */
	if(printer->spool_state.status==PRNSTATUS_DELETED)
		{
		error("%s(): attempt to change state of deleted printer", function);
		unlock();
		return;
		}

	printer->spool_state.previous_status = printer->spool_state.status;		/* used by pprdrv_start() */
	printer->spool_state.status = newstatus;
	printer->spool_state.printer_state_change_time = time(NULL);

	/* Write out the status for use during restarts and by ppop. */
	printer_spool_state_save(&(printer->spool_state), printer->name);

	/* If ppop is waiting (ppop wstop), inform it that printer has stopt. */
	if(printer->spool_state.status == PRNSTATUS_STOPT && printer->ppop_pid)
		{
		kill(printer->ppop_pid, SIGUSR1);
		printer->ppop_pid = (pid_t)0;
		}

	/*
	** Inform queue display programs of the new printer status.
	** Act differenly according to the new status.
	**
	** Notice that we must do this after the new status has gone into
	** effect, otherwise, the retry count and retry interval will
	** not yet be correct.
	*/
	switch(printer->spool_state.status)
		{
		case PRNSTATUS_PRINTING:
			state_update("PST %s printing %s %d",
				printer,
				jobid(destid_to_name(printer->job_destid), printer->job_id, printer->job_subid),
				printer->spool_state.next_error_retry
				);
			break;
		case PRNSTATUS_IDLE:
			state_update("PST %s idle", printer);
			break;
		case PRNSTATUS_CANCELING:
			state_update("PST %s canceling %s",
				printer,
				jobid(destid_to_name(printer->job_destid), printer->job_id, printer->job_subid)
				);
			break;
		case PRNSTATUS_SEIZING:
			state_update("PST %s seizing %s",
				printer,
				jobid(destid_to_name(printer->job_destid), printer->job_id, printer->job_subid)
				);
			break;
		case PRNSTATUS_FAULT:
			state_update("PST %s fault %d %d",
				printer,
				printer->spool_state.next_error_retry,
				printer->spool_state.countdown
				);
			break;
		case PRNSTATUS_ENGAGED:
			state_update("PST %s engaged %d %d",
				printer,
				printer->spool_state.next_engaged_retry,
				printer->spool_state.countdown
				);
			break;
		case PRNSTATUS_STARVED:
			state_update("PST %s starved", printer);
			break;
		case PRNSTATUS_STOPT:
			state_update("PST %s stopt", printer);
			break;
		case PRNSTATUS_STOPPING:
			state_update("PST %s stopping (printing %s)",
				printer,
				jobid(destid_to_name(printer->job_destid), printer->job_id, printer->job_subid)
				);
			break;
		case PRNSTATUS_HALTING:
			state_update("PST %s halting (printing %s)",
				printer,
				jobid(destid_to_name(printer->job_destid), printer->job_id, printer->job_subid)
				);
			break;
		}

	unlock();
	} /* end of printer_new_status() */

/*
** This function is called every TICK_INTERVAL seconds to allow us to retry
** printer operations.
*/
void printer_tick(void)
	{
	int x;
	static int hungry_countdown = 0;	/* seconds till next mercy for starving printers */
	static int hungry_x = 0;			/* where we left off in search for starving printers */

	DODEBUG_TICK(("printer_tick(): active_printers=%d, starving_printers=%d", active_printers, starving_printers));

	/*
	** If a printer is in fault retry mode or was printing for another
	** computer and the retry time has expired then set its state
	** to idle and look for a job to start on it.  (Perhaps it would
	** help to explain that printers[x].next_error_retry will be zero if the
	** printer is in fault-no-retry mode.)
	*/
	for(x=0; x < printer_count; x++)
		{
		if((printers[x].spool_state.status==PRNSTATUS_FAULT && printers[x].spool_state.next_error_retry )
				|| printers[x].spool_state.status==PRNSTATUS_ENGAGED)
			{						/* if faulted and retry allowed, */
			if((printers[x].spool_state.countdown -= TICK_INTERVAL) <= 0)
				{
				printer_new_status(&printers[x], PRNSTATUS_IDLE);
				printer_look_for_work(x);
				}
			}
		}

	/*
	** If we have starving printers and the STARVING_RETRY_INTERVAL has
	** expired, then try to restart some starving printers.
	*/
	if((hungry_countdown -= TICK_INTERVAL) <= 0)
		{
		hungry_countdown = STARVING_RETRY_INTERVAL;

		x = 0;
		while( starving_printers && (active_printers < MAX_ACTIVE) && (x++ < printer_count) )
			{
			if(hungry_x >= printer_count)		/* wrap around if necessary */
				hungry_x = 0;

			if(printers[hungry_x].spool_state.status == PRNSTATUS_STARVED)
				{
				printer_new_status(&printers[hungry_x], PRNSTATUS_IDLE);
				starving_printers--;			/* now considered fed */
				printer_look_for_work(hungry_x);
				}

			hungry_x++;							/* move on to next printer */
			}
		}

	} /* end of printer_tick() */

/* end of file */

