/*
** mouse:~ppr/src/pprd/pprd_queue.c
** Copyright 1995--2002, Trinity College Computing Center.
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
** Last modified 5 March 2002.
*/

/*
** This module contains routines for maintaining the print queue array,
** loading queue files into the array, and changing the status of jobs.
*/

#include "before_system.h"
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
#include "cexcept.h"

define_exception_type(int);
static struct exception_context the_exception_context[1];

/*================================================================
** Insert a job into the queue structure.  If for some reason we
** can't, we are allowed to return a NULL pointer.  The rank1 and
** rank2 parameters will be set to its position in the "all" queue
** and the individual queue in which it is placed, respectively.
================================================================*/
static struct QEntry *queue_insert(const char qfname[], struct QEntry *newentry, int *rank1, int *rank2)
    {
    const char function[] = "queue_insert";
    int destmates_passed = 0;
    int x;

    DODEBUG_NEWJOB(("%s(): destid=%d id=%d subid=%d status=%d", function, newentry->destid, newentry->id, newentry->subid, newentry->status));

    /*
    ** If this is a group job, set pass number
    ** to one, otherwise, set pass number to zero
    ** which will indicate the pprdrv that it is not
    ** a group job.
    */
    if(destid_local_is_group(newentry->destid))
	newentry->pass = 1;
    else
    	newentry->pass = 0;

    /* Sanity check: */
    if(queue_entries > queue_size || queue_entries < 0)
	fatal(1, "%s(): assertion failed: queue_entries=%d, queue_size=%d", function, queue_entries, queue_size);

    /*
    ** The the queue is already full, try to expand the queue array,
    ** otherwise just don't put it in the queue.  If that is done,
    ** the job will not be printed until pprd is restarted.
    */
    if(queue_entries == queue_size)
	{
	if( (queue_size + QUEUE_SIZE_GROWBY) <= QUEUE_SIZE_MAX )
	    {
	    DODEBUG_NEWJOB(("%s(): expanding %d entry queue to %d entries", function, queue_size, queue_size+QUEUE_SIZE_GROWBY));
	    queue_size += QUEUE_SIZE_GROWBY;
	    queue = (struct QEntry *)gu_realloc(queue, queue_size, sizeof(struct QEntry));
	    }
	else
	    {
	    error("%s(): queue array overflow", function);
	    unlock();
	    return (struct QEntry*)NULL;
	    }
	}

    destmates_passed = 0;
    for(x=0; x < queue_entries; x++)	/* Find or make a space in the queue array. */
	{
	/*
	** Lower priority number mean more urgent jobs.  If we have found
	** a job with a higher priority number than the job we are inserting,
	** move all the jobs from here on one slot furthur toward the end
	** of the queue and break out of the loop.
	*/
	if(newentry->priority < queue[x].priority)
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
	if(queue[x].destid == newentry->destid)
	    destmates_passed++;
	}

    /* Copy the new queue entry into the place prepared for it. */
    memcpy(&queue[x], newentry, sizeof(struct QEntry));

    queue_entries++;            /* increment our count of queue entries */

    *rank1 = x;			/* fill in queue rank of new entry */
    *rank2 = destmates_passed;

    return &queue[x];           /* return pointer to what we put it in */
    } /* end of queue_insert() */

/*===========================================================================
** Unlink a job.
===========================================================================*/
static void queue_unlink_job_2(const char *destnode, const char *queuename, int id, int subid, const char *homenode_name)
    {
    char filename[MAX_PPR_PATH];

    ppr_fnamef(filename, "%s/%s:%s-%d.%d(%s)",
	QUEUEDIR, destnode, queuename, id, subid, homenode_name);
    unlink(filename);

    ppr_fnamef(filename, "%s/%s:%s-%d.%d(%s)-comments",
	DATADIR, destnode, queuename, id, subid, homenode_name);
    unlink(filename);

    ppr_fnamef(filename, "%s/%s:%s-%d.%d(%s)-pages",
	DATADIR, destnode, queuename, id, subid, homenode_name);
    unlink(filename);

    ppr_fnamef(filename, "%s/%s:%s-%d.%d(%s)-text",
	DATADIR, destnode, queuename, id, subid, homenode_name);
    unlink(filename);

    ppr_fnamef(filename, "%s/%s:%s-%d.%d(%s)-log",
	DATADIR, destnode, queuename, id, subid, homenode_name);
    unlink(filename);

    ppr_fnamef(filename, "%s/%s:%s-%d.%d(%s)-infile",
	DATADIR, destnode, queuename, id, subid, homenode_name);
    unlink(filename);

    ppr_fnamef(filename, "%s/%s:%s-%d.%d(%s)-barbar",
	DATADIR, destnode, queuename, id, subid, homenode_name);
    unlink(filename);
    } /* end of queue_unlink_job_2() */

