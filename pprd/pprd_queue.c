/*
** mouse:~ppr/src/pprd/pprd_queue.c
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
** Last modified 7 April 2006.
*/

/*
** This module contains routines for maintaining the print queue array,
** loading queue files into the array, and changing the status of jobs.
*/

#include "config.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "pprd.h"
#include "./pprd.auto_h"
#include "respond.h"

/*===========================================================================
** Remove a job's files.  We do this either when the job is completed
** or when we fail to insert it into a queue.
===========================================================================*/
static void delete_job_files(const char *queuename, int id, int subid)
	{
	char filename[MAX_PPR_PATH];

	ppr_fnamef(filename, "%s/%s-%d.%d", QUEUEDIR, queuename, id, subid);
	unlink(filename);

	ppr_fnamef(filename, "%s/%s-%d.%d-comments", DATADIR, queuename, id, subid);
	unlink(filename);

	ppr_fnamef(filename, "%s/%s-%d.%d-pages", DATADIR, queuename, id, subid);
	unlink(filename);

	ppr_fnamef(filename, "%s/%s-%d.%d-text", DATADIR, queuename, id, subid);
	unlink(filename);

	ppr_fnamef(filename, "%s/%s-%d.%d-log", DATADIR, queuename, id, subid);
	unlink(filename);

	ppr_fnamef(filename, "%s/%s-%d.%d-infile", DATADIR, queuename, id, subid);
	unlink(filename);

	ppr_fnamef(filename, "%s/%s-%d.%d-barbar", DATADIR, queuename, id, subid);
	unlink(filename);
	} /* end of delete_job_files() */

/*===========================================================================
** Unlink a job and remove its entry from the queue array.
===========================================================================*/
static void job_count_adjust(int destid, int increment, gu_boolean flush)
	{
	if(destid_is_group(destid))
		{
		int gindex = destid_to_gindex(destid);
		groups[gindex].spool_state.job_count += increment;
		if(flush)
			group_spool_state_save(&(groups[gindex].spool_state), groups[gindex].name);
		}
	else
		{
		printers[destid].spool_state.job_count += increment;
		if(flush)
			printer_spool_state_save(&(printers[destid].spool_state), printers[destid].name);
		}
	}

/*===========================================================================
** Unlink a job and remove its entry from the queue array.
===========================================================================*/
void queue_dequeue_job(int destid, int id, int subid)
	{
	FUNCTION4DEBUG("queue_dequeue_job")
	int x;
	const char *destname = destid_to_name(destid);
	const char *full_job_id = jobid(destname, id, subid);

	DODEBUG_DEQUEUE(("%s(destid=%d, id=%d, subid=%d)", function, destid, id, subid));

	/* Inform queue display programs of job deletion. */
	state_update("DEL %s", full_job_id);

	lock();				/* lock the queue array while we modify it */

	for(x=0; x < queue_entries; x++)
		{
		if(queue[x].destid == destid && queue[x].id==id && queue[x].subid==subid)
			{
			DODEBUG_DEQUEUE(("removing job %s at position %d from queue", full_job_id, x));

			/* Remove the actual job files. */
			delete_job_files(destname, id, subid);

			if((queue_entries-x) > 1)			/* do a move if not last entry */
				{
				memmove(&queue[x], &queue[x+1],
					sizeof(struct QEntry) * (queue_entries-x-1) );
				}

			queue_entries--;					/* one less in queue */
			job_count_adjust(destid, -1, TRUE);
			break;								/* and we needn't look farther */
			}
		}

	unlock();			/* unlock queue array */
	} /* end of queue_dequeue_job() */

/*=========================================================================
** Update the "PPRD: XX XXXX\n" line at the start of the queue file.
=========================================================================*/
void queue_write_status_and_flags(struct QEntry *job)
	{
	const char function[] = "queue_write_status_and_flags";
	char filename[MAX_PPR_PATH];
	int fd;
	char buf[65];
	size_t written_size;

	ppr_fnamef(filename, "%s/%s-%d.%d", QUEUEDIR, destid_to_name(job->destid), job->id, job->subid);
	if((fd = open(filename, O_RDWR)) == -1)
		{
		error("%s(): can't open \"%s\", errno=%d (%s)", function, filename, errno, strerror(errno));
		return;
		}

	/* Write the new line.  If the job is printing, substitute 0 for the actual printer index. */
	if(snprintf(buf, sizeof(buf), "PPRD: %02X %08X %02X %04X                                      \n",
			job->priority,
			(unsigned int)job->priority_time,
			job->status >= 0 ? 0 : (job->status * -1),
			job->flags) != 64
			)
		gu_Throw("Length of PPRD line is not 64 bytes!");

	if((written_size = write(fd, buf, 64)) == -1)
		error("%s(): write() failed, errno=%d (%s)", function, errno, strerror(errno));
	else if(written_size != 64)
		error("%s(): tried to write %d bytes but wrote %d instead", function, 64, (int)written_size);

	close(fd);
	} /* end of queue_write_status_and_flags() */

