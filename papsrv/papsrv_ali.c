/*
** mouse:~ppr/src/papsrv/papsrv_ali.c
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
** Last modified 21 November 2000.
*/

/*
** AT&T/Apple AppleTalk Library Interface module.
**
** This works with the AppleTalk Network Program which
** comes with StarLAN LAN Manager.  It also works with
** Netatalk and David Chappell's Netatalk ALI compatibility
** library.
*/

#include "before_system.h"

#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

#include <at/appletalk.h>
#include <at/nbp.h>
#include <at/pap.h>
#include "pap_proto.h"		/* prototypes missing from AT&T ALI include files */

#include "gu.h"		/* generally useful functions */
#include "global_defines.h"	/* PPR global include file */
#include "papsrv.h"		/* prototypes for this program */

/* Globals related to the input buffer: */
char readbuf[READBUF_SIZE];	/* Data we have just read from client */
u_char eoj;			/* detects end of file from client */
int bytestotal;
int bytesleft;
char *cptr;			/* next byte for cli_getc() */
int buffer_count;		/* Number of full or partial buffers we have read */
int onebuffer=FALSE;		/* When TRUE, we are not allowed to start the second buffer */

/* Global related to debug messages and shutting down. */
extern int i_am_master;

/*========================================================================
** Output buffer routines.
========================================================================*/

char out[10000];		/* the output buffer */
int blocked = FALSE;		/* TRUE if last write was blocked and data remains in 2ndary buffer */
int hindex=0;			/* output buffer head index */
int tindex=0;			/* output buffer tail index */
int ocount=0;			/* bytes in output buffer */
char writebuf[WRITEBUF_SIZE];	/* secondary output buffer */
int write_unit;			/* max size client will allow us to send */

/*
** Place a string in the reply buffer.
*/
void reply(int sesfd, char *s)
    {
    DODEBUG_WRITEBUF(("reply(sesfd=%d, s=\"%s\")", sesfd, debug_string(s) ));

    while(*s)
	{
	if(ocount==sizeof(out))
	    fatal(1,"reply(): output buffer overflow");

	out[hindex++]=*(s++);
	ocount++;

	if(hindex==sizeof(out))
	    hindex=0;
	}
    } /* end of reply() */

/*
** Attempt to take data from the big output buffer and send
** it to the client.  Return the number of characters we have
** left to send.
**
** This uses the global variables ocount, blocked, out[], writebuf[],
** write_unit, and tindex.
*/
void do_xmit(int sesfd)
    {
    static int wcnt = 0;	/* initialized to zero to satisfy GNU-C */

    DODEBUG_WRITEBUF(("do_xmit(%d): ocount=%d, wcnt=%d, blocked=%s", sesfd, ocount, wcnt, blocked ? "TRUE" : "FALSE"));

    /*
    ** Loop while buffer is not empty or
    ** writes were blocked.
    **
    ** (We will break out if writes are still blocked.)
    */
    while(ocount || blocked)
    	{
	/*
	** Try to stuff more data into the packet buffer.
	*/
	for( ; ocount && wcnt < write_unit; ocount-- )
	   {
	   writebuf[wcnt++] = out[tindex++];

	   if(tindex==sizeof(out))	/* wrap at end of buffer */
		tindex = 0;
    	    }

	/*
	** Send the packet on its way.
	*/
	DODEBUG_WRITEBUF(("do_xmit(): writing %d bytes", wcnt));
	if(pap_write(sesfd, writebuf, wcnt, 0, PAP_NOWAIT ) == -1)
	    {
	    if(pap_errno==PAPBLOCKED)
	    	{
		DODEBUG_WRITEBUF(("do_xmit(): writes are blocked"));
		blocked = TRUE;
		return;
	    	}
	    else
	    	{
		fatal(1,"do_xmit(): pap_write() failed, pap_errno=%d (%s), errno=%d (%s)", pap_errno, pap_strerror(pap_errno), errno, gu_strerror(errno) );
		}
	    }

	/* We did it!  That proves writes are not blocked. */
	blocked = FALSE;
	wcnt = 0;		/* write buffer is empty */
	} /* end of while(ocount || blocked) */
    } /* end of do_xmit() */

