/*
** mouse:~ppr/src/lprsrv/lprsrv_list.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 29 July 1999.
*/

#include "before_system.h"
#include <string.h>
#include <unistd.h>	/* for getuid() */
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

#include "lprsrv.h"
#include "uprint.h"

/*=================================================================
** List the files in the queue
** This function is passed the command character so it can know
** if it should produce a long or short listing.
=================================================================*/
void do_request_lpq(char *command)
    {
    int format;				/* long or short listing */
    char *queue = command + 1;		/* name queue to list */
    #define MAX 100
    const char *list[MAX + 1];

    char *p;
    int i = 0;

    /* Long or short queue format: */
    if(command[0] == 3)
    	format = 0;
    else
    	format = 1;

    /* Find and terminate queue name: */
    p = queue + strcspn(queue, RFC1179_WHITESPACE);
    *p = '\0';

    /* Find the other parameters and build them
       into an array of string pointers. */
    p++;
    p += strspn(p, RFC1179_WHITESPACE);
    while(*p && i < MAX)
    	{
	list[i++] = p;
	p += strcspn(p, RFC1179_WHITESPACE);
	*p = '\0';
	p++;
    	}
    list[i] = (char*)NULL;

    /*
    ** Use the UPRINT routine to run an appropriate command.
    ** If uprint_lpq() returns -1 then the reason is in uprint_errno.
    ** Unless the error is UPE_UNDEST, it will already have called
    ** uprint_error_callback().  If uprint_lpq() runs a command that
    ** fails, it will return the (positive) exit code of that
    ** command.
    */
    if(uprint_lpq(getuid(), "???", queue, format, list, FALSE) == -1)
	{
	if(uprint_errno == UPE_UNDEST)
	    {
	    printf(_("The queue \"%s\" does not exist on the print server \"%s\".\n"), queue, this_node());
	    }
	else
	    {
    	    printf(_("Could not get a queue listing due to a problem with the print server\n"
		"called \"%s\".  Please ask the print server's\n"
    		"system administrator to examine the log file \"%s\"\n"
    		"to learn the details.\n"), this_node(), LPRSRV_LOGFILE);
	    }
    	fflush(stdout);
    	}
    } /* end of do_request_lpq() */

