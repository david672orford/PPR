/*
** mouse.trincoll.edu:~ppr/src/libppr/ali_str.c
** Copyright 1995, 1998, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last revised 24 March 1998.
*/

/*
** The three functions in this file convert ALI AppleTalk return codes
** to descriptive strings.
*/

#ifdef ATALKTYPE_ali

#include "before_system.h"
#include <sys/types.h>
#include <at/appletalk.h>
#include <at/nbp.h>
#include <at/pap.h>
#include "gu.h"
#include "global_defines.h"


/*
** Convert a pap_errno value to a string.
*/
const char *pap_strerror(int err)
    {
    switch(err)
    	{
	case 0:
	    return "No error";
	case PAPBADPARM:
	    return "Bad parameter";
	case PAPSYSERR:
	    return "System error";
	case PAPTOOMANY:
	    return "Too many sessions";
	case PAPBUSY:
	    return "PAP is busy";
	case PAPTIMEOUT:
	    return "Timeout";
	case PAPHANGUP:
	    return "Hangup";
	case PAPSIZERR:
	    return "Size error";
	case PAPBLOCKED:
	    return "Writes are blocked";
	case PAPDATARECD:
	    return "Incoming data";
	default:
	    return "Unknown";
    	}
    } /* end of pap_strerror() */

/*
** Convert an nbp_errno value to a string.
*/
const char *nbp_strerror(int err)
    {
    switch(err)
    	{
	case 0:
	    return "No error";
	case NBPSYSERR:
	    return "System error";
	case NBPBADNAME:
	    return "Name is invalid";
	case NBPBADPARM:
	    return "Bad parameter";
	case NBPNORESOURCE:
	    return "Out of resources";
	case NBPTABLEFULL:
	    return "Name table full";
	case NBPDUPNAME:
	    return "Duplicate name";
	case NBPNONAME:
	    return "Name no longer registered";
	default:
	    return "Unknown";
    	}
    } /* end of nbp_strerror() */

/*
** Convert a pap_look() return value to a descriptive string.
*/
const char *pap_look_string(int n)
    {
    switch(n)
    	{
	case 0:
	    return "nothing";
	case PAP_CONNECT_RECVD:
	    return "PAP_CONNECT_RECVD";
	case PAP_DATA_RECVD:
	    return "PAP_DATA_RECVD";
	case PAP_WRITE_ENABLED:
	    return "PAP_WRITE_ENABLED";
	case PAP_DISCONNECT:
	    return "PAP_DISCONNECT";
	default:
	    return "<invalid>";
    	}
    } /* end of pap_look_string() */

#endif

/* end of file */
