/*
** mouse:~ppr/src/interfaces/atalk_ali.c
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
** Last modified 10 October 2003.
*/

/*
** Apple Printer Access Protocol (PAP) interface.
**
** This version works with the AT&T/Apple AppleTalk Library Interface
** or the ALI compatibility library which David Chappell wrote for Netatalk.
**
** The AppleTalk Library Interface was jointly defined by AT&T and Apple
** Computer.  It is described in "NetBIOS and Transport Interface
** Programmer's Reference, NCR publication number D2-0678-A, November 1991.
*/

#include "before_system.h"
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "interface.h"
#include "libppr_int.h"

/* Includes for ALI */
#ifdef PPR_DARWIN
#include <netat/appletalk.h>
#include <netat/nbp.h>
#include <netat/pap.h>
#else
#include <at/appletalk.h>
#include <at/nbp.h>
#include <at/pap.h>
#endif

/* Prototypes which are missing from the AT&T ALI include files: */
#include "pap_proto.h"

/*
** This enables a hack which causes copy_job() to finish if
** idle_status_interval is non-zero and it notices that
** the printer status has been idle for approximately the
** indicated number of seconds after the job is done.
**
** This should not be necessary.  Don't enable this for no reason.
*/
/* #define IDLE_HACK_TIMEOUT 180 */

/* Values for address_status parameter of open_printer() */
#define ADDRESS_STATUS_UNKNOWN 0
#define ADDRESS_STATUS_MUST_CONFIRM 1
#define ADDRESS_STATUS_RECENT 2

#define LocalQuantum 1					/* The number of 512 byte buffers we provide */
#define MaxWriteQuantum 8				/* Maximum size of output buffer in 512 byte blocks */
volatile int sigusr1_caught = FALSE;	/* set TRUE by the SIGUSR1 signal handler */
sigset_t sigset_sigusr1;				/* a signal set containing SIGUSR1 */
struct timeval *select_timeout_to_use;	/* select() timeout to use */
struct timeval timeval_zero = {0,0};	/* zero seconds */

/* Forward reference. */
int receive(int fd, int flag);

/* Option variables: */
int opt_open_retries = 10;				/* number of times to retry pap_open() */
int opt_lookup_retries = 8;				/* number of times to retry NBP lookup */
int opt_lookup_interval = 1;			/* interval between retries */
int opt_is_laserwriter = TRUE;			/* Is this a LaserWriter compatible PostScript printer? */
int opt_idle_status_interval = 0;		/* ask for status this often */
int opt_address_cache = TRUE;			/* Should we use the address cache? */

#if 0
#define DEBUG 1
#define DODEBUG(a) int_debug a
#else
#undef DEBUG
#define DODEBUG(a)
#endif

/*
** Catch user signal 1.
** This signal indicates that we should consider a zero byte
** read() from the pipe on stdin to be an indication of end of job.
*/
static void sigusr1_handler(int signum)
	{
	DODEBUG(("SIGUSR1 caught"));
	sigusr1_caught = TRUE;						/* set indicator */
	select_timeout_to_use = &timeval_zero;		/* sabatoge any select() which may slip thru in copy_job() */
	} /* end of sigusr1_handler */

/*
** Handler for signals which are likely to terminate this interface.
** We must catch these signals so we can call exit().  The NATALI
** AppleTalk library uses atexit() to install termination handlers.
*/
static void term_handler(int signum)
	{
	DODEBUG(("Killed by signal \"%s\".", gu_strsignal(signum)));
	exit(EXIT_SIGNAL);			/* what about int_exit()? !!! */
	} /* end of term_handler() */

/*
** Call this to explain what went wrong if the connection breaks.
** Basically, this function is used to explain a failed pap_read()
** or a failed pap_write().
*/
static void fatal_pap_error(int pap_error, int sys_error)
	{
	DODEBUG(("fatal pap error, pap_error=%d (%s), sys_error=%d (%s)", pap_error, pap_strerror(pap_error), sys_error, gu_strerror(sys_error)));

	switch(pap_error)
		{
		case PAPSYSERR:
			alert(int_cmdline.printer, TRUE, "Unix system error, sys_error=%d (%s).", sys_error, gu_strerror(sys_error) );
			break;
		case PAPHANGUP:
			alert(int_cmdline.printer, TRUE, "Unexpected hangup.  (Printer turned off while printing?)");
			break;
		case PAPTIMEOUT:
			alert(int_cmdline.printer, TRUE, "Timeout error.  (Network communication problem.)");
			break;
		default:
			alert(int_cmdline.printer, TRUE, "atalk interface: PAP error, pap_error=%d (%s)", pap_error, pap_strerror(pap_error));
			break;
		}
	int_exit(EXIT_PRNERR);
	} /* end of fatal_pap_error() */

