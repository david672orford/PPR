/*
** mouse:~ppr/src/pprd/pprd_load.c
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
** Last modified 15 January 2001.
*/

/*
** This module contains routines which are called only once or
** only when a printer or group needs to be reloaded.  These
** routines load the queue, media lists, printers, or groups.
*/

#include "before_system.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <stdlib.h>
#include "gu.h"
#include "global_defines.h"

#include "global_structs.h"
#include "pprd.h"
#include "./pprd.auto_h"

/*
** Load the data on a single printer into the array.
** This routine is called with a pointer to a printer array entry
** and the name of the printer.
*/
static void load_printer(struct Printer *printer, const char filename[])
    {
    const char function[] = "load_printer";
    FILE *prncf;
    char tempstr[256];			/* for reading lines */
    char tempstr2[MAX_BINNAME+1];	/* for extracting bin names */
    mode_t newmode;			/* new file mode */
    struct stat pstat;
    int count; float x1, x2;

    {
    char fname[MAX_PPR_PATH];
    ppr_fnamef(fname, "%s/%s", PRCONF, filename);
    if((prncf = fopen(fname,"r")) == (FILE*)NULL)
	fatal(0, "%s(): can't open printer config file \"%s\", errno=%d.", function, fname, errno);
    }

    strcpy(printer->name, filename);		/* store the printer name */

    printer->alert_interval = 0;		/* no alerts */
    printer->alert_method = (char*)NULL;	/* (At least not until we */
    printer->alert_address = (char*)NULL;	/* read an "Alert:" line.) */

    printer->next_error_retry = 0;		/* Not in fault state */
    printer->next_engaged_retry = 0;

    printer->protect = FALSE;			/* not protected that we know of yet */
    printer->charge_per_duplex = 0;
    printer->charge_per_simplex = 0;

    printer->nbins = 0;				/* start with zero bins */
    printer->AutoSelect_exists = FALSE;		/* start with no "AutoSelect" bin */
    printer->ppop_pid = (pid_t)0;		/* nobody waiting for stop */
    printer->previous_status = PRNSTATUS_IDLE;
    printer->status = PRNSTATUS_IDLE;		/* it is idle now */
    printer->accepting = TRUE;			/* is accepting */

    printer->cancel_job = FALSE;		/* don't cancel a job on next pprdrv exit */
    printer->hold_job = FALSE;			/* don't hold job on next pprdrv exit */

    fstat(fileno(prncf), &pstat);
    if(pstat.st_mode & S_IXUSR)			/* if user execute set, */
	printer->status = PRNSTATUS_STOPT;	/* printer is stop */
    if(pstat.st_mode & S_IXGRP)			/* if group execute is set, */
	printer->accepting = FALSE;		/* printer not accepting */

    while(fgets(tempstr, sizeof(tempstr), prncf))
	{
	if(*tempstr==';' || *tempstr=='#')
	    continue;

	/* For "Alert:" lines, read the interval, method, and address. */
	else if(strncmp(tempstr, "Alert: ", 7) == 0)
	    {
	    int x = 7;					/* len of "Alert: " */
	    int len;

	    x+=strspn(&tempstr[x], " \t");		/* skip spaces */
	    sscanf(&tempstr[x],"%d",&printer->alert_interval);
	    x+=strspn(&tempstr[x]," \t-0123456789");	/* skip spaces and */
							/* digits */
	    len=strcspn(&tempstr[x]," \t");             /* get word length */
	    printer->alert_method = (char*)gu_alloc(len+1,sizeof(char));
	    strncpy(printer->alert_method,&tempstr[x],len);  /* copy */
	    printer->alert_method[len] = '\0';		/* terminate */
	    x+=len;                                     /* move past word */
	    x+=strspn(&tempstr[x]," \t");               /* skip spaces */

	    len=strcspn(&tempstr[x]," \t\n");           /* get length */
	    printer->alert_address = (char*)gu_alloc(len+1,sizeof(char));
	    strncpy(printer->alert_address,&tempstr[x],len); /* copy */
	    printer->alert_address[len] = (char)NULL;	/* terminate */
	    }

	/* For each "Bin:" line, add the bin to the list */
	else if(strncmp(tempstr, "Bin: ", 5) == 0)
	    {
	    if(printer->nbins < MAX_BINS)
		{
		int t = gu_sscanf(tempstr, "Bin: %#s", sizeof(tempstr2), tempstr2);

		if(t)                   /* if it has 1 or more arguments */
		    {                   /* use the bin name */
		    printer->bins[printer->nbins] = get_bin_id(tempstr2);
		    if(strcmp(tempstr2,"AutoSelect")==0)	/* If this is an "AutoSelect" bin, */
		    	printer->AutoSelect_exists = TRUE;	/* then set the flag. */
		    printer->media[printer->nbins] = -1; 	/* nothing mounted yet */
		    printer->nbins++;
		    }
		}
	    else
		{
		error("Printer \"%s\" has too many bins.", printer->name);
		}
	    }

	/*
	** Set per sheet/page charge.  Actually, here we just see if
	** we will charge and if so, make the printer a
	** protected printer
	*/
	else if((count=sscanf(tempstr, "Charge: %f %f", &x1, &x2)) >= 1)
	    {
	    printer->charge_per_duplex = (int)(x1 * 100.0);

	    /* In order to be backwards compatible, we will set the per-page
	       charge the same as the per sheet charge if it is missing. */
	    if(count == 2)
	    	printer->charge_per_simplex = (int)(x2 * 100.0);
	    else
	    	printer->charge_per_simplex = printer->charge_per_duplex;

	    printer->protect = TRUE;
	    }
	} /* end of while(), unknown lines are ignored */

    /*
    ** If printer is protected, turn on ``other'' execute bit,
    ** otherwise, turn it off.  We do this for the convienence of ppr.
    */
    if(printer->protect)
	newmode = pstat.st_mode | S_IXOTH;
    else
	newmode = pstat.st_mode & (~ S_IXOTH);

    if(newmode != pstat.st_mode)
	fchmod(fileno(prncf), newmode);

    /* Close that configuration file! */
    fclose(prncf);
    } /* end of load_printer() */

