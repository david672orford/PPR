/*
** mouse:~ppr/src/libuprint/uprint_lprm_rfc1179.c
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
** Last modified 12 August 1999.
*/

#include "before_system.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"

#include "uprint.h"
#include "uprint_private.h"

/*
** Handle an lprm style cancel request.  The file names
** list is filled with a list of job numbers and
** user names.  The queue has previously been specified
** with uprint_set_dest().
*/
int uprint_lprm_rfc1179(const char *user, const char *athost, const char *queue, const char **arglist, struct REMOTEDEST *scratchpad)
    {
    const char function[] = "uprint_lprm_rfc1179";
    int sockfd;
    int x, y;
    char temp[1024];

    DODEBUG(("%s(queue = \"%s\", arglist = ?)", function, queue != (const char *)NULL ? queue : ""));

    if(user == (const char *)NULL)
    	{
	uprint_error_callback("%s(): user is NULL", function);
	uprint_errno = UPE_BADARG;
    	return -1;
    	}

    if(strlen(user) > LPR_MAX_P)
    	{
	uprint_error_callback("%s(): user is too long", function);
	uprint_errno = UPE_BADARG;
    	return -1;
    	}

    if(queue == (const char *)NULL)
    	{
	uprint_error_callback("%s(): queue is NULL", function);
	uprint_errno = UPE_BADARG;
    	return -1;
    	}

    if(arglist == (const char **)NULL)
    	{
	uprint_error_callback("%s(): arglist is NULL", function);
	uprint_errno = UPE_BADARG;
    	return -1;
    	}

    if(scratchpad == (struct REMOTEDEST *)NULL)
    	{
	uprint_error_callback("%s(): scratchpad is NULL", function);
	uprint_errno = UPE_BADARG;
    	return -1;
    	}

    /* Check that the information from /etc/ppr/uprint-remote.conf
       has been filled in. */
    if(scratchpad->node[0] == '\0' || scratchpad->printer[0] == '\0')
    	{
	uprint_error_callback("%s(): printdest_claim_rfc1179() must suceed first", function);
	uprint_errno = UPE_BADORDER;
	return -1;
    	}

    /* Build the list queue command: */
    snprintf(temp, sizeof(temp), "%c%s %s", 5, scratchpad->printer, user);
    y = strlen(temp);

    for(x = 0; arglist[x] != (const char *)NULL; x++)
	{
	if((y + 2 + strlen(arglist[x])) > sizeof(temp))
	    {
	    uprint_errno = UPE_BADARG;
	    uprint_error_callback("%s(): arglist too long", function);
	    return -1;
	    }

	temp[y++] = ' ';
	strcpy(temp + y, arglist[x]);
	y += strlen(arglist[x]);
	}

    temp[y++] = '\n';

    /* Connect to the remote system: */
    if((sockfd = uprint_lpr_make_connection_with_failover(scratchpad->node)) == -1)
    	return -1;

    /* Transmit the command: */
    if(uprint_lpr_send_cmd(sockfd, temp, y) == -1)
	return -1;

    /* Copy from the socket to stdout until the connexion
       is closed by the server. */
    while((x = read(sockfd, temp, sizeof(temp))) > 0)
	{
	write(1, temp, x);
	}

    /* If there was an error, */
    if(x == -1)
	{
	uprint_errno = UPE_TEMPFAIL;
	uprint_error_callback("%s(): read() from remote system failed, errno=%d (%s)", function, errno, gu_strerror(errno));
	return -1;
	}

    close(sockfd);

    return 0;
    } /* end of uprint_lprm_rfc1179() */

/* end of file */