/*
** Rename the printer in order to hide it.
**
** This routine must be called after the printer has been
** opened under its original type of "LaserWriter".
**
** This routine is called by open_printer() below.
*/
static void hide_printer(int fd, const char newtype[])
	{
	char writebuf[256];

	DODEBUG(("hiding printer, newtype=\"%s\"",newtype));

	/*
	** Code to change the printer type.  This code seems to
	** temporarily change the type of all LaserWriter compatible
	** printers.  Notice that it does not get the exitserver code
	** or exitserver password from the PPD file.  This could
	** be considered a deficiency, but it is not a serious one.
	*/
	snprintf(writebuf, sizeof(writebuf),
		"%%!PS-Adobe-3.0 ExitServer\n"
		"0 serverdict begin exitserver\n"
		"statusdict begin\n"
		"currentdict /appletalktype known{/appletalktype}{/product}ifelse\n"
		"(%s) def\n"
		"end\n"
		"%%%%EOF\n", newtype);

	/* Write the block, appending an end of job indication. */
	if(pap_write(fd, writebuf, strlen(writebuf), 1, PAP_WAIT) == -1)
		fatal_pap_error(pap_errno, errno);

	/* Throw away the output until EOJ. */
	while( ! receive(fd, FALSE) ) ;
	} /* end of hide_printer() */

/*
** Return a handle which refers to the printer after filling in addr.
** If it fails, return -1.
*/
static int basic_open_printer(at_entity_t *entity, at_nbptuple_t *addr, int *wlen, int do_lookup)
	{
	at_retry_t retries;
	u_short quantum;
	int fd;						/* file discriptor of open printer connexion */
	int ret;					/* return value for various functions */
	u_char more;
	unsigned char status[256];	/* status string from pap_open() */

	DODEBUG(("basic_open_printer(entity={ {%d, \"%.*s\"}, {%d, \"%.*s\"}, {%d, \"%.*s\"} }, addr={%d:%d:%d}, wlen=?, do_lookup=%d)",
		entity->object.len, entity->object.len, entity->object.str,
		entity->type.len, entity->type.len, entity->type.str,
		entity->zone.len, entity->zone.len, entity->zone.str,
		(int)addr->enu_addr.net, (int)addr->enu_addr.node, (int)addr->enu_addr.socket, do_lookup));

	/*
	** Set the retry parameters to the values specified
	** by the interface options.
	*/
	retries.retries = opt_lookup_retries;
	retries.interval = opt_lookup_interval;

	/* Resolve the printer name to an address. */
	if(do_lookup && (ret=nbp_lookup(entity, addr, 1, &retries, &more)) != 1)
		{								/* if failed to return 1 entry */
		if(ret == -1)					/* if lookup error */
			{							/* (i.e. worse than `not found') */
			alert(int_cmdline.printer, TRUE, _("Printer name lookup failed, nbp_errno=%d (%s), errno=%d (%s)."),
				nbp_errno, nbp_strerror(nbp_errno), errno, gu_strerror(errno) );
			int_exit(EXIT_PRNERR);
			}
		if(ret > 1)						/* if multiple printers */
			{
			alert(int_cmdline.printer, TRUE, _("%d printers answer to the name \"%s\"."), ret, int_cmdline.printer);
			int_exit(EXIT_PRNERR);
			}

		DODEBUG(("basic_open_printer(): printer not found"));

		return -1;
		} /* end of if name lookup fails */

	/* Open a circuit to the printer. */
	quantum = LocalQuantum;				/* tell how many 512 byte buffers we will privide */
	if((fd = pap_open(addr, &quantum, status, (short)opt_open_retries)) == -1)
		{
		if(pap_errno == PAPBUSY)		/* If error is because printer */
			{							/* is busy, */
			DODEBUG(("basic_open_printer(): printer is busy"));

			/*
			** Print the status on stdout in the form in which the
			** printer itself would print it under other circumstances.
			** We do this so that ppop status can be more informative.
			*/
			print_pap_status(status);

			int_exit(EXIT_ENGAGED);		/* Use special exit code. */
			}
		else							/* if other error, */
			{							/* other errors, just print message. */
			if(pap_errno == PAPSYSERR)
				{
				alert(int_cmdline.printer, TRUE, "atalk interface: pap_open() failed, pap_errno=%d (%s), errno=%d (%s)",
						pap_errno, pap_strerror(pap_errno), errno, gu_strerror(errno) );
				}
			else
				{
				alert(int_cmdline.printer, TRUE, "atalk interface: pap_open() failed, pap_errno=%d (%s)",
						pap_errno, pap_strerror(pap_errno));
				}
			int_exit(EXIT_PRNERR);
			}
		}

	/*
	** If the status contains the string "status: PrinterError:",
	** print it even if the open was sucessful.
	*/
	if(is_pap_PrinterError(status))
		print_pap_status(status);

	/*
	** Limit remote flow quantum to the max we can do
	** and compute how many bytes we can write at a time.
	*/
	if(quantum > MaxWriteQuantum)
		quantum = MaxWriteQuantum;
	*wlen = quantum * 512;

	DODEBUG(("circuit open, status is: {%d, \"%.*s\"}", (int)status[0], (int)status[0], &status[1] ));

	return fd;
	} /* end of basic_open_printer() */

