/*
** mouse:~ppr/src/papd/papd_ali.c
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
** Last modified 23 January 2004.
*/

/*============================================================================
** AT&T/Apple AppleTalk Library Interface (ALI) module.
**
** This works with the AppleTalk Network Program which comes with StarLAN LAN 
** Manager.  It also works with Netatalk and David Chappell's Netatalk ALI 
** compatibility library.
**
** The names of all exported functions in this module begin with "at_".
** It should not use any global variables.
============================================================================*/

#include "before_system.h"
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <at/appletalk.h>
#include <at/nbp.h>
#include <at/pap.h>
#include "pap_proto.h"			/* prototypes missing from AT&T ALI include files */
#include "gu.h"					/* generally useful functions */
#include "global_defines.h"		/* PPR global include file */
#include "papd.h"				/* prototypes for this program */

/*========================================================================
** Output buffer routines.
========================================================================*/

static char out[10240];					/* the output buffer */
static int blocked = FALSE;				/* TRUE if last write was blocked and data remains in 2ndary buffer */
static int hindex=0;					/* output buffer head index */
static int tindex=0;					/* output buffer tail index */
static int ocount=0;					/* bytes in output buffer */
static char writebuf[WRITEBUF_SIZE];	/* secondary output buffer */
static int write_unit;					/* max size client will allow us to send */

/*
** Place a string in the reply buffer.
*/
void at_reply(int sesfd, char *s)
	{
	const char function[] = "at_reply";
	DODEBUG_WRITEBUF(("%s(sesfd=%d, s=\"%s\")", function, sesfd, debug_string(s) ));

	while(*s)
		{
		if(ocount == sizeof(out))
			gu_Throw("%s(): output buffer overflow", function);

		out[hindex++] = *(s++);
		ocount++;

		if(hindex==sizeof(out))
			hindex=0;
		}
	} /* end of at_reply() */

/*
** Attempt to take data from the big output buffer and send
** it to the client.  Return the number of characters we have
** left to send.
**
** This uses the global variables ocount, blocked, out[], writebuf[],
** write_unit, and tindex.
*/
static void do_xmit(int sesfd)
	{
	const char function[] = "do_xmit";
	static int wcnt = 0;		/* initialized to zero to satisfy GNU-C */

	DODEBUG_WRITEBUF(("%s(sesfd=%d): ocount=%d, wcnt=%d, blocked=%s", function, sesfd, ocount, wcnt, blocked ? "TRUE" : "FALSE"));

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

		   if(tindex==sizeof(out))		/* wrap at end of buffer */
				tindex = 0;
			}

		/*
		** Send the packet on its way.
		*/
		DODEBUG_WRITEBUF(("%s(): writing %d bytes", function, wcnt));
		if(pap_write(sesfd, writebuf, wcnt, 0, PAP_NOWAIT ) == -1)
			{
			if(pap_errno==PAPBLOCKED)
				{
				DODEBUG_WRITEBUF(("%s(): writes are blocked", function));
				blocked = TRUE;
				return;
				}
			else
				{
				gu_Throw("%s(): pap_write() failed, pap_errno=%d (%s), errno=%d (%s)", function, pap_errno, pap_strerror(pap_errno), errno, gu_strerror(errno) );
				}
			}

		/* We did it!  That proves writes are not blocked. */
		blocked = FALSE;
		wcnt = 0;				/* write buffer is empty */
		} /* end of while(ocount || blocked) */
	} /* end of do_xmit() */

/*
** Send an end of job indication to the client.
*/
void at_reply_eoj(int sesfd)
	{
	const char function[] = "at_reply_eoj";
	DODEBUG_WRITEBUF(("%s(sesfd=%d)", function, sesfd));

	while(ocount || blocked)	/* Wait until every last character */
		{						/* has been sent. */
		DODEBUG_WRITEBUF(("%s(): draining output buffer first", function));
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
			gu_Throw("%s(): pap_write(): pap_errno=%d (%s), errno=%d (%s)",
				function,
				pap_errno, pap_strerror(pap_errno),
				errno, gu_strerror(errno));
			}
		}
	} /* end of at_reply_eoj() */

