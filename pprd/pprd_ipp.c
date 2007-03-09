/*
** mouse:~ppr/src/pprd/pprd_ipp.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 15 November 2006.
*/

/*
 * In this module we handle Internet Printing Protocol requests which have
 * been received by ppr-httpd and passed on to us by the ipp CGI program.
 */

#include "config.h"
#include <string.h>
#include <ctype.h>
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "ipp_constants.h"
#include "pprd.h"
#include "pprd.auto_h"
#include "respond.h"

/** Constructor for struct PPRD_CALL_RETVAL values. */
static struct PPRD_CALL_RETVAL new_retval(int status_code, int extra_code)
	{
	struct PPRD_CALL_RETVAL retval;
	retval.status_code = status_code;
	retval.extra_code = extra_code;
	return retval;
	}

/** cancel specified job or jobs
 * This is called by ipp_cancel_job() and ipp_purge_jobs().
 */
static struct PPRD_CALL_RETVAL ipp_cancel_job_core(int destid, int jobid)
	{
	const char function[] = "ipp_cancel_job_core";
	int i;
	int prnid;
	int status_code = IPP_NOT_FOUND;	/* yet */

	lock();

	/* Loop over the queue entries. */
	for(i=0; i < queue_entries; i++)
		{
		/* Skip non-matching entries. */
		if(jobid != WILDCARD_JOBID && queue[i].id != jobid)
			continue;
		if(destid != QUEUEID_WILDCARD && queue[i].destid != destid)
			continue;

		status_code = IPP_OK;

		/* If pprdrv is working on this job, */
		if((prnid = queue[i].status) >= 0)
			{
			/* If it is printing we can say it is now canceling, but
			   if it is halting or stopping we don't want to mess with
			   that.
			   */
			if(printers[prnid].spool_state.status == PRNSTATUS_PRINTING)
				printer_new_status(&printers[prnid], PRNSTATUS_CANCELING);

			/* Set flag so that job will be deleted when pprdrv dies. */
			printers[prnid].cancel_job = TRUE;

			/* Change the job status to "being canceled". */
			queue_p_job_new_status(&queue[i], STATUS_CANCEL);

			/* Kill pprdrv. */
			pprdrv_kill(prnid);
			}

		/* If a cancel is in progress, */
		else if(prnid == STATUS_CANCEL)
			{
			/* nothing to do */
			}

		/* If a hold is in progress, do what we would do if the job were being
		   printed, but without the need to kill() pprdrv again.  This
		   is tough because the queue doesn't contain the printer
		   id anymore. */
		else if(prnid == STATUS_SEIZING)
			{
			for(prnid = 0; prnid < printer_count; prnid++)
				{
				if(printers[prnid].job_destid == queue[i].destid
						&& printers[prnid].job_id == queue[i].id
						&& printers[prnid].job_subid == queue[i].subid
						)
					{
					if(printers[prnid].spool_state.status == PRNSTATUS_SEIZING)
						printer_new_status(&printers[prnid], PRNSTATUS_CANCELING);
					printers[prnid].hold_job = FALSE;
					printers[prnid].cancel_job = TRUE;
					queue_p_job_new_status(&queue[i], STATUS_CANCEL);
					break;
					}
				}
			if(prnid == printer_count)
				error("%s(): couldn't find printer that job is printing on", function);
			}

		/* If the job is not being printed, we can delete it right now. */
		else
			{
			/*
			** If job status is not arrested,
			** use the responder to inform the user that we are canceling it.
			*/
			if(queue[i].status != STATUS_ARRESTED)
				{
				respond(queue[i].destid, queue[i].id, queue[i].subid,
						-1,	  /* impossible printer */
						RESP_CANCELED);
				}

			/* Remove the job from the queue array and its files form the spool directories. */
			queue_dequeue_job(queue[i].destid, queue[i].id, queue[i].subid);

			i--;		/* compensate for deletion */
			}
		}
	
	unlock();

	return new_retval(status_code, 0);
	} /* ipp_cancel_core() */

/** Handler for IPP_CANCEL_JOB
 */
static struct PPRD_CALL_RETVAL ipp_cancel_job(const char command_args[])
	{
	int jobid;
	jobid = atoi(command_args);
	return ipp_cancel_job_core(QUEUEID_WILDCARD, jobid);
	}