/*===========================================================================
** Change the status of a job.
**
** The version p_job_new_status() takes a pointer to the job structure.
** The version job_new_status() takes the id and subid and finds the
** job in the queue.
**
** Notice that p_job_new_status() must be called
** with the queue locked but job_new_status() need not be since
** it locks the queue itself.
===========================================================================*/

struct QEntry *queue_p_job_new_status(struct QEntry *job, int newstat)
	{
	const char function[] = "queue_p_job_new_status";
	char *status_string = (char*)NULL;

	job->status = newstat;

	queue_write_status_and_flags(job);

	switch(job->status)
		{
		case STATUS_WAITING:
			status_string = "waiting for printer";
			break;
		case STATUS_HELD:
			status_string = "held";
			break;
		case STATUS_SEIZING:
			status_string = "being seized";
			break;
		case STATUS_WAITING4MEDIA:						/* never used */
			status_string = "waiting for media";		/* see media_set_job_wait_reason() */
			break;
		case STATUS_ARRESTED:
			status_string = "arrested";
			break;
		case STATUS_STRANDED:
			status_string = "stranded";
			break;
		case STATUS_CANCEL:
			status_string = "being canceled";
			break;
		default:										/* >=0 means printing */
			if(job->status < 0)
				fatal(0, "%s(): assertion failed", function);
			state_update("JST %s printing on %s",
				jobid(destid_to_name(job->destid), job->id, job->subid),
				destid_to_name(job->status));	/* yes, this looks weird */
			break;
		}

	if(status_string)
		{
		state_update("JST %s %s",
			jobid(destid_to_name(job->destid), job->id, job->subid),
			status_string);
		}
	
	return job;
	} /* end of queue_p_job_new_status() */

struct QEntry *queue_job_new_status(int destid, int id, int subid, int newstat)
	{
	const char function[] = "queue_job_new_status";
	int x;

	lock();								/* lock queue array while we work on it */

	for(x=0; x < queue_entries; x++)	/* Find the job in the */
		{								/* queue array. */
		if(queue[x].destid == destid && queue[x].id == id && queue[x].subid == subid)
			{
			queue_p_job_new_status(&queue[x], newstat);
			break;						/* break for loop */
			}
		}
	if(x==queue_entries)
		fatal(0, "%s(): %d %d not found in queue", function, id, subid);

	unlock();					/* unlock queue array */

	return &queue[x];			/* return a pointer to the queue entry */
	} /* end of queue_job_new_status() */

