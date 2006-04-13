/*
** mouse:~ppr/src/pprd/pprd_ipp.c
** Copyright 1995--2006, Trinity College Computing Center.
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
** Last modified 13 April 2006.
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

			/* !!! Access checking should go here !!! */
			
			if((prnid = queue[i].status) >= 0)	/* if pprdrv is active */
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

			/* If a hold is in progress, do what we woudld do if the job were being
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
		return;
		}

	operation_id = atoi(p);
	p += strspn(p, "0123456789");
	p += strspn(p, " ");

	switch(operation_id)
		{
		case IPP_CANCEL_JOB:
			result_code = ipp_cancel_job(p);
			break;
		default:
			error("unsupported operation: 0x%.2x (%s)", operation_id, ipp_operation_id_to_str(operation_id));
			result_code = IPP_OPERATION_NOT_SUPPORTED;
			break;
		}

	DODEBUG_IPP(("%s(): done", function));
	return result_code;
	} /* end of ipp_dispatch() */

/* end of file */