/* Handler for IPP_HOLD_JOB
*/
static struct PPRD_CALL_RETVAL ipp_hold_job(const char command_args[])
	{
	const char *function = "ipp_hold_job";
	int job_id;
	int x;
	struct PPRD_CALL_RETVAL retval = {IPP_NOT_FOUND, 0};	/* yet */

	job_id = atoi(command_args);

	lock();										/* lock the queue */

	for(x=0; x < queue_entries; x++)			/* and search it */
		{
		if(queue[x].id != job_id)
			continue;

		retval.status_code = IPP_OK;
		
		switch(queue[x].status)
			{
			case STATUS_WAITING:		/* if not printing, */
			case STATUS_WAITING4MEDIA:	/* just quitely go to `hold' */
				queue_p_job_new_status(&queue[x], STATUS_HELD);
				break;
			case STATUS_HELD:			/* if already held, nothing to do */
				retval.extra_code = 1;
				break;
			case STATUS_ARRESTED:		/* can't hold an arrested job */
				retval.status_code = IPP_NOT_POSSIBLE;
				break;
			case STATUS_SEIZING:		/* already going to held */
				retval.extra_code = 1;
				break;
			case STATUS_CANCEL:			/* if being canceled, hijack the operation */
				printer_new_status(&printers[queue[x].status], PRNSTATUS_SEIZING);
				printers[queue[x].status].cancel_job = FALSE;
				printers[queue[x].status].hold_job = TRUE;
				queue_p_job_new_status(&queue[x], STATUS_SEIZING);
				break;
			default:						/* printing? */
				if(queue[x].status >= 0)
					{
					int prnid = queue[x].status;

					queue_p_job_new_status(&queue[x], STATUS_SEIZING);
					printer_new_status(&printers[prnid], PRNSTATUS_SEIZING);
					printers[prnid].hold_job = TRUE;

					DODEBUG_PPOPINT(("killing pprdrv (printer=%s, pid=%ld)", destid_to_name(prnid), (long)printers[prnid].job_pid));
					if(printers[prnid].job_pid <= 0)
						error("%s(): assertion failed, printers[%d].pid = %ld", function, prnid, (long)printers[prnid].job_pid);
					else
						pprdrv_kill(prnid);
					}
				else
					{
					retval.status_code = IPP_INTERNAL_ERROR;
					}
				break;
			}

		break;	/* we found it */
		}

	unlock();

	return retval;
	} /* end of ipp_hold_job() */

/* Handler for IPP_RELEASE_JOB
*/
static struct PPRD_CALL_RETVAL ipp_release_job(const char command_args[])
	{
	FUNCTION4DEBUG("ipp_release_job")
	int job_id;
	int x;
	struct PPRD_CALL_RETVAL retval = {IPP_NOT_FOUND, 0};	/* yet */

	job_id = atoi(command_args);

	DODEBUG_IPP(("%s(): job_id=%d", function, job_id));
	
	lock();										/* lock the queue */

	for(x=0; x < queue_entries; x++)			/* and search it */
		{
		if(queue[x].id != job_id)
			continue;

		retval.status_code = IPP_OK;

		switch(queue[x].status)
			{
			case STATUS_HELD:
				DODEBUG_IPP(("%s(): job is held", function));
				queue_p_job_new_status(&queue[x], STATUS_WAITING);
				media_set_notnow_for_job(&queue[x], TRUE);
				if(queue[x].status == STATUS_WAITING)
					printer_try_start_suitable_4_this_job(&queue[x]);
				break;
			case STATUS_ARRESTED:
				DODEBUG_IPP(("%s(): job is arrested", function));
				retval.status_code = IPP_NOT_POSSIBLE;
				break;
			case STATUS_SEIZING:			/* in transition to held */
				DODEBUG_IPP(("%s(): job is already in transition to held", function));
				/* This should be fixed */
				retval.status_code = IPP_NOT_POSSIBLE;
				break;
			case STATUS_WAITING:			/* not held */
			case STATUS_WAITING4MEDIA:
			default:						/* printing */
				DODEBUG_IPP(("%s(): job is not held", function));
				retval.extra_code = 1;
				break;
			}

		break;
		}

	#ifdef DEBUG_IPP
	if(x == queue_entries)
		debug("%s(): job not found", function);
	#endif

	unlock();

	return retval;
	} /* end of ipp_release_job() */

/** Handler for IPP_PAUSE_PRINTER
 *
 * RFC 2911 leaves it up to the implementor to decide whether this function
 * stops the printer immediately or after the completion of the current job.
 */