/*================================================================
** Unlink a job and remove an entry from the queue array.
================================================================*/
void queue_dequeue_job(int destnode_id, int destid, int id, int subid, int homenode_id)
    {
    FUNCTION4DEBUG("queue_dequeue_job")
    int x;
    const char *destnode = nodeid_to_name(destnode_id);
    const char *destname = destid_to_name(destnode_id, destid);
    const char *homenode = nodeid_to_name(homenode_id);
    const char *full_job_id = remote_jobid(homenode, destname, id, subid, homenode);

    DODEBUG_DEQUEUE(("%s(destid=%d, id=%d, subid=%d, homenode_id=%d)", function, destid, id, subid, homenode_id));

    /* Inform queue display programs of job deletion. */
    state_update("DEL %s", full_job_id);

    lock();		/* lock the queue array while we modify it */

    for(x=0; x < queue_entries; x++)
	{
	if(queue[x].id==id && queue[x].subid==subid && queue[x].destnode_id == destnode_id && queue[x].homenode_id == homenode_id)
	    {
	    DODEBUG_DEQUEUE(("removing job %s at position %d from queue", full_job_id, x));

	    /* Remove the actual job files. */
	    queue_unlink_job_2(destnode, destname, id, subid, homenode);

	    /* Decrement a few reference counts. */
	    destid_free(queue[x].destnode_id, queue[x].destid);
	    nodeid_free(queue[x].destnode_id);
	    nodeid_free(queue[x].homenode_id);

	    if( (queue_entries-x) > 1 )		/* do a move if not last entry */
		{				/* (BUG: was queue_entries was queue_size prior to version 1.30) */
		memmove(&queue[x], &queue[x+1],
		    sizeof(struct QEntry) * (queue_entries-x-1) );
		}

	    queue_entries--;			/* one less in queue */
	    break;				/* and we needn't look farther */
	    }
	}

    unlock();		/* unlock queue array */

    } /* end of queue_dequeue_job() */

/*=========================================================================
** Edit the "Status-and-Flags: XX XXXX\n" in the queue file.
=========================================================================*/
static void queue_write_status_and_flags(struct QEntry *job)
    {
    const char function[] = "queue_write_status_and_flags";
    /* start of exception handling block */
    do
	{
	#define STATUS_BLOCK_SIZE 50
	int fd;
	char buf[STATUS_BLOCK_SIZE + 1];
	ssize_t read_size;
	size_t to_write_size, written_size;
	char *p;

	{
	char filename[MAX_PPR_PATH];
	ppr_fnamef(filename, "%s/%s:%s-%d.%d(%s)", QUEUEDIR, ppr_get_nodename(), destid_to_name(job->destnode_id, job->destid), job->id, job->subid, nodeid_to_name(job->homenode_id));
	if((fd = open(filename, O_RDWR)) == -1)
    	    {
	    error("%s(): can't open \"%s\", errno=%d (%s)", function, filename, errno, strerror(errno));
	    break;
	    }
	}

	/* start of inner exception handling block */
	while(1)
	    {
	    if((read_size = read(fd, buf, STATUS_BLOCK_SIZE)) == -1)
		{
		error("%s(): read() failed, errno=%d (%s)", function, errno, strerror(errno));
		break;
		}
	    if(read_size != STATUS_BLOCK_SIZE)
	    	{
	    	error("%s(): expected %d bytes, get %d", function, (int)STATUS_BLOCK_SIZE, (int)read_size);
	    	break;
	    	}
	    buf[STATUS_BLOCK_SIZE] = '\0';
	    if(!(p = strstr(buf, "Status-and-Flags: ")))
	    	{
		error("%s(): can't find place", function);
		break;
	    	}
	    if(lseek(fd, (p - buf), SEEK_SET) == -1)
	    	{
	    	error("%s(): lseek() failed, errno=%d (%s)", function, errno, strerror(errno));
	    	break;
	    	}

	    /* Write the new line.  If the job is printing, substitute 0 for the actual printer index. */
	    snprintf(buf, sizeof(buf), "Status-and-Flags: %02d %04X\n", job->status >= 0 ? 0 : (job->status * -1), job->flags);

	    to_write_size = strlen(buf);
	    if((written_size = write(fd, buf, to_write_size)) == -1)
	    	{
	    	error("%s(): write() failed, errno=%d (%s)", function, errno, strerror(errno));
	    	break;
	    	}
	    if(written_size != to_write_size)
	    	{
		error("%s(): tried to write %d bytes but wrote %d instead", function, to_write_size, written_size);
		}

	    break;	/* end of inner exception handling block */
	    }

	close(fd);

    	} while(FALSE);			/* end of exception handling block */
    } /* end of queue_write_status_and_flags() */

