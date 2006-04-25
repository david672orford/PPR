/*
** mouse:~ppr/src/pprd/pprd_ipp.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 25 April 2006.
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

/** cancel specified job or jobs
 * This is called by ipp_cancel_job() and ipp_purge_jobs().
 */
static int ipp_cancel_job_core(int destid, int jobid)
	{
	const char function[] = "ipp_cancel_job_core";
	int i;
	int prnid;

	lock();

	/* Loop over the queue entries. */
	for(i=0; i < queue_entries; i++)
		{
		/* Skip non-matching entries. */
		if(jobid != WILDCARD_JOBID && queue[i].id != jobid)
			continue;
		if(destid != QUEUEID_WILDCARD && queue[i].destid != destid)
			continue;

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

	return IPP_OK;
	} /* ipp_cancel_core() */

/** Handler for IPP_CANCEL_JOB
 */
static int ipp_cancel_job(const char command_args[])
	{
	int jobid;
	jobid = atoi(command_args);
	return ipp_cancel_job_core(QUEUEID_WILDCARD, jobid);
	}

/* Handler for IPP_HOLD_JOB
*/
static int ipp_hold_job(const char command_args[])
	{
	const char *function = "ipp_hold_job";
	int job_id;
	int x;
	int retcode = IPP_OK;

	job_id = atoi(command_args);

	lock();										/* lock the queue */

	for(x=0; x < queue_entries; x++)			/* and search it */
		{
		if(queue[x].id != job_id)
			continue;

		switch(queue[x].status)
			{
			case STATUS_WAITING:		/* if not printing, */
			case STATUS_WAITING4MEDIA:	/* just quitely go to `hold' */
				queue_p_job_new_status(&queue[x], STATUS_HELD);
				break;
			case STATUS_HELD:			/* if already held, nothing to do */
				break;
			case STATUS_ARRESTED:		/* can't hold an arrested job */
				retcode = IPP_NOT_POSSIBLE;
				break;
			case STATUS_SEIZING:		/* already going to held */
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
					retcode = IPP_INTERNAL_ERROR;
					}
				break;
			}

		break;	/* we found it */
		}

	unlock();

	return retcode;
	} /* end of ipp_hold_job() */

/* Handler for IPP_RELEASE_JOB
*/
static int ipp_release_job(const char command_args[])
	{
	const char function[] = "ipp_release_job";
	int job_id;
	int x;
	int retcode = IPP_OK;

	job_id = atoi(command_args);

	DODEBUG_IPP(("%s(): job_id=%d", function, job_id));
	
	lock();										/* lock the queue */

	for(x=0; x < queue_entries; x++)			/* and search it */
		{
		if(queue[x].id != job_id)
			continue;

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
				retcode = IPP_NOT_POSSIBLE;
				break;
			case STATUS_SEIZING:			/* in transition to held */
				DODEBUG_IPP(("%s(): job is already in transition to held", function));
				/* This should be fixed */
				retcode = IPP_NOT_POSSIBLE;
				break;
			case STATUS_WAITING:			/* not held */
			case STATUS_WAITING4MEDIA:
			default:						/* printing */
				DODEBUG_IPP(("%s(): job is not held", function));
				break;
			}

		break;
		}

	#ifdef DEBUG_IPP
	if(x == queue_entries)
		debug("%s(): job not found", function);
	#endif

	unlock();

	return retcode;
	} /* end of ipp_release_job() */

/** Handler for IPP_PAUSE_PRINTER
 *
 * RFC 2911 leaves it up to the implementor to decide whether this function
 * stops the printer immediately or after the completion of the current job.
 */
static int ipp_pause_printer(const char command_args[])
	{
	int prnid;
	int retcode = IPP_OK;

	if((prnid = destid_by_printer(command_args)) == -1)
		return IPP_NOT_FOUND;

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
			retcode = IPP_INTERNAL_ERROR;
			break;
		}

	return retcode;
	} /* ipp_pause_printer() */

/** Handler for IPP_RESUME_PRINTER
 */
static int ipp_resume_printer(const char command_args[])
	{
	int prnid;
	int retcode = IPP_OK;

	if((prnid = destid_by_printer(command_args)) == -1)
		return IPP_NOT_FOUND;

	switch(printers[prnid].spool_state.status)
		{
		case PRNSTATUS_IDLE:
		case PRNSTATUS_CANCELING:
		case PRNSTATUS_SEIZING:
		case PRNSTATUS_ENGAGED:
		case PRNSTATUS_STARVED:
			break;
		case PRNSTATUS_PRINTING:
			break;
		case PRNSTATUS_FAULT:
		case PRNSTATUS_STOPT:
			printer_new_status(&printers[prnid], PRNSTATUS_IDLE);
			media_startstop_update_waitreason(prnid);
			printer_look_for_work(prnid);
			break;
		case PRNSTATUS_HALTING:
			retcode = IPP_NOT_POSSIBLE;
			break;
		case PRNSTATUS_STOPPING:
			printer_new_status(&printers[prnid], PRNSTATUS_PRINTING);
			media_startstop_update_waitreason(prnid);
			break;
		default:
			error(X_("printer \"%s\" is in undefined state %d"), command_args, printers[prnid].spool_state.status);
			retcode = IPP_INTERNAL_ERROR;
			break;
		}

	return retcode;
	} /* ipp_resume_printer() */

static int ipp_purge_jobs(const char command_args[])
	{
	int destid;
	if((destid = destid_by_name(command_args)) == -1)
		return IPP_NOT_FOUND;
	return ipp_cancel_job_core(destid, WILDCARD_JOBID);
	} /* ipp_purge_jobs() */

int ipp_dispatch(const char command[])
	{
	FUNCTION4DEBUG("ipp_dispatch")
	const char *p;
	int operation_id;
	int result_code;

	DODEBUG_IPP(("%s(): %s", function, command));
	
	if(!(p = lmatchsp(command, "IPP")))
		{
		error("%s(): command missing", function);
		return IPP_INTERNAL_ERROR;
		}

	operation_id = atoi(p);
	p += strspn(p, "0123456789");
	p += strspn(p, " ");

	switch(operation_id)
		{
		case IPP_CANCEL_JOB:
			result_code = ipp_cancel_job(p);
			break;
		case IPP_HOLD_JOB:
			result_code = ipp_hold_job(p);
			break;
		case IPP_RELEASE_JOB:
			result_code = ipp_release_job(p);
			break;
		case IPP_PAUSE_PRINTER:
			result_code = ipp_pause_printer(p);
			break;
		case IPP_RESUME_PRINTER:
			result_code = ipp_resume_printer(p);
			break;
		case IPP_PURGE_JOBS:
			result_code = ipp_purge_jobs(p);
			break;
		default:
			error("unsupported operation: 0x%.2x (%s)", operation_id, ipp_operation_id_to_str(operation_id));
			result_code = IPP_OPERATION_NOT_SUPPORTED;
			break;
		}

	DODEBUG_IPP(("%s(): done, result = 0x%04x", function, result_code));
	return result_code;
	} /* end of ipp_dispatch() */

/* end of file */
