/*
** mouse:~ppr/src/ppr-papd/ppr-papd_cap.c
** Copyright 1995--2002, Trinity College Computing Center.
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
** Last modified 19 November 2002.
*/

/*
** CAP AppleTalk module for ppr-papd.
*/

#include "before_system.h"
#include <sys/time.h>
#include <netat/appletalk.h>
#include "cap_proto.h"		/* <-- prototypes CAP does not provide */
#include <netinet/in.h>		/* <-- for ntohs() */
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "gu.h"
#include "global_defines.h"
#include "ppr-papd.h"

/* Globals related to the input buffer: */
char readbuf[READBUF_SIZE];	/* Data we have just read from client */
int eoj;			/* Detects end of file from client */
int bytestotal;
int bytesleft;
char *cptr;			/* Next byte for cli_getc() */
int buffer_count;		/* Number of full or partial buffers we have read */
int onebuffer=FALSE;		/* When TRUE, we are not allowed to start the second buffer */

/* Globals related to the output buffer */
char writebuf[WRITEBUF_SIZE];	/* Data waiting to be sent to client */
int write_unit;			/* Max size client will allow us to send */

/* The status of the spooler: */
PAPStatusRec status;

/* The global read and write completion flags: */
int rcomp=0;
int wcomp=0;

/* Global related to debug messages and shutting down. */
extern int i_am_master;

/* forward function reference */
void do_xmit(int sesfd);

/* structure to hold the PAP endpoints */
struct {
    int fd;		/* descriptor of name */
    int gcomp;		/* getnextjob completion flag */
    int sesfd;		/* descriptor returned when getnextjob completes */
    } endpoints[PAPSRV_MAX_NAMES];

/*=====================================================================
** Input buffer routines
=====================================================================*/

/*
** Start buffering the input.
**
** If the parameter is TRUE, we do a hard reset, that is,
** we throw away any buffer we have got.  If it is FALSE,
** we do a soft reset, that is, we just go back to the
** begining of the buffer.
*/
void reset_buffer(int hard)	/* this resets the end of file stuff */
    {				/* and resets the buffer variables to */
    eoj = 0;			/* show that nothing is in the buffer */
    if(hard)
	{
	buffer_count = 0;
	bytestotal = 0;
	}
    bytesleft = bytestotal;
    onebuffer = FALSE;
    } /* end of reset_buffer() */

/*
** Get a character from the client.
*/
int cli_getc(int sesfd)
    {
    int paperr;

    while(bytesleft==0)         	/* If buffer empty, */
	{                       	/* here is where we fill it. */
	if(onebuffer && buffer_count)   /* If one buffer mode and one */
	    return -1;                  /* read done, don't read more. */

	if(eoj)                 	/* If end of job, don't */
	    return -1;          	/* try any more. */

	buffer_count++;			/* Add to count of buffers read. */

	DODEBUG_READBUF(("cli_getc(): attempting to read"));
	if( (paperr=PAPRead(sesfd, readbuf, &bytestotal, &eoj, &rcomp)) != 0)
	    {
	    debug("cli_getc(): PAPRead() returned %d, rcomp=%d", paperr, rcomp);
	    eoj=1;
	    return -1;
	    }

	DODEBUG_READBUF(("cli_getc(): waiting for read to complete"));
	do  {				/* wait for the read to complete */
	    do_xmit(sesfd);		/* allow output routine to work */
	    abSleep(1,TRUE);
	    } while(rcomp > 0);
	DODEBUG_READBUF(("done, eoj=%d", eoj));

	if(rcomp < 0)		/* if read completed with error, */
	    {
	    if(rcomp==sktClosed)
	    	{
	    	DODEBUG_READBUF(("cli_getc(): other party hung up first"));
	    	eoj = 1;
	    	return -1;
	    	}
	    else
	        {
	        fatal(1, "cli_getc(): PAPRead() completed with code %d", rcomp);
	        }
	    }

	bytesleft = bytestotal;	/* set initial value of bytesleft */
	cptr = readbuf;		/* move pointer to begining of buffer */
	}

    /*
    ** If we get this far, we know there
    ** are bytes in the buffer.
    */
    bytesleft--;                /* take a byte */
    return *(cptr++);           /* from the buffer */
    } /* end of cli_getc() */

/*========================================================================
** Output buffer routines.
========================================================================*/

char out[10000];	/* the output buffer */
int hindex=0;		/* head index */
int tindex=0;		/* tail index */
int ocount=0;		/* bytes in out */

/*
** Place a string in the reply buffer.
*/
void reply(int sesfd,char *s)
    {
    DODEBUG_WRITEBUF(("reply(sesfd=%d, s=%s)", s));

    while(*s)
	{
	if(ocount==sizeof(out))		/* check if room in output buffer */
	    fatal(1,"reply(): output buffer overflow");

	out[hindex++]=*(s++);		/* place in output buffer */
	ocount++;

	if(hindex==sizeof(out))		/* wrap around if necessary */
	    hindex=0;
	}

    } /* end of reply() */

