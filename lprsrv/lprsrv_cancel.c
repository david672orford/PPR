/*
** mouse:~ppr/src/lprsrv/lprsrv_cancel.c
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
** Last modified 2 February 2000.
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

/*
** Handler for the ^E command.  It does some preliminary parsing
** and then passes the work on to a spooler specific function
** defined above.
*/
void do_request_lprm(char *command, const char fromhost[], const struct ACCESS_INFO *access_info)
    {
    const char *queue;		/* queue to delete the jobs from */
    const char *remote_user;	/* user requesting the deletion */
    uid_t uid_to_use;
    const char *proxy_class = (const char *)NULL;
    #define MAX 100
    const char *list[MAX + 1];

    char *p = command;
    int i = 0;

    p++;
    queue = p;				/* first is queue to delete from */
    p += strcspn(queue, RFC1179_WHITESPACE);
    *p = '\0';

    p++;				/* second is remote_user making request */
    remote_user = p;
    p += strcspn(p, RFC1179_WHITESPACE);
    *p = '\0';

    p++;
    DODEBUG_LPRM(("remove_jobs(): queue=\"%s\", remote_user=\"%s\", list=\"%s\"", queue, remote_user, p));

    while(*p && i < MAX)
    	{
	list[i++] = p;
	p += strcspn(p, RFC1179_WHITESPACE);
	*p = '\0';
	p++;
    	}

    list[i] = (char*)NULL;

    get_proxy_identity(&uid_to_use, &proxy_class, fromhost, remote_user, printdest_claim_ppr(queue), access_info);

    /*
    ** Use the UPRINT routine to run an appropriate command.
    ** If uprint_lprm() returns -1 then the reason is in uprint_errno.
    ** Unless the error is UPE_UNDEST, it will already have called
    ** uprint_error_callback().  If uprint_lprm() runs a command that
    ** fails, it will return the (positive) exit code of that
    ** command.
    */
    if(uprint_lprm(uid_to_use, remote_user, proxy_class, queue, list, FALSE) == -1)
	{
	if(uprint_errno == UPE_UNDEST)
	    {
	    printf(_("The queue \"%s\" does not exist on the print server \"%s\".\n"), queue, this_node());
	    }
	else if(uprint_errno == UPE_DENIED)
	    {
	    /* message already printed on stdout */
	    }
	else
	    {
	    printf(_("Could not delete the job or jobs due to a problem with the print server\n"
		"called \"%s\".  Please ask the print server's\n"
		"system administrator to examine the log file \"%s\"\n"
		"to learn the details.\n"), this_node(), LPRSRV_LOGFILE);
	    }
	fflush(stdout);
	}
    } /* end of do_request_lprm() */

/* end of file */