/*
** Return a handle which refers to the printer.
*/
static int open_printer(const char atalk_name[], at_nbptuple_t *addr, int *wlen, int address_status)
	{
	at_entity_t entity;		/* broken up name */
	int fd;					/* file discriptor of open printer */
	char newtype[33];		/* type to rename printer to */
	int renamed = FALSE;
	int do_lookup = TRUE;

	DODEBUG(("open_printer(\"%s\", wlen)", atalk_name));

	/* Parse the printer name, type, and zone into seperate data items. */
	if(nbp_parse_entity(&entity, atalk_name))
		{
		alert(int_cmdline.printer, TRUE, _("Syntax error in printer address \"%s\"."), atalk_name);
		int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
		}

	if(address_status != ADDRESS_STATUS_UNKNOWN)
		{
		do_lookup = FALSE;

		if(address_status == ADDRESS_STATUS_MUST_CONFIRM)
			{
			if(nbp_confirm(&entity, &(addr->enu_addr), (at_retry_t*)NULL) == -1)
				do_lookup = TRUE;
			}
		}

	/*
	** We will pass at most twice thru three times thru this loop.
	**
	** Pass 1: Try to open the printer with the name as given.  Stop
	**		   here if the entity type is "LaserWriter" or the interface
	**		   option opt_is_laserwriter is FALSE.
	** Pass 2: Try to open as type "LaserWriter" and rename
	**		   if sucessful
	** Pass 3: Try again to open with name as given
	*/
	while(TRUE)
		{
		/*
		** First, try the name as it is.
		** (Second time thru the loop, try it as it originally was.)
		*/
		if((fd = basic_open_printer(&entity, addr, wlen, do_lookup)) >= 0)
			return fd;

		/*
		** If we were already looking for an entity of type "LaserWriter"
		** or we have already tried to hide it or we aren't sending
		** to a laserwriter at all, then give up now.
		*/
		if( (entity.type.len==11 && strncmp(entity.type.str,"LaserWriter",11)==0) || renamed || !opt_is_laserwriter )
			{
			alert(int_cmdline.printer, TRUE, "Printer \"%s\" not found.", atalk_name);
			int_exit(EXIT_PRNERR_NOT_RESPONDING);
			}

		/* Save the type we looked for in "newtype". */
		strncpy(newtype, entity.type.str, entity.type.len);
		newtype[(int)entity.type.len] = '\0';					/* <-- (int) is for GNU-C */

		/* Change the type to "LaserWriter". */
		strcpy(entity.type.str, "LaserWriter");
		entity.type.len = 11;

		/* Try again with the type of "LaserWriter". */
		if((fd = basic_open_printer(&entity, addr, wlen, TRUE)) == -1)
			{
			alert(int_cmdline.printer,TRUE,"Printer \"%s\" not found,", atalk_name);
			alert(int_cmdline.printer,FALSE,"nor is \"%.*s:LaserWriter@%.*s\".",
				(int)entity.object.len,entity.object.str,
				(int)entity.zone.len,entity.zone.str);
			int_exit(EXIT_PRNERR_NOT_RESPONDING);
			}

		/* Now, hide the printer. */
		hide_printer(fd, newtype);
		if(pap_close(fd) == -1)			/* close connexion */
			{
			alert(int_cmdline.printer,TRUE,"atalk interface: pap_close(): failed, pap_errno=%d (%s), errno=%d (%s)",
				pap_errno, pap_strerror(pap_errno), errno, gu_strerror(errno) );
			int_exit(EXIT_PRNERR);
			}

		/* Say we renamed it and get ready for the next try. */
		renamed = TRUE;
		entity.type.len = strlen(newtype);
		strncpy(entity.type.str, newtype, entity.type.len);
		do_lookup = TRUE;
		} /* end of while(TRUE) */

	} /* end of open_printer() */

