/*
** mouse:~ppr/src/pprdrv/pprdrv_media.c
** Copyright 1995--2002, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 18 April 2002.
*/

/*
** This module contains routines related to
** media selection.
*/

#include "before_system.h"
#include <errno.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"

#include "pprdrv.h"
#include "interface.h"

/* Global list of mounted media. */
struct MOUNTED mounted[MAX_BINS];

/*
** If the file requires one and only one kind of medium, we will
** set this pointer to point to the name of that medium.
** (The name is the name that appears in the PPR media list,
** not the name that the document's "%%Media:" comment assigns to
** the medium.
*/
char *single_media = (char*)NULL;

/*
** A table which can be used to translate between "%%Media:" comment
** media names and PPR media list media names.
*/
struct Media_Xlate media_xlate[MAX_DOCMEDIA];

/*
** A global variable representing the number of types
** of media in this document.
*/
int media_count;

/*
** The first time this routine is called it will load
** the mounted media list into memory.
** On subsequent calls, it will do nothing.
**
** This routine returns the number of bins, or if the mounted
** media file count not be opened, -1.
*/
int load_mountedlist(void)
	{
	char fname[MAX_PPR_PATH];
	FILE *mountfile;
	static int bincount;
	static int mountedlist_loaded = 0;

	if(mountedlist_loaded++)			/* if list already loaded, */
		return bincount;				/* don't load it again */

	ppr_fnamef(fname, "%s/%s", MOUNTEDDIR, printer.Name);
	if((mountfile = fopen(fname, "r")) != (FILE*)NULL )
		{
		bincount=fread(mounted,sizeof(struct MOUNTED),MAX_BINS,mountfile);
		fclose(mountfile);

		if(bincount==-1)						/* file error */
			fatal(EXIT_PRNERR_NORETRY,"Error %d while reading \"%s\"",errno,fname);

		return bincount;
		}

	return bincount=-1;						/* couldn't open mounted file */
	} /* end of load_mountedlist() */

/*
** Insert the appropriate bin select code to select the input tray which 
** contains the specified medium.
**
** This code is called during banner page generation and at the top of the 
** Document Setup Section (if automatic bin selection is on and the document
** is all to be printed on one type of media).  It may also be called by 
** select_medium_by_dsc_name() which may be called during page setup.
**
** The media name specified is a valid name in the PPR media list, not this 
** jobs media list.	 (Jobs assign their own names to the media they specify.)
**
** If this function suceeds, it will return TRUE.  The code which write the
** document setup section uses this return value to decide if old bin select 
** code should be stripped out.
*/
int select_medium(const char name[])
	{
	char pname[MAX_MEDIANAME];			/* padded version of media name */
	int bincount;
	int x;
	const char *fptr;					/* pointer to feature code */
	const char *tf;						/* set to "True" or "False" */
	char abin[MAX_BINNAME+1];			/* ASCIIZ version of bin name */
	int AutoSelect_exists=FALSE;		/* start by assuming there is no "AutoSelect" bin */

	ASCIIZ_to_padded(pname,name,sizeof(pname));

	if( (bincount=load_mountedlist()) > 0 )		/* if mounted list available and */
		{										/* one or more bins defined */
		for(x=0;x<bincount;x++)					/* loop thru all bins */
			{
			padded_to_ASCIIZ(abin,mounted[x].bin,MAX_BINNAME);

			if(strcmp(abin,"AutoSelect")==0)	/* don't settle for autoselect yet */
				{								/* just note its existence */
				AutoSelect_exists=TRUE;
				continue;
				}

			if(padded_cmp(mounted[x].media,pname,MAX_MEDIANAME))
				{								/* if this bin has the media */
				if((fptr = find_feature("*InputSlot",abin)))
					{							/* if PPD file has fragment */
					begin_stopped();
					printer_printf("%%%%BeginFeature: *InputSlot %s\n",abin);
					printer_puts(fptr);					/* insert it within */
					printer_puts("\n%%EndFeature\n");  /* Feature comments */
					end_stopped("*InputSlot",abin);

					/* Set trayswitch feature */
					if(strcmp(abin,"Upper")==0 || strcmp(abin,"Lower")==0)
						{								/* if bin was "Upper" or "Lower", */
						fptr=(char*)NULL;
						tf="False";						/* start with trayswitch flag of "False" */
						for(x++ ;x<bincount; x++)		/* look for another bin */
							{							/* starting after the one we found */
							if(padded_cmp(mounted[x].media,pname,MAX_MEDIANAME)
								&& (strcmp(abin,"Upper")==0				/* if we found another */
									|| strcmp(abin,"Lower")==0 ) )		/* Upper or Lower */
								{										/* with the same media, */
								tf="True";								/* set trayswitch flag to True */
								break;
								}
							}
						if((fptr = find_feature("*TraySwitch", tf)))
							{											/* If this printer has trayswitch feature */
							begin_stopped();
							printer_printf("%%%%BeginFeature: *TraySwitch %s\n",tf);
							printer_puts(fptr);
							printer_puts("\n%%EndFeature\n");
							end_stopped("*TraySwitch",tf);
							}
						} /* end if if "Upper" or "Lower" bin */

					} /* end of if *InputSlot code in PPD file */
				return TRUE;			/* do strip binselects */
				} /* end of if this bin has the media */
			} /* end of outer loop thru all bins and their mounted media */

		/* We have not yet matched the media to a normal bin. */
		/* If there is an "AutoSelect" bin, select it. */
		if(AutoSelect_exists)
			{
			if((fptr = find_feature("*InputSlot", "AutoSelect")))
				{
				begin_stopped();
				printer_puts("%%BeginFeature: *InputSlot AutoSelect\n");
				printer_puts(fptr);
				printer_puts("\n%%EndFeature\n");
				end_stopped("*InputSlot","AutoSelect");
				}
			} /* end of if(AutoSelect_exists) */

		return TRUE;			/* do strip binselects */
		} /* end of if mounted media list available */

	return FALSE;		/* don't strip binselects */
	} /* end of select_medium() */

/*
** This function is called at the start of each page to select the
** correct media for that page if automatic bin selection is on
** and the document contains more than one type of media.
*/
int select_medium_by_dsc_name(const char media[])
	{
	int x;

	for(x=0; x < media_count; x++)
		{
		if( strcmp(media_xlate[x].dscname,media)==0)
			{
			select_medium(media_xlate[x].pprname);
			break;
			}
		}

	return FALSE;
	} /* end of select_medium_by_dsc_name() */

/*
** Read the "Media:" lines from the queue file.
*/
void read_media_lines(FILE *q)
	{
	char *line = NULL;
	int line_available = 80;

	media_count=0;

	while((line = gu_getline(line, &line_available, q)))
		{
		if(strcmp(line, "EndMedia") == 0)		/* Until the end of queue file section. */
			break;

		if(strncmp(line, "Media: ", 7) == 0)
			{
			if(media_count == MAX_DOCMEDIA)
				fatal(EXIT_JOBERR, "Queue files contains more than %d \"Media:\" lines", MAX_DOCMEDIA);

			if(gu_sscanf(line, "Media: %S %S", &media_xlate[media_count].pprname,&media_xlate[media_count].dscname) != 2)
				fatal(EXIT_PRNERR_NORETRY, "Invalid \"Media:\" line in queue file");

			media_count++;
			}
		}

	if(line) gu_free(line);
	} /* end of read_media_lines() */

/* end of file */
