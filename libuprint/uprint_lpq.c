/*
** mouse:~ppr/src/libuprint/uprint_lpq.c
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
** Last modified 22 April 2002.
*/

#include "before_system.h"
#include <stdio.h>
#include "gu.h"
#include "global_defines.h"
#include "uprint.h"
#include "uprint_private.h"

#define ARGS_SIZE 100

/*
** Handle an lpq style queue request.  The file names
** list is filled with a list of job numbers and
** user names.  This function is used by uprint-lpq
** to process local requests and from the new lprsrv in order
** to process requests from across the network.
**
** The term "agent" is from RFC-1179 and should probably
** be "remote_user".  Notice that it is not used.
*/
int uprint_lpq(uid_t uid, const char agent[], const char queue[], int format, const char *arglist[], gu_boolean remote_too)
    {
    DODEBUG(("uprint_lpq(agent = \"%s\", queue = \"%s\", format = %d, arglist = ?)", agent, queue ? queue : "", format));

    if(queue == (char*)NULL)
    	{
	uprint_error_callback("uprint_lpq(): queue is NULL");
	uprint_errno = UPE_NODEST;
    	return -1;
    	}

    /*
    ** PPR spooler:
    ** Use ppop.
    */
    if(printdest_claim_ppr(queue))
	{
	const char *args[ARGS_SIZE + 1];
	int i, x;

	i = 0;
	args[i++] = "ppop";

	if(uprint_arrest_interest_interval)
	    {
	    args[i++] = "--arrest-interest-interval";
	    args[i++] = uprint_arrest_interest_interval;
	    }

	if(format == 0)
	    args[i++] = "lpq";
	else
	    args[i++] = "nhlist";

	args[i++] = queue;

	if(arglist != (const char **)NULL)
	    {
	    for(x = 0; arglist[x] != (const char *)NULL && x < ARGS_SIZE; x++, i++)
		{
		args[i] = arglist[x];
		}
	    }

	args[i] = (const char *)NULL;

	return uprint_run(uid, PPOP_PATH, args);
    	}

    /*
    ** System V lp:
    ** Use the lpstat program.
    */
    else if(printdest_claim_lp(queue))
	{
	const char *args[ARGS_SIZE];
	int i, x;
	#ifdef LP_LPSTAT_BROKEN
	char temp[32+3];
	#endif

	i = 0;
	args[i++] = "lpstat";

	/* Certain very old versions of lpstat can't parse
	   options arranged according to POSIX rules, with
	   a space between option and argument.  The 32
	   character limit is chosen arbitrarily. */
	#ifdef LP_LPSTAT_BROKEN
	snprintf(temp, sizeof(temp), "-o%s", queue);
	args[i++] = temp;
	#else
	args[i++] = "-o";
	args[i++] = queue;
	#endif

	/* If there are file names (as opposed to no file names
	   which indicates stdin) then add them now. */
	if(arglist != (const char **)NULL)
	    {
	    for(x = 0; arglist[x] && x < ARGS_SIZE; x++, i++)
		{
		args[i] = arglist[x];
		}
	    }

	/* Terminate the argument list. */
	args[i] = (const char *)NULL;

	return uprint_run(uid, uprint_path_lpstat(), args);
    	}

    /*
    ** BSD lpr:
    ** Use the lpq program.
    */
    if(printdest_claim_lpr(queue))
	{
	const char *args[ARGS_SIZE];
	int i, x;

	args[0] = "lpq";
	args[1] = "-P";
	args[2] = queue;
	i = 3;
	if(format != 0)
	    args[i++] = "-l";

	if(arglist != (const char **)NULL)
	    {
	    for(x = 0; arglist[x] != (const char *)NULL && x < ARGS_SIZE; x++, i++)
		{
		args[i] = arglist[x];
		}
	    }

	args[i] = (const char *)NULL;

	return uprint_run(uid, uprint_path_lpq(), args);
	}

    /*
    ** Thru lpr/lpd protocol over the network:
    */
    {
    struct REMOTEDEST info;
    if(remote_too && printdest_claim_remote(queue, &info))
	return uprint_lpq_rfc1179(queue, format, arglist, &info);
    }

    /*
    ** We will only reach here if none of
    ** the spoolers claimed the queue.
    */
    uprint_errno = UPE_UNDEST;
    return -1;
    } /* end of uprint_lpq() */

/* end of file */
