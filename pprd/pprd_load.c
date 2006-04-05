/*
** mouse:~ppr/src/pprd/pprd_load.c
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
** Last modified 5 April 2006.
*/

/*
** This module contains routines which are called only once or
** only when a printer or group needs to be reloaded.  These
** routines load the queue, media lists, printers, or groups.
*/

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <stdlib.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
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
static void load_printer(struct Printer *printer, const char prnname[])
	{
	const char function[] = "load_printer";
	FILE *prncf;
	char *line = NULL;
	int line_space = 128;
	int count; float x1, x2;

	{
	char fname[MAX_PPR_PATH];
	ppr_fnamef(fname, "%s/%s", PRCONF, prnname);
	if((prncf = fopen(fname,"r")) == (FILE*)NULL)
		fatal(0, "%s(): can't open printer config file \"%s\", errno=%d.", function, fname, errno);
	}

	printer->name = gu_strdup(prnname);

	printer->alert_interval = 0;				/* no alerts */
	printer->alert_method = (char*)NULL;		/* (At least not until we */
	printer->alert_address = (char*)NULL;		/* read an "Alert:" line.) */

	printer->accepting = TRUE;					/* is accepting */
	printer->protect = FALSE;					/* not protected that we know of yet */
	printer->charge_per_duplex = 0;				/* we don't charge any money to use it */
	printer->charge_per_simplex = 0;

	printer->nbins = 0;							/* start with zero bins */
	printer->AutoSelect_exists = FALSE;			/* start with no "AutoSelect" bin */

	printer->ppop_pid = (pid_t)0;				/* nobody waiting for stop */
	printer->cancel_job = FALSE;				/* don't cancel a job on next pprdrv exit */
	printer->hold_job = FALSE;					/* don't hold job on next pprdrv exit */

	/* Is the printer accepting jobs? */
	if(get_file_boolean(PRINTERS_PERSISTENT_STATEDIR, prnname, "rejecting"))	
		printer->accepting = FALSE;

	/* Is the printer stopt or in fault state? */
	spool_state_load(printer);
		
	/* Handle state transitions brought about by killing pprd. */
	switch(printer->status)
		{
		case PRNSTATUS_PRINTING:
		case PRNSTATUS_CANCELING:
		case PRNSTATUS_SEIZING:
			printer_new_status(printer, PRNSTATUS_IDLE);
			break;
		case PRNSTATUS_HALTING:
			printer_new_status(printer, PRNSTATUS_STOPT);
			break;
		}

	while((line = gu_getline(line, &line_space, prncf)))
		{
		if(*line==';' || *line=='#')
			continue;

		/* For "Alert:" lines, read the interval, method, and address. */
		else if(lmatch(line, "Alert:"))
			{
			if(printer->alert_method)
				{
				gu_free(printer->alert_method);
				printer->alert_method = NULL;
				}
			if(printer->alert_address)
				{
				gu_free(printer->alert_address);
				printer->alert_address = NULL;
				}
			gu_sscanf(line, "Alert: %d %S %S", &printer->alert_interval, &printer->alert_method, &printer->alert_address);
			}

		/* For each "Bin:" line, add the bin to the list */
		else if(lmatch(line, "Bin:"))
			{
			if(printer->nbins < MAX_BINS)
				{
				char *bin;
				if(gu_sscanf(line, "Bin: %S", &bin) == 1)
					{
					printer->bins[printer->nbins] = bin;
					printer->media[printer->nbins] = -1;		/* nothing mounted yet */
					printer->nbins++;

					if(strcmp(bin,"AutoSelect")==0)				/* If this is an "AutoSelect" bin, */
						printer->AutoSelect_exists = TRUE;		/* then set the flag. */
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
		else if((count = gu_sscanf(line, "Charge: %f %f", &x1, &x2)) >= 1)
			{
			printer->charge_per_duplex = (int)(x1 * 100.0 + 0.5);

			/* In order to be backwards compatible, we will set the per-page
			   charge the same as the per sheet charge if it is missing. */
			if(count == 2)
				printer->charge_per_simplex = (int)(x2 * 100.0 + 0.5);
			else
				printer->charge_per_simplex = printer->charge_per_duplex;

			printer->protect = TRUE;
			}

		} /* end of while(), unknown lines are ignored */

	/* Close that configuration file! */
	fclose(prncf);

	/* If printer is protected, create the "protected" file, otherwise
	 * delete it.  We do this for the convienence of ppr.
	*/
	set_file_boolean(PRINTERS_PERSISTENT_STATEDIR, prnname, "protected", printer->protect);

	/* Create the directories which will hold this printer's dynamic 
	   state information. */
	{
	char fname[MAX_PPR_PATH];
	ppr_fnamef(fname, "%s/%s", PRINTERS_PERSISTENT_STATEDIR, prnname);
	if(mkdir(fname, UNIX_755) == -1 && errno != EEXIST)
		fatal(0, _("%s(): %s(\"%s\", 0%o) failed, errno=%d (%s)"), function, "mkdir", fname, UNIX_755, errno, gu_strerror(errno));

	ppr_fnamef(fname, "%s/%s", PRINTERS_PURGABLE_STATEDIR, prnname);
	if(mkdir(fname, UNIX_755) == -1 && errno != EEXIST)
		fatal(0, _("%s(): %s(\"%s\", 0%o) failed, errno=%d (%s)"), function, "mkdir", fname, UNIX_755, errno, gu_strerror(errno));

	}
	} /* end of load_printer() */

/*
** Search the printer configuration directory and
** call load_printer() once for each printer.
*/
void load_printers(void)
	{
	const char function[] = "load_printers";
	DIR *dir;			/* directory to search */
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
		if(len > 0 && direntp->d_name[len-1] == '~')
			continue;

		if(x == MAX_PRINTERS)
			{
			error("%s(): too many printers", function);
			break;				/* break out of loop */
			}

		load_printer(&printers[x], direntp->d_name);
		printer_count++;				/* do now so destid_to_name() ok */
		media_mounted_recover(x);		/* get those forms back */
		media_mounted_save(x);			/* this list must be up to date for pprdrv */
		x++;
		}

	closedir(dir);
	} /* end of load_printers() */

/*
** Load a new printer configuration file.
**
** This is called when ppad sends a printer-touch command over the pipe.  It 
** does this whenever the printer configuration changes in a way that might
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
	int first_deleted = -1;		/* index of first deleted printer entry */
	int is_new = FALSE;
	int saved_status;
	pid_t saved_ppop_pid;
	char fname[MAX_PPR_PATH];
	FILE *testopen;

	lock();				/* don't let printer state change while we do this */

	/*
	** Find the printer in the printer array
	** and free its memory blocks.
	*/
	for(prnid = 0; prnid < printer_count; prnid++)
		{
		if(printers[prnid].status == PRNSTATUS_DELETED) /* take note */
			{
			if(first_deleted == -1)						/* if there are any */
				first_deleted = prnid;					/* deleted slots we can use */
			continue;
			}
		if(strcmp(printers[prnid].name, printer) == 0)	/* If the name matches the one we are looking for, */
			{
			gu_free(printers[prnid].name);
			printers[prnid].name = NULL;
			gu_free_if(printers[prnid].alert_method);
			gu_free_if(printers[prnid].alert_address);
			break;
			}
		}

	if(prnid == printer_count)			/* if x points to just after end of list, */
		{
		is_new = TRUE;					/* this is a new printer */
		if(first_deleted != -1)			/* if we have an empty slot, */
			prnid = first_deleted;		/* re-use it */
		}

	if(prnid == MAX_PRINTERS)	/* if new printer and no more room, */
		{						/* just say there is an error */
		error("%s(): too many printers", function);
		unlock();				/* and ignore the request */
		return;
		}

	if(prnid == printer_count)	/* If we are appending to printer list, */
		printer_count++;		/* add to count of printers. */

	/*
	** Try to open the printer configuration file
	** in order to determine if it is being deleted.
	** (When a printer is deleted, ppad asks us to
	** update its status anyway.  This is where we
	** find out it was deleted.)
	*/
	ppr_fnamef(fname, "%s/%s", PRCONF, printer);
	if(!(testopen = fopen(fname, "r")))
		{
		if(is_new)								/* if new printer */
			{
			error("attempt to delete printer \"%s\" which never existed", printer);
			}
		else
			{
			state_update("PRNDELETE %s", printer);		/* inform queue display programs */
			printers[prnid].status = PRNSTATUS_DELETED; /* mark as deleted */
			}
		}

	else
		{
		fclose(testopen);			/* We don't want this, so close it. */
	
		state_update("PRNRELOAD %s", printer);		/* Inform queue display programs. */
	
		saved_status = printers[prnid].status;		/* We will use these in a moment */
		saved_ppop_pid = printers[prnid].ppop_pid;	/* if the printer is not new. */
	
		load_printer(&printers[prnid], printer);	/* load printer configuration */
		media_mounted_recover(prnid);				/* load the list of mounted media */
		media_mounted_save(prnid);					/* save updated (very important for pprdrv) */
	
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
						queue_p_job_new_status(&queue[x], STATUS_WAITING);
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
		} /* end if if printer still exists */

	unlock();			/* ok, let things move again */
	} /* end of new_printer_config() */

/*
** Load the configuration of a single group of printers into the array.
** This routine is called with a pointer to a group array entry and
** the name of the group to put in it.
*/
static void load_group(struct Group *cl, const char grpname[])
	{
	const char function[] = "load_group";
	char conf_fname[MAX_PPR_PATH];
	FILE *f;				/* group config file */
	char *line = NULL;		/* for reading lines */
	int line_space = 128;
	char *extract;
	int y;
	int linenum = 0;

	ppr_fnamef(conf_fname, "%s/%s", GRCONF, grpname);
	if(!(f = fopen(conf_fname, "r")))
		fatal(0, "%s(): can't open \"%s\", errno=%d (%s)", function, conf_fname, errno, gu_strerror(errno));

	cl->name = gu_strdup(grpname);		/* store the group name */
	cl->last = -1;						/* initialize last used member value */

	cl->held = get_file_boolean(GROUPS_PERSISTENT_STATEDIR, grpname, "held");
	cl->accepting = !get_file_boolean(GROUPS_PERSISTENT_STATEDIR, grpname, "rejecting");

	cl->protect = FALSE;				/* don't protect by default */
	cl->deleted = FALSE;				/* it is not a deleted group! */
	cl->rotate = TRUE;					/* rotate is default */

	y=0;
	while((line = gu_getline(line, &line_space, f)))
		{
		linenum++;

		/* Read the name of a group member */
		if(gu_sscanf(line, "Printer: %S", &extract) == 1)
			{
			if(y >= MAX_GROUPSIZE)		/* if group has overflowed, */
				{						/* note error and ignore member */
				error("group \"%s\" exceeds %d member limit", cl->name, MAX_GROUPSIZE);
				}
			else if((cl->printers[y] = destid_by_printer(extract)) == -1)
				{
				error("group \"%s\":  member \"%s\" does not exist", cl->name, extract);
				}
			else
				{
				if(printers[cl->printers[y]].protect)
										/* if this printer is protected, */
					cl->protect = TRUE; /* protect the group */
				y++;					/* increment members index */
				}
			gu_free(extract);
			continue;
			}

		/* read a rotate flag value */
		if((extract = lmatchp(line, "Rotate:")))
			{
			if(gu_torf_setBOOL(&cl->rotate, extract) == -1)
				fatal(0, "Invalid Rotate option (%s, line %d)", conf_fname, linenum);
			continue;
			}
		}

	fclose(f);

	/* If group is protected, create the "protected" file, otherwise
	 * delete it.  We do this for the convienence of ppr.
	*/
	set_file_boolean(GROUPS_PERSISTENT_STATEDIR, grpname, "protected", cl->protect);

	cl->members=y;			/* set the members count */
	} /* end of load_group() */

/*
** Call load_group() once for each group in the groups directory.
*/
void load_groups(void)
	{
	const char function[] = "load_groups";
	DIR *dir;			/* directory to search */
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
	int x;								/* group array index */
	int y;								/* members array index */
	int destid;
	int first_deleted = -1;
	int is_new = FALSE;
	char fname[MAX_PPR_PATH];
	FILE *testopen;

	lock();								/* prevent clashing queue changes */

	for(x=0; x<group_count; x++)		/* find the group in the group array */
		{
		if(groups[x].deleted)			/* if we see any deleted group slots, */
			{							/* remember where the 1st one is */
			if(first_deleted == -1)
				first_deleted = x;
			continue;
			}
		if(strcmp(groups[x].name, group) == 0)
			{
			gu_free(groups[x].name);
			groups[x].name = NULL;
			break;
			}
		}

	if(x == group_count)
		{
		is_new = TRUE;
		if(first_deleted != -1)
			x = first_deleted;
		else
			group_count++;
		}

	if(x == MAX_GROUPS)					/* if adding, make sure there is room */
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
			state_update("GRPDELETE %s",group);			/* inform queue display programs */
			groups[x].deleted = TRUE;					/* mark as deleted */
			}
		}
	else
		{
		fclose(testopen);
	
		state_update("GRPRELOAD %s",group); /* inform queue display programs */
	
		load_group(&groups[x],group);		/* read the group file */
	
		/* fix all the jobs for this group */
		destid = destid_by_gindex(x);
		for(x = 0; x < queue_entries; x++)
			{
			if(queue[x].destid==destid)		/* if job is for this group, */
				{							/* reset the media ready lists */
				media_set_notnow_for_job(&queue[x], TRUE);
				queue[x].never = 0;			/* since the membership may have changed, the never bits may be */
				}							/* invalid, so just clear them */
			}								/* (they will be set again if necessary). */
	
	
		/* look for work for any group members which are idle */
		for(y=0; y<groups[x].members; y++)
			{
			if(printers[groups[x].printers[y]].status == PRNSTATUS_IDLE)
				printer_look_for_work(groups[x].printers[y]);
			}
		}

	unlock();
	} /* end of new_group_config() */

