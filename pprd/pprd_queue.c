/*
** mouse:~ppr/src/pprd/pprd_queue.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 6 December 2000.
*/

/*
** This module contains routines for maintaining the print queue array and
** spool directories.
*/

#include "before_system.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "gu.h"
#include "global_defines.h"

#include "global_structs.h"
#include "pprd.h"
#include "./pprd.auto_h"
#include "respond.h"

/*
** Enqueue a job.  If for some reason we can't, we are allowed
** to return a NULL pointer.  The rank1 and rank2 parameters
** will be set to its position in the "all" queue and the
** individual queue in which it is placed, respectively.
*/
struct QEntry *queue_enqueue_job(struct QEntry *newentry, int *rank1, int *rank2)
    {
    const char function[] = "queue_enqueue_job";
    int x;
    FILE *qfile;
    char qfname[MAX_PPR_PATH];
    char tline[256];
    char tmedia[MAX_MEDIANAME+1];
    int destmates_passed = 0;

    DODEBUG_NEWJOB(("%s(): destid=%d id=%d subid=%d status=%d", function, newentry->destid, newentry->id, newentry->subid, newentry->status));

    /* Read in the list of required media. */

    /* First we open the queue file. */
    ppr_fnamef(qfname, "%s/%s:%s-%d.%d(%s)",
	QUEUEDIR, nodeid_to_name(newentry->destnode_id), destid_to_name(newentry->destnode_id, newentry->destid),
	newentry->id, newentry->subid, nodeid_to_name(newentry->homenode_id) );

    if((qfile = fopen(qfname, "r")) == (FILE*)NULL)
	{
	error("%s(): can't open \"%s\", errno=%d (%s)", function, qfname, errno, gu_strerror(errno));
	return (struct QEntry*)NULL;
	}

    /* Then we read the "Media:" lines. */
    x=0;
    while(x < MAX_DOCMEDIA && fgets(tline,sizeof(tline), qfile))
	{
	if(gu_sscanf(tline, "Media: %#s", sizeof(tmedia), tmedia) == 1)
	    newentry->media[x++] = get_media_id(tmedia);
	}

    fclose(qfile);

    DODEBUG_MEDIA(("%s(): %d media requirement(s) read", function, x));

    while(x < MAX_DOCMEDIA)		/* fill in entra spaces with -1 */
	newentry->media[x++] = -1;

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

    lock();                             /* must lock before media_set_notnow... */

    /* Set the bitmask which shows which printers have the form. */
    if(nodeid_is_local_node(newentry->destnode_id))
	{
	media_set_notnow_for_job(newentry, FALSE);
	}
    else
	{
	newentry->notnow = 0;
	newentry->status = STATUS_WAITING;
	}

    /* Initialize the bitmask of printers which can't possibly print it. */
    newentry->never = 0;

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
	    queue = (struct QEntry *)ppr_realloc(queue, queue_size, sizeof(struct QEntry));
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
	** the jbo we are inserting, add one to the count which will
	** be stored in rank2.
	*/
	if(queue[x].destid == newentry->destid)
	    destmates_passed++;
	}

    /* Copy the new queue entry into the place prepared for it. */
    memcpy(&queue[x], newentry, sizeof(struct QEntry));

    queue_entries++;            /* increment our count of queue entries */

    unlock();			/* release our lock on the queue array */

    *rank1 = x;			/* fill in queue rank of new entry */
    *rank2 = destmates_passed;

    return &queue[x];           /* return pointer to what we put it in */
    } /* end of queue_enqueue_job() */