/*
** Receive data from the printer.
**
** Unlike the receive() function in atalk_cap.c, this function will
** block if there is no data to read.
**
** If the printer sends us EOJ, we will return TRUE.
**
** If the flag is TRUE, the data is copied to stdout,
** otherwise it is discarded.
*/
int receive(int papfd, int flag)
	{
	u_char eoj;							/* used with pap_read() */
	int len;							/* number of bytes read */
	char readbuf[LocalQuantum*512];		/* buffer for messages from the printer */

	DODEBUG(("receive(papfd=%d, flag=%s)", papfd, flag ? "TRUE" : "FALSE"));

	if( (len=pap_read(papfd,readbuf,(LocalQuantum*512),&eoj)) == -1 )
		fatal_pap_error(pap_errno, errno);

	DODEBUG(("%d bytes read from printer, eoj=%d", len, eoj));

	/*
	** Copy the printer message to stdout and flush to
	** make sure pprdrv gets it right away.
	*/
	if(flag)
		{
		fwrite(readbuf,sizeof(char),len,stdout);
		fflush(stdout);
		}

	DODEBUG(("%.*s", len, readbuf));

	DODEBUG(("receive() done"));

	return (eoj ? TRUE : FALSE);
	} /* end of receive() */

/*
** Copy one job.
**
** Return TRUE if we detect that pprdrv has closed its
** end of the pipe.
**
** Notice that the select() code differs slightly depending on
** whether _NATALI_PAP is defined.  It will by defined if
** David Chappell's AppleTalk Library Interface compatiblity
** library for Netatalk is being used in stead of the AT&T
** implementation.
*/
static int copy_job(int papfd, at_nbptuple_t *addr, int wlen)
	{
	int stdin_closed = FALSE;			/* return value: TRUE if end of file on pipe on stdin */
	int lookret;						/* return value of pap_look() */
	int tosend = -1;					/* bytes left to send, -1 means buffer empty, -2 means EOJ sent */
	fd_set rfds;						/* set of file descriptors for select() */
	int server_eoj = FALSE;				/* TRUE if a pap_read() detected EOJ */
	char writebuf[MaxWriteQuantum*512]; /* data to be sent to the printer */
	int previous_sigusr1_caught;
	int selret;							/* return value from select() */
	int writes_blocked = FALSE;
	struct timeval idle_delay;			/* used for select() while waiting for something to do */
	int idle_status_countdown = 1;
	#ifdef IDLE_HACK_TIMEOUT
	int idle_hack_count = 0;
	#endif

	DODEBUG(("copy_job(papfd=%d, wlen=%d)", papfd, wlen));

	sigusr1_caught = FALSE;

	/*
	** Read bytes from the pipe and write them to the printer
	** reading data from the printer whenever necessary.
	**
	** This will continue until we get an end of job indication
	** from the printer in acknowledgement of the one that
	** we will send.
	**
	** If the EOJ block has been sent, tosend will equal -2.
	** If an EOJ block has been received from the printer,
	** server_eoj will be true.
	**
	** So, if we are sending to a LaserWriter, this loop continues
	** until we receive an EOJ indication from the LaserWriter in
	** acknowledgement of our EOJ indicator.
	**
	** If we are not sending to a LaserWriter, this loop continues
	** only until we have sent an EOJ indication.
	*/
	while( ! server_eoj && (opt_is_laserwriter || tosend != -2) )
		{
		/*
		** If the send buffer is empty, see if there is data on stdin.
		** We know that the send buffer is empty when tosend equals -1.
		**
		** If there is no data available now, read() will return -1, setting
		** errno to EAGAIN.  If the other end has closed the pipe, read()
		** will return 0.
		*/
		if(tosend == -1)
			{
			DODEBUG(("copy_job(): reading stdin"));

			if((tosend = read(0, writebuf, wlen)) == -1)
				{
				if(errno == EAGAIN)				/* If no data to read, */
					{
					if(sigusr1_caught)			/* If at a job break, */
						tosend = 0;				/* arrange to send EOJ block, */
					else						/* otherwise, */
						tosend = -1;			/* the send buffer is still empty. */
					}
				else							/* If other read() error, */
					{							/* interface has failed. */
					alert(int_cmdline.printer, TRUE, "atalk interface: copy_job(): read error, errno=%d (%s)", errno, gu_strerror(errno));
					int_exit(EXIT_PRNERR);
					}
				}
			else if(tosend == 0)				/* If end of file, */
				{
				DODEBUG(("copy_job(): pipe closed"));
				stdin_closed = TRUE;
				}
			DODEBUG(("copy_job(): %d bytes read", tosend));
			}

		/*
		** Use pap_look() to see what is happening on the PAP connexion.
		*/
		if((lookret = pap_look(papfd)) == -1)
			{
			alert(int_cmdline.printer, TRUE, "atalk interface: copy_job(): pap_look() failed, pap_errno=%d (%s), errno=%d (%s)", pap_errno, pap_strerror(errno), errno, gu_strerror(errno) );
			int_exit(EXIT_PRNERR);
			}
		DODEBUG(("copy_job(): pap_look() returned %d (%s)", lookret, pap_look_string(lookret) ));

		/*
		** If data has been received from the printer,
		** read it now and go back to the top of the loop.
		**
		** Reading as soon as the printer wants us to is
		** the trick to preventing a deadlock.
		*/
		if(lookret == PAP_DATA_RECVD)
			{
			DODEBUG(("copy_job(): calling receive()"));
			server_eoj = receive(papfd, TRUE);
			continue;
			}

		/*
		** If pap_look() indicates that a previously blocked write
		** can now be made or that it is all right to try to write,
		** though it may block and we have something to send, try now.
		*/
		if( (lookret==PAP_WRITE_ENABLED || (!writes_blocked && lookret==0)) && tosend >= 0 )
			{
			DODEBUG(("copy_job(): sending %d bytes to printer", tosend));

			if( pap_write(papfd, writebuf, tosend, tosend>0?0:1, PAP_NOWAIT) == -1 )
				{
				if(pap_errno == PAPBLOCKED)
					{
					writes_blocked = TRUE;
					DODEBUG(("copy_job(): writes blocked"));
					}
				else	/* a real error */
					{
					fatal_pap_error(pap_errno, errno);
					}
				}
			else		/* pap_write() was entirely sucessful, */
				{
				writes_blocked = FALSE;

				if(tosend != 0)			/* If not the EOJ block, */
					tosend = -1;		/* prompt to load another one. */
				else					/* If it was the EOJ block, use a */
					tosend = -2;		/* value to say "don't read any more from stdin". */
				}
			}

		/*
		** Has pap_look() returned something which indicates an error?
		** (Note: we will have handled PAP_DATA_RECVD above.)
		*/
		if(lookret != 0 && lookret != PAP_WRITE_ENABLED)
			{
			if(lookret == PAP_DISCONNECT)
				fatal_pap_error(PAPHANGUP, 0);
			alert(int_cmdline.printer, TRUE, "atalk interface: copy_job(): pap_look() returned %d (%s)", lookret, pap_look_string(lookret));
			int_exit(EXIT_PRNERR);
			}

		/*
		** If we get here we can assume we are blocked on some front.
		**
		** We can't just keep calling pap_look() because that would eat
		** up scads of CPU time.  We will use select() to block right here
		** until there is some likelyhood that pap_look() will return
		** something different from what it returned last time.
		**
		** More specifically, we use select() to wait for something to
		** happen on the PAP connection, data to be received on stdin,
		** or SIGUSR1 to be received.
		*/
		previous_sigusr1_caught = sigusr1_caught;
		selret = 0;
		while(selret == 0 && (!sigusr1_caught || previous_sigusr1_caught))
			{
			FD_ZERO(&rfds);								/* empty set of read file descriptors */
			if(tosend == -1) FD_SET(0, &rfds);			/* If we want data to send, tell select() to return if we get it. */
			FD_SET(papfd, &rfds);						/* We want to know if anything happens on PAP connexion */

			#define SELECT_TIME 5
			idle_delay.tv_sec = SELECT_TIME;			/* kept short for NATALI */
			idle_delay.tv_usec = 0;
			select_timeout_to_use = &idle_delay;

			/*
			** Unblock SIGUSR1 and call select().  If SIGUSR1 is received
			** the moment we unblock it, it will change select_timeout_to_use
			** so that select() will return immediately.  Otherwise, select()
			** will return when it is interupted by SIGUSR1 or when the timeout
			** expires.
			*/
			DODEBUG(("copy_job(): calling select()"));
			sigprocmask(SIG_UNBLOCK, &sigset_sigusr1, (sigset_t*)NULL);
			selret = select(papfd+1, &rfds, (fd_set*)NULL, (fd_set*)NULL, select_timeout_to_use);
			sigprocmask(SIG_BLOCK, &sigset_sigusr1, (sigset_t*)NULL);

			if(selret == -1)
				{
				if(errno != EINTR)
					{
					alert(int_cmdline.printer, TRUE, "atalk interface: copy_job(): select() failed, errno=%d (%s)", errno, gu_strerror(errno) );
					int_exit(EXIT_PRNERR);
					}
				DODEBUG(("copy_job(): select() interupted"));
				}

			DODEBUG(("copy_job(): select() returned %d", selret));

			/*
			** We are still inside the inner loop which waits
			** at the bottom of the outer loop for something
			** to happen.
			**
			** If nothing has happened on the PAP endpoint and
			** SIGUSR1 has not just been caught,
			*/
			if(selret == 0 && (!sigusr1_caught || previous_sigusr1_caught))
				{
				/*
				** If we are using NATALI rather than the AT&T STREAMS
				** based AppleTalk, we must call a PAP function every
				** so often, otherwise important protocol things such as
				** tickle packets and timeouts won't work.
				*/
				#ifdef _NATALI_PAP
				if(pap_look(papfd) != 0)
					break;
				#endif

				/*
				** Since we don't have anything better to do, we might as well
				** inquire into the present status of the printer.
				*/
				if(opt_idle_status_interval > 0)
					{
					idle_status_countdown -= SELECT_TIME;
					DODEBUG(("idle_status_countdown = %d", idle_status_countdown));

					if(idle_status_countdown <= 0)
						{
						unsigned char status[256];

						if(pap_status(addr, status) == -1)
							{
							DODEBUG(("copy_job(): pap_status() failed, pap_errno=%d", pap_errno));
							}
						else
							{
							DODEBUG(("copy_job(): printer status: %.*s", (int)status[0], status + 1));
							print_pap_status(status);
							fflush(stdout);

							/* This is a hack for queue hangs: */
							#ifdef IDLE_HACK_TIMEOUT
							if(tosend == -2 && strncmp(&status[1], "status: idle", 12) == 0)
								{
								if( (idle_hack_count++ * opt_idle_status_interval) > IDLE_HACK_TIMEOUT )
									{
									DODEBUG(("Printer idle time has exceeded %d seconds,", IDLE_HACK_TIMEOUT));
									DODEBUG(("    exiting dispite printer status:"));
									DODEBUG(("    \"%.*s\"", (int)status[0], &status[1]));
									server_eoj = 1;		/* so outer loop will stop */
									break;				/* break out of inner loop */
									}
								}
							#endif
							}
						idle_status_countdown = opt_idle_status_interval;
						}
					}
				} /* end of if nothing happened */
			} /* end of idle loop */

		} /* until EOJ from printer (at least for LaserWriters) */

	DODEBUG(("copy_job(): returning %s", stdin_closed ? "TRUE" : "FALSE"));
	return stdin_closed;
	} /* end of copy_job() */

