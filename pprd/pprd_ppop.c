/*
** mouse:~ppr/src/pprd/pprd_ppop.c
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
** Last modified 25 January 2002.
*/

/*
** This module handles requests from "ppop".
** The only externally callable function in this module is ppop_dispatch().
**
** You may notice that this module often calls nodeid_by_name() when it might
** seem it should call nodeid_assign() and later nodeid_free().  This is ok
** because nodeid_by_name() returns NODEID_NOTFOUND for non-existent nodes
** and NODEID_NOTFOUND won't match anything in the print queue.
**
** You may also notice that the ppop subcommand functions don't free the
** memory allocated by gu_sscanf().  That is ok too because the
** gu_sscanf_rollback() calls in ppop_dispatch() take care of that.
*/

#include "before_system.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "pprd.h"
#include "./pprd.auto_h"
#include "respond.h"
#include "util_exits.h"

static long reply_pid;		/* <-- notice that this is not pid_t! */
static FILE *reply_file;

/*=======================================================================
** List print jobs by writing them into a file.
** This if for the commands "ppop list", "ppop lpq", "ppop qquery", etc.
=======================================================================*/
static void ppop_list(const char command[])
    {
    const char *function = "ppop_list";
    char *destnode;
    int destnode_id;			/* Destination node id to match */
    char *destname;
    int destname_id;			/* Destination queue id to match */
    int id;				/* Queue job id to match */
    int subid;				/* Queue job sub id to match */
    char *homenode;
    int homenode_id;			/* Home node id to match */
    int x;
    char fname[MAX_PPR_PATH];
    int qfile;
    char buffer[1024];
    int len;
    struct stat statbuf;

    DODEBUG_PPOPINT(("%s(\"%s\")", function, command));

    /*
    ** Pull the relevent information from the command we received.
    */
    if(gu_sscanf(command, "l %S %S %d %d %S",
		&destnode,
		&destname,
		&id,
		&subid,
		&homenode) != 5)
	{
	error("%s(): invalid list command: %s", function, command);
	return;
	}

    DODEBUG_PPOPINT(("%s(): destnode=\"%s\", destname=\"%s\", id=%d, subid=%d, homenode=\"%s\"", function, destnode, destname, id, subid, homenode));

    /* It is ok if these return NODEID_UNKNOWN. */
    destnode_id = nodeid_by_name(destnode);
    homenode_id = nodeid_by_name(homenode);

    /*
    ** Convert the destination (printer or group) name into an id
    ** number.  The destination "all" is converted into the wildcard
    ** id number -1.
    */
    if(strcmp(destname, "all") == 0 || strcmp(destname, "any") == 0)
	{				/* If wildcard name, use special destination id. */
	destname_id = QUEUEID_WILDCARD;
	}
    else                                /* Otherwise, */
	{				/* get the destination id number. */
	if((destname_id = destid_by_name(destnode_id, destname)) == -1)
	    {
	    fprintf(reply_file, "%d\n", EXIT_BADDEST);
	    fprintf(reply_file, _("Destination \"%s\" does not exist.\n"), destname);
	    return;
	    }
	}


    DODEBUG_PPOPINT(("%s(): destname_id=%d, id=%d, subid=%d, homenode_id=%d", function, destname_id, id, subid, homenode_id));

    fprintf(reply_file, "%d\n", EXIT_OK_DATA);

    lock();

    for(x=0; x<queue_entries; x++)
	{
	/*
	** If we are printing all queue entries, then we print this one,
	** otherwise, it must match the destname_id and it must match the id
	** and the subid if they are non-zero.
	*/
	if( (destname_id == QUEUEID_WILDCARD || queue[x].destid == destname_id)
		   && (id == WILDCARD_JOBID || queue[x].id == id)
		   && (subid == WILDCARD_SUBID || queue[x].subid == subid)
		   && (homenode_id == NODEID_WILDCARD || queue[x].homenode_id == homenode_id) )
	    {
	    /* Open the queue file: */
	    ppr_fnamef(fname, "%s/%s:%s-%d.%d(%s)", QUEUEDIR,
		nodeid_to_name(queue[x].destnode_id),
		destid_to_name(queue[x].destnode_id, queue[x].destid),
		queue[x].id,
		queue[x].subid,
		nodeid_to_name(queue[x].homenode_id));

	    /* If job id's wrap around to the number of an
	       arrested job, ppr will accidently delete
	       some of its files.  This is a bug that needs
	       fixing, but for now, if we can't open a job's
	       queue file we will assume it has been stomped
	       on and just skip it. */
	    if((qfile = open(fname, O_RDONLY)) < 0)
		{
		error("%s(): can't open \"%s\", errno=%d (%s)", function, fname, errno, gu_strerror(errno) );
		continue;
		}

	    /*
	    ** If the job was arrested, figure out when.  The date we
	    ** use is the date of the last inode change.  This information
	    ** is used by the ppop -A option.
	    */
	    if(queue[x].status == STATUS_ARRESTED)
		fstat(qfile, &statbuf);
	    else
	    	statbuf.st_ctime = 0;

	    /*
	    ** Print a line with the information from our job array and
	    ** when the job was arrested if it was.
	    */
	    fprintf(reply_file, "%s %s %d %d %s %d %d %s %d %d %d %ld\n",
	    	nodeid_to_name(queue[x].destnode_id),
	    	destid_to_name(queue[x].destnode_id, queue[x].destid),
	    	queue[x].id,
	    	queue[x].subid,
	    	nodeid_to_name(queue[x].homenode_id),
	    	queue[x].priority,
	    	queue[x].status,
	    	queue[x].status >= 0 ? destid_to_name(queue[x].destnode_id, queue[x].status) : "?",
	    	queue[x].never,
	    	queue[x].notnow,
	    	queue[x].pass,
	    	(long)statbuf.st_ctime);

	    /*
	    ** Copy the queue file to the reply file and
	    ** append a line with a single period to
	    ** indicate the end of the reply file.
	    */
	    while((len = read(qfile, buffer, sizeof(buffer))) > 0)
	    	{
		fwrite(buffer, sizeof(char), len, reply_file);
	    	}

	    if(len == -1)
	    	fatal(0, "%s(): read() failed, errno=%d (%s)", function, errno, gu_strerror(errno) );

	    close(qfile);

	    fputs(QF_ENDTAG1, reply_file);
	    fputs(QF_ENDTAG2, reply_file);
	    }
	}

    unlock();			/* we are done with the queue */
    } /* end of ppop_list() */