/*
** Send an end of job indication to the client.
*/
void reply_eoj(int sesfd)
    {
    DODEBUG_WRITEBUF(("reply_eoj(sesfd=%d)", sesfd));

    while(ocount || blocked)	/* Wait until every last character */
	{			/* has been sent. */
	DODEBUG_WRITEBUF(("reply_eoj(): draining output buffer first"));
	sleep(1);
    	do_xmit(sesfd);
    	}

    if(pap_write(sesfd, "", 0, 1, PAP_WAIT))
	{
	if(pap_errno == PAPHANGUP)
	    {
	    DODEBUG_WRITEBUF(("other party hung up first"));
	    }
	else
	    {
	    fatal(1,"reply_eoj(): pap_write(): pap_errno=%d (%s), errno=%d (%s)",
	    	pap_errno, pap_strerror(pap_errno),
	    	errno, gu_strerror(errno));
	    }
    	}
    } /* end of reply_eoj() */

/*
** Now that all replies are done, close the chanel to the client.
** Notice that we assume the buffer has already been flushed
** by reply_eoj().
*/
void close_reply(int sesfd)
    {
    DODEBUG_WRITEBUF(("close_reply(%d)", sesfd));
    pap_close(sesfd);
    } /* end of close_reply() */

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
    eoj=0;			/* show that nothing is in the buffer */

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
**
** This is called by pap_getline() to read the first line received to see
** if it is a query or a printjob.  If it is a query, getline() will be
** used to read the rest of the query, if it is a printjob, it will not.
**
** This routine will return the character read, unless the PAP connextion
** is at end of job, in which case it will return -1.
**
** This routine relies on the fact David Chappell's ALI library for
** Netatalk defines _NATALI_PAP.  Current versions of this library have
** dificiencies which require work-arounds.
*/
int cli_getc(int sesfd)
    {
    int look_result;		/* the value pap_look() returns */
    fd_set read_fd;		/* lists of file descriptors */
    #ifdef _NATALI_PAP		/* if using NATALI atalk library, */
    struct timeval tv;		/* we will need this to */
    #endif			/* prevent select() from blocking too long. */

    /*
    ** If the input buffer is empty, try to fill it again.
    ** While we are doing that we will dispatch any write
    ** data we can.
    */
    while(bytesleft==0)
	{
	if(onebuffer && buffer_count>0)	/* if one buffer mode */
	    return -1;			/* and one read, don't read more */

	if(eoj)			/* If end of job, don't */
	    return -1;		/* try to read any more. */

	buffer_count++;		/* Add to count of buffers read. */

	DODEBUG_READBUF(("cli_getc(): waiting to reload buffer"));

	/* See what is happening. */
	while(TRUE)
	    {
	    /* Find out what is happening on the PAP endpoint. */
	    look_result = pap_look(sesfd);

	    DODEBUG_READBUF(("cli_getc(): pap_look(%d) = %d", sesfd, look_result));

	    /* Error */
	    if( look_result == -1 )
	    	fatal(1,"cli_getc(): pap_look() failed");
	    /* Other end hung up */
	    else if( look_result == PAP_DISCONNECT )
	    	return -1;
	    /* If data was received, break out */
	    else if( look_result == PAP_DATA_RECVD )
		break;
	    /* Time to try to write */
	    else if( (ocount || blocked) && (!blocked || look_result == PAP_WRITE_ENABLED) )
	    	do_xmit(sesfd);

	    /* Wait for something else to happen */
	    #ifdef _NATALI_PAP		/* If using NATALI, e mustn't let select() */
	    tv.tv_sec = 5;		/* block for more than 5 seconds. */
	    tv.tv_usec = 0;
	    #endif

	    /* We are interested in stuff to read. */
	    FD_ZERO(&read_fd);
	    FD_SET(sesfd, &read_fd);

	    #ifdef _NATALI_PAP
	    while( select(FD_SETSIZE, &read_fd, (fd_set*)NULL, (fd_set*)NULL, &tv) == -1 )
	    #else
	    while( select(FD_SETSIZE, &read_fd, (fd_set*)NULL, (fd_set*)NULL, (struct timeval*)NULL) == -1 )
	    #endif
		{
		if( errno == EINTR )
		    continue;
		fatal(1,"cli_getc(): select() failed, errno=%d (%s)", errno, gu_strerror(errno) );
		}
	    } /* end of while(TRUE) */

	/*
	** If we reach this point, then pap_look() had indicated
	** that there is data to be read.
	**
	** Read a block from the client.
	*/
	DODEBUG_READBUF(("cli_getc(): calling pap_read()"));
	bytesleft = bytestotal = pap_read(sesfd, readbuf, READBUF_SIZE, &eoj);

	#ifdef DEBUG_READBUF
	if(bytesleft != -1)	/* if no error */
	    debug("cli_getc(): pap_read(): %d bytes, eoj=%d", bytestotal, eoj);
	#endif

	if(bytesleft == -1)		/* if pap_read() error */
	    {
	    if(pap_errno==PAPHANGUP)	/* hangup is just end of file */
		{
		DODEBUG_READBUF(("cli_getc(): other party hung up first"));
		eoj = 1;
		return -1;
		}
	    else
		{
		fatal(1,"cli_getc(): pap_read() failed, pap_errno=%d (%s), errno=%d (%s)",
		    pap_errno, pap_strerror(pap_errno), errno, gu_strerror(errno) );
		}
	    }

	cptr = readbuf;           /* reset the read pointer to start of buffer */
	}

    bytesleft--;                /* take a byte */
    return *(cptr++);           /* from the buffer */
    } /* end of cli_getc() */