/*
** This routine is called by the query answering loop at the
** completion of each query.  It might do something similiar to
** the PostScript operator "flush" which often appears at the end
** of query code.
**
** In the CAP version, this
** routine does nothing because cli_getc() calls do_xmit().
*/
void flush_reply(int sesfd)
    {
    } /* end of flush_reply() */

/*
** Transmit something from the output buffer if we can.
** If there is nothing to transmit or a write is already in
** progress, return immediaty.
*/
void do_xmit(int sesfd)
    {
    int x;
    int paperr;

    if(wcomp > 0)		/* If last write not done, */
    	return;			/* do nothing. */

    if(wcomp < 0)		/* If last write had error, */
	fatal(1,"do_xmit(): PAPWRite() completed with error %d",wcomp);

    if(ocount)			/* If there are bytes in output buffer, */
    	{
	/* Copy a packet sized chunk from the big buffer to the packet buffer. */
	for(x=0;ocount && x<write_unit;ocount--)
    	    {
	    writebuf[x++]=out[tindex++];

	    if(tindex==sizeof(out))		/* If necessary, */
	        tindex=0;			/* wrap around. */
    	    }

	DODEBUG_WRITEBUF(("do_xmit(): writing %d bytes", x));
	if( (paperr=PAPWrite(sesfd,writebuf,x,FALSE,&wcomp)) != 0)
	    fatal(1,"PAPWrite() returned %d",paperr);
	}
    } /* end of do_xmit() */

/*
** Send an end of job indication to the client.
*/
void reply_eoj(int sesfd)
    {
    int paperr;

    DODEBUG_WRITEBUF(("reply_eoj(sesfd=%d)", sesfd));

    /* Completely drain output buffer. */
    while(ocount || wcomp)
	{
	while(wcomp > 0)
	    abSleep(4,TRUE);
	do_xmit(sesfd);
	}

    /* Send the EOJ packet. */
    if( (paperr=PAPWrite(sesfd,"",0,TRUE,&wcomp)) != 0)
	    fatal(1,"reply_eoj(): PAPWrite() returned %d",paperr);

    /* Wait for write to complete. */
    do	{
    	abSleep(4,TRUE);
    	} while(wcomp > 0);

    /* See if write failed. */
    if(wcomp < 0)
    	fatal(1,"PAPWrite() completed with error %d",wcomp);

    } /* end of reply_eoj() */

/*
** Now that all replies are done, close the channel to the client.
*/
void close_reply(int sesfd)
    {
    DODEBUG_WRITEBUF(("close_reply(sesfd=%d)", sesfd));

    PAPClose(sesfd);	/* don't test return code */
    } /* end of close_reply() */

/*
** Put a name on the network and return the file descriptor.
** This is called by read_conf().
*/
void add_name(int prnid)
    {
    static int appletalk_started = FALSE;
    int fd;                     /* server endpoint file descriptor */
    int err;
    const char *name;

    if( ! appletalk_started )	/* Since add_name() will be the first */
	{			/* function in this module to be called, */
	DODEBUG_STARTUP(("initializing appletalk"));
	abInit(FALSE);		/* we will use it to initialize the AppleTalk. */
	nbpInit();
	PAPInit();
	appletalk_started = TRUE;
	strcpy(status.StatusStr, "xThe PPR spooler is starting up.");
	status.StatusStr[0] = (unsigned char)strlen(&status.StatusStr[1]);
	}

    name = adv[prnid].PAPname;

    debug("registering name: %s", name);

    while((err = SLInit(&fd, name, MY_QUANTUM, &status)) != 0)
    	{
	if(err == nbpNoConfirm)
	    {
	    debug("Name registration failed");
	    sleep(1);
	    debug("Retry...");
	    }
	if(err==nbpDuplicate)
	    fatal(1, "Name \"%s\" already exists", name);
	else
    	    fatal(1, "SLInit() failed, err=%d",err);
    	}

    adv[prnid].fd = fd;
    endpoints[prnid].fd = fd;	/* Remember the file descriptor. */
    } /* add_name() */

/*
** AppleTalk dependent part of printjob()
**
** Copy the job to ppr.  We will not use the buffering
** routines to do this, though we will use the buffer
** and the buffering routine global variables.
*/
int appletalk_dependent_printjob(int sesfd, int pipe)
    {
    int writelen;		/* bytes written to pipe to ppr */
    int err;			/* PAPRead error indication */

    bytesleft = bytestotal;	/* undo pap_getline() */
    do	{
	while(bytesleft>0)	/* empty the whole buffer */
	    {			/* into the pipe to ppr */
	    writelen=write(pipe,readbuf,bytesleft);
	    if(writelen==-1)
		fatal(1,"appletalk_dependent_printjob(): write error on pipe, errno=%d (%s)",errno,gu_strerror(errno));
	    bytesleft-=writelen;
	    }
	if(!eoj)
	    {
	    if( (err=PAPRead(sesfd,readbuf,&bytesleft,&eoj,&rcomp)) != 0 )
	    	fatal(1,"PAPRead() returned %d",err);

	    do	{			/* let PAPRead() work */
	    	abSleep(4,TRUE);
	    	} while(rcomp > 0);

	    if(rcomp < 0)
	    	fatal(1,"appletalk_dependent_printjob(): PAPRead() completed with error code %d",rcomp);
	    }
	} while(bytesleft);

    return eoj;
    } /* end of appletalk_dependent_printjob() */