/*
** Initialize the queue, loading existing jobs into it.
*/
void initialize_queue(void)
	{
	const char function[] = "initialize_queue";
	DIR *dir;					/* the open queue directory */
	struct dirent *direntp;		/* pointer to the current queue entry */

	DODEBUG_RECOVER(("%s()", function));

	/* Allocate memory to hold the queue. */
	queue_size = QUEUE_SIZE_INITIAL;
	queue = (struct QEntry *)gu_alloc(queue_size, sizeof(struct QEntry));

	/* Open the queue directory. */
	if((dir = opendir(QUEUEDIR)) == (DIR*)NULL)
		fatal(0, "%s(): can't open directory \"%s\", errno=%d (%s)", function, QUEUEDIR, errno, gu_strerror(errno));

	/* loop to read file names */
	while((direntp = readdir(dir)))
		{
		if(direntp->d_name[0] == '.')			/* ignore "." and ".." */
			continue;

		DODEBUG_RECOVER(("%s(): inheriting queue file \"%s\"", function, direntp->d_name));

		queue_accept_queuefile(direntp->d_name, FALSE);
		}

	/* We are done.  Close the queue directory. */
	closedir(dir);

	DODEBUG_RECOVER(("%s(): done", function));
	} /* end of initialize_queue() */

/* end of file */