/*
** Search the printer configuration directory and
** call load_printer() once for each printer.
*/
void load_printers(void)
    {
    const char function[] = "load_printers";
    DIR *dir;           /* directory to search */
    struct dirent *direntp;
    int x;
    int len;

    printers = (struct Printer*)gu_alloc(MAX_PRINTERS, sizeof(struct Printer));

    if((dir = opendir(PRCONF)) == (DIR*)NULL)
	fatal(0, "%s(): can't open directory \"%s\", errno=%d (%s)", function, PRCONF, errno, gu_strerror(errno));

    x=0; printer_count=0;
    while((direntp = readdir(dir)))
	{
	/* Skip . and .. and hidden files. */
	if(direntp->d_name[0] == '.')
	    continue;

	/* Skip Emacs style backup files. */
	len = strlen(direntp->d_name);
	if( len > 0 && direntp->d_name[len-1]=='~' )
	    continue;

	if(x == MAX_PRINTERS)
	    {
	    error("%s(): too many printers", function);
	    break;		/* break out of loop */
	    }

	load_printer(&printers[x], direntp->d_name);
	printer_count++;		/* do now so destid_to_name() ok */
	media_mounted_recover(x);	/* get those forms back */
	media_mounted_save(x);		/* this list must be up to date for pprdrv */
	x++;
	}

    closedir(dir);
    } /* end of load_printers() */