/*
** This is the daemon's main loop where we accept incoming connections.
** This loop never ends.
*/
void appletalk_dependent_daemon_main_loop(void)
    {
    int rquantum=1;		/* too bad the other end does not tell use */
    int x;			/* used to look thru names */
    int papfd;			/* server endpoint handle */
    int sesfd;			/* session handle */
    pid_t pid;			/* process id of our child */
    int err;
    AddrBlock remote;		/* Address of remote */

    /* Run GetNextJob() on each endpoint in the array created by add_name(). */
    for(x=0; x < name_count; x++)
    	{
	if((err = GetNextJob(endpoints[x].fd, &endpoints[x].sesfd, &endpoints[x].gcomp)) != 0)
	    fatal(1, "GetNextJob() returned %d", err);
    	}

    /* Change the status from `starting up' to `receiving'. */
    strcpy(status.StatusStr, "xThe PPR spooler is receiving your job.");
    status.StatusStr[0] = (unsigned char)strlen(&status.StatusStr[1]);

    while(TRUE)					/* loop until killed */
	{
	DODEBUG_LOOP(("waiting for connexion"));

	/* Give the AppleTalk stack time to work. */
	abSleep(10,TRUE);

	/* Try the file descriptor representing each name in turn. */
	for(x=0; x < name_count; x++)
	    {
	    DODEBUG_LOOP(("checking endpoint %d, gcomp=%d", x, endpoints[x].gcomp));

	    if(endpoints[x].gcomp > 0)      /* If command not completed, */
	    	continue;		    /* try the next one. */

	    if(endpoints[x].gcomp < 0)	    /* if error, */
	    	fatal(1, "GetNextJob() completed with error %d on \"%s\"", endpoints[x].gcomp, adv[x].PAPname);

	    DODEBUG_LOOP(("connexion found, sesfd=%d", endpoints[x].sesfd));

	    /* Note this session number and get ready to accept another. */
	    papfd = endpoints[x].fd;		/* name fd */
	    sesfd = endpoints[x].sesfd;		/* session fd */
	    if((err = GetNextJob(endpoints[x].fd, &endpoints[x].sesfd, &endpoints[x].gcomp)) != 0)
		fatal(1, "GetNextJob() returned %d",err);

	    /* Fork so that one copy of this process can continue
	    ** to be the daemon while the other goes off and
	    ** talks with the client.
	    */
	    while( (pid=fork()) == -1 )     /* If we can't fork(), */
		{			    /* then wait and try again. */
		debug("out of processes");  /* (Of course, forking for ppr */
		sleep(60);		    /* may fail later.) */
		}

	    if(pid==0)                  /* if we are the child */
		{
		DODEBUG_LOOP(("Hello, I am the child"));

		i_am_master = FALSE;

		SLClose(papfd);		/* close server's listening socket */

		/* reset termination signal handlers */
		signal(SIGHUP, SIG_DFL);
		signal(SIGINT, SIG_DFL);
		signal(SIGTERM, SIG_DFL);

		/* change SIGCHLD handler */
		signal(SIGCHLD, child_reapchild);

		/*
		** Set up a handler for SIGPIPE which may occur
		** if PPR exits suddenly.
		*/
		signal(SIGPIPE, sigpipe_handler);

		/* compute usable size of write buffer */
		write_unit = (rquantum<=MAX_REMOTE_QUANTUM?rquantum:MAX_REMOTE_QUANTUM) * 512;

		/*
		** In CAP we must make a seperate call in order
		** to find the address of the client.  This address
		** is passed to child_main_loop().  It eventually finds
		** its way into the ppr -r switch.
		*/
		PAPGetNetworkInfo(sesfd, &remote);

		/*
		** Call child_main_loop() which accepts all the
		** queries and jobs.
		*/
		child_main_loop(sesfd, x, (int)ntohs(remote.net), (int)remote.node);

		/*
		** Since child_main_loop() has returned,
		** we are done, the child can exit.
		*/
		PAPClose(sesfd);	/* don't test return code */
		exit(0);		/* sucessful exit */
		}

	    else                        /* if parent */
		{
		children++;             /* add to count of children */
		PAPShutdown(sesfd);	/* close server's copy of connection */
		}

	    } /* end of for loop which tries each endpoint */

	} /* end of outside loop which never ends */
    } /* end of appletalk_dependent_main_loop() */

/*
** Cleanup routine.  This is called by the daemon if it
** is killed or exits due to a fatal error.
*/
void appletalk_dependent_cleanup(void)
    {
    int x;

    for(x=0;x<name_count;x++)	/* remove all the AppleTalk names. */
	{
	debug("Removing name: %s", adv[x].PAPname);
	PAPRemName(adv[x].fd, adv[x].PAPname);
	}

    } /* end of appletalk_dependent_cleanup() */

/* end of file */