/*========================================================================
** Send a state update message for a job status change.
** This code should be sending symbolic values and not English messages.
========================================================================*/
static void queue_send_state_update(struct QEntry *job)
    {
    const char function[] = "queue_send_state_update";
    char *status_string = (char*)NULL;

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
	case STATUS_WAITING4MEDIA:			/* never used */
	    status_string = "waiting for media";	/* see media_set_job_wait_reason() */
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
    	default:					/* >=0 means printing */
	    if(job->status < 0)
	    	fatal(0, "%s(): assertion failed", function);
	    state_update("JST %s printing on %s",
	    	local_jobid(destid_to_name(job->destnode_id,job->destid),job->id,job->subid,nodeid_to_name(job->homenode_id)),
	   	destid_to_name(job->destnode_id, job->status));
    	    break;
    	}

    if(status_string)
	{
	state_update("JST %s %s",
		remote_jobid(nodeid_to_name(job->destnode_id),destid_to_name(job->destnode_id, job->destid),job->id,job->subid,nodeid_to_name(job->homenode_id)),
		status_string);
	}
    } /* end of queue_send_state_update() */

/*========================================================================
** Change the status of a job.
**
** We will update the execute bits on the queue file if required.
**
** The version p_job_new_status() takes a pointer to the job structure.
** The version job_new_status() takes the id and subid and finds the
** job in the queue.
**
** Notice that p_job_new_status() must be called
** with the queue locked but job_new_status() need not be since
** it locks the queue itself.
**
** Probably obsolete comment:  This routine is sometimes called from
** within a signal handler, so it must not call routines which are not
** reentrant.
========================================================================*/

struct QEntry *queue_p_job_new_status(struct QEntry *job, int newstat)
    {
    job->status = newstat;
    queue_write_status_and_flags(job);
    queue_send_state_update(job);
    return job;
    } /* end of queue_p_job_new_status() */

struct QEntry *queue_job_new_status(int id, int subid, int homenode_id, int newstat)
    {
    const char function[] = "queue_job_new_status";
    int x;

    lock();				/* lock queue array while we work on it */

    for(x=0; x < queue_entries; x++)	/* Find the job in the */
	{				/* queue array. */
	if(queue[x].id == id && queue[x].subid == subid && queue[x].homenode_id == homenode_id)
	    {
	    queue_p_job_new_status(&queue[x], newstat);
	    break;			/* break for loop */
	    }
	}
    if(x==queue_entries)
	fatal(0, "%s(): %d %d not found in queue", function, id, subid);

    unlock();			/* unlock queue array */

    return &queue[x];		/* return a pointer to the queue entry */
    } /* end of queue_job_new_status() */

/*===========================================================================
** Read necessary information from the queuefile into the queue structure.
**
===========================================================================*/
int queue_read_queuefile(const char qfname[], struct QEntry *newentry)
    {
    const char function[] = "queue_read_queuefile";
    FILE *qfile;
    char tmedia[MAX_MEDIANAME+1];
    int x;

    DODEBUG_NEWJOB(("%s(qfname=\"%s\", newentry=?)", function, qfname));

    /* First we open the queue file. */
    {
    char qfname_path[MAX_PPR_PATH];
    ppr_fnamef(qfname_path, "%s/%s", QUEUEDIR, qfname);
    if((qfile = fopen(qfname_path, "r")) == (FILE*)NULL)
	{
	error("%s(): can't open \"%s\", errno=%d (%s)", function, qfname_path, errno, gu_strerror(errno));
	return -1;
	}
    }

    /*
    ** Read the priority from the queue file.  There is no good reason
    ** for the default since all queue files have a "Priority:" line.
    ** We are just being paranoid.
    */
    newentry->status = STATUS_WAITING;
    newentry->flags = 0;
    newentry->priority = 20;
    {
    char *line = NULL;
    int line_available = 80;
    x=0;
    while((line = gu_getline(line, &line_available, qfile)))
	{
	if(sscanf(line, "Priority: %hd", &newentry->priority) == 1)
	    {
	    continue;
	    }
	if(sscanf(line, "Status-and-Flags: %02hd %hx", &newentry->status, &newentry->flags) == 2)
	    {
	    newentry->status *= -1;
	    continue;
	    }
	if(x < MAX_DOCMEDIA && gu_sscanf(line, "Media: %#s", sizeof(tmedia), tmedia) == 1)
	    {
	    newentry->media[x++] = get_media_id(tmedia);
	    continue;
	    }
	}
    }

    fclose(qfile);

    DODEBUG_MEDIA(("%s(): %d media requirement(s) read", function, x));

    while(x < MAX_DOCMEDIA)		/* fill in entra spaces with -1 */
	newentry->media[x++] = -1;

    return 0;
    } /* end of queue_read_queuefile() */