/*==============================================================
** Put a name on the network and return the file descriptor.
** This is called by read_conf().
==============================================================*/
void add_name(int prnid)
    {
    int fd;                     /* server endpoint file descriptor */
    unsigned char status[257];
    at_entity_t entity;
    const char *name;

    name = adv[prnid].PAPname;

    debug("registering name: %s", name);

    if(nbp_parse_entity(&entity, name))
	fatal(1, "syntax error in server name");

    if((fd = paps_open(MY_QUANTUM)) == -1)
	fatal(1, "paps_open() failed, pap_errno=%d (%s), errno=%d (%s)", pap_errno, pap_strerror(pap_errno), errno, gu_strerror(errno) );

    strcpy((char*)&status[1], "status: The PPR spooler is receiving your job.");
    status[0] = (unsigned char)strlen((char*)&status[1]);
    if(paps_status(fd, status) == -1)
	fatal(1,"paps_status() failed, pap_errno=%d (%s), errno=%d (%s)", pap_errno, pap_strerror(pap_errno), errno, gu_strerror(errno) );

    if(nbp_register(&entity, fd, (at_retry_t*)NULL) == -1)
	fatal(1,"nbp_register() failed, nbp_errno=%d (%s), errno=%d (%s)", nbp_errno, nbp_strerror(nbp_errno), errno, gu_strerror(errno) );

    adv[prnid].fd = fd;
    } /* end of add_name() */