static struct PPRD_CALL_RETVAL ipp_pause_printer(const char command_args[])
	{
	const char *p;
	struct PPRD_CALL_RETVAL retval = {IPP_OK, 0};

	if((p = lmatchp(command_args, "group")))
		{
		int destid, gindex;
		if((destid = destid_by_group(p)) == -1)
			return new_retval(IPP_NOT_FOUND, 0);
		gindex = destid_to_gindex(destid);
		groups[gindex].spool_state.held = TRUE;
		groups[gindex].spool_state.printer_state_change_time = time(NULL);
		group_spool_state_save(&(groups[gindex].spool_state), groups[gindex].name);
		}
	else if((p = lmatchp(command_args, "printer")))
		{
		int prnid;
		if((prnid = destid_by_printer(p)) == -1)
			return new_retval(IPP_NOT_FOUND, 0);

		switch(printers[prnid].spool_state.status)
			{
			case PRNSTATUS_FAULT:	/* If not printing now, */
			case PRNSTATUS_IDLE:	/* we may go directly to stopt. */
			case PRNSTATUS_ENGAGED:
			case PRNSTATUS_STARVED:
				printer_new_status(&printers[prnid], PRNSTATUS_STOPT);
				media_startstop_update_waitreason(prnid);
				break;
			case PRNSTATUS_HALTING:
			case PRNSTATUS_STOPT:
			case PRNSTATUS_STOPPING:
				retval.extra_code = 1;
				break;
			case PRNSTATUS_CANCELING:			/* if pprdrv already sent kill signal */
			case PRNSTATUS_SEIZING:
				printer_new_status(&printers[prnid], PRNSTATUS_HALTING);
				media_startstop_update_waitreason(prnid);
				break;
			case PRNSTATUS_PRINTING:
				#if 1		/* immediate halt */
				printer_new_status(&printers[prnid], PRNSTATUS_HALTING);
				pprdrv_kill(prnid);
				#else		/* stop after end of current job */
				printer_new_status(&printers[prnid], PRNSTATUS_STOPPING);
				#endif
				media_startstop_update_waitreason(prnid);
				break;
			default:
				error(X_("printer \"%s\" is in undefined state %d"), command_args, printers[prnid].spool_state.status);
				retval.status_code = IPP_INTERNAL_ERROR;
				break;
			}
		}
	else	/* error in ippd */
		retval.status_code = IPP_INTERNAL_ERROR;

	return retval;
	} /* ipp_pause_printer() */

/** Handler for IPP_RESUME_PRINTER
 */
static struct PPRD_CALL_RETVAL ipp_resume_printer(const char command_args[])
	{
	const char *p;
	struct PPRD_CALL_RETVAL retval = {IPP_OK, 0};

	if((p = lmatchp(command_args, "group")))
		{
		int destid, gindex;
		if((destid = destid_by_group(p)) == -1)
			return new_retval(IPP_NOT_FOUND, 0);
		gindex = destid_to_gindex(destid);

		groups[gindex].spool_state.held = FALSE;
		groups[gindex].spool_state.printer_state_change_time = time(NULL);
		group_spool_state_save(&(groups[gindex].spool_state), groups[gindex].name);

		group_look_for_work(gindex);
		}
	else if((p = lmatchp(command_args, "printer")))
		{
		int prnid;
		if((prnid = destid_by_printer(p)) == -1)
			return new_retval(IPP_NOT_FOUND, 0);
	
		switch(printers[prnid].spool_state.status)
			{
			case PRNSTATUS_IDLE:
			case PRNSTATUS_CANCELING:
			case PRNSTATUS_SEIZING:
			case PRNSTATUS_ENGAGED:
			case PRNSTATUS_STARVED:
			case PRNSTATUS_PRINTING:
				retval.extra_code = 1;
				break;
			case PRNSTATUS_FAULT:
			case PRNSTATUS_STOPT:
				printer_new_status(&printers[prnid], PRNSTATUS_IDLE);
				media_startstop_update_waitreason(prnid);
				printer_look_for_work(prnid);
				break;
			case PRNSTATUS_HALTING:
				retval.status_code = IPP_NOT_POSSIBLE;
				break;
			case PRNSTATUS_STOPPING:
				printer_new_status(&printers[prnid], PRNSTATUS_PRINTING);
				media_startstop_update_waitreason(prnid);
				break;
			default:
				error(X_("printer \"%s\" is in undefined state %d"), command_args, printers[prnid].spool_state.status);
				retval.status_code = IPP_INTERNAL_ERROR;
				break;
			}
		}
	else		/* error in ippd */
		retval.status_code = IPP_INTERNAL_ERROR;

	return retval;
	} /* ipp_resume_printer() */