/*
** Now that all replies are done, close the chanel to the client.
** Notice that we assume the buffer has already been flushed
** by at_reply_eoj().
*/
void at_close_reply(int sesfd)
	{
	DODEBUG_WRITEBUF(("at_close_reply(%d)", sesfd));
	pap_close(sesfd);
	} /* end of at_close_reply() */

/*=====================================================================
** Input buffer routines
=====================================================================*/

static char readbuf[READBUF_SIZE];		/* Data we have just read from client */
static u_char eoj;						/* detects end of file from client, size dictated by pap_read() */
static int bytestotal;
static int bytesleft;
static char *cptr;						/* next byte for at_getc() */
static int buffer_count;				/* Number of full or partial buffers we have read */

/*
** Start buffering the input.
*/
void at_reset_buffer(void)		/* this resets the end of file stuff */
	{							/* and resets the buffer variables to */
	eoj = 0;					/* show that nothing is in the buffer */
	buffer_count = 0;
	bytestotal = 0;
	bytesleft = bytestotal;
	} /* end of at_reset_buffer() */

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
int at_getc(int sesfd)
	{
	const char function[] = "at_getc";
	int look_result;			/* The value pap_look() returns */
	fd_set read_fd;				/* lists of file descriptors. */
	#ifdef _NATALI_PAP			/* If using NATALI atalk library, */
	struct timeval tv;			/* we will need this to */
	#endif						/* prevent select() from blocking too long. */

	/*
	** If the input buffer is empty, try to fill it again.
	** While we are doing that we will dispatch any write
	** data we can.
	*/
	while(bytesleft==0)
		{
		if(eoj)					/* If end of job, don't */
			return -1;			/* try to read any more. */

		buffer_count++;			/* Add to count of buffers read. */

		DODEBUG_READBUF(("%s(): waiting to reload buffer", function));

		/* See what is happening. */
		while(TRUE)
			{
			/* Find out what is happening on the PAP endpoint. */
			look_result = pap_look(sesfd);

			DODEBUG_READBUF(("%s(): pap_look(%d) = %d", function, sesfd, look_result));

			/* Error */
			if( look_result == -1 )
				gu_Throw("%s(): pap_look() failed", function);

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
			#ifdef _NATALI_PAP			/* If using NATALI, e mustn't let select() */
			tv.tv_sec = 5;				/* block for more than 5 seconds. */
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
				gu_Throw("%s(): select() failed, errno=%d (%s)", function, errno, gu_strerror(errno) );
				}
			} /* end of while(TRUE) */

		/*
		** If we reach this point, then pap_look() had indicated
		** that there is data to be read.
		**
		** Read a block from the client.
		*/
		DODEBUG_READBUF(("%s(): calling pap_read()", function));
		bytesleft = bytestotal = pap_read(sesfd, readbuf, READBUF_SIZE, &eoj);

		#ifdef DEBUG_READBUF
		if(bytesleft != -1)		/* if no error */
			debug("%s(): pap_read(): %d bytes, eoj=%d", function, bytestotal, eoj);
		#endif

		if(bytesleft == -1)				/* if pap_read() error */
			{
			if(pap_errno==PAPHANGUP)	/* hangup is just end of file */
				{
				DODEBUG_READBUF(("%s(): other party hung up first", function));
				eoj = 1;
				return -1;
				}
			else
				{
				gu_Throw("%s(): pap_read() failed, pap_errno=%d (%s), errno=%d (%s)",
					function, pap_errno, pap_strerror(pap_errno), errno, gu_strerror(errno) );
				}
			}

		cptr = readbuf;			  /* reset the read pointer to start of buffer */
		}

	bytesleft--;				/* take a byte */
	return *(cptr++);			/* from the buffer */
	} /* end of at_getc() */


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
	int fd;						/* server endpoint file descriptor */
	unsigned char status[257];
	at_entity_t entity;

	debug("registering name: %s", papname);

	if(nbp_parse_entity(&entity, papname))
		{
		debug("syntax error in server name");
		return -1;
		}

	if((fd = paps_open(MY_QUANTUM)) == -1)
		{
		debug("paps_open() failed, pap_errno=%d (%s), errno=%d (%s)", pap_errno, pap_strerror(pap_errno), errno, gu_strerror(errno) );
		return -1;
		}

	strcpy((char*)&status[1], "status: The PPR spooler is receiving your job.");
	status[0] = (unsigned char)strlen((char*)&status[1]);
	if(paps_status(fd, status) == -1)
		{
		debug("paps_status() failed, pap_errno=%d (%s), errno=%d (%s)", pap_errno, pap_strerror(pap_errno), errno, gu_strerror(errno) );
		return -1;
		}

	if(nbp_register(&entity, fd, (at_retry_t*)NULL) == -1)
		{
		debug("nbp_register() failed, nbp_errno=%d (%s), errno=%d (%s)", nbp_errno, nbp_strerror(nbp_errno), errno, gu_strerror(errno) );
		pap_close(fd);
		return -1;
		}

	return fd;
	} /* end of at_add_name() */