/*===========================================================================
** Receive a new job into the queue.
**
** The job is entered in the queue.  If it is for this node, then the
** pprd_printer.c module is informed of its arrival, otherwise the
** pprd_remote.c module is informed.
**
** For local jobs sucessfuly entered into the queue, this function calls
** state_update() in order to inform queue display programs that there
** is a new job in the queue.  This function is `instrumented' for this
** purpose rather than enqueue_job() because enqueue_job() is used to
** re-load jobs that are already in the queue when pprd starts.
===========================================================================*/
void queue_accept_queuefile(const char qfname[], gu_boolean job_is_new)
    {
    const char function[] = "queue_insert_job";
    int e;
    char *scratch = NULL;
    const char *ptr_destnode, *ptr_destname, *ptr_homenode;
    struct QEntry newent;		/* space for building new queue entry */
    struct QEntry *newentp;		/* new queue entry */
    int rank1, rank2;

    Try {
	{
	int err;
	scratch = gu_strdup(qfname);
	err = parse_qfname(scratch, &ptr_destnode, &ptr_destname, &newent.id, &newent.subid, &ptr_homenode);
	if(err < 0)
	    {
	    error("%s(): can't parse \"%s\" (%d)", function, qfname, err);
	    Throw(-1);
	    }
	}

	/* Convert the home node name to an id number. */
	newent.destnode_id = nodeid_assign(ptr_destnode);
	newent.homenode_id = nodeid_assign(ptr_homenode);

	Try {
	    /* Convert the destination queue name to a queue id number.  If the queue
	       does not exist, inform the user.  The exception handler will delete the
	       job files.
	       */
	    if((newent.destid = destid_assign(newent.destnode_id, ptr_destname)) == -1)
		{
		if(job_is_new)
		    respond2(ptr_destnode, ptr_destname, newent.id, newent.subid, ptr_homenode, -1, ptr_destname, RESP_CANCELED_BADDEST);
		else
		    error("%s(): destination \"%s\" no longer exists", function, ptr_destname);
		Throw(-1);
		}

	    Try {
		/* If this is a new job and the destination is not accepting new
		   jobs, tell the user and ditch the job.
		   */
		if(job_is_new && ! destid_accepting(newent.destnode_id, newent.destid))
		    {
		    respond(newent.destnode_id, newent.destid, newent.id, newent.subid, newent.homenode_id, -1, RESP_CANCELED_REJECTING);
		    Throw(-1);
		    }

		if(queue_read_queuefile(qfname, &newent) == -1)
		    {
		    Throw(-1);
		    }

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
                   This may change the printer status too.  For remote jobs,
                   this is defered until it reaches to target system. */
                if(nodeid_is_local_node(newent.destnode_id))
                    media_set_notnow_for_job(&newent, FALSE);

		/* If there is room in the queue array, */
		if(!(newentp = queue_insert(qfname, &newent, &rank1, &rank2)))
		    {
		    /* error() already called */
		    /* Don't Throw() as that would leave the queue locked! */
		    }
		else
		    {
		    /* Inform queue display programs that there is a new job in the queue. */
		    state_update("JOB %s %d %d",
			remote_jobid(ptr_destnode, ptr_destname, newent.id, newent.subid, ptr_homenode),
			rank1, rank2);

		    /* If there is an outstanding question, then let the question system
		       know about this job. */
		    if(newentp->flags & JOB_FLAG_QUESTION_UNANSWERED)
		    	question_job(newentp);

		    /* Start sending it, or if it is local and ready to print, try to start a printer. */
		    if(!nodeid_is_local_node(newentp->destnode_id))
			remote_job(newentp);
		    else if(newentp->status == STATUS_WAITING)
			printer_try_start_suitable_4_this_job(newentp);
		    }

		unlock();
		}
	    Catch(e)
	    	{
		destid_free(newent.destnode_id, newent.destid);
		Throw(e);
	    	}
	    }
        Catch(e)
	    {
	    queue_unlink_job_2(ptr_destnode, ptr_destname, newent.id, newent.subid, ptr_homenode);
	    nodeid_free(newent.destnode_id);
	    nodeid_free(newent.homenode_id);
	    Throw(e);
	    }
        }
    Catch(e)
	{
	/* nothing to do */
	}

    if(scratch)
    	gu_free(scratch);

    } /* end of queue_accept_queuefile() */

/*============================================================
** This handles the j command from ppr.  The j command is
** used to inform pprd that a new job has been placed in
** the queue.
============================================================*/
void queue_new_job(char *command)
    {
    const char function[] = "queue_new_job";
    char *qfname;

    if(gu_sscanf(command, "j %S", &qfname) != 1)
	{
	error("%s(): bad j command: %s", function, command);
	}
    else
	{
	queue_accept_queuefile(qfname, TRUE);
	gu_free(qfname);
	}
    } /* end of queue_new_job() */

/* end of file */