/*
** Load a new printer configuration file.
**
** This is called when ppad sends a command over the pipe.  It does this
** whenever the printer configuration changes in a way that might
** interest us.  You can also use "ppad touch" to send the command any
** time you want.
**
** We must be careful to free any allocated memory blocks
** in an old printer configuration.
*/
void new_printer_config(char *printer)
    {
    const char function[] = "new_printer_config";
    int prnid;
    int first_deleted = -1;	/* index of first deleted printer entry */
    int is_new = FALSE;
    int saved_status;
    pid_t saved_ppop_pid;
    char fname[MAX_PPR_PATH];
    FILE *testopen;

    lock();		/* don't let printer state change while we do this */

    /*
    ** Find the printer in the printer array
    ** and free its memory blocks.
    */
    for(prnid = 0; prnid < printer_count; prnid++)
	{
	if(printers[prnid].status == PRNSTATUS_DELETED)	/* take note */
	    {
	    if(first_deleted == -1)			/* if there are any */
		first_deleted = prnid;			/* deleted slots we can use */
	    continue;
	    }
	if(strcmp(printers[prnid].name, printer) == 0)	/* If the name of this printer */
	    {						/* is the one we are looking for, */
	    if(printers[prnid].alert_method)		/* free its memory blocks. */
	        gu_free(printers[prnid].alert_method);
	    if(printers[prnid].alert_address)
	        gu_free(printers[prnid].alert_address);
	    break;
	    }
	}

    if(prnid == printer_count)		/* if x points to just after end of list, */
    	{
	is_new = TRUE;			/* this is a new printer */
    	if(first_deleted != -1)		/* if we have an empty */
    	    {				/* slot, */
    	    prnid = first_deleted;	/* re-use it */
    	    }
    	}

    if(prnid == MAX_PRINTERS)	/* if new printer and no more room, */
	{			/* just say there is an error */
	error("%s(): too many printers", function);
	unlock();		/* and ignore the request */
	return;
    	}

    if(prnid == printer_count)	/* If we are appending to printer list, */
	printer_count++;	/* add to count of printers. */

    /*
    ** Try to open the printer configuration file
    ** in order to determine if it is being deleted.
    ** (When a printer is deleted, ppad asks us to
    ** update its status anyway.  This is where we
    ** find out it was deleted.)
    */
    ppr_fnamef(fname, "%s/%s", PRCONF, printer);
    if((testopen = fopen(fname, "r")) == (FILE*)NULL)
    	{
	if(is_new)				/* if new printer */
	    {
	    error("attempt to delete printer \"%s\" which never existed", printer);
	    }
	else
	    {
	    state_update("PRNDELETE %s", printer);	/* inform queue display programs */
	    printers[prnid].status = PRNSTATUS_DELETED;	/* mark as deleted */
	    }
	unlock();
	return;
    	}
    fclose(testopen);		/* We don't want this, so close it. */

    state_update("PRNRELOAD %s", printer);	/* Inform queue display programs. */

    saved_status = printers[prnid].status;	/* We will use these in a moment */
    saved_ppop_pid = printers[prnid].ppop_pid;	/* if the printer is not new. */

    load_printer(&printers[prnid], printer);	/* load printer configuration */
    media_mounted_recover(prnid);		/* load the list of mounted media */
    media_mounted_save(prnid);			/* save updated (very important for pprdrv) */

    /* If this printer already existed, we have to fix things up a bit. */
    if( ! is_new)
	{
	/* restore its status */
	printers[prnid].status = saved_status;
	printers[prnid].ppop_pid = saved_ppop_pid;

        /* Since the configuration is new, this printer may be able to print
           jobs it couldn't have printed before.  Scan the queue and clear
           the never flag for all of this printer's jobs and clear this printer's
           never bit for all jobs for groups to which this printer belongs.
           */
	{
	int x, prnbit;
	for(x=0; x < queue_entries; x++)
	    {
	    prnbit = 1;
	    if(queue[x].destid == prnid || (prnbit = destid_printer_bit(queue[x].destid, prnid)) )
		{
		queue[x].never &= (0xFF ^ prnbit);
		if(queue[x].status == STATUS_STRANDED)
		    p_job_new_status(&queue[x], STATUS_WAITING);
		}
    	    }
	}

	/* It is possible that the bins changed.  Update the notnow flags for
	   all jobs that could print on this printer.
	   */
	media_update_notnow(prnid);

	/* If the printer is idle, look for something for it to do.
	   Questions: why is this necessary?
	   Possible answer: because the bins may have changed.
	   */
	if(printers[prnid].status == PRNSTATUS_IDLE)
	    printer_look_for_work(prnid);
	}

    unlock();		/* ok, let things move again */
    } /* end of new_printer_config() */

