/*
** mouse:~ppr/src/libppr/ali_str.c
** Copyright 1995--2003, Trinity College Computing Center.
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
** Last modified 21 May 2003.
*/

/*
** The three functions in this file convert ALI AppleTalk return codes
** to descriptive strings.  ALI is the AppleTalk Library Interface defined
** by Apple and AT&T as an AppleTalk API for Unix.
*/

#ifdef ATALKTYPE_ali

#include "config.h"
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
