/*
** mouse:~ppr/src/pprd/pprd_media.c
** Copyright 1995--2001, Trinity College Computing Center.
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
** Last modified 6 December 2001.
*/

#include "before_system.h"
#include <stdio.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "pprd.h"
#include "pprd.auto_h"

#define MAX_MEDIA 20					/* media name to id translate table */
static char *media[MAX_MEDIA];			/* up to twenty media */
static int media_count = 0;				/* how many do we have now? */

#define MAX_BINNAMES 50					/* bin name to id translate table */
static char *binname[MAX_BINNAMES];
static int binnames_count = 0;

/*=======================================================================================
** Routines for making the mounted media list survive a spooler shutdown and restart
=======================================================================================*/

/*
** Save the mounted media list of a certain printer.
*/
void media_mounted_save(int prnid)
	{
	const char function[] = "media_mounted_save";
	FILE *sf;
	char fname[MAX_PPR_PATH];
	char pbin[MAX_BINNAME];
	char pmedia[MAX_MEDIANAME];
	int x;
	struct Printer *p;

	ppr_fnamef(fname, "%s/%s", MOUNTEDDIR, destid_to_name(nodeid_local(), prnid));
	if((sf = fopen(fname, "w")) == (FILE*)NULL)
		fatal(ERROR_DIE, "%s(): can't open \"%s\"", function, fname);

	p = &printers[prnid];
	for(x=0; x < p->nbins; x++)
		{
		ASCIIZ_to_padded(pbin, get_bin_name(p->bins[x]), sizeof(pbin));
		ASCIIZ_to_padded(pmedia, get_media_name(p->media[x]), sizeof(pmedia));
		fwrite(pbin, sizeof(pbin), 1, sf);
		fwrite(pmedia, sizeof(pmedia), 1, sf);
		}

	fclose(sf);
	} /* end of media_mounted_save() */

/*
** Recover the mounted media list of a certain printer.
*/
void media_mounted_recover(int prnid)
	{
	FILE *rf;
	char fname[MAX_PPR_PATH];
	char pbin[MAX_BINNAME];				/* padded bin name */
	char pmedia[MAX_MEDIANAME];			/* padded media name */
	char abin[MAX_BINNAME+1];			/* ASCIIZ bin name */
	char amedia[MAX_MEDIANAME+1];		/* ASCIIZ media name */
	int x;
	struct Printer *p;

	#ifdef DEBUG_RECOVER
	debug("media_mounted_recover(%d): \"%s\"", prnid, destid_local_to_name(prnid));
	#endif

	ppr_fnamef(fname, "%s/%s", MOUNTEDDIR, destid_to_name(nodeid_local(), prnid));
	if((rf = fopen(fname, "r")) == (FILE*)NULL)
		return;				/* missing file means nothing mounted */

	p = &printers[prnid];
	while(fread(pbin,sizeof(pbin),1,rf) && fread(pmedia,sizeof(pmedia),1,rf))
		{
		padded_to_ASCIIZ(abin,pbin,sizeof(pbin));
		padded_to_ASCIIZ(amedia,pmedia,sizeof(pmedia));
		#ifdef DEBUG_RECOVER
		debug("recovering: abin=\"%s\", amedia=\"%s\"", abin, amedia);
		#endif
		for(x=0; x < p->nbins; x++)
			{
			if(strcmp(get_bin_name(p->bins[x]), abin) == 0)
				{
				p->media[x] = get_media_id(amedia);
				break;
				}
			}
		}

	fclose(rf);
	} /* end of media_mounted_recover() */

/*=======================================================================================
** Media related utility routines
=======================================================================================*/

/*
** Convert a medium id to a name.
*/
const char *get_media_name(int mediaid)
	{
	if(mediaid == -1)									/* none mounted */
		return "";
	else if(mediaid < media_count && mediaid >= 0)		/* valid */
		return media[mediaid];
	else												/* doesn't exist */
		return "<invalid>";
	} /* end of get_media_name() */

/*
** Convert a media name to a media id.
*/
int get_media_id(char *medianame)
	{
	int x;

	/* The value -1 is used to indicate that nothing is mounted on a bin. */
	if(medianame[0] == '\0')
		return -1;

	/* Search for an existing media table entry. */
	for(x=0; x < media_count; x++)
		{
		if(strcmp(media[x], medianame) == 0)	/* if found, */
			return x;							/* pass id back to caller */
		}

	/* Make sure there is room for a new entry. */
	if(x == MAX_MEDIA)
		fatal(0, "get_media_id(): media array overflow");

	/* Add new entry. */
	media[x] = gu_strdup(medianame);
	media_count++;
	return x;
	} /* end of get_media_id() */

/*
** Convert a bin id to a name.
*/
const char *get_bin_name(int binid)
	{
	if(binid < binnames_count && binid >= 0)	/* if we have such a bin name */
		return binname[binid];					/* give its name */
	else
		return "<invalid>";
	} /* end of get_bin_name() */