/*
** Load the configuration of a single group of printers into the array.
** This routine is called with a pointer to a group array entry and
** the name of the group to put in it.
*/
static void load_group(struct Group *cl, const char filename[])
    {
    const char function[] = "load_group";
    FILE *clcf;             /* class (group) config file */
    char fname[MAX_PPR_PATH];
    char tempstr[256];      /* for reading lines */
    char tempstr2[32];      /* for extractions */
    int y;
    struct stat cstat;
    mode_t newmod;
    int line = 0;

    ppr_fnamef(fname, "%s/%s", GRCONF, filename);
    if((clcf = fopen(fname, "r")) == (FILE*)NULL)
	fatal(0, "%s(): can't open \"%s\", errno=%d (%s)", function, fname, errno, gu_strerror(errno));

    strcpy(cl->name, filename);		/* store the group name */
    cl->last = -1;			/* initialize last value */

    fstat(fileno(clcf), &cstat);

    if(cstat.st_mode & S_IXGRP)		/* if group execute is set, */
	cl->accepting = FALSE;		/* group not accepting */
    else
	cl->accepting = TRUE;		/* is accepting */

    if(cstat.st_mode & S_IXUSR)		/* if user execute bit is set, */
	cl->held = TRUE;		/* group is held */
    else
	cl->held = FALSE;

    cl->protect = FALSE;		/* don't protect by default */
    cl->deleted = FALSE;		/* it is not a deleted group! */
    cl->rotate = TRUE;			/* rotate is default */

    y=0;
    while(fgets(tempstr, sizeof(tempstr), clcf))
	{
	line++;

	/* Read the name of a group member */
	if(gu_sscanf(tempstr, "Printer: %#s", sizeof(tempstr2), tempstr2) == 1)
	    {
	    if(y >= MAX_GROUPSIZE)	/* if group has overflowed, */
		{			/* note error and ignore member */
		error("group \"%s\" exceeds %d member limit", cl->name, MAX_GROUPSIZE);
		continue;
		}
	    if((cl->printers[y] = destid_local_by_printer(tempstr2)) == -1)
		{
		error("group \"%s\":  member \"%s\" does not exist", cl->name, tempstr2);
		}
	    else
		{
		if(printers[cl->printers[y]].protect)
					/* if this printer is protected, */
		    cl->protect = TRUE;	/* protect the group */
		y++;            /* increment members index */
		}
	    continue;
	    }

	/* read a rotate flag value */
	if(strncmp(tempstr, "Rotate:", 7) == 0)
	    {
	    if(gu_torf_setBOOL(&cl->rotate, &tempstr[7]) == -1)
		fatal(0, "Invalid Rotate option (%s, line %d)", fname, line);
	    continue;
	    }
	}

    /*
    ** If group is protected, turn on user execute bit,
    ** otherwise, turn it off.  This bit is read by ppr.
    */
    if(cl->protect)
	newmod = cstat.st_mode | S_IXOTH;
    else
	newmod = cstat.st_mode & (~ S_IXOTH);

    if(newmod != cstat.st_mode)		/* (only act if new mode is different) */
	fchmod(fileno(clcf),newmod);

    fclose(clcf);           /* close the group configuration file */
    cl->members=y;          /* set the members count */
    } /* end of load_group() */

