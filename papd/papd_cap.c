/*
** mouse:~ppr/src/papd/papd_cap.c
** Copyright 1995--2004, Trinity College Computing Center.
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
** Last modified 6 May 2004.
*/

/*============================================================================
** CAP AppleTalk-implementation-dependent module for papd.
============================================================================*/

#include "config.h"
#include <sys/time.h>
#include <netat/appletalk.h>
#include "cap_proto.h"			/* <-- prototypes CAP does not provide */
#include <netinet/in.h>			/* <-- for ntohs() */
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "gu.h"
#include "global_defines.h"
#include "papd.h"

/* The status of the spooler: */
PAPStatusRec status;

/* The global read and write completion flags: */
int rcomp=0;
int wcomp=0;

/* forward function reference */
void do_xmit(int sesfd);

/* structure to hold the PAP endpoints */
struct {
	int fd;				/* descriptor of name */
	int gcomp;			/* getnextjob completion flag */
	int sesfd;			/* descriptor returned when getnextjob completes */
	} endpoints[PAPSRV_MAX_NAMES];

/*=====================================================================
** Input buffer routines
=====================================================================*/

/* Globals related to the input buffer: */
static char readbuf[READBUF_SIZE];		/* Data we have just read from client */
static int eoj;							/* Detects end of file from client */
static int bytestotal;
static int bytesleft;
static char *cptr;						/* Next byte for at_getc() */
static int buffer_count;				/* Number of full or partial buffers we have read */

/*
** Start buffering the input.
*/
void at_reset_buffer(int hard)	/* this resets the end of file stuff */
	{							/* and resets the buffer variables to */
	eoj = 0;					/* show that nothing is in the buffer */
	buffer_count = 0;
	bytestotal = 0;
	bytesleft = bytestotal;
	} /* end of at_reset_buffer() */

/*
** Get a character from the client.
*/
int at_getc(int sesfd)
	{
	const char function[] = "at_getc";
	int paperr;

	while(bytesleft==0)					/* If buffer empty, */
		{								/* here is where we fill it. */
		if(eoj)							/* If end of job, don't */
			return -1;					/* try any more. */

		buffer_count++;					/* Add to count of buffers read. */

		DODEBUG_READBUF(("%s(): attempting to read", function));
		if((paperr = PAPRead(sesfd, readbuf, &bytestotal, &eoj, &rcomp)) != 0)
			{
			debug("%s(): PAPRead() returned %d, rcomp=%d", function, paperr, rcomp);
			eoj = 1;
			return -1;
			}

		DODEBUG_READBUF(("%s(): waiting for read to complete", function));
		do	{							/* wait for the read to complete */
			do_xmit(sesfd);				/* allow output routine to work */
			abSleep(1,TRUE);
			} while(rcomp > 0);
		DODEBUG_READBUF(("done, eoj=%d", eoj));

		if(rcomp < 0)			/* if read completed with error, */
			{
			if(rcomp==sktClosed)
				{
				DODEBUG_READBUF(("%s(): other party hung up first", function));
				eoj = 1;
				return -1;
				}
			else
				{
				gu_Throw("%s(): PAPRead() completed with code %d", function, rcomp);
				}
			}

		bytesleft = bytestotal; /* set initial value of bytesleft */
		cptr = readbuf;			/* move pointer to begining of buffer */
		}

	/*
	** If we get this far, we know there
	** are bytes in the buffer.
	*/
	bytesleft--;				/* take a byte */
	return *(cptr++);			/* from the buffer */
	} /* end of at_getc() */

/*========================================================================
** Output buffer routines.
========================================================================*/

static char out[10240];					/* the output buffer */
static int hindex=0;					/* head index */
static int tindex=0;					/* tail index */
static int ocount=0;					/* bytes in out */
static char writebuf[WRITEBUF_SIZE];	/* data waiting to be sent to client */
static int write_unit;					/* max size client will allow us to send */

/*
** Place a string in the reply buffer.
*/
void at_reply(int sesfd, const char *s)
	{
	const char function[] = "at_reply";
	DODEBUG_WRITEBUF(("%s(sesfd=%d, s=%s)", function, s));

	while(*s)
		{
		if(ocount == sizeof(out))		/* check if room in output buffer */
			gu_Throw("%s(): output buffer overflow", function);

		out[hindex++]=*(s++);			/* place in output buffer */
		ocount++;

		if(hindex==sizeof(out))			/* wrap around if necessary */
			hindex=0;
		}

	} /* end of at_reply() */