/*
** Remove an entry from the queue array.
*/
void queue_dequeue_job(int destnode_id, int destid, int id, int subid, int homenode_id)
    {
    FUNCTION4DEBUG("queue_dequeue_job")
    int x;

    DODEBUG_DEQUEUE(("%s(destid=%d, id=%d, subid=%d, homenode_id=%d)", function, destid, id, subid, homenode_id));

    /* Inform queue display programs of job deletion. */
    state_update("DEL %s", remote_jobid(nodeid_to_name(destnode_id),
    				destid_to_name(destnode_id,destid),
    				id,subid,
    				nodeid_to_name(homenode_id)) );

    lock();		/* lock the queue array while we modify it */

    for(x=0; x < queue_entries; x++)
	{
	if(queue[x].id==id && queue[x].subid==subid)
	    {
	    DODEBUG_DEQUEUE(("job %s-%d.%d removed from queue",
		destid_to_name(destnode_id, queue[x].destid), queue[x].id, queue[x].subid));

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

/*
** Unlink a job.  This is called by queue_unlink_job().  It is also called
** to cancel rejected jobs (which haven't been put in the queue yet.)
*/
void queue_unlink_job_2(const char *destnode, const char *queuename, int id, int subid, const char *homenode_name)
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

/*
** Unlink job files for a job that is in the queue or was
** about to go into the queue.  (I.E., one for which we
** have already built a queue entry structure.)
*/
void queue_unlink_job(int destnode_id, int destid, int id, int subid, int homenode_id)
    {
    const char *destnode_name;
    const char *queuename;
    const char *homenode_name;

    DODEBUG_DEQUEUE(("unlink_job(destid=%d, id=%d, subid=%d, homenode_id=%d)", destid, id, subid, homenode_id));

    destnode_name = nodeid_to_name(destnode_id);
    queuename = destid_to_name(destnode_id, destid);
    homenode_name = nodeid_to_name(homenode_id);

    queue_unlink_job_2(destnode_name, queuename, id, subid, homenode_name);
    } /* end of queue_unlink_job() */

/*
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
** This routine is sometimes called from within a signal handler, so
** it must not call routines which are not reentrant.
*/
struct QEntry *p_job_new_status(struct QEntry *job, int newstat)
    {
    char filename[MAX_PPR_PATH];
    const char *queuename;

    queuename = destid_to_name(job->destnode_id, job->destid);

    /*
    ** Inform queue display programs.
    */
    {
    char *status_string = (char*)NULL;

    switch(newstat)
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
    	default:
	    if(newstat < 0)
	    	fatal(0, "p_job_new_status(): assertion failed");
	    state_update("JST %s printing on %s",
	    	local_jobid(queuename,job->id,job->subid,nodeid_to_name(job->homenode_id)),
	    	destid_to_name(job->destnode_id, newstat));
    	    break;
    	}

    if(status_string)
	{
	state_update("JST %s %s",
		local_jobid(queuename,job->id,job->subid,nodeid_to_name(job->homenode_id)),
		status_string);
	}
    }

    /* Re-construct the name of the queue file. */
    ppr_fnamef(filename, QUEUEDIR"/%s:%s-%d.%d(%s)", ppr_get_nodename(), queuename, job->id, job->subid, nodeid_to_name(job->homenode_id));

    /*
    ** If necessary, do a chmod() on the queue file to reflect the new status.
    */
    switch(newstat)
	{
	case STATUS_HELD:
	    chmod(filename, BIT_JOB_BASE | BIT_JOB_HELD);
	    break;
	case STATUS_ARRESTED:
	    chmod(filename, BIT_JOB_BASE | BIT_JOB_ARRESTED);
	    break;
	case STATUS_STRANDED:
	    chmod(filename, BIT_JOB_BASE | BIT_JOB_STRANDED);
	    break;
	default:
	     if(job->status == STATUS_HELD || job->status==STATUS_ARRESTED || job->status==STATUS_STRANDED)
		chmod(filename, BIT_JOB_BASE);
	}

    job->status = newstat;		/* save new status */

    return job;				/* return a pointer to the queue entry */
    } /* end of p_job_new_status() */

struct QEntry *job_new_status(int id, int subid, int homenode_id, int newstat)
    {
    const char function[] = "job_new_status";
    int x;

    lock();				/* lock queue array while we work on it */

    for(x=0; x < queue_entries; x++)	/* Find the job in the */
	{				/* queue array. */
	if(queue[x].id == id && queue[x].subid == subid && queue[x].homenode_id == homenode_id)
	    {
	    p_job_new_status(&queue[x], newstat);
	    break;			/* break for loop */
	    }
	}
    if(x==queue_entries)
	fatal(0, "%s(): %d %d not found in queue", function, id, subid);

    unlock();			/* unlock queue array */

    return &queue[x];		/* return a pointer to the queue entry */
    } /* end of job_new_status() */

/*
** Receive a new job.
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
*/
void queue_new_job(char *command)
    {
    FUNCTION4DEBUG("queue_new_job")
    struct QEntry newent;		/* space for building new queue entry */
    char queuename[MAX_DESTNAME+1];	/* name of destination */
    char destnode[MAX_NODENAME+1];
    char homenode[MAX_NODENAME+1];
    int hold;
    struct QEntry *newentp;		/* new queue entry */
    int rank1, rank2;

    /* Parse command: */
    gu_sscanf(command, "j %s %s %hd %hd %s %hd %d",
		destnode,
		queuename,			/* group or printer name */
		&newent.id,			/* read major queue id */
		&newent.subid,			/* minor queue id */
		homenode,
		&newent.priority,		/* and queue priority */
		&hold);

    DODEBUG_NEWJOBS(("%s(): destnode=%s queuename=%s id=%d subid=%d homenode=%s priority=%d hold=%d", function,
		destnode, queuename, newent.id, newent.subid, homenode, newent.priority, hold));

    /* Assign the initial job status: */
    newent.status = hold ? STATUS_HELD : STATUS_WAITING;

    /* Convert the home node name to an id number. */
    newent.destnode_id = nodeid_assign(destnode);
    newent.homenode_id = nodeid_assign(homenode);

    /* Convert the destination queue name to a queue id number.  If the queue
       does not exist, inform the user and cancel the job.  We can't use
       queue_unlink_job() here because we don't have a valid destination id to
       pass to it.
       */
    if((newent.destid = destid_assign(newent.destnode_id, queuename)) == -1)
        {
        respond2(destnode, queuename, newent.id, newent.subid, homenode, queuename, -1, RESP_CANCELED_BADDEST);
        queue_unlink_job_2(destnode, queuename, newent.id, newent.subid, homenode);
        nodeid_free(newent.destnode_id);
        nodeid_free(newent.homenode_id);
        return;
        }

    /* If destination is not accepting new jobs, tell the user
       and ditch the job.
       */
    if(! destid_accepting(newent.destnode_id, newent.destid))
        {
        respond(newent.destnode_id, newent.destid, newent.id, newent.subid, newent.homenode_id, -1, RESP_CANCELED_REJECTING);
        queue_unlink_job(newent.destnode_id, newent.destid, newent.id, newent.subid, newent.homenode_id);
	destid_free(newent.destnode_id, newent.destid);
        nodeid_free(newent.destnode_id);
        nodeid_free(newent.homenode_id);
        return;
        }

    /* If there is no room in the queue array, abort.  An error()
       call will have already been made.
       */
    if(! (newentp = queue_enqueue_job(&newent, &rank1, &rank2)))
        {
	destid_free(newent.destnode_id, newent.destid);
        nodeid_free(newent.destnode_id);
        nodeid_free(newent.homenode_id);
	return;
        }

    /* If it is for a remote node, */
    if(! nodeid_is_local_node(newent.destnode_id))
	{
	remote_new_job(newentp);
	return;
	}

    /* Inform queue display programs that there is a new job in the queue. */
    state_update("JOB %s %d %d",
            local_jobid(queuename, newent.id, newent.subid, homenode),
            rank1, rank2);

    /* If the job is ready to go, try to start it on a printer.  Otherwise,
       just leave it.
       */
    if(newent.status == STATUS_WAITING)
        printer_try_start_suitable_4_this_job(newentp);
    } /* end of queue_new_job() */

/* end of file */