static struct PPRD_CALL_RETVAL ipp_purge_jobs(const char command_args[])
	{
	const char *p;
	int destid;

	if((p = lmatchp(command_args, "group")))
		{
		if((destid = destid_by_group(command_args)) == -1)
			return new_retval(IPP_NOT_FOUND, 0);
		}
	else if((p = lmatchp(command_args, "printer")))
		{
		if((destid = destid_by_printer(command_args)) == -1)
			return new_retval(IPP_NOT_FOUND, 0);
		}
	else		/* error in ippd */
		{
		return new_retval(IPP_INTERNAL_ERROR, 0);
		}

	return ipp_cancel_job_core(destid, WILDCARD_JOBID);
	} /* ipp_purge_jobs() */

static struct PPRD_CALL_RETVAL cups_accept_or_reject_jobs(const char command_args[], gu_boolean accepting)
	{
	const char *p;
	int destid;
	if((p = lmatchp(command_args, "group")))
		{
		int gindex;
		if((destid = destid_by_group(p)) == -1)
			return new_retval(IPP_NOT_FOUND, 0);
		gindex = destid_to_gindex(destid);
		groups[gindex].spool_state.accepting = accepting;
		groups[gindex].spool_state.printer_state_change_time = time(NULL);
		group_spool_state_save(&(groups[gindex].spool_state), groups[gindex].name);
		return new_retval(IPP_OK, 0);
		}
	if((p = lmatchp(command_args, "printer")))
		{
		if((destid = destid_by_group(p)) == -1)
			return new_retval(IPP_NOT_FOUND, 0);
		printers[destid].spool_state.accepting = accepting;
		printers[destid].spool_state.printer_state_change_time = time(NULL);
		printer_spool_state_save(&(printers[destid].spool_state), printers[destid].name);
		return new_retval(IPP_OK, 0);
		}
	return new_retval(IPP_BAD_REQUEST, 0);
	} /* ipp_pause_printer() */