/*
** Convert a bin name to a media id.
*/
int get_bin_id(char *nbinname)
	{
	int x;

	for(x=0; x < binnames_count; x++)			/* search for it */
		{
		if(strcmp(binname[x], nbinname) == 0)	/* if found, */
			return x;							/* pass id back to caller */
		}

	if(x == MAX_BINNAMES)
		fatal(0, "get_bin_id(): binname[] overflow");

	binname[x] = gu_strdup(nbinname);			  /* add new entry */
	binnames_count++;
	return x;
	} /* end of get_bin_id() */

/*=====================================================================
** These routines are hooks for spooler events such as new job,
** medium mounted, printer started, printer stopt, etc.
=====================================================================*/

static gu_boolean hasmedia(int prnid, struct QEntry *job);
static int stoptmask(int destid);
static void media_set_job_wait_reason(struct QEntry *job, int stopt_members_mask, int inqueue);
static void media_startstop_update_waitreason2(int destid);
static void media_update_notnow2(int destid, int prnbit, int prnid);

/*
** ppop start <printer>
** ppop stop <printer>
**
** This routine is called whenever a printer is "started"
** or "stopt" with ppop.  It updates the status of any job which
** could possibly print on that printer.
**
** startstop_update_waitreason2() is a supporting routine and
** is called only by startstop_update_waitreason().
*/
void media_startstop_update_waitreason(int prnid)
	{
	lock();

	/* Update for jobs with this printer as their dest. */
	media_startstop_update_waitreason2(prnid);

	/* Check each group to see if this printer is a member. */
	{
	int g, g_destid;
	for(g=0; g < group_count; g++)
		{
		g_destid = destid_local_by_gindex(g);
		if(destid_local_get_member_offset(g_destid, prnid) != -1)
			media_startstop_update_waitreason2(g_destid);
		}
	}

	unlock();
	} /* end of media_startstop_update_waitreason() */

static void media_startstop_update_waitreason2(int destid)
	{
	int x;
	int stopt = stoptmask(destid);		/* mask of stop members */

	for(x=0; x < queue_entries; x++)	/* scan the entire queue */
		{
		/* if job is for this destination, */
		if(queue[x].destid == destid && nodeid_is_local_node(queue[x].destnode_id))
			{
			/* set waiting to prn or media */
			media_set_job_wait_reason(&queue[x],stopt,TRUE);
			}
		}
	} /* end of media_startstop_update_waitreason2() */

/*
** ppop mount <printer> <bin> <medium>
**
** Update the notnow bitmask for every job which might print on the specified
** printer.
**
** Code in pprd_ppop.c calls update_notnow() whenever new media is mounted on
** a printer.  This function calls update_notnow2() once for the printer as the
** destination, and once for each group which has the printer as a member.
** It is the job of update_notnow2() to scan the queue for jobs for a specific
** destination and set the specified printer's notnow bit in each matching
** queue entry.
*/
void media_update_notnow(int prnid)
	{

	DODEBUG_NOTNOW(("media_update_notnow()"));

	lock();

	/* Update for jobs with this printer as their dest. */
	media_update_notnow2(prnid, 1, prnid);

	/* For each group if this prn is a member, update it. */
	{
	int g, g_destid, prnbit;
	for(g = 0; g < group_count; g++)
		{
		g_destid = destid_local_by_gindex(g);
		if((prnbit = destid_printer_bit(g_destid, prnid)))
			media_update_notnow2(g_destid, prnbit, prnid);
		}
	}

	unlock();
	} /* end of media_update_notnow() */

static void media_update_notnow2(int destid, int prnbit, int prnid)
	{
	int x;
	int stopt = stoptmask(destid);		/* mask of stop members */

	for(x = 0; x < queue_entries; x++)	/* scan the entire queue */
		{
		if(queue[x].destid == destid)	/* if job is for this destination */
			{							/* then */
			if( hasmedia(prnid, &queue[x]) )
				queue[x].notnow &= (0xFF ^ prnbit);	 /* !!! */
			else
				queue[x].notnow |= prnbit;

			/* set waiting to prn or media */
			media_set_job_wait_reason(&queue[x], stopt, TRUE);
			}
		}
	} /* end of media_update_notnow2() */