/*
** Call load_group() once for each group in the groups directory.
*/
void load_groups(void)
    {
    const char function[] = "load_groups";
    DIR *dir;           /* directory to search */
    struct dirent *direntp;
    int x;
    int len;

    if((groups = (struct Group*)calloc(MAX_GROUPS, sizeof(struct Group))) == (struct Group*)NULL)
	fatal(0, "%s(): out of memory", function);

    if((dir = opendir(GRCONF)) == (DIR*)NULL)
	fatal(0, "%s(): can't open directory \"%s\", errno=%d (%s)", function, GRCONF, errno, gu_strerror(errno));

    x = 0;
    while((direntp = readdir(dir)))
	{
	/* Skip "." and ".." and hidden files. */
	if(direntp->d_name[0] == '.')
	    continue;

	/* Skip Emacs style backup files. */
	len=strlen(direntp->d_name);
	if( len > 0 && direntp->d_name[len-1] == '~' )
	    continue;

	if(x==MAX_GROUPS)
	    {
	    error("%s(): too many groups", function);
	    break;
	    }

	load_group(&groups[x], direntp->d_name);
	x++;
	}

    group_count = x;	/* remember how many groups we have */

    closedir(dir);
    } /* end of load_groups() */

/*
** Load a new group configuration file.
** This is called when a command from ppad is
** received over the pipe.
*/
void new_group_config(char *group)
    {
    const char function[] = "new_group_config";
    int x;				/* group array index */
    int y;				/* members array index */
    int destid;
    int first_deleted = -1;
    int is_new = FALSE;
    char fname[MAX_PPR_PATH];
    FILE *testopen;

    lock();				/* prevent clashing queue changes */

    for(x=0; x<group_count; x++)	/* find the group in the group array */
    	{
	if(groups[x].deleted)		/* if we see any deleted group slots, */
	    {				/* remember where the 1st one is */
	    if(first_deleted == -1)
	    	first_deleted = x;
	    continue;
	    }
	if(strcmp(groups[x].name, group) == 0)
    	    break;
    	}

    if(x == group_count)
    	{
    	is_new = TRUE;
    	if(first_deleted != -1)
    	    x = first_deleted;
    	else
    	    group_count++;
    	}

    if(x == MAX_GROUPS)			/* if adding, make sure there is room */
	{
	unlock();
	error("%s(): too many groups", function);
	return;
	}

    /* see if we are deleting this group */
    ppr_fnamef(fname, "%s/%s", GRCONF, group);
    if((testopen = fopen(fname, "r")) == (FILE*)NULL)
    	{
    	if(is_new)
	    {
    	    error("attempt to delete group \"%s\" which never existed", group);
    	    }
	else
	    {
	    state_update("GRPDELETE %s",group);		/* inform queue display programs */
	    groups[x].deleted = TRUE;			/* mark as deleted */
	    }
	unlock();
	return;
	}
    fclose(testopen);

    state_update("GRPRELOAD %s",group);	/* inform queue display programs */

    load_group(&groups[x],group);	/* read the group file */

    /* fix all the jobs for this group */
    destid = destid_local_by_gindex(x);
    for(x = 0; x < queue_entries; x++)
    	{
    	if(queue[x].destid==destid)	/* if job is for this group, */
     	    {				/* reset the media ready lists */
	    media_set_notnow_for_job(&queue[x], TRUE);
	    queue[x].never = 0;		/* Since the membership may have changed, the never bits may be */
    	    }				/* invalid, so just clear them */
    	}				/* (they will be set again if necessary). */

    unlock();

    /* look for work for any group members which are idle */
    for(y=0; y<groups[x].members; y++)
    	{
	if(printers[groups[x].printers[y]].status == PRNSTATUS_IDLE)
	    printer_look_for_work(groups[x].printers[y]);
	}
    } /* end of new_group_config() */

