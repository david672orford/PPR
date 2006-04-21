/*
** mouse:~ppr/src/pprd/pprd_ipp.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 21 April 2006.
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

/** Handler for IPP_CANCEL_JOB
 */
static int ipp_cancel_job(const char command_args[])
	{
	const char function[] = "ipp_cancel_job";
	int jobid;
	int i;

	jobid = atoi(command_args);
	
	lock();

	/* Loop over the queue entries.
	 * This code is copied from pprd_ppop.c.  We haven't tried to generalize 
	 * it because we plan to remove the command from pprd_ppop.c and have
	 * ppop use the IPP command.
	 */ 
	for(i=0; i < queue_entries; i++)
		{
		if(queue[i].id == jobid)
			{
			int prnid;
			
			/* If pprdrv is active, */
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
		}
	
	unlock();

	return IPP_OK;
	} /* ipp_cancel_job() */

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
			case STATUS_HELD:
				queue_p_job_new_status(&queue[x], STATUS_WAITING);
				media_set_notnow_for_job(&queue[x], TRUE);
				if(queue[x].status == STATUS_WAITING)
					printer_try_start_suitable_4_this_job(&queue[x]);
				break;
			case STATUS_ARRESTED:
				retcode = IPP_NOT_POSSIBLE;
				break;
			case STATUS_SEIZING:			/* in transition to held */
				/* This should be fixed */
				retcode = IPP_NOT_POSSIBLE;
				break;
			case STATUS_WAITING:			/* not held */
			case STATUS_WAITING4MEDIA:
			default:						/* printing */
				break;
			}

		break;
		}

	unlock();

	return retcode;
	} /* end of ipp_release_job() */

int ipp_dispatch(const char command[])
	{
	const char function[] = "ipp_dispatch";
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
		default:
			error("unsupported operation: 0x%.2x (%s)", operation_id, ipp_operation_id_to_str(operation_id));
			result_code = IPP_OPERATION_NOT_SUPPORTED;
			break;
		}

	DODEBUG_IPP(("%s(): done, result = 0x%04x", function, result_code));
	return result_code;
	} /* end of ipp_dispatch() */

/* end of file */