/* CUPS_MOVE_JOB */
struct PPRD_CALL_RETVAL cups_move_job(const char command_args[])
	{
	const char *p;
	int job_id;
	const char *new_destname;
	int new_destid;
	int x, rank2;
	struct QEntry *q;
	char oldname[MAX_PPR_PATH];
	char newname[MAX_PPR_PATH];
	int moved = 0;

	p = command_args;
	job_id = atoi(p);
	p += strspn(p, "0123456789");
	p += strspn(p, " ");

	if((new_destname = lmatchp(p, "group")))
		{
		if((new_destid = destid_by_group(new_destname)) == -1)
			return new_retval(IPP_NOT_FOUND, 0);
		}
	else if((new_destname = lmatchp(p, "printer")))
		{
		if((new_destid = destid_by_printer(new_destname)) == -1)
			return new_retval(IPP_NOT_FOUND, 0);
		}
	else
		return new_retval(IPP_INTERNAL_ERROR, 0);

	lock();								/* lock the queue array */

	for(rank2=x=0; x < queue_entries; x++)
		{
		if(queue[x].id != job_id)
			{
			if(queue[x].destid == new_destid)
				rank2++;
			continue;
			}

		/* for easier reference */
		q = &queue[x];
		
		/* We can't move printing jobs. */
		if(q->status >= 0)
			return new_retval(IPP_NOT_POSSIBLE, 0);

		/*
		** If this is a real move an not just an attempt to reset the
		** "never" flags,
		*/
		if(q->destid != new_destid)
			{
			/* Inform queue monitoring programs of the move. */
			state_update("MOV %s %s %d",
					jobid(destid_to_name(q->destid), q->id, q->subid),
					destid_to_name(new_destid),
					rank2++);

			/* Rename the queue file. */
			ppr_fnamef(oldname,"%s/%s-%d.%d", QUEUEDIR,
				destid_to_name(q->destid),q->id,q->subid);
			ppr_fnamef(newname,"%s/%s-%d.%d", QUEUEDIR,
				destid_to_name(new_destid),q->id,q->subid);
			rename(oldname, newname);

			/* Rename all of the data files. */
			{
			char *list[] = {"comments", "pages", "text", "log", "infile", "barbar", NULL};
			int x;
			for(x=0; list[x]; x++)
				{
				ppr_fnamef(oldname,"%s/%s-%d.%d-%s", DATADIR,
					destid_to_name(q->destid),
					q->id, q->subid,
					list[x]
					);
				ppr_fnamef(newname,"%s/%s-%d.%d-%s", DATADIR,
					destid_to_name(new_destid),
					q->id, q->subid,
					list[x]
					);
				rename(oldname, newname);
				}
			}

			}

		/*
		** In the job's log file, make a note of the fact
		** that it was moved from one destination to another.
		*/
		{
		FILE *logfile;
		ppr_fnamef(newname,"%s/%s-%d.%d-log", DATADIR,
			destid_to_name(new_destid),
			q->id, q->subid
			);
		if((logfile = fopen(newname, "a")))
			{
			fprintf(logfile,
				"Job moved from destination \"%s\" to \"%s\".\n",
				destid_to_name(q->destid), destid_to_name(new_destid));
			fclose(logfile);
			}
		}

		/*
		** Change the destination id in the queue array.  This must come
		** after the rename code or the rename code will break.
		*/
		q->destid = new_destid;

		/* If this job was stranded, maybe it will print here. */
		if(q->status == STATUS_STRANDED)
			queue_p_job_new_status(q, STATUS_WAITING);

		/*
		** Clear any "never" (printer unsuitable) flags, set new "notnow" 
		** (required media not present) flags, and update the job status.
		*/
		q->never = 0;

		/* Reset pass number just like in queue_insert(). */
		if(destid_is_group(q->destid))
			q->pass = 1;
		else
			q->pass = 0;

		/* Set the "notnow" bits and the printer status according to the mounted media */
		media_set_notnow_for_job(q, TRUE);

		/* If the job is ready to print, try to start it on a printer. */				
		if(q->status == STATUS_WAITING)
			printer_try_start_suitable_4_this_job(q);

		/* We found it. */
		moved++;
		break;
		} /* end of loop over queue array */

	unlock();					/* we are done modifying the queue array */

	return new_retval(moved ? IPP_OK : IPP_NOT_FOUND, 0);
	} /* cups_move_job() */

/* This is called for each command received over the socket.
 * 
 * These commands look like this:
 *   IPP <op> <arguments>
 * Where <op> is the IPP operation ID in base 10 and <arguments>
 * are the arguments to the operation.
 */
struct PPRD_CALL_RETVAL ipp_dispatch(const char command[])
	{
	const char function[] = "ipp_dispatch";
	const char *p;
	int operation_id;
	struct PPRD_CALL_RETVAL result;

	DODEBUG_IPP(("%s(): %s", function, command));
	
	if(!(p = lmatchsp(command, "IPP")))
		{
		error("%s(): command missing", function);
		return new_retval(IPP_INTERNAL_ERROR, 0);
		}

	operation_id = atoi(p);
	p += strspn(p, "0123456789");
	p += strspn(p, " ");

	switch(operation_id)
		{
		case IPP_CANCEL_JOB:
			result = ipp_cancel_job(p);
			break;
		case IPP_HOLD_JOB:
			result = ipp_hold_job(p);
			break;
		case IPP_RELEASE_JOB:
			result = ipp_release_job(p);
			break;
		case IPP_PAUSE_PRINTER:
			result = ipp_pause_printer(p);
			break;
		case IPP_RESUME_PRINTER:
			result = ipp_resume_printer(p);
			break;
		case IPP_PURGE_JOBS:
			result = ipp_purge_jobs(p);
			break;
		case CUPS_ACCEPT_JOBS:
			result = cups_accept_or_reject_jobs(p, TRUE);
			break;
		case CUPS_REJECT_JOBS:
			result = cups_accept_or_reject_jobs(p, FALSE);
			break;
		case CUPS_MOVE_JOB:
			result = cups_move_job(p);
			break;
		default:
			error("unsupported operation: 0x%.2x (%s)", operation_id, ipp_operation_id_to_str(operation_id));
			result.status_code = IPP_OPERATION_NOT_SUPPORTED;
			result.extra_code = 0;
			break;
		}

	DODEBUG_IPP(("%s(): done, result = {0x%04x, %d}", function, result.status_code, result.extra_code));
	return result;
	} /* end of ipp_dispatch() */

/* end of file */