/*========================================================
** AppleTalk dependent part of printjob()
**
** Copy the job to ppr.  We will not use the buffering
** routines to do this, though we will use the buffer
** and the buffering routine global variables.
========================================================*/
int appletalk_dependent_printjob(int sesfd, int pipe)
    {
    int writelen;

    bytesleft = bytestotal;	/* undo the pap_getline() which read %!PS-Adobe- */
    do	{
	while(bytesleft > 0)	/* call write() while there are bytes in the buffer */
	    {
	    #ifdef DEBUG_PRINTJOB_DETAILED
	    debug("appletalk_dependent_printjob(): pipe write, %d bytes",bytesleft);
	    #endif

	    writelen = write(pipe, readbuf, bytesleft);

	    #ifdef DEBUG_PRINTJOB_DETAILED
	    debug("appletalk_dependent_printjob(): wrote %d bytes",writelen);
	    #endif

	    if(writelen == -1)
		fatal(1,"appletalk_dependent_printjob(): write error on pipe, errno=%d (%s)", errno, gu_strerror(errno));

	    bytesleft -= writelen;
	    }

	if(!eoj)		/* If end of job not yet received, */
	    {			/* read from the PAP client. */
	    #ifdef DEBUG_PRINTJOB_DETAILED
	    debug("appletalk_dependent_printjob(): reading from PAP connection");
	    #endif

	    bytesleft = pap_read(sesfd, readbuf, READBUF_SIZE, &eoj);

	    if(bytesleft==-1)
		fatal(1,"appletalk_dependent_printjob(): pap_read() failed, pap_errno=%d (%s), errno=%d (%s)",
			pap_errno, pap_strerror(pap_errno), errno, gu_strerror(errno) );

	    #ifdef DEBUG_PRINTJOB_DETAILED
	    debug("appletalk_dependent_printjob(): pap_read(): %d bytes, eoj=%d",bytesleft,eoj);
	    #endif
	    }
	} while(bytesleft);	/* If the PAP client gave us something, */

    return eoj;
    } /* end of appletalk_dependent_printjob() */