/*
** Initialize the queue, loading existing jobs into it.
*/
#if 0
#ifdef COLON_FILENAME_BUG	/* If this operating system doesn't allow colons in file names, */
#define COLON_STR "!"		/* substitute exclamation points. */
#define COLON_CHAR '!'
#else
#define COLON_STR ":"
#define COLON_CHAR ':'
#endif
#endif
void initialize_queue(void)
    {
    const char function[] = "initialize_queue";
    DIR *dir;			/* the open queue directory */
    struct dirent *direntp;	/* one directory entry */
    struct QEntry newent;	/* the new terse queue entry we are building */
    char *scratch = NULL;
    size_t scratch_len;
    const char *ptr_destnode, *ptr_destname, *ptr_homenode;
#if 0
    char *ptr;
    int len;
#endif

    DODEBUG_RECOVER(("%s()", function));

    /* Allocate memory to hold the queue. */
    queue_size = QUEUE_SIZE_INITIAL;
    queue = (struct QEntry *)gu_alloc(queue_size, sizeof(struct QEntry));

    /* Open the queue directory. */
    if((dir = opendir(QUEUEDIR)) == (DIR*)NULL)
	fatal(0, "%s(): can't open directory \"%s\", errno=%d (%s)", function, QUEUEDIR, errno, gu_strerror(errno));

    /* Protect the queue array during modification. */
    lock();

    /* loop to read file names */
    while((direntp = readdir(dir)))
	{
	if(direntp->d_name[0] == '.')		/* ignore "." and ".." */
	    continue;

	DODEBUG_RECOVER(("%s(): inheriting queue file \"%s\"", function, direntp->d_name));

	/* Make a copy of the file name that we can stomp on. */
	scratch = gu_restrdup(scratch, &scratch_len, direntp->d_name);

#if 0
	/* Find the extent of the destination node name. */
	ptr_destnode = scratch;
	len = strcspn(ptr_destnode, COLON_STR);
	if(ptr_destnode[len] != COLON_CHAR)
	    {
	    error("%s(): can't parse \"%s\" (1)", function, direntp->d_name);
	    continue;
	    }
	ptr_destnode[len] = '\0';

	/* Find the extent of the destination name by getting
	   the length of part before dash followed by digit.
	   */
	{
	int c;
	ptr_destname = &ptr_destnode[len+1];
	for(len=0; TRUE; len++)
	    {
	    len += strcspn(&ptr_destname[len], "-");

            if(ptr_destname[len] == '\0')	/* check for end of string */
                break;

            if((c = ptr_destname[len+1+strspn(&ptr_destname[len+1], "0123456789")]) == '.' || c == '(' || c=='\0')
                break;
	    }
	if(ptr_destname[len] != '-')
	    {
	    error("%s(): can't parse \"%s\" (2)", function, direntp->d_name);
	    continue;
	    }
	ptr_destname[len] = '\0';				/* terminate destination name */
	}

	/* Scan for id number, subid number, and home node name. */
	ptr = &ptr_destname[len+1];
	if(sscanf(ptr, "%hd.%hd", &newent.id, &newent.subid) != 2
		|| newent.id < 0 || newent.subid < 0)
	    {
	    error("%s(): can't parse \"%s\" (3)", function, direntp->d_name);
	    continue;
	    }

	/* Find the home node name. */
	ptr_homenode = &ptr[strspn(ptr, "0123456789.")];
	if(*(ptr_homenode++) != '(' || (len = strlen(ptr_homenode)) < 2 || ptr_homenode[--len] != ')')
	    {
	    error("%s(): can't parse \"%s\" (4)", function, direntp->d_name);
	    continue;
	    }
	ptr_homenode[len] = '\0';
#else
	{
	int e = parse_qfname(scratch, &ptr_destnode, &ptr_destname, &newent.id, &newent.subid, &ptr_homenode);
	if(e < 0)
	    {
	    error("%s(): can't parse \"%s\" (%d)", function, direntp->d_name, e);
	    continue;
	    }
	}
#endif

	/*
	** This section handles opening the queue file and extracting
	** information from it an its permissions..
	*/
	{
	FILE *qfile;
	struct stat qstat;

	/* Open the queue file. */
	{
	char qfname[MAX_PPR_PATH];
	ppr_fnamef(qfname, "%s/%s", QUEUEDIR, direntp->d_name);
	if((qfile = fopen(qfname, "r")) == (FILE*)NULL)
	    {
	    error("%s(): can't open \"%s\", errno=%d (%s)", function, direntp->d_name, errno, gu_strerror(errno));
	    continue;
	    }
	}

	/* stat the file, we will use the information below */
	fstat(fileno(qfile), &qstat);

	/* Recover the job status from the permission bits.  We start with
	   the assumption that it is waiting for a printer, but if certain
	   execute bits are set we will understand that it is held, stranded,
	   or arrested.
	   */
	newent.status = STATUS_WAITING;
	if(qstat.st_mode & BIT_JOB_HELD)
	    newent.status = STATUS_HELD;
	if(qstat.st_mode & BIT_JOB_STRANDED)
	    newent.status = STATUS_STRANDED;
	if(qstat.st_mode & BIT_JOB_ARRESTED)
	    newent.status = STATUS_ARRESTED;

	/*
	** If the queue file is of zero length, then it is defective,
	** delete the job files.
	*/
	if(qstat.st_size == 0)
	    {
	    error("%s(): queue file \"%s\" was of zero length", function, direntp->d_name);
	    fclose(qfile);
	    queue_unlink_job_2(ptr_destnode, ptr_destname, newent.id, newent.subid, ptr_homenode);
	    continue;
	    }

	/*
	** Read the priority from the queue file.  There is no good reason
	** for the default since all queue files have a "Priority:" line.
	** We are just being paranoid.
	*/
	newent.priority = 20;
	{
	char *line = NULL;
	int line_available = 80;
	while((line = gu_getline(line, &line_available, qfile)))
	    {
	    if(sscanf(line, "Priority: %hd", &newent.priority) == 1)
		{
		break;
		}
	    }
	}

	/* That is all we needed the queue file for. */
	fclose(qfile);
	}

	/*
	** Convert the destination node, the destination queue, and the
	** home node names to numbers.  (We keep this at the end in order
	** to keep cleanup simple.  Oh! for a try {} catch {} construct!
	*/
	newent.destnode_id = nodeid_assign(ptr_destnode);
        if((newent.destid = destid_assign(newent.destnode_id, ptr_destname)) == -1)
            {
            error("%s(): destination \"%s\" no longer exists", function, ptr_destname);
            nodeid_free(newent.destnode_id);
            continue;
            }
	newent.homenode_id = nodeid_assign(ptr_homenode);

	/*
	** If there is room for the job in the queue and is local,
	** an attempt is made to start a printer for it.
	**
	** If it is remote, the xmit stuff is told it is ready to go.
	*/
	{
	int rank1, rank2;
	struct QEntry *newent_ptr;
	if((newent_ptr = queue_enqueue_job(&newent, &rank1, &rank2)))
	    {
	    if(newent_ptr->status == STATUS_WAITING)
	    	{
	    	if(nodeid_is_local_node(newent_ptr->destnode_id))
		    printer_try_start_suitable_4_this_job(newent_ptr);
	    	else
	    	    remote_new_job(newent_ptr);
	    	}
	    }
	else
	    {
	    error("%s() failed to add job to queue", function);
	    destid_free(newent.destnode_id, newent.destid);
	    nodeid_free(newent.destnode_id);
	    nodeid_free(newent.homenode_id);
	    }
	}

	} /* end directory search loop */

    unlock();
    closedir(dir);
    if(scratch) gu_free(scratch);

    DODEBUG_RECOVER(("%s(): done", function));
    } /* end of initialize_queue() */

/* end of file */