void at_remove_name(const char papname[], int fd)
	{
	const char function[] = "at_remove_name";
	at_entity_t name;

	if(fd != -1)
		{
		debug("Removing name: %s", papname);

		if(nbp_parse_entity(&name, papname) == -1)
			{
			debug("nbp_parse_entity() failed");
			return;
			}

		if(nbp_remove(&name, fd) == -1)
			{
			debug("%s(): nbp_remove() failed, nbp_errno=%d (%s), errno=%d (%s)", function, nbp_errno, nbp_strerror(nbp_errno), errno, gu_strerror(errno) );
			}
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
	int writelen;

	bytesleft = bytestotal;		/* undo the pap_getline() which read %!PS-Adobe- */
	do	{
		while(bytesleft > 0)	/* call write() while there are bytes in the buffer */
			{
			#ifdef DEBUG_PRINTJOB_DETAILED
			debug("%s(): pipe write, %d bytes", function, bytesleft);
			#endif

			writelen = write(pipe, readbuf, bytesleft);

			DODEBUG_PRINTJOB_DETAILED(("%s(): wrote %d bytes", function, writelen));

			if(writelen == -1)
				gu_Throw("%s(): write error on pipe, errno=%d (%s)", function, errno, gu_strerror(errno));

			bytesleft -= writelen;
			}

		if(!eoj)				/* If end of job not yet received, */
			{					/* read from the PAP client. */
			DODEBUG_PRINTJOB_DETAILED(("%s(): reading from PAP connection", function));

			bytesleft = pap_read(sesfd, readbuf, READBUF_SIZE, &eoj);

			if(bytesleft==-1)
				gu_Throw("(): pap_read() failed, pap_errno=%d (%s), errno=%d (%s)",
						pap_errno, pap_strerror(pap_errno), errno, gu_strerror(errno) );

			DODEBUG_PRINTJOB_DETAILED(("%s(): pap_read(): %d bytes, eoj=%d", function, bytesleft, eoj));
			}
		} while(bytesleft);		/* If the PAP client gave us something, */

	return eoj;
	} /* end of at_printjob_copy() */

/*==========================================================================
** This is the daemon's main loop where we accept incoming connections.
** This loop never ends of its own accord.
==========================================================================*/

void at_service(struct ADV *adv)
	{
	const char function[] = "at_service";
	int x;
	fd_set select_fds;			/* List of fds we will look for activity on */
	int maxfd;					/* first argument for select() */
	at_inet_t remote_addr;		/* AppleTalk "internet" address of client */
	u_short rquantum;			/* remote flow quantum */
	int look_result;			/* result of pap_look() */
	int sesfd;					/* session handle */
	pid_t pid;					/* pid of child which handles connexion */

	/*
	** Wait forever for activity on AppleTalk file descriptors.
	*/
	DODEBUG_LOOP(("waiting for connexion"));

	FD_ZERO(&select_fds);				/* clear list of files desciptors we want watched */

	for(maxfd=-1, x=0; adv[x].adv_type != ADV_LAST; x++)
		{
		if(adv[x].adv_type == ADV_ACTIVE && adv[x].fd != -1)
			{
			FD_SET(adv[x].fd, &select_fds);
			if(adv[x].fd > maxfd)
				maxfd = adv[x].fd;
			}
		}
			
	while(select(maxfd + 1, &select_fds, (fd_set*)NULL, (fd_set*)NULL, (struct timeval*)NULL) == -1)
		{
		if(errno == EINTR)
			{
			DODEBUG_LOOP(("%s(): EINTR", function));
			return;
			}
		gu_Throw("%s(): select() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
		}

	/*
	** Something happened!  Find the file descriptor
	** it happened on and act on it.
	*/
	for(x=0; adv[x].adv_type != ADV_LAST; x++)
			{
			if(adv[x].adv_type != ADV_ACTIVE)
				continue;

			if(adv[x].fd == -1)
				continue;

			if( ! FD_ISSET(adv[x].fd, &select_fds) )
				continue;

			DODEBUG_LOOP(("%s(): something happened on fd %d", function, adv[x].fd));

			/* The only event we expect is connection received. */
			if((look_result = pap_look(adv[x].fd)) != PAP_CONNECT_RECVD)
				{
				if(look_result == -1)
					{
					gu_Throw("%s(): pap_look() failed, pap_errno=%d (%s), errno=%d (%s)",
						function, pap_errno, pap_strerror(pap_errno), errno, gu_strerror(errno) );
					}
				else if(look_result == 0)		/* No real activity. */
					{							/* (Netatalk ALI compatibility only.) */
					DODEBUG_LOOP(("%s(): status request?", function));
					continue;
					}
				else
					{
					debug("%s(): unexpected activity on fd %d (\"%s\"), pap_look() returned %d", function, adv[x].fd, adv[x].PAPname, look_result);
					continue;
					}
				}

			DODEBUG_LOOP(("%s(): connection received on fd %d (\"%s\")", function, adv[x].fd, adv[x].PAPname));

			/* Accept the new job. */
			rquantum = 1;				/* meaningless operation */
			if((sesfd = paps_get_next_job(adv[x].fd, &rquantum, &remote_addr)) == -1)
				gu_Throw("%s(): paps_get_next_job() failed, pap_errno=%d (%s), errno=%d (%s)", function, pap_errno, pap_strerror(pap_errno), errno, gu_strerror(errno) );

			DODEBUG_LOOP(("%s(): remote fd=%d quantum=%hd, net=%d, node=%d, socket=%d",
				function,
				sesfd, rquantum, remote_addr.net,
				remote_addr.node, remote_addr.socket));

			/*
			** Fork off a child to handle this new connexion.
			*/
			while((pid = fork()) == -1)					/* if we can't fork() */
				{										/* then wait and try again */
				debug("main loop: out of processes");	/* (of course, forking for ppr */
				sleep(60);								/* may fail later) */
				}

			if(pid == 0)						/* if we are the child */
				{
				/* Close our copies of the server endpoints. */
				{
				int y;
				for(y=0; adv[y].adv_type != ADV_LAST; y++)
					{
					if(adv[y].adv_type == ADV_ACTIVE && adv[y].fd != -1)
						pap_abrupt_close(adv[y].fd);
					}
				}

				/*
				** Be polite to the PAP library
				** This is a do-nothing function in NATALI.
				*/
				pap_sync(sesfd);

				/*
				** We don't want ppr in inherit the PAP endpoint.
				*/
				gu_set_cloexec(sesfd);

				/*
				** Compute usable size of write buffer
				*/
				write_unit = (rquantum <= MAX_REMOTE_QUANTUM ? rquantum : MAX_REMOTE_QUANTUM) * 512;

				/*
				** This callback function contains the loop which services a client connexion.
				** It recognizes the start of queries and print jobs and dispatches them
				** to the proper service functions.
				**
				** We invoke connexion_callback() with the file descriptor of the connexion 
				** to the client, the ADV record for the advertised AppleTalk name which the client
				** connected to, and a string describing the network and node of the client.
				*/
				connexion_callback(sesfd, &adv[x], (int)remote_addr.net, (int)remote_addr.node);

				DODEBUG_LOOP(("shutting down child server"));

				if(pap_close(sesfd) == -1)
					gu_Throw("%s(): pap_close() failed, pap_errno=%d (%s), errno=%d (%s)",
						function, pap_errno, pap_strerror(pap_errno), errno, gu_strerror(errno) );

				exit(0);				/* child exits here */
				}

			else						/* if parent */
				{
				children++;				/* add to count of children */

				/* pap_sync(sesfd); */			/* <-- don't do this! */
				/* pap_close(sesfd); */			/* <-- don't do this! */
				pap_abrupt_close(sesfd);		/* Close without tearing down child's connexion */
				}								/* (If we are using AT&T ALI, this is a macro for close().) */

			} /* end of for loop which tries each fd after select() returns */

	} /* end of at_service() */

/* end of file */