/*
** Transmit something from the output buffer if we can.
** If there is nothing to transmit or a write is already in
** progress, return immediaty.
*/
static void do_xmit(int sesfd)
	{
	const char function[] = "do_xmit";
	int x;
	int paperr;

	if(wcomp > 0)				/* If last write not done, */
		return;					/* do nothing. */

	if(wcomp < 0)				/* If last write had error, */
		gu_Throw("%s(): PAPWRite() completed with error %d", function, wcomp);

	if(ocount)					/* If there are bytes in output buffer, */
		{
		/* Copy a packet sized chunk from the big buffer to the packet buffer. */
		for(x=0;ocount && x<write_unit;ocount--)
			{
			writebuf[x++] = out[tindex++];

			if(tindex==sizeof(out))				/* If necessary, */
				tindex=0;						/* wrap around. */
			}

		DODEBUG_WRITEBUF(("%s(): writing %d bytes", function, x));
		if( (paperr=PAPWrite(sesfd, writebuf, x, FALSE, &wcomp)) != 0)
			gu_Throw("PAPWrite() returned %d",paperr);
		}
	} /* end of do_xmit() */

/*
** Send an end of job indication to the client.
*/
void at_reply_eoj(int sesfd)
	{
	const char function[] = "at_reply_eoj";
	int paperr;

	DODEBUG_WRITEBUF(("at_reply_eoj(sesfd=%d)", sesfd));

	/* Completely drain output buffer. */
	while(ocount || wcomp)
		{
		while(wcomp > 0)
			abSleep(4,TRUE);
		do_xmit(sesfd);
		}

	/* Send the EOJ packet. */
	if((paperr = PAPWrite(sesfd, "", 0, TRUE, &wcomp)) != 0)
			gu_Throw("%s(): PAPWrite() returned %d", function, paperr);

	/* Wait for write to complete. */
	do	{
		abSleep(4,TRUE);
		} while(wcomp > 0);

	/* See if write failed. */
	if(wcomp < 0)
		gu_Throw("PAPWrite() completed with error %d",wcomp);

	} /* end of at_reply_eoj() */

/*
** Now that all replies are done, close the channel to the client.
*/
void at_close_reply(int sesfd)
	{
	DODEBUG_WRITEBUF(("at_close_reply(sesfd=%d)", sesfd));

	PAPClose(sesfd);	/* don't test return code */
	} /* end of at_close_reply() */

/*==============================================================
** Put a name on the network and return the file descriptor.
**
** If this function fails it should return -1.  The caller
** will not notice this, so other routines in this module
** should recognized the -1 as a dummy file descriptor number
** which indicates a dead entry.
==============================================================*/

int at_add_name(const char papname[])
	{
	static int appletalk_started = FALSE;
	int fd;						/* server endpoint file descriptor */
	int err;

	/*
	** Since at_add_name() will be the first function in this module to be 
	** called, we will initialize the AppleTalk the first time it is
	** called.  This saves us the bother of having a separate module
	** initialization function.
	*/
	if( ! appletalk_started )
		{
		DODEBUG_STARTUP(("initializing appletalk"));
		abInit(FALSE);
		nbpInit();
		PAPInit();
		appletalk_started = TRUE;
		strcpy(status.StatusStr, "xThe PPR spooler is starting up.");
		status.StatusStr[0] = (unsigned char)strlen(&status.StatusStr[1]);
		}

	DODEBUG_STARTUP(("registering name: %s", papname));

	while((err = SLInit(&fd, papname, MY_QUANTUM, &status)) != 0)
		{
		if(err == nbpNoConfirm)
			{
			debug("Name registration failed");
			sleep(1);
			debug("Retry...");
			}
		if(err==nbpDuplicate)
			gu_Throw("Name \"%s\" already exists", papname);
		else
			gu_Throw("SLInit() failed, err=%d", err);
		}

	endpoints[prnid].fd = fd;	/* Remember the file descriptor. */

	if((err = GetNextJob(endpoints[prnid].fd, &endpoints[prnid].sesfd, &endpoints[prnid].gcomp)) != 0)
		gu_Throw("GetNextJob() returned %d", err);

	return fd;
	} /* at_add_name() */

/*
** Cleanup routine.  This is called by the daemon if it
** is killed or exits due to a fatal error.
*/
void at_remove_name(const char papname[], int fd)
	{
	if(fd != -1)
		{
		DODEBUG_STARTUP(("Removing name: %s", papname));
		PAPRemName(fd, papname);
		}
	} /* end of at_remove_name() */

/*===========================================================================
** AppleTalk-implementation-dependent part of printjob()
**
** Copy the job to ppr.  We will not use the buffering routines to do this, 
** though we will use the buffer and the buffering routine global variables.
===========================================================================*/