/*============================================================================
** Return the status of one or more printers.
** This is acomplished with these two functions.
============================================================================*/
static void ppop_status_do_printer(FILE *outfile, int prnid)
    {
    int status = printers[prnid].status;

    /*
    ** The first line is a dump of the printers[] entry.
    */
    if(status == PRNSTATUS_PRINTING || status == PRNSTATUS_CANCELING
    		|| status == PRNSTATUS_STOPPING || status == PRNSTATUS_HALTING
    		|| status == PRNSTATUS_SEIZING)
	{
	fprintf(outfile, "%s %s %d %d %d %s %d %d %s\n",
		ppr_get_nodename(),				/* node printer is on (this node) */
		destid_to_name(nodeid_local(), prnid),		/* printer name */
		printers[prnid].status,				/* printer status */
		printers[prnid].next_error_retry + printers[prnid].next_engaged_retry,  /* retry number of next retry */
		printers[prnid].countdown,			/* seconds until next retry */
		destid_to_name(nodeid_local(), printers[prnid].jobdestid),	/* destination of job being printed */
		printers[prnid].id,
		printers[prnid].subid,
		nodeid_to_name(printers[prnid].homenode_id));
	}
    else	/* not printing */
	{
	fprintf(outfile, "%s %s %d %d %d ? 0 0 ?\n",
		ppr_get_nodename(),
		destid_to_name(nodeid_local(), prnid),
		printers[prnid].status,
		printers[prnid].status == PRNSTATUS_ENGAGED ? printers[prnid].next_engaged_retry : printers[prnid].next_error_retry,
		printers[prnid].countdown);
	}

    /*
    ** The second line and subsequent lines are the auxiliary status lines.
    ** These status lines are deposited in a file by pprdrv.
    */
    {
    char fname[MAX_PPR_PATH];
    FILE *statusfile;

    ppr_fnamef(fname, "%s/%s", STATUSDIR, destid_to_name(nodeid_local(), prnid) );
    if((statusfile = fopen(fname, "r")))
	{
	char message[MAX_STATUS_MESSAGE+1];
	while(fgets(message, sizeof(message), statusfile))
	    {
	    fputs(message, outfile);
	    }
	fclose(statusfile);
	}
    }

    /* End of record mark */
    fputs(".\n", outfile);
    } /* end of ppop_status_do_printer() */

static void ppop_status(const char command[])
    {
    const char function[] = "ppop_status";
    char *destnode, *destname;
    int x;
    int destid;

    DODEBUG_PPOPINT(("ppop_status(\"%s\")", command));

    if(gu_sscanf(command,"s %S %S", &destnode, &destname) != 2)
	{
	error("%s(): invalid \"s\" command", function);
	return;
	}

    /*
    ** Make sure that the request is for the status of
    ** a local printer.  Status requests for remote
    ** printers should be sent to the remote pprd.
    */
    if( strcmp(destnode, ppr_get_nodename()) )
    	{
	error("%s(): incorrect destnode", function);
	return;
    	}

    if(strcmp(destname, "all") == 0)		/* if "all", then step */
	{
	fprintf(reply_file, "%d\n", EXIT_OK_DATA);
	lock();
	for(x=0; x < printer_count; x++)	/* thru all the printers */
	    {
	    if(printers[x].status != PRNSTATUS_DELETED)
	    	ppop_status_do_printer(reply_file, x);
	    }
	unlock();
	}
    else if((destid = destid_local_by_name(destname)) == -1)
	{
	fprintf(reply_file, "%d\n", EXIT_BADDEST);
	fprintf(reply_file, _("The destination \"%s\" does not exist.\n"), destname);
	}
    else				/* do just that destination */
	{				/* whose id was determined in the previous clause */
	fprintf(reply_file, "%d\n", EXIT_OK_DATA);

	if(destid_local_is_group(destid))	/* be it a group */
	    {
	    int x, y;
	    x = destid_local_to_gindex(destid);	/* x is group index */
	    lock();
	    for(y=0; y < groups[x].members; y++)
		ppop_status_do_printer(reply_file, groups[x].printers[y]);
	    unlock();
	    }
	else					/* or a printer */
	    {
	    lock();
	    ppop_status_do_printer(reply_file, destid);
	    unlock();
	    }
	}

    } /* end of ppop_status() */

/*
** action 0:   ppop start	(start now)
** action 1:   ppop stop	(stop at end of current job)
** action 2:   ppop halt	(stop immediately)
** action 129: ppop wstop	(stop at end of current job, wait)
**
** There are race condition bugs in this function which could
** surface if reapchild() were allowed to interupt it!!!
*/
static void ppop_start_stop_wstop_halt(const char command[], int action)
    {
    const char function[] = "ppop_start_stop_wstop_halt";
    char *prnnode, *prnname;
    int prnid;
    int delayed_action = FALSE;

    DODEBUG_PPOPINT(("%s(\"%s\")", function, command));

    if(gu_sscanf(command+1," %S %S", &prnnode, &prnname) != 2)
	{
	error("%s(): invalid \"t\", \"p\", ,\"P\", or \"b\" command", function);
	return;
	}

    if(strcmp(prnnode, ppr_get_nodename()))
    	{
	fprintf(reply_file, "%d\n", EXIT_BADDEST);
	fprintf(reply_file, X_("Operation not yet supported for remote printers.\n"));
    	return;
    	}

    /*
    ** Get the id number of the printer.  If there is a group
    ** with the same name, we want the id of the printer.
    */
    if((prnid = destid_local_by_name_reversed(prnname)) == -1)
	{
	fprintf(reply_file, "%d\n", EXIT_BADDEST);
	fprintf(reply_file, _("The printer \"%s\" does not exist.\n"), prnname);
	return;
	}

    if(destid_local_is_group(prnid))	/* If it is a group name, */
	{			/* complain. */
	fprintf(reply_file, "%d\n", EXIT_PRNONLY);
	fprintf(reply_file, _("\"%s\" is a group of printers, not a printer.\n"), prnname);
	return;
	}

    lock();				/* lock the printers database */

    if( (action&3) == 0 )		/* If should start the printer */
	{				/* (action 0), */
	switch(printers[prnid].status)	/* act according to current */
	    {				/* printer state. */
	    case PRNSTATUS_IDLE:
	    case PRNSTATUS_CANCELING:
	    case PRNSTATUS_SEIZING:
	    case PRNSTATUS_ENGAGED:
	    case PRNSTATUS_STARVED:
		fprintf(reply_file, "%d\n", EXIT_ALREADY);
		fprintf(reply_file, _("The printer \"%s\" is not stopt.\n"), prnname);
		break;

	    case PRNSTATUS_PRINTING:
		fprintf(reply_file, "%d\n", EXIT_ALREADY);
		fprintf(reply_file, _("The printer \"%s\" is already printing.\n"), prnname);
		break;

	    case PRNSTATUS_FAULT:
	    case PRNSTATUS_STOPT:
		fprintf(reply_file, "%d\n", EXIT_OK);
		printer_new_status(&printers[prnid], PRNSTATUS_IDLE);
		media_startstop_update_waitreason(prnid);
		printer_look_for_work(prnid);
		break;

	    case PRNSTATUS_HALTING:
		fprintf(reply_file, "%d\n", EXIT_NOTPOSSIBLE);
		fprintf(reply_file, _("There is an outstanding halt command for the printer \"%s\".\n"), prnname);
		break;

	    case PRNSTATUS_STOPPING:
		fprintf(reply_file, "%d\n", EXIT_OK);
		if(printers[prnid].ppop_pid > 0)		/* If another ppop waiting for a stop command */
		    {						/* to finish, let it wait no longer. */
		    kill(printers[prnid].ppop_pid, SIGUSR1);
		    printers[prnid].ppop_pid = (uid_t)0;	/* (Unfortunately, it won't know it failed.) */
		    }
		printer_new_status(&printers[prnid], PRNSTATUS_PRINTING);
		media_startstop_update_waitreason(prnid);
		break;

	    default:
		fprintf(reply_file, "%d\n", EXIT_INTERNAL);
		fprintf(reply_file, _("Internal pprd error: printer \"%s\" is in an undefined state.\n"), prnname);
		break;
	    } /* end of switch() */
	}
    else			/* action = 1 or 2, stop or halt */
	{			/* stop may be with or without wait (128) */
	switch(printers[prnid].status)
	    {
	    case PRNSTATUS_FAULT:   /* If not printing now, */
	    case PRNSTATUS_IDLE:    /* we may go directly to stopt. */
	    case PRNSTATUS_ENGAGED:
	    case PRNSTATUS_STARVED:
		fprintf(reply_file, "%d\n", EXIT_OK);
		printer_new_status(&printers[prnid], PRNSTATUS_STOPT);
		media_startstop_update_waitreason(prnid);
		break;

	    case PRNSTATUS_HALTING:
		fprintf(reply_file, "%d\n", EXIT_ALREADY);
		fprintf(reply_file, _("The printer \"%s\" is already halting.\n"), prnname);
		break;

	    case PRNSTATUS_STOPT:
		fprintf(reply_file, "%d\n", EXIT_ALREADY);
		fprintf(reply_file, _("The printer \"%s\" is already stopt.\n"), prnname);
		break;

	    case PRNSTATUS_CANCELING:		/* if pprdrv already sent kill signal */
	    case PRNSTATUS_SEIZING:
		printer_new_status(&printers[prnid], PRNSTATUS_HALTING);
		media_startstop_update_waitreason(prnid);
		delayed_action = TRUE;		/* say we must wait */
		break;

	    case PRNSTATUS_STOPPING:
		if( (action&3) == 1 )	/* if "ppop stop" */
		    {			/* we can't do better than "stopping" */
		    fprintf(reply_file, "%d\n", EXIT_ALREADY);
		    fprintf(reply_file, _("The printer \"%s\" is already stopping.\n"), prnname);
		    break;
		    }			/* if halt, drop thru */

	    case PRNSTATUS_PRINTING:
		if( (action&3) == 2 )		/* if halt, not stop (i.e., don't wait for job to finish) */
		    {				/* change state to halting */
		    printer_new_status(&printers[prnid], PRNSTATUS_HALTING);
		    pprdrv_kill(prnid);
		    }
		else
		    {				/* if stop requested, arrange to stop */
		    printer_new_status(&printers[prnid], PRNSTATUS_STOPPING);
		    }				/* at the end of this job */
		media_startstop_update_waitreason(prnid);
		delayed_action = TRUE;		/* either way, we must wait */
		break;

	    default:
		fprintf(reply_file, "%d\n", EXIT_INTERNAL);
		fprintf(reply_file, _("Internal pprd error: printer \"%s\" is in an undefined state.\n"), prnname);
		break;
	    }
	}

    /* If action was not immedate */
    if(delayed_action)
	{
	if(action & 128)			/* If we are instructed to wait if */
	    {					/* for completion of action and */
	    if(printers[prnid].ppop_pid)	/* if someone else is */
		{				/* using notify, we can't */
		fprintf(reply_file, "%d\n", EXIT_CANTWAIT);
		fprintf(reply_file, _("Another process is already using the notify\n"
				"feature for the printer \"%s\".\n"), prnname);
		}
	    else				/* If no one else waiting, */
		{
		fprintf(reply_file, "%d\n", EXIT_OK);	/* put message in reply file */
		fclose(reply_file);			/* and close without notifing ppop */
		reply_file = (FILE*)NULL;
		printers[prnid].ppop_pid = (pid_t)reply_pid;	/* and register for future notification. */
		}
	    }
	else			/* if no wait, signal now */
	    {
	    fprintf(reply_file, "%d\n", EXIT_OK);
	    }
	}

    unlock();			/* allow other things to hapen again */
    } /* end of ppop_start_stop_wstop_halt_printer() */