/*
** main function
*/
int main(int argc, char *argv[])
	{
	int printer_fd;										/* open circuit to the printer */
	int wlen;											/* max bytes to write to printer */
	at_nbptuple_t printer_numberic_address;				/* AppleTalk address of the printer */
	int address_status;									/* Do we need to look up the AppleTalk address? */

	/* Initialize international messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_INTERFACES, LOCALEDIR);
	textdomain(PACKAGE_INTERFACES);
	#endif

	/* Copy our command line parameters, possibly overridden
	   by certain environment variables into a special structure.
	   */
	int_cmdline_set(argc, argv);

	DODEBUG(("============================================================"));
	DODEBUG(("\"%s\", \"%s\", \"%s\", %d, %s %d",
		int_cmdline.printer,
		int_cmdline.address,
		int_cmdline.options,
		int_cmdline.jobbreak,
		int_cmdline.feedback ? "TRUE" : "FALSE",
		int_cmdline.codes));

	/* Make sure all address components are no longer than
	   the legal length.  Presumably nbp_parse() would catch
	   this later, so I don't know why this code is here. */
	if(strlen(int_cmdline.address) > (32+1+32+1+32))
		{
		alert(int_cmdline.printer, TRUE, _("Printer address \"%s\" is too long."), int_cmdline.address);
		int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
		}

	/* Parse the options string, searching for name=value pairs. */
	{
	struct OPTIONS_STATE o;
	char name[25];
	char value[16];
	int retval;

	options_start(int_cmdline.options, &o);
	while((retval = options_get_one(&o, name, sizeof(name), value, sizeof(value))) > 0)
		{
		if(strcmp(name, "is_laserwriter") == 0)
			{
			if((opt_is_laserwriter = gu_torf(value)) == ANSWER_UNKNOWN)
				{
				o.error = N_("value must be \"true\" or \"false\"");
				retval = -1;
				break;
				}
			}
		else if(strcmp(name, "open_retries") == 0)
			{
			opt_open_retries = atoi(value);
			if(opt_open_retries < -1 || opt_open_retries == 0)
				{
				o.error = N_("value must be a positive integer or -1 (infinity)");
				retval = -1;
				break;
				}
			}
		else if(strcmp(name, "lookup_retries") == 0)
			{
			opt_lookup_retries = atoi(value);
			if(opt_lookup_retries < 1)
				{
				o.error = N_("value must be a positive integer");
				retval = -1;
				break;
				}
			}
		else if(strcmp(name, "lookup_interval") == 0)
			{
			opt_lookup_interval = atoi(value);
			if(opt_lookup_interval < 1)
				{
				o.error = N_("value must be a positive integer");
				retval = -1;
				break;
				}
			}
		/* status_update_interval has been "undocumented" */
		else if(strcmp(name, "status_update_interval") == 0 || strcmp(name, "idle_status_interval") == 0)
			{
			if((opt_idle_status_interval = atoi(value)) < 0)
				{
				o.error = N_("value must be a positive integer or zero");
				retval = -1;
				break;
				}
			}
		else if(strcmp(name, "address_cache") == 0)
			{
			if((opt_address_cache = gu_torf(value)) == ANSWER_UNKNOWN)
				{
				o.error = N_("value must be \"true\" or \"false\"");
				retval = -1;
				break;
				}
			}
		else
			{
			o.error = N_("unrecognized keyword");
			o.index = o.index_of_name;
			retval = -1;
			break;
			}
		}

	if(retval == -1)
		{
		alert(int_cmdline.printer, TRUE, _("Option parsing error: %s"), gettext(o.error));
		alert(int_cmdline.printer, FALSE, "%s", o.options);
		alert(int_cmdline.printer, FALSE, "%*s^ %s", o.index, "", _("right here"));
		int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
		}
	}

	/* Create a signal set for blocking SIGUSR1. */
	sigemptyset(&sigset_sigusr1);
	sigaddset(&sigset_sigusr1, SIGUSR1);

	/* Make sure stdin is in non-blocking mode. */
	gu_nonblock(0, TRUE);

	/*
	** Install a signal handler for the end of job signal
	** and tell our parent (pprdrv) that it is in place
	** by sending it SIGUSR1.
	**
	** (If the jobbreak method is "none" we will not do this.
	** This condition exists so that ppad may invoke the
	** interface to make queries during setup without fear
	** of being hit by a signal.  As of version 1.32, ppad does
	** not yet use this feature.)
	*/
	signal(SIGUSR1, sigusr1_handler);
	sigprocmask(SIG_BLOCK,&sigset_sigusr1,(sigset_t*)NULL);
	if(int_cmdline.jobbreak == JOBBREAK_SIGNAL || int_cmdline.jobbreak == JOBBREAK_SIGNAL_PJL)
		kill(getppid(), SIGUSR1);

	/*
	** Install a handler for any signal which is likely to be used
	** to terminate this interface in order to cancel the job
	** or halt the printer.
	*/
	signal(SIGTERM, term_handler);		/* cancel or halt printer */
	signal(SIGHUP, term_handler);		/* parent (pprdrv) died */
	signal(SIGPIPE, term_handler);		/* conceivable as signal of parent's death */
	signal(SIGINT, term_handler);		/* control-C (during testing) */

	/*
	** If the address_cache= option is true try to
	** read cached printer address information.
	*/
	address_status = ADDRESS_STATUS_UNKNOWN;
	if(opt_address_cache)
		{
		char *p;
		int age;
		if((p = int_addrcache_load(int_cmdline.printer, int_cmdline.int_basename, int_cmdline.address, &age)))
			{
			int net, node, socket;

			/* Read the AppleTalk address */
			if(sscanf(p, "%d %d %d", &net, &node, &socket) != 3)
				{
				alert(int_cmdline.printer, TRUE, X_("atalk interface: sscanf() failed on cached address"));
				}
			else
				{
				printer_numberic_address.enu_addr.net = net;
				printer_numberic_address.enu_addr.node = node;
				printer_numberic_address.enu_addr.socket = socket;

				if(age < 20)
					address_status = ADDRESS_STATUS_RECENT;
				else
					address_status = ADDRESS_STATUS_MUST_CONFIRM;
				}
			gu_free(p);
			}
		}

	/*
	** Open a PAP connexion to the printer.
	*/
	gu_write_string(1, "%%[ PPR connecting ]%%\n");
	printer_fd = open_printer(int_cmdline.address, &printer_numberic_address, &wlen, address_status);
	gu_write_string(1, "%%[ PPR connected ]%%\n");

	/*
	** The main loop.
	** Call copy_job() until it detects that stdin has been closed.
	*/
	while( ! copy_job(printer_fd, &printer_numberic_address, wlen) )
		{
		sigusr1_caught = FALSE;			/* Clear flag which indicates EOJ signal from pprdrv. */
		kill(getppid(), SIGUSR1);		/* Respond to pprdrv's SIGUSR1. */
		}

	/*
	** Close the PAP ciruit to the printer.
	*/
	DODEBUG(("closing PAP connection"));
	if(pap_close(printer_fd) == -1)
		{
		alert(int_cmdline.printer, TRUE, "atalk interface: pap_close(): failed, pap_errno=%d (%s), errno=%d (%s)",
				pap_errno, pap_strerror(pap_errno), errno, gu_strerror(errno) );
		int_exit(EXIT_PRNERR);
		}

	/*
	** Cache the printer address:
	*/
	if(opt_address_cache)
		{
		char temp[16];
		snprintf(temp, sizeof(temp), "%d %d %d",
				(int)printer_numberic_address.enu_addr.net,
				(int)printer_numberic_address.enu_addr.node,
				(int)printer_numberic_address.enu_addr.socket);
		int_addrcache_save(int_cmdline.printer, int_cmdline.int_basename, int_cmdline.address, temp);
		}

	/* We have done everything correctly.  We don't
	   need to call int_exit() because it would do
	   nothing but exit(). */
	return EXIT_PRINTED;
	} /* end of main() */

/* end of file */