/*===========================================================================
** Receive a new job into the queue.
**
** The job is entered in the queue, then the pprd_printer.c module is
** informed of its arrival.
===========================================================================*/
void queue_accept_queuefile(const char qfname[], gu_boolean job_is_new)
	{
	const char function[] = "queue_accept_queuefile";
	char *scratch = NULL;
	const char *destname = NULL;
	struct QEntry newent, *newentp;

	scratch = gu_strdup(qfname);

	gu_Try
		{
		if(parse_qfname(scratch, &destname, &newent.id, &newent.subid) == -1)
			gu_Throw("can't parse queue file name");

		/* Coinvert the destination queue name to a queue id number. */
		if((newent.destid = destid_by_name(destname)) == -1)
			{
			/* Destination doesn't exist, throw an exception.  The exception handler
			 * will delete the job. */
			if(job_is_new)
				{
				respond2(destname, newent.id, newent.subid, -1, destname, RESP_CANCELED_BADDEST);
				gu_Throw("destination \"%s\" does not exist", destname);
				}
			else
				{
				/* shouldn't happen, so no call to respond2() */
				gu_Throw("destination \"%s\" no longer exists", destname);
				}
			}

		/* If this is a new job and the destination is not accepting new
		   jobs, tell the user and ditch the job.
		   */
		if(job_is_new && !destid_accepting(newent.destid))
			{
			respond(newent.destid, newent.id, newent.subid, -1, RESP_CANCELED_REJECTING);
			gu_Throw("destination \"%s\" is not accepting jobs", destname);
			}

		{
		char qfname_path[MAX_PPR_PATH];
		FILE *qfile;
		char *line = NULL;
		int line_available = 80;
		gu_boolean pprd_line_seen = FALSE;
		char tmedia[MAX_MEDIANAME+1];
		int media_index = 0;

		DODEBUG_NEWJOB(("%s(qfname=\"%s\", newentry=?)", function, qfname));

		/* First we open the new job's queue file. */
		ppr_fnamef(qfname_path, "%s/%s", QUEUEDIR, qfname);
		if((qfile = fopen(qfname_path, "r")) == (FILE*)NULL)
			gu_Throw("can't open \"%s\", errno=%d (%s)", qfname_path, errno, gu_strerror(errno));

		while((line = gu_getline(line, &line_available, qfile)))
			{
			if(gu_sscanf(line, "PPRD: %hx %x %hx %hx",
					&newent.priority, &newent.priority_time,
					&newent.status, &newent.flags
					) == 4
				)
				{
				newent.status *= -1;
				pprd_line_seen = TRUE;
				continue;
				}
			if(media_index < MAX_DOCMEDIA && gu_sscanf(line, "Media: %@s", sizeof(tmedia), tmedia) == 1)
				{
				newent.media[media_index++] = get_media_id(tmedia);
				continue;
				}
			}

		fclose(qfile);

		if(!pprd_line_seen)
			gu_Throw("no PPRD line");
		
		DODEBUG_MEDIA(("%s(): %d media requirement(s) read", function, media_index));
		while(media_index < MAX_DOCMEDIA)			/* fill in entra spaces with -1 */
			newent.media[media_index++] = -1;
		}

		/*
		** If this is a group job, set pass number to one, otherwise, set pass 
		** number to zero which will indicate the pprdrv that it is not
		** a group job.
		*/
		if(destid_is_group(newent.destid))
			newent.pass = 1;
		else
			newent.pass = 0;

		/* Clear the time of next response. */
		newent.resend_message_at = 0;

		/* Clear the bitmaps of printers which it is known can't print it
		   and of those that can't print it right now because they don't
		   have the required media mounted. */
		newent.never = 0;
		newent.notnow = 0;

		/* If the job was printing (as indicated by a status of 0), then set its status to waiting. */
		if(newent.status == 0)
			newent.status = STATUS_WAITING;

		lock();

		/* Set the bitmask which shows which printers have the form.
		   This may change the printer status too. */
		media_set_notnow_for_job(&newent, FALSE);

		{
		int destmates_passed = 0;
		int x;

		/* Queue sanity check: */
		if(queue_entries > queue_size || queue_entries < 0)
			fatal(1, "%s(): assertion failed: queue_entries=%d, queue_size=%d", function, queue_entries, queue_size);

		/*
		** The the queue is already full, try to expand the queue array,
		** otherwise just don't put it in the queue.  If that is done,
		** the job will not be printed until pprd is restarted.
		*/
		if(queue_entries == queue_size)
			{
			if((queue_size + QUEUE_SIZE_GROWBY) <= QUEUE_SIZE_MAX)		/* there are limits */
				{
				DODEBUG_NEWJOB(("%s(): expanding %d entry queue to %d entries", function, queue_size, queue_size+QUEUE_SIZE_GROWBY));
				queue_size += QUEUE_SIZE_GROWBY;
				queue = (struct QEntry *)gu_realloc(queue, queue_size, sizeof(struct QEntry));
				}
			else
				{
				gu_Throw("queue array overflow");
				}
			}

		for(x=0; x < queue_entries; x++)	/* Find or make a space in the queue array. */
			{
			/*
			** A higher priority number means a more urgent job.  If we have found
			** a job with a lower priority number than the job we are inserting,
			** or a job with an equal priority number but a later priority time,
			** move all the jobs from here on one slot furthur toward the end
			** of the queue and break out of the loop.
			*/
			if(newent.priority > queue[x].priority ||
				(newent.priority == queue[x].priority && newent.priority_time < queue[x].priority_time)
				)
				{
				int y;

				/* We must do this in segments due to problems with overlapping copies. */
				for(y=queue_entries; y >= x; y--)
					memmove(&queue[y+1], &queue[y], sizeof(struct QEntry));

				break;
				}

			/*
			** If we are passing a job which is for the same destination as
			** the job we are inserting, add one to the count which will
			** be stored in rank2.
			*/
			if(queue[x].destid == newent.destid)
				destmates_passed++;
			}

		/* Copy the new queue entry into the place prepared for it. */
		memcpy(&queue[x], &newent, sizeof(struct QEntry));
		newentp = &queue[x];

		/* increment our count of queue entries */
		queue_entries++;

		/* increment destination's job count */
		job_count_adjust(newent.destid, 1, job_is_new);
		
		/* Inform queue display programs that there is a new job in the queue. */
		state_update("JOB %s %d %d",
				jobid(destname, newent.id, newent.subid),
				x, destmates_passed);
		}

		/* If there is an outstanding question, then let the question system
		   know about this job. */
		if(newentp->flags & JOB_FLAG_QUESTION_UNANSWERED)
			question_job(newentp);

		/* If pprd isn't restarting and the job is ready to print, try to start a printer. */
		if(job_is_new && newentp->status == STATUS_WAITING)
			printer_try_start_suitable_4_this_job(newentp);

		unlock();
		}
	gu_Catch
		{
		error("%s(): failed to enqueue %s: %s", function, qfname, gu_exception);
		/* If we were able to parse the name of the bad job, we can delete its files. */
		if(destname)
			delete_job_files(destname, newent.id, newent.subid);
		}

	gu_free_if(scratch);
	} /* end of queue_accept_queuefile() */

/*===========================================================================
** This handles the j command from ppr.  The j command is used to inform
** pprd that a new job has been placed in the queue.  It calls
** queue_accept_queuefile() with the job_is_new argument ste to TRUE whereas
** initialize_queue() calls it with the argument set to FALSE.
===========================================================================*/
void queue_new_job(char *command)
	{
	const char function[] = "queue_new_job";
	char *qfname;
	if(!(qfname = lmatchp(command, "j ")))
		{
		error("%s(): bad j command: %s", function, command);
		return;
		}
	queue_accept_queuefile(qfname, TRUE);
	} /* end of queue_new_job() */

/* end of file */