/*
** hold or release a print job
** action=0 means hold
** action=1 means release
*/
static void ppop_hold_release(const char command[], int action)
    {
    const char *function = "ppop_hold_release";
    char *destnode, *destname, *homenode;
    int id, subid;
    int destnode_id, destid, homenode_id;
    int x;

    DODEBUG_PPOPINT(("%s(\"%s\")", function, command));

    if(gu_sscanf(command+1, " %S %S %d %d %S", &destnode, &destname, &id, &subid, &homenode) != 5)
	{
	error("%s(): invalid \"h\", or \"r\" command", function);
	return;
	}

    /* Note that these may return NODEID_NOTFOUND.  That is fine
       since it won't match anything.
       */
    destnode_id = nodeid_by_name(destnode);
    homenode_id = nodeid_by_name(homenode);

    if((destid = destid_by_name(destnode_id, destname)) == -1)	/* if lookup fails, */
	{					/* say destination bad */
	fprintf(reply_file, "%d\n", EXIT_BADDEST);
	fprintf(reply_file, _("The destination \"%s\" does not exist.\n"), destname);
	return;
	}

    DODEBUG_PPOPINT(("%s(): %sing jobs matching destnode_id=%d, destid=%d, id=%d, subid=%d, homenode_id=%d", function, (action ? "releas" : "hold"), destnode_id, destid, id, subid, homenode_id));

    lock();					/* lock the queue */

    for(x=0; x < queue_entries; x++)		/* and search it */
        {
	DODEBUG_PPOPINT(("%s(): considering destnode_id=%d, destid=%d, id=%d, subid=%d, homenode_id=%d", function, queue[x].destnode_id, queue[x].destid, queue[x].id, queue[x].subid, queue[x].homenode_id));

        if(queue[x].destid == destid
                    && queue[x].id == id
                    && (subid == WILDCARD_SUBID || queue[x].subid == subid)
                    && (destnode_id == NODEID_WILDCARD || queue[x].destnode_id == destnode_id)
                    && (homenode_id == NODEID_WILDCARD || queue[x].homenode_id == homenode_id)
            )
            {
            if(action==0)                   /* if we should hold the job */
                {
                switch(queue[x].status)
                    {
                    case STATUS_WAITING:	/* if not printing, */
                    case STATUS_WAITING4MEDIA:	/* just quitely go to `hold' */
                        fprintf(reply_file, "%d\n", EXIT_OK);
                        queue_p_job_new_status(&queue[x], STATUS_HELD);
                        break;
                    case STATUS_HELD:		/* if already held, say so */
                        fprintf(reply_file, "%d\n", EXIT_ALREADY);
                        fprintf(reply_file, _("The print job \"%s\" is already held.\n"), remote_jobid(destnode,destname,id,subid,homenode));
                        break;
                    case STATUS_ARRESTED:	/* can't hold an arrested job */
                        fprintf(reply_file, "%d\n", EXIT_ALREADY);
                        fprintf(reply_file, _("The print job \"%s\" is arrested.\n"), remote_jobid(destnode,destname,id,subid,homenode));
                        break;
                    case STATUS_SEIZING:	/* already going to held */
                        fprintf(reply_file, "%d\n", EXIT_ALREADY);
                        fprintf(reply_file,
				_("The print job \"%s\" is already undergoing a\n"
				"transition to the held state.\n"),
				remote_jobid(destnode,destname,id,subid,homenode));
                        break;
                    case STATUS_CANCEL:		/* if being canceled, hijack the operation */
                        fprintf(reply_file, "%d\n", EXIT_OK);
                        fprintf(reply_file,
				_("Converting outstanding cancel order for\n"
				"job \"%s\" to a hold order.\n"),
				remote_jobid(destnode,destname,id,subid,homenode));
                        printer_new_status(&printers[queue[x].status], PRNSTATUS_SEIZING);
                        printers[queue[x].status].cancel_job = FALSE;
                        printers[queue[x].status].hold_job = TRUE;
                        queue_p_job_new_status(&queue[x], STATUS_SEIZING);
                        break;
                    default:                        /* printing? */
                        if(queue[x].status >= 0)
                            {
                            int prnid = queue[x].status;

                            fprintf(reply_file, "%d\n", EXIT_OK);
                            fprintf(reply_file,
                                    _("Seizing job \"%s\" which is printing on \"%s\".\n"),
                                    remote_jobid(destnode,destname,id,subid,homenode),
                                    destid_to_name(nodeid_local(), prnid));

                            queue_p_job_new_status(&queue[x], STATUS_SEIZING);
                            printer_new_status(&printers[prnid], PRNSTATUS_SEIZING);
                            printers[prnid].hold_job = TRUE;

                            DODEBUG_PPOPINT(("killing pprdrv (printer=%s, pid=%ld)", destid_local_to_name(prnid), (long)printers[prnid].pid));
			    if(printers[prnid].pid <= 0)
			    	{
			    	error("%s(): assertion failed, printers[%d].pid = %ld", function, prnid, (long)printers[prnid].pid);
			    	}
			    else
				{
				pprdrv_kill(prnid);
                                }
                            }
                        else
                            {
                            fprintf(reply_file, "%d\n", EXIT_INTERNAL);
                            fprintf(reply_file,
                                    _("Internal pprd error: job \"%s\" has unknown status %d.\n"),
                                    remote_jobid(destnode,destname,id,subid,homenode),
                                    queue[x].status);
                            }
                        break;
                    }
                }
            else                            /* action: release a job */
                {
                switch(queue[x].status)
                    {
                    case STATUS_HELD:       /* "held" or "arrested" jobs */
                    case STATUS_ARRESTED:   /* may be made "waiting" */
                        fprintf(reply_file, "%d\n", EXIT_OK);
                        queue_p_job_new_status(&queue[x], STATUS_WAITING);
                        if(nodeid_is_local_node(queue[x].destnode_id))
                            {
                            media_set_notnow_for_job(&queue[x], TRUE);
                            if(queue[x].status == STATUS_WAITING)
                                printer_try_start_suitable_4_this_job(&queue[x]);
                            }
                        break;
                    case STATUS_SEIZING:
                        fprintf(reply_file, "%d\n", EXIT_ALREADY);
                        fprintf(reply_file,
                            _("The print job \"%s\" can't be released until an\n"
                            "outstanding hold order has been fully executed.\n"),
                            remote_jobid(destnode,destname,id,subid,homenode));
                        break;
                    case STATUS_WAITING:
                    case STATUS_WAITING4MEDIA:
                    default:                        /* printing */
                        fprintf(reply_file, "%d\n", EXIT_ALREADY);
                        fprintf(reply_file,
                            _("The print job \"%s\" is not being held.\n"),
                            remote_jobid(destnode,destname,id,subid,homenode));
                        break;
                    }
                }
            break;
            }
        }

    unlock();

    if(x == queue_entries)  /* If ran off end of queue, */
        {
        fprintf(reply_file, "%d\n", EXIT_BADJOB);
        fprintf(reply_file, _("The print job \"%s\" does not exist.\n"), remote_jobid(destnode,destname,id,subid,homenode));
        }

    } /* end of ppop_hold_release() */