/*
** Compute the `notnow' bitmask for a job.  The notnow bitmask indicates
** which printers in the group currently have the required media mounted.
**
** This can operate on a structure which is not in the queue yet, so it need
** not be done with the queue locked, however, the caller may want to lock the
** queue first to be sure the information will not become outdated before it
** can be placed in the queue.
**
** In addition to being used when a job is added to the queue, this routine is
** used when processing a "ppop mount" command and when a group's member list
** changes.
*/
void media_set_notnow_for_job(struct QEntry *nj, gu_boolean inqueue)
	{
	const char function[] = "set_nownow_for_job";

	DODEBUG_NOTNOW(("%s()", function));

	if(!nodeid_is_local_node(nj->destnode_id))
		fatal(0, "%s(): assertion failed", function);

	if(destid_local_is_group(nj->destid))		/* check for each printer: */
		{
		struct Group *gptr;				/* ptr to group array entry */
		int x;
		gptr = &groups[destid_local_to_gindex(nj->destid)];

		nj->notnow = 0;					/* start with clear mask */

		for(x=0;x<gptr->members;x++)	/* do for each printer */
			if( ! hasmedia(gptr->printers[x],nj) )
				{
				nj->notnow |= 1<<x;		/* if hasn't media, set bit */
				}
		}

	else								/* just one printer: */
		{
		if( hasmedia(nj->destid,nj) )	/* if printer has all forms */
			{
			nj->notnow = 0;				/* bit zero clear */
			}
		else							/* else */
			{
			nj->notnow = 1;				/* bit zero set */
			}
		}

	media_set_job_wait_reason(nj, stoptmask(nj->destid), inqueue);
	} /* end of media_set_notnow_for_newjob() */

/*
** Check to see if a particular printer has the proper media
** mounted to print a certain job.  Return TRUE if it has.
*/
static gu_boolean hasmedia(int prnid, struct QEntry *job)
	{
	int x,y;

	DODEBUG_MEDIA(("hasmedia(prnid=%d, struct QEntry job->destid=%d)", prnid, job->destid));

	/* If printer has no bins, assume it can print anything! */
	if(printers[prnid].nbins == 0)
		return TRUE;

	/* If printer has a bin called "AutoSelect",
	   assume it can print anything. */
	if(printers[prnid].AutoSelect_exists)
		return TRUE;

	/* test each requred media type */
	for(x=0;x<MAX_DOCMEDIA && job->media[x]!=-1; x++)
		{
		for(y=0; ;y++)								/* look in every bin */
			{
			if(y==printers[prnid].nbins)			/* if not in any bin */
				{
				DODEBUG_MEDIA(("media absent"));
				return FALSE;						/* then prn hasn't form */
				}
			if(job->media[x]==printers[prnid].media[y])
				break;								/* if found in one bin */
			}										/* that is good enough */
		}

	DODEBUG_MEDIA(("all media present"));
	return TRUE;				  /* return true if all passed test */
	} /* end of hasmedia() */

/*
** Get the stopt mask for a queue.  The stopt mask tells which
** member printers of a group are stopt.  For a printer queue
** it is 1 if the printer is stopt, 0 if it isn't.
*/
static int stoptmask(int destid)
	{
	struct Group *g;
	int mask=0;
	int x;

	if(!destid_local_is_group(destid))
		{
		if(printers[destid].status < PRNSTATUS_DELIBERATELY_DOWN)
			return 0;
		else
			return 1;
		}

	g = &groups[destid_local_to_gindex(destid)];

	for(x=0; x<g->members; x++)
		{
		if(printers[g->printers[x]].status >= PRNSTATUS_DELIBERATELY_DOWN)
			mask |= (1 << x);
		}

	return mask;
	} /* end of stoptmask() */

/*
** If the job status is `waiting for printer' or `waiting for media',
** examine the notnow bits and the list of potential printers which
** are stopt and set the status to the one of the oformentioned
** values that is appropriate.
**
** The caller should call this from a section of code bracketed by
** lock() and unlock().  For this reason, the routine does not call them.
**
** The inqueue parameter is TRUE if the job is already in the queue.  If it
** is, we must call queue_p_job_new_status().
*/
static void media_set_job_wait_reason(struct QEntry *job, int stopt_members_mask, int inqueue)
	{
	const char function[] = "media_set_job_wait_reason";

	if(!nodeid_is_local_node(job->destnode_id))
		fatal(0, "%s(): assertion failed", function);

	if(job->status == STATUS_WAITING || job->status == STATUS_WAITING4MEDIA)
		{
		int new_status;

		if(destid_local_is_group(job->destid))	/* set for a group */
			{
			int allmask = ((1 << groups[destid_local_to_gindex(job->destid)].members) - 1);

			if( ((job->notnow | stopt_members_mask) == allmask) && (stopt_members_mask != allmask) )
				new_status = STATUS_WAITING4MEDIA;
			else
				new_status = STATUS_WAITING;
			}

		else							/* set for a printer */
			{
			if( job->notnow && (stopt_members_mask == 0) )
				new_status = STATUS_WAITING4MEDIA;
			else
				new_status = STATUS_WAITING;
			}

		/* If we have selected a new status, write it now. */
		if(new_status != job->status)
			{
			if(inqueue)
				queue_p_job_new_status(job, new_status);
			else
				job->status = new_status;
			}
		}
	} /* end of media_set_job_wait_reason() */

/* end of file */