int at_printjob_copy(int sesfd, int pipe)
	{
	const char function[] = "at_printjob_copy";
	int writelen;				/* bytes written to pipe to ppr */
	int err;					/* PAPRead error indication */

	bytesleft = bytestotal;		/* undo pap_getline() */
	do	{
		while(bytesleft>0)		/* empty the whole buffer */
			{					/* into the pipe to ppr */
			writelen=write(pipe,readbuf,bytesleft);
			if(writelen==-1)
				gu_Throw("%s(): write error on pipe, errno=%d (%s)", function, errno, gu_strerror(errno));
			bytesleft-=writelen;
			}
		if(!eoj)
			{
			if( (err=PAPRead(sesfd,readbuf,&bytesleft,&eoj,&rcomp)) != 0 )
				gu_Throw("PAPRead() returned %d",err);

			do	{						/* let PAPRead() work */
				abSleep(4,TRUE);
				} while(rcomp > 0);

			if(rcomp < 0)
				gu_Throw("%s(): PAPRead() completed with error code %d", function, rcomp);
			}
		} while(bytesleft);

	return eoj;
	} /* end of at_printjob_copy() */

/*=======================================================================
** This called repeatedly from the daemon's main loop.  It accepts
** incoming connections.
=======================================================================*/

void at_service(struct ADV *adv)
	{
	int rquantum=1;				/* too bad the other end does not tell use */
	int x;						/* used to look thru names */
	int papfd;					/* server endpoint handle */
	int sesfd;					/* session handle */
	pid_t pid;					/* process id of our child */
	int err;
	AddrBlock remote;			/* Address of remote */

	/* Change the status from `starting up' to `receiving'. */
	strcpy(status.StatusStr, "xThe PPR spooler is receiving your job.");
	status.StatusStr[0] = (unsigned char)strlen(&status.StatusStr[1]);

		DODEBUG_LOOP(("waiting for connexion"));

		/* Give the AppleTalk stack time to work. */
		abSleep(10,TRUE);

		/* Try the file descriptor representing each name in turn. */
		for(x=0; adv[x].adv_type != ADV_LAST; x++)
			{
			if(adv[x].adv_type != ADV_ACTIVE)
				continue;

			DODEBUG_LOOP(("checking endpoint %d, gcomp=%d", x, endpoints[x].gcomp));

			if(endpoints[x].gcomp > 0)		/* If command not completed, */
				continue;					/* try the next one. */

			if(endpoints[x].gcomp < 0)		/* if error, */
				gu_Throw("GetNextJob() completed with error %d on \"%s\"", endpoints[x].gcomp, adv[x].PAPname);

			DODEBUG_LOOP(("connexion found, sesfd=%d", endpoints[x].sesfd));

			/* Note this session number and get ready to accept another. */
			papfd = endpoints[x].fd;			/* name fd */
			sesfd = endpoints[x].sesfd;			/* session fd */
			if((err = GetNextJob(endpoints[x].fd, &endpoints[x].sesfd, &endpoints[x].gcomp)) != 0)
				gu_Throw("GetNextJob() returned %d",err);

			/* Fork so that one copy of this process can continue
			** to be the daemon while the other goes off and
			** talks with the client.
			*/
			while((pid = fork()) == -1)			/* If we can't fork(), */
				{								/* then wait and try again. */
				debug("out of processes");	/* (Of course, forking for ppr */
				sleep(60);					/* may fail later.) */
				}

			if(pid==0)					/* if we are the child */
				{
				DODEBUG_LOOP(("Hello, I am the child"));

				SLClose(papfd);			/* close server's listening socket */

				/* compute usable size of write buffer */
				write_unit = (rquantum<=MAX_REMOTE_QUANTUM?rquantum:MAX_REMOTE_QUANTUM) * 512;

				/*
				** In CAP we must make a seperate call in order to find the address of 
				** the client.  This address will be handed on to connexion_callback().
				** It eventually finds its way into the ppr -r switch.
				*/
				PAPGetNetworkInfo(sesfd, &remote);

				/*
				** This callback function contains the loop which services a client connexion.
				** It recognizes the start of queries and print jobs and dispatches them
				** to the proper service functions.
				**
				** We invoke connexion_callback() with the file descriptor of the connexion 
				** to the client, the ADV record for the advertised AppleTalk name which the client
				** connected to, and a string describing the network and node of the client.
				*/
				connexion_callback(sesfd, &adv[x], (int)ntohs(remote.net), (int)remote.node);

				/*
				** Since connexion_callback() has returned, we are done, this child may exit.
				*/
				PAPClose(sesfd);		/* don't test return code */
				exit(0);				/* sucessful exit */
				}

			else						/* if parent */
				{
				PAPShutdown(sesfd);		/* close server's copy of connection */
				}

			} /* end of for loop which tries each endpoint */

	} /* end of at_service() */

/* end of file */