/*
** This is the daemon's main loop where we accept incoming connections.
** This loop never ends.
*/
void appletalk_dependent_daemon_main_loop(void)
    {
    int x;
    u_short rquantum;		/* remote flow quantum */
    at_inet_t other_end;	/* "internet" address of client */
    int look_result;		/* result of pap_look() */
    int papfd;			/* server endpoint handle */
    int sesfd;			/* session handle */
    pid_t pid;			/* pid of child which handles connexion */
    fd_set select_fds;

    /*
    ** The actual daemon main loop starts here.
    */
    while(TRUE)					/* loop until killed */
	{
	/*
	** Wait forever for activity on AppleTalk file descriptors.
	*/
	DODEBUG_LOOP(("waiting for connexion"));

	FD_ZERO( &select_fds );		/* clear list of files desciptors we want watched */

	for(x=0; x<name_count; x++)
	    FD_SET( adv[x].fd, &select_fds );

	while( select(FD_SETSIZE, &select_fds, (fd_set*)NULL, (fd_set*)NULL, (struct timeval*)NULL) == -1 )
	    {
	    if( errno == EINTR )
		{
		DODEBUG_LOOP(("main loop: EINTR, restarting select()"));
	    	continue;
	    	}
	    fatal(1, "main loop: select() failed, errno=%d (%s)", errno, gu_strerror(errno) );
	    }

	/*
	** Something happened!  Find the file descriptor
	** it happened on and act on it.
	*/
	for(x=0; x<name_count; x++)		/* try each file descriptor */
	    {
	    if( ! FD_ISSET( adv[x].fd, &select_fds ) )
	    	continue;

	    DODEBUG_LOOP(("main loop: something happened on fd %d", adv[x].fd));

	    /* The only event we expect is connection received. */
	    if( (look_result = pap_look(adv[x].fd)) != PAP_CONNECT_RECVD )
		{
		if( look_result == -1 )
		    {
		    fatal(1,"pap_look() failed, pap_errno=%d (%s), errno=%d (%s)",
		    	pap_errno, pap_strerror(pap_errno), errno, gu_strerror(errno) );
		    }
		else if( look_result == 0 )	/* No real activity. */
		    {				/* (Netatalk ALI compatibility only.) */
		    DODEBUG_LOOP(("main loop: status request?"));
		    continue;
		    }
		else
		    {
		    debug("main loop: unexpected activity on \"%s\", pap_look() returned %d", adv[x].PAPname, look_result);
		    continue;
		    }
		}

	    papfd = adv[x].fd;	    /* something happened, use this one! */

	    DODEBUG_LOOP(("main loop: connection received on fd %d for \"%s\"", papfd, adv[x].PAPname));

	    /* Accept the new job. */
	    rquantum = 1;		/* meaningless operation */
	    if((sesfd=paps_get_next_job(papfd, &rquantum, &other_end)) == -1)
		fatal(1,"paps_get_next_job() failed, pap_errno=%d (%s), errno=%d (%s)",
			pap_errno, pap_strerror(pap_errno), errno, gu_strerror(errno) );

	    DODEBUG_LOOP(("main loop: remote fd=%d quantum=%hd, net=%d, node=%d, socket=%d",
		sesfd, rquantum, other_end.net,
		other_end.node, other_end.socket));

	    /*
	    ** Fork off a child to handle this new connexion.
	    */
	    while( (pid=fork()) == -1 )     		/* if we can't fork() */
		{			    		/* then wait and try again */
		debug("main loop: out of processes");	/* (of course, forking for ppr */
		sleep(60);		    		/* may fail later) */
		}

	    if(pid==0)                  	/* if we are the child */
		{
		/* remove the termination signal handler */
		signal(SIGHUP,SIG_DFL);
		signal(SIGINT, SIG_DFL);
		signal(SIGTERM, SIG_DFL);

		/* Set flag to show this is the child. */
		i_am_master = FALSE;

		/* close the server endpoints */
		{
		int y;
		for(y=0; y <name_count; y++)
		    pap_abrupt_close( adv[y].fd );
		}

		/*
		** Be polite to the PAP library
		** This is a do-nothing function in NATALI.
		*/
		pap_sync(sesfd);

		/*
		** We don't want ppr in inherit the PAP endpoint.
		*/
		fcntl(sesfd, F_SETFD, 1);

		/*
		** Change SIGCHLD handler to the one for catching ppr termination.
		*/
		signal(SIGCHLD, printjob_reapchild);

		/*
		** Set up a handler for SIGPIPE which may occur
		** if PPR exits suddenly.
		*/
		signal(SIGPIPE,sigpipe_handler);

		/*
		** Compute usable size of write buffer
		*/
		write_unit = (rquantum<=MAX_REMOTE_QUANTUM?rquantum:MAX_REMOTE_QUANTUM) * 512;

		/*
		** Accept all the queries and jobs.
		**
		** We invoke child_main_loop() with the file descriptor
		** of the connection to the client, the index of the
		** name the client connected to, and a string describing
		** the network and node of the client.
		*/
		child_main_loop( sesfd, x, (int)other_end.net, (int)other_end.node );

		DODEBUG_LOOP(("shutting down child server"));

		if( pap_close(sesfd) == -1 )
		    fatal(1,"pap_close() failed, pap_errno=%d (%s), errno=%d (%s)",
		    	pap_errno, pap_strerror(pap_errno), errno, gu_strerror(errno) );

		exit(0);                /* child exits here */
		}

	    else                        /* if parent */
		{
		children++;             /* add to count of children */

		/* pap_sync(sesfd); */		/* <-- don't do this! */
		/* pap_close(sesfd); */		/* <-- don't do this! */
		pap_abrupt_close(sesfd);	/* close without tearing down child's connexion */
		}				/* (If we are using AT&T ALI, this is a macro for close().) */

	    } /* end of for loop which tries each fd after select() returns */

	} /* end of outside loop which never ends */
    } /* end of appletalk_dependent_main_loop() */

/*
** Cleanup routine.
**
** The AT&T ALI library automatically removes all names,
** but the Netatalk ALI compatibility library does not.
*/
void appletalk_dependent_cleanup(void)
    {
    int x;
    at_entity_t name;

    for(x=0; x<name_count; x++)	/* remove all the AppleTalk names. */
	{
	debug("Removing name: %s",adv[x].PAPname);

	if( nbp_parse_entity(&name, adv[x].PAPname) == -1 )
	    debug("nbp_parse_entity() failed");

	if( nbp_remove( &name, adv[x].fd ) == -1 )
	    {
	    debug("nbp_remove() failed, nbp_errno=%d (%s), errno=%d (%s)",
		nbp_errno, nbp_strerror(nbp_errno), errno, gu_strerror(errno) );
	    }
	}

    } /* end of appletalk_independent_cleanup() */

/* end of file */