/*
** ppop cancel
** ppop purge
**
** This command is used to cancel a job or all jobs queued for a
** destination.  If all jobs are to be canceled, then the id
** will be -1.  If the subid was not specified, it will be -1
** and all subjobs will be canceled.
*/
static void ppop_cancel_purge(const char command[])
    {
    const char function[] = "ppop_cancel_purge";
    char *destnode, *destname, *homenode;
    int destnode_id, destid, id, subid, homenode_id;
    gu_boolean inform;			/* should the user be notified? */
    int prnid;				/* temporary printer id */
    int canceled_count = 0;		/* number of jobs canceled */

    DODEBUG_PPOPINT(("%s(\"%s\")", function, command));

    if(gu_sscanf(command, "c %S %S %d %d %S %d", &destnode, &destname, &id, &subid, &homenode, &inform) != 6)
	{
	error("%s(): invalid command: %s", function, command);
	return;
	}

    /* The result may be NODEID_NOTFOUND.  That is ok. */
    destnode_id = nodeid_by_name(destnode);
    homenode_id = nodeid_by_name(homenode);

    if((destid = destid_by_name(destnode_id, destname)) == -1 && strcmp(destname, "all") && strcmp(destname, "any"))
	{
	fprintf(reply_file, "%d\n", EXIT_BADDEST);
	fprintf(reply_file, _("The destination \"%s\" does not exist.\n"), destname);
	}
    else                        /* printer or group exists */
	{
	int x;			/* index into queue array */

	DODEBUG_PPOPINT(("%s(): canceling jobs matching destnode_id=%d, destid=%d, id=%d, subid=%d, homenode_id=%d", function, destnode_id, destid, id, subid, homenode_id));

	lock();

	/* Search the whole queue */
	for(x=0; x < queue_entries; x++)
	    {
	    DODEBUG_PPOPINT(("%s(): considering destnode_id=%d, destid=%d, id=%d, subid=%d, homenode_id=%d", function, queue[x].destnode_id, queue[x].destid, queue[x].id, queue[x].subid, queue[x].homenode_id));

	    /* If this job matches, */
	    if( (destid == QUEUEID_WILDCARD || queue[x].destid == destid)
			&& (id == WILDCARD_JOBID || queue[x].id == id)
			&& (subid == WILDCARD_SUBID || queue[x].subid == subid)
			&& (destnode_id == NODEID_WILDCARD || queue[x].destnode_id == destnode_id)
			&& (homenode_id == NODEID_WILDCARD || queue[x].homenode_id == homenode_id) )
		{
		canceled_count++;

		/* If the job is being printed, */
		if((prnid = queue[x].status) >= 0)
		    {
		    /* If it is printing we can say it is now canceling, but
		       if it is halting or stopping we don't want to mess with
		       that.
		       */
		    if(printers[prnid].status == PRNSTATUS_PRINTING)
			printer_new_status(&printers[prnid], PRNSTATUS_CANCELING);

		    /* Set flag so that job will die when pprdrv dies. */
		    printers[prnid].cancel_job = TRUE;

		    /* Change the job status to "being canceled". */
		    queue_p_job_new_status(&queue[x], STATUS_CANCEL);

		    /* Kill pprdrv. */
		    pprdrv_kill(prnid);
		    }

		/* If a cancel is in progress, */
		else if(prnid == STATUS_CANCEL)
		    {
		    /* nothing to do */
		    }

		/* If a hold is in progress, do what we do if the job is being
		   printed, but without the need to kill() pprdrv again.  This
		   is tough because the queue doesn't contain the printer
		   id anymore. */
		else if(prnid == STATUS_SEIZING)
		    {
		    for(prnid = 0; prnid < printer_count; prnid++)
		    	{
			if(printers[prnid].jobdestid == queue[x].destid
					&& printers[prnid].id == queue[x].id
					&& printers[prnid].subid == queue[x].subid
					&& printers[prnid].homenode_id == queue[x].homenode_id)
			    {
                            if(printers[prnid].status == PRNSTATUS_SEIZING)
                                printer_new_status(&printers[prnid], PRNSTATUS_CANCELING);
                            printers[prnid].hold_job = FALSE;
                            printers[prnid].cancel_job = TRUE;
                            queue_p_job_new_status(&queue[x], STATUS_CANCEL);
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
		    ** If we have not been instructed not to inform the user and this job is not arrested,
		    ** use the responder to inform the user that we are canceling it.
		    */
		    if(inform && queue[x].status != STATUS_ARRESTED)
			{
	                respond(queue[x].destnode_id, queue[x].destid, queue[x].id, queue[x].subid, queue[x].homenode_id,
				-1,   /* impossible printer */
				RESP_CANCELED);
			}

		    /* Remove the job from the queue array and its files form the spool directories. */
		    queue_dequeue_job(queue[x].destnode_id, queue[x].destid, queue[x].id, queue[x].subid, queue[x].homenode_id);

		    x--;        /* compensate for deletion */
		    }
		}
	    }

	unlock();

	if(canceled_count == 0 && id != -1 && subid !=- 1 ) 	/* if no match, and no wildcard used, */
	    {
	    fprintf(reply_file, "%d\n", EXIT_BADJOB);
	    fprintf(reply_file, _("The print job \"%s\" does not exist.\n"), remote_jobid(nodeid_to_name(destnode_id),destname,id,subid,nodeid_to_name(homenode_id)) );
	    }
	else
	    {
	    fprintf(reply_file, "%d\n", EXIT_OK_DATA);
	    fprintf(reply_file, "%d\n", canceled_count);
	    }
	} /* end if else printer or group exists */
    } /* end of ppop_cancel_purge() */

/*
** List media currently mounted on a printer or printers.
*/
static void ppop_media(const char command[])
    {
    FUNCTION4DEBUG("ppop_media")
    char *destnode, *destname;
    int destnode_id, destid;
    struct fcommand1 f1;
    struct fcommand2 f2;
    int x, y;
    int group = FALSE;

    #ifdef DEBUG_PPOPINT
    debug("%s(\"%s\")", function, command);
    #endif

    if(gu_sscanf(command, "f %S %S", &destnode, &destname) != 2)
	{
	error("ppop_media(): invalid \"f\" command");
	return;
	}

    destnode_id = nodeid_by_name(destnode);

    /* set all, group, and destid */
    if(strcmp(destname, "all") == 0)
	{
	destid = -1;
	}
    else
	{
	if((destid = destid_by_name(destnode_id, destname)) == -1)
	    {
	    fprintf(reply_file, "%d\n", EXIT_BADDEST);
	    fprintf(reply_file, _("The destination \"%s\" does not exist.\n"), destname);
	    return;
	    }
	if(destid_local_is_group(destid))
	    group = TRUE;
	}

    fprintf(reply_file, "%d\n", EXIT_OK_DATA);

    for(x=0; x < printer_count; x++)
	{
	if(destid == -1 || x==destid || (group && destid_local_get_member_offset(destid,x)!=-1) )
	    {
	    f1.nbins = printers[x].nbins;
	    strcpy(f1.prnname, printers[x].name);
	    fwrite(&f1, sizeof(struct fcommand1), 1, reply_file);
	    for(y=0; y < printers[x].nbins; y++)
		{
		DODEBUG_MEDIA(("x=%d, y=%d, media=%d",x,y,printers[x].media[y]));
		strcpy(f2.bin, get_bin_name(printers[x].bins[y]) );
		strcpy(f2.media, get_media_name(printers[x].media[y]));
		fwrite(&f2, sizeof(struct fcommand2), 1, reply_file);
		}
	    }
	} /* end of for(x....) */

    } /* end of ppop_media() */

/*
** Mount a new medium on a specified bin.
*/
static void ppop_mount(const char command[])
    {
    const char function[] = "ppop_mount";
    char *prnnode, *printer;
    int prnnode_id, printer_id;
    char binname[MAX_BINNAME+1];
    char medianame[MAX_MEDIANAME+1];
    int binid;                      /* bin id number of specified bin */
    int x;

    DODEBUG_PPOPINT(("%s(\"%s\")", function, command));

    /* Parse the command we received over the pipe. */
    medianame[0] = '\0';
    if(gu_sscanf(command,"M %S %S %#s %#s",
                        &prnnode,
                        &printer,
                        sizeof(binname), binname,
                        sizeof(medianame), medianame) < 3)
        {
        error("%s(): invalid \"M\" command", function);
        return;
        }

    prnnode_id = nodeid_by_name(prnnode);

    if(!nodeid_is_local_node(prnnode_id))
    	{
	fprintf(reply_file, "%d\n", EXIT_BADDEST);
	fprintf(reply_file, X_("Operation not yet supported for remote printers.\n"));
	return;
    	}

    /*
    ** Get the id of the printer.  If there is a group with the
    ** same name, we want the id of the printer, not of the group.
    */
    if((printer_id = destid_local_by_name_reversed(printer))==-1)
        {
        fprintf(reply_file, "%d\n", EXIT_BADDEST);
        fprintf(reply_file, _("Printer \"%s\" does not exist.\n"), printer);
        return;
        }

    if(destid_local_is_group(printer_id))
        {
        fprintf(reply_file, "%d\n", EXIT_PRNONLY);
        fprintf(reply_file, _("\"%s\" is not a printer.\n"), printer);
        return;
        }

    binid = get_bin_id(binname);

    /* find the bin in question */
    for(x=0; printers[printer_id].bins[x] != binid; x++)
        {
        if(x == printers[printer_id].nbins)	/* if bin not found */
            {
            fprintf(reply_file, "%d\n", EXIT_BADBIN);
            fprintf(reply_file, _("\"%s\" does not have a bin called \"%s\".\n"), printer, binname);
            return;
            }
        }

    printers[printer_id].media[x] = get_media_id(medianame);

    fprintf(reply_file, "%d\n", EXIT_OK);

    /* update list of printable jobs */
    media_update_notnow(printer_id);

    /* if printer idle,
       see if anything to do (needn't lock). */
    if(printers[printer_id].status == PRNSTATUS_IDLE)
        printer_look_for_work(printer_id);

    media_mounted_save(printer_id);		/* save for restart */

    /* Let the queue display programs know about the change. */
    state_update("MOUNT %s %s %s", printer, binname, medianame);
    } /* end of ppop_mount() */

/*
** Set a printer or group to accept or reject print jobs.
** action=0 means reject
** action=1 means accept
*/
static void ppop_accept_reject(const char command[], int action)
    {
    const char function[] = "ppop_accept_reject";
    char *destnode, *destname;
    int destnode_id, destid;
    char conffile[MAX_PPR_PATH];
    struct stat st;

    #ifdef DEBUG_PPOPINT
    debug("%s(command=\"%s\", action=%d)", function, command, action);
    #endif

    if(gu_sscanf(&command[1], " %S %S", &destnode, &destname) != 2)
	{
	error("%s(): invalid \"A\" or \"R\" command", function);
	return;
	}

    destnode_id = nodeid_by_name(destnode);

    if(!nodeid_is_local_node(destnode_id))
    	{
	fprintf(reply_file, "%d\n", EXIT_BADDEST);
	fprintf(reply_file, X_("Operation not yet supported for remote queues.\n"));
	return;
    	}

    /*
    ** If there is both a printer and a group with the specified
    ** name, we will operate on the group.  This is probably the
    ** best thing to do because ppr(1) will attempt to submit to
    ** the group, not the printer.
    */
    if((destid = destid_by_name(destnode_id, destname)) == -1)
	{
	fprintf(reply_file, "%d\n", EXIT_BADDEST);
	fprintf(reply_file, _("\"%s\" is not a valid destination.\n"), destname);
	return;
	}

    if(nodeid_is_local_node(destnode_id))
	{
        if(!destid_local_is_group(destid))
            {                               /* printer: */
            printers[destid].accepting = action;
            ppr_fnamef(conffile, "%s/%s", PRCONF, destname);
            stat(conffile, &st);            /* modify group execute bit */
            }
        else                                /* group: */
            {
            groups[destid_local_to_gindex(destid)].accepting = action;
            ppr_fnamef(conffile, "%s/%s", GRCONF, destname);
            stat(conffile, &st);            /* modify group execute bit */
            }

        if(action==0)                       /* reject */
            chmod(conffile, st.st_mode | S_IXGRP );
        else                                /* accept */
            chmod(conffile, st.st_mode & (0777 ^ S_IXGRP) );
	}

    fprintf(reply_file, "%d\n", EXIT_OK);
    } /* end of ppop_accept_reject() */

/*
** Show destinations, their type, and whether or not they are accepting.
*/
static void ppop_dest_show_printer(FILE *outfile, int prnid)
    {
    fprintf(outfile,"%s 0 %d %d\n",printers[prnid].name,printers[prnid].accepting,printers[prnid].protect);
    }

static void ppop_dest_show_group(FILE *outfile, int groupnum)
    {
    fprintf(outfile,"%s 1 %d %d\n",groups[groupnum].name,groups[groupnum].accepting,groups[groupnum].protect);
    }

static void ppop_dest(const char command[])
    {
    const char function[] = "ppop_dest";
    char *destnode, *destname;
    int destnode_id, destid;
    int x;

    DODEBUG_PPOPINT(("%s(\"%s\")", function, command));

    if(gu_sscanf(&command[1], " %S %S", &destnode, &destname) != 2)
	{
	error("%s(): invalid \"D\" command", function);
	return;
	}

    destnode_id = nodeid_by_name(destnode);

    if(!nodeid_is_local_node(destnode_id))
    	{
	fprintf(reply_file, "%d\n", EXIT_BADDEST);
	fprintf(reply_file, X_("Operation not yet supported for remote queues.\n"));
	return;
    	}

    if(strcmp(destname, "all") == 0)
	{
	fprintf(reply_file, "%d\n", EXIT_OK_DATA);

	for(x=0; x < printer_count; x++)
	    {
	    if(printers[x].status != PRNSTATUS_DELETED)
		ppop_dest_show_printer(reply_file, x);
	    }

	for(x=0; x < group_count; x++)
	    {
	    if( ! groups[x].deleted )
		ppop_dest_show_group(reply_file, x);
	    }
	}
    else	/* a specific destination, */
	{
	if((destid = destid_by_name(destnode_id, destname)) == -1)
	    {
	    fprintf(reply_file, "%d\n", EXIT_BADDEST);
	    fprintf(reply_file, _("\"%s\" is not a valid destination.\n"), destname);
	    return;
	    }

	fprintf(reply_file, "%d\n", EXIT_OK_DATA);

	if(destid_local_is_group(destid))
	    ppop_dest_show_group(reply_file, destid_local_to_gindex(destid));
	else
	    ppop_dest_show_printer(reply_file, destid);
	}

    } /* end of ppop_dest() */

/*
** Move jobs from one queue to another.
*/
static void ppop_move(const char command[])
    {
    const char function[] = "ppop_move";
    char *destnode, *destname, *homenode;
    int destnode_id, destid, id, subid, homenode_id;
    char *new_destnode, *new_destname;
    char oldname[MAX_PPR_PATH];
    char newname[MAX_PPR_PATH];
    int x;
    int moved=0;                        /* count of files moved */
    int printing=0;                     /* not moved because printing */
    FILE *logfile;                      /* used to write to log */
    int rank2;				/* rank amoung jobs for new destination */
    int new_destnode_id, new_destname_id;

    DODEBUG_PPOPINT(("ppop_move(\"%s\")", command));

    if(gu_sscanf(command, "m %S %S %d %d %S %S %S", &destnode, &destname, &id, &subid, &homenode, &new_destnode, &new_destname) != 7)
	{
	error("%s(): invalid \"m\" command", function);
	return;
	}

    destnode_id = nodeid_by_name(destnode);
    homenode_id = nodeid_by_name(homenode);

    if((destid = destid_by_name(destnode_id, destname)) == -1)
	{                               /* get id of source */
	fprintf(reply_file, "%d\n", EXIT_BADDEST);
	fprintf(reply_file, _("No printer or group is called \"%s\".\n"), network_destspec(destnode, destname));
	return;
	}

    /* We must assign these because they must exist. */
    new_destnode_id = nodeid_assign(new_destnode);
    if((new_destname_id = destid_assign(new_destnode_id, new_destname)) == -1)
	{                               /* get id of destination */
	fprintf(reply_file, "%d\n", EXIT_BADDEST);
	fprintf(reply_file, _("No printer or group is called \"%s\".\n"), network_destspec(new_destnode, new_destname));
	nodeid_free(new_destnode_id);
	return;
	}

    lock();				/* lock the queue array */

    for(rank2=x=0; x < queue_entries; x++)
	{
	if( queue[x].destid == destid	/* if match, */
		&& (id == WILDCARD_JOBID || queue[x].id == id)
		&& (subid == WILDCARD_SUBID || queue[x].subid == subid)
		&& (destnode_id == NODEID_WILDCARD || queue[x].destnode_id == destnode_id)
		&& (homenode_id == NODEID_WILDCARD || queue[x].homenode_id == homenode_id) )
	    {
	    struct QEntry *q = &queue[x];

	    if(q->status >= 0)			/* If it is printing, we */
		{				/* can't move it. */
		printing++;
		continue;
		}

	    /*
	    ** If this is a real move an not just an attempt to reset the
	    ** "never" flags,
	    */
	    if(q->destid != new_destname_id || q->destnode_id != new_destnode_id)
		{
	        /* Inform queue monitoring programs of the move. */
		state_update("MOV %s %s %d",
			remote_jobid(nodeid_to_name(q->destnode_id), destid_to_name(q->destnode_id, q->destid), q->id, q->subid, nodeid_to_name(q->homenode_id)),
			network_destspec(new_destnode, new_destname),
			rank2++);

                /* Rename all the queue file. */
                ppr_fnamef(oldname,"%s/%s:%s-%d.%d(%s)", QUEUEDIR,
                    nodeid_to_name(q->destnode_id),destid_to_name(q->destnode_id,q->destid),q->id,q->subid,nodeid_to_name(q->homenode_id));
                ppr_fnamef(newname,"%s/%s:%s-%d.%d(%s)", QUEUEDIR,
                    nodeid_to_name(q->destnode_id),new_destname,q->id,q->subid,nodeid_to_name(q->homenode_id));
                rename(oldname, newname);

		/* Rename all of the data files. */
                {
                char *list[] = {"comments", "pages", "text", "log", "infile", "barbar", NULL};
                int x;
                for(x=0; list[x]; x++)
                    {
                    ppr_fnamef(oldname,"%s/%s:%s-%d.%d(%s)-%s", DATADIR,
                        nodeid_to_name(q->destnode_id),
                                    destid_to_name(q->destnode_id, q->destid),
                                    q->id, q->subid,
                                    nodeid_to_name(q->homenode_id),
                        list[x]);
                    ppr_fnamef(newname,"%s/%s:%s-%d.%d(%s)-%s", DATADIR,
                        nodeid_to_name(q->destnode_id),
                                    new_destname,
                                    q->id, q->subid,
                                    nodeid_to_name(q->homenode_id),
                        list[x]);
                    rename(oldname, newname);
                    }
		}

                /*
                ** Fix the reference counts.  Order may matter here as correct
                ** order may prevent reference counts from going to 0 and back
                ** to 1 (resulting in unnecessary destruction and re-creation).
                ** However, the fact that we have 1 of each assigned that we
                ** are going to throw away at the end of the functions probably
                ** means that this fear is groundless.
                */
		if(nodeid_assign(new_destnode) != new_destnode_id)
		    fatal(0, "%s(): assertion failed", function);
                if(destid_assign(new_destnode_id, new_destname) != new_destname_id)
                    fatal(0, "%s(): assertion failed", function);
                destid_free(q->destnode_id, q->destid);
                nodeid_free(q->destnode_id);

                /*
                ** Change the destination id in the queue array.  This must come
                ** after the rename code or the rename code will break.
                */
                queue[x].destnode_id = new_destnode_id;
                queue[x].destid = new_destname_id;
		}

	    /*
	    ** In the job's log file, make a note of the fact
	    ** that it was moved from one destination to another.
	    */
	    if((logfile = fopen(newname,"a")))
		{
		fprintf(logfile,
		    "Job moved from destination \"%s\" to \"%s\".\n",
		    destname, new_destname);
		fclose(logfile);
		}

	    /*
	    ** If this job was stranded, maybe it will print here.
	    */
	    if(queue[x].status == STATUS_STRANDED)
	    	{
		queue_p_job_new_status(&queue[x], STATUS_WAITING);
	    	}

	    /*
	    ** Clear any "never" (printer unsuitable) flags set new
	    ** "notnow" (required media not present) flags and update the
	    ** job status.
	    **
	    ** If the result is that the job is ready to print, try to start
	    ** a printer for it.
	    */
	    queue[x].never = 0;
	    if(nodeid_is_local_node(destnode_id))
		{
                media_set_notnow_for_job(&queue[x], TRUE);
                if(queue[x].status == STATUS_WAITING)
                    printer_try_start_suitable_4_this_job(&queue[x]);
		}
	    else
	    	{
	    	q->notnow = 0;
	    	}

	    moved++;				/* increment count */
	    }

	/*
	** We don't have to move this job, but do we have to count it as a job
	** that is ahead of our job in the destination queue?
	*/
	else if(queue[x].destid == new_destname_id)
	    {
	    rank2++;
	    }
	}

    unlock(); 			/* we are done modifying the queue array */

    /* Tell how many files were moved. */
    switch(moved)
	{
	case 0:
	    if(id == -1)		/* If we tried to move all files on destination, */
		{
		fprintf(reply_file, "%d\n", EXIT_BADJOB);
		fprintf(reply_file, _("No jobs are queued for \"%s\".\n"), destname);
		}
	    else if(printing > 0)	/* If files not moved because they were printing, */
		{
		fprintf(reply_file, "%d\n", EXIT_OK);	/* Is this right? !!! */
		}
	    else			/* If no matching file, */
		{
		fprintf(reply_file, "%d\n", EXIT_BADJOB);
		fprintf(reply_file, _("Job \"%s\" does not exist.\n"), local_jobid(destname,id,subid,nodeid_to_name(homenode_id)));
		}
	    break;
	case 1:
	    fprintf(reply_file, "%d\n", EXIT_OK);
	    fprintf(reply_file, _("1 file was moved.\n"));
	    break;
	default:
	    fprintf(reply_file, "%d\n", EXIT_OK);
	    fprintf(reply_file, _("%d files where moved.\n"), moved);
	    break;
	}

    /* Say how many files wern't moved because they were being printed. */
    if(printing > 0)
    	{
	if(printing == 1)
	    fprintf(reply_file, _("1 file was not moved because it was being printed.\n"));
	else
	    fprintf(reply_file, _("%d files were not moved because they were being printed.\n"), printing);
	}

    destid_free(new_destnode_id, new_destname_id);
    nodeid_free(new_destnode_id);
    } /* end of ppop_move() */

/*
** Rush a job.
*/
static void ppop_rush(const char command[])
    {
    const char function[] = "ppop_rush";
    char *destnode, *destname, *homenode;
    int destnode_id, destid, id, subid, homenode_id;
    struct QEntry t;                    /* temp storage for queue entry */
    int x;
    int newpos;

    DODEBUG_PPOPINT(("%s(\"%s\")", function, command));

    /* Parse the command we received over the pipe. */
    if(gu_sscanf(command, "U %S %S %d %d %S %d", &destnode, &destname, &id, &subid, &homenode, &newpos) != 6)
	{
	error("%s(): invalid \"U\" command", function);
	return;
	}

    destnode_id = nodeid_by_name(destnode);
    homenode_id = nodeid_by_name(homenode);

    /*
    ** Look up the destination id for the destination name.
    ** If the lookup fails it return -1 indicating that the
    ** destination in question does not exist.
    */
    if((destid = destid_by_name(destnode_id, destname)) == -1)
	{
	fprintf(reply_file, "%d\n", EXIT_BADDEST);
	fprintf(reply_file, _("No printer or group is called \"%s\".\n"), destname);
	return;
	}

    lock();                             /* exclusive right to modify queue */

    for(x=0; x<queue_entries; x++)	/* Examine the whole */
	{				/* queue if we must. */
	if(queue[x].destid == destid	/* If we have a match, */
		&& queue[x].id == id
		&& (subid == WILDCARD_SUBID || queue[x].subid == subid)
		&& (destnode_id == NODEID_WILDCARD || queue[x].destnode_id == destnode_id)
		&& (homenode_id == NODEID_WILDCARD || queue[x].homenode_id == homenode_id))
	    {                           /* save the matching entry, */
	    fprintf(reply_file, "%d\n", EXIT_OK);

	    /* Inform queue display programs. */
	    state_update("RSH %s", local_jobid(destname,queue[x].id,queue[x].subid,nodeid_to_name(queue[x].homenode_id)));

	    memcpy(&t, &queue[x], sizeof(struct QEntry));
	    if(newpos == 0)
                {
		/* rush job */
		while(x)                    /* and slide the others up. */
		    {
		    memcpy(&queue[x], &queue[x-1], sizeof(struct QEntry));
		    x--;
		    }
	        memcpy(&queue[0], &t, sizeof(struct QEntry));
	        queue[0].priority = 0;	/* highest priority */
		}
            else
	        {
                /* last job */
                while(x<(queue_entries-1))   /* and slide the others down */
	 	    {
		    memcpy(&queue[x],&queue[x+1],sizeof(struct QEntry));
		    x++;
                    }
		memcpy(&queue[x],&t,sizeof(struct QEntry));
                }
	    break;
	    }
	}

    unlock();                           /* done with queue */

    if(x==queue_entries)                /* if ran to end with no match */
	{
	fprintf(reply_file, "%d\n", EXIT_BADJOB);
	fprintf(reply_file, _("Queue entry \"%s\" does not exist.\n"), local_jobid(destname,id,subid,nodeid_to_name(homenode_id)));
	}

    } /* end of ppop_rush() */

/*
** This is the ppop interface command dispatcher.
*/
void ppop_dispatch(const char command[])
    {
    const char function[] = "ppop_dispatch";
    char reply_fname[MAX_PPR_PATH];
    const char *ppop_command;

    /* Read the PID of the ppop process that is waiting for a reply. */
    if((reply_pid = atol(command)) == 0)
    	{
    	error("%s(): no PID for reply", function);
    	return;
    	}

    /*
    ** Create a communications file to receive the message to ppop.  We have
    ** to be careful because this will be in the /tmp directory and a bad
    ** guy could have put a symbolic link there so that we will overwrite
    ** some file.  Notice that this only prevents the overwrite, ppop
    ** will hang.
    */
    ppr_fnamef(reply_fname, "%s/ppr-ppop-%ld", TEMPDIR, reply_pid);
    {
    int fd;
    if((fd = open(reply_fname, O_WRONLY | O_EXCL | O_CREAT, S_IRUSR | S_IWUSR)) == -1)
	{
	error("%s(): can't open \"%s\", errno=%d (%s)", function, reply_fname, errno, gu_strerror(errno));
	return;
	}

    /* If we don't do this, then "ppop start" will leak the file descriptor to pprdrv! */
    gu_set_cloexec(fd);

    if(!(reply_file = fdopen(fd, "w")))
    	{
	error("%s(): fdopen() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
	close(fd);
	return;
    	}
    }

    /* The command comes after the process id. */
    ppop_command = command + strspn(command, "0123456789 ");

    /* Do the command. */
    switch(ppop_command[0])
	{
	case 'l':                   /* list print jobs */
	    gu_alloc_checkpoint();
	    gu_sscanf_checkpoint();
	    ppop_list(ppop_command);
	    gu_sscanf_rollback();
	    gu_alloc_assert(0);
	    break;

	case 's':                   /* show printer status */
	    gu_alloc_checkpoint();
	    gu_sscanf_checkpoint();
	    ppop_status(ppop_command);
	    gu_sscanf_rollback();
	    gu_alloc_assert(0);
	    break;

	case 't':                   /* starT a printer */
	    gu_alloc_checkpoint();
	    gu_sscanf_checkpoint();
	    ppop_start_stop_wstop_halt(ppop_command,0);
	    gu_sscanf_rollback();
	    gu_alloc_assert(0);
	    break;

	case 'p':                   /* stoP a printer */
	    gu_alloc_checkpoint();
	    gu_sscanf_checkpoint();
	    ppop_start_stop_wstop_halt(ppop_command,1);
	    gu_sscanf_rollback();
	    gu_alloc_assert(0);
	    break;

	case 'P':                   /* stoP a printer, wait */
	    gu_alloc_checkpoint();
	    gu_sscanf_checkpoint();
	    ppop_start_stop_wstop_halt(ppop_command,129);
	    gu_sscanf_rollback();
	    gu_alloc_assert(0);
	    break;

	case 'b':                   /* stop printer with a Bang */
	    gu_alloc_checkpoint();
	    gu_sscanf_checkpoint();
	    ppop_start_stop_wstop_halt(ppop_command,2);
	    gu_sscanf_rollback();
	    gu_alloc_assert(0);
	    break;

	case 'h':                   /* place job on hold */
	    gu_alloc_checkpoint();
	    gu_sscanf_checkpoint();
	    ppop_hold_release(ppop_command,0);
	    gu_sscanf_rollback();
	    gu_alloc_assert(0);
	    break;

	case 'r':                   /* release a job */
	    gu_alloc_checkpoint();
	    gu_sscanf_checkpoint();
	    ppop_hold_release(ppop_command,1);
	    gu_sscanf_rollback();
	    gu_alloc_assert(0);
	    break;

	case 'c':                   /* cancel */
	    gu_sscanf_checkpoint();
	    ppop_cancel_purge(ppop_command);
	    gu_sscanf_rollback();
	    break;

	case 'f':                   /* media */
	    gu_alloc_checkpoint();
	    gu_sscanf_checkpoint();
	    ppop_media(ppop_command);
	    gu_sscanf_rollback();
	    gu_alloc_assert(0);
	    break;

	case 'M':                   /* mount media */
	    gu_sscanf_checkpoint();
	    ppop_mount(ppop_command);
	    gu_sscanf_rollback();
	    break;

	case 'A':                   /* accept for printer or group */
	    gu_alloc_checkpoint();
	    gu_sscanf_checkpoint();
	    ppop_accept_reject(ppop_command,1);
	    gu_sscanf_rollback();
	    gu_alloc_assert(0);
	    break;

	case 'R':                   /* reject for printer or group */
	    gu_alloc_checkpoint();
	    gu_sscanf_checkpoint();
	    ppop_accept_reject(ppop_command,0);
	    gu_sscanf_rollback();
	    gu_alloc_assert(0);
	    break;

	case 'D':                   /* show destinations */
	    gu_alloc_checkpoint();
	    gu_sscanf_checkpoint();
	    ppop_dest(ppop_command);
	    gu_sscanf_rollback();
	    gu_alloc_assert(0);
	    break;

	case 'm':                   /* move job(s) */
	    gu_alloc_checkpoint();
	    gu_sscanf_checkpoint();
	    ppop_move(ppop_command);
	    gu_sscanf_rollback();
	    gu_alloc_assert(0);
	    break;

	case 'U':                   /* rush job */
	    gu_alloc_checkpoint();
	    gu_sscanf_checkpoint();
	    ppop_rush(ppop_command);
	    gu_sscanf_rollback();
	    gu_alloc_assert(0);
	    break;

	default:
	    error("unknown command: %s", command);
	    break;
	}

    /* The "ppop wstop" command will set reply_file to NULL (after calling fclose()
       in order to prevent us from sending the signal. */
    if(reply_file)
	{
	fclose(reply_file);

	/* Try to send a signal to ppop to let it know the file is done.  If kill()
	   fails, assume that ppop died before it could get the reply and
	   delete the communications file. */
        if(kill((pid_t)reply_pid, SIGUSR1) == -1)
            {
            debug("%s(): kill() failed, errno=%d (%s), deleting reply file", function, errno, gu_strerror(errno));
            unlink(reply_fname);
            }
	}

    } /* end of ppop_dispatch() */

/* end of file */

