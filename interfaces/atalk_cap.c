/*
** mouse:~ppr/src/interfaces/atalk_cap.c
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
** Last modified 23 October 2003.
*/

/*
** PPR AppleTalk printer interface for use with
** the Columbia AppleTalk Program (CAP).
*/

#include "before_system.h"
#include <sys/time.h>
#include <netat/appletalk.h>
#include <netat/abpap.h>				/* <-- not installed by default */
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include "cap_proto.h"
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "interface.h"
#include "libppr_int.h"

#if 0
#define DODEBUG(a) int_debug a
#else
#define DODEBUG(a)
#endif

/* misc globals */
#define LocalQuantum 1
#define MaxRemoteQuantum 8
volatile int sigusr1_caught = FALSE;
volatile int sigalrm_caught = FALSE;
sigset_t sigset_sigusr1;

/* a forward reference */
int receive(int cno, int flag);

/* Retry variables.  There is as yet no code to consult these values. */
int open_retries = 10;					/* number of times to retry pap_open() */
int lookup_retries = 8;					/* number of times to retry NBP lookup */
int lookup_interval = 1;				/* interval between retries */
int is_laserwriter = TRUE;				/* Is this a LaserWriter compatible PostScript printer? */
int idle_status_interval;

/*
** Handler for SIGUSR1.  We will receive SIGUSR1 from pprdrv when pprdrv
** wants us to send a EOJ indication over the AppleTalk connexion.
*/
void sigusr1_handler(int signum)
	{
	sigusr1_caught = TRUE;
	gu_nonblock(0, TRUE);
	} /* end of sigusr1_handler() */

/*
** Handler for SIGALRM.
*/
void sigalrm_handler(int signum)
	{
	sigalrm_caught = TRUE;
	} /* end of sigalrm_handler() */

/*
** Open a connection to the printer.
** Return -1 if the name is not found.
**
** This routine is called from open_printer().
*/
int basic_open_printer(const char *address, int *wlen)
	{
	PAPStatusRec status;
	int ocomp;
	int cno;
	int ret;
	PAPSOCKET *ps;

	DODEBUG(("basic_open_printer(\"%s\")", address));

	if((ret = PAPOpen(&cno, address, LocalQuantum, &status, &ocomp)) != 0)
		{
		/*
		** CAP returns this dummy status value if NBP
		** fails to find the printer name on the network.
		**
		** The CAP source file abpapc.c, function PAPOpen()
		** contains the snprintf() format string "%Can't find %s".
		** The "%C" is not valid and the result is undefined.
		** Therefor, we look for the intended output (in case
		** someone fixes the bug) and two different observed
		** results.
		*/
		if( strncmp(&status.StatusStr[1], "%Can't find ", 11) == 0
				|| strncmp(&status.StatusStr[1], "Can't find ", 11) == 0
				|| strncmp(&status.StatusStr[1], "an't find ", 11) == 0 )
			{
			DODEBUG(("Can't find it"));
			return -1;					/* let open_printer() deal with it */
			}
		/*
		** We don't know how to deal with other errors.
		*/
		else
			{
			alert(int_cmdline.printer, TRUE, "atalk interface: PAPOpen() returned %d, %s", ret, &status.StatusStr[1]);
			int_exit(EXIT_PRNERR);
			}
		}

	/*
	** Get pointer to PAP socket internal data.
	** We will use this to determine how long we
	** have been waiting for a connection and to
	** determine the remote flow quantum.
	*/
	ps = cnotopapskt(cno);

	/* Wait for PAPOpen() to complete. */
	do	{
		DODEBUG(("Waiting"));

		abSleep(4, TRUE);				/* Let ATalk work for up to 1 second */

		if(ps->po.papO.wtime > 10)		/* If we have been waiting too long */
			{							/* (that is, more than 10 seconds), */
			DODEBUG(("basic_open_printer() timeout"));
			DODEBUG(("(%.*s)", (int)status.StatusStr[0], &status.StatusStr[1]));

			/* Write the status string in printer response format. */
			print_pap_status(status.StatusStr);

			/* Try to cancel the PAPOpen(). */
			PAPClose(cno);

			/*
			** Tell pprdrv that the printer is otherwise
			** engaged or off line.
			*/
			int_exit(EXIT_ENGAGED);
			}

		} while(ocomp > 0);

	if(ocomp < 0)		/* if ocomp indicates an error */
		{
		alert(int_cmdline.printer, TRUE, "atalk interface: PAPOpen() completion code: %d", ocomp);
		int_exit(EXIT_PRNERR);
		}

	/*
	** If the status contains the string "status: PrinterError:",
	** print it even if the open was sucessful.
	*/
	if(is_pap_PrinterError(status.StatusStr))
		print_pap_status(status.StatusStr);

	/*
	** Determine how many bytes at most we
	** may send to the remote end.
	*/
	*wlen = ps->rflowq > MaxRemoteQuantum ? MaxRemoteQuantum * 512 : ps->rflowq * 512;

	DODEBUG(("basic_open_printer() returning cno=%d", cno));

	return cno;
	} /* end of basic_open_printer() */

/*
** This routine is called from open_printer().
*/
void hide_printer(int cno, const char type[], int typelen)
	{
	char buf[512];
	int ret;
	int wcomp;

	DODEBUG(("hide_printer(%d,%s,%d", cno, type, typelen));

	snprintf(buf, sizeof(buf),
		"%%!PS-Adobe-3.0 ExitServer\n"
		"0 serverdict begin exitserver\n"
		"statusdict begin\n"
		"currentdict /appletalktype known{/appletalktype}{/product}ifelse\n"
		"(%.*s) def\n"
		"end\n%%%%EOF\n", typelen, type);

	if((ret = PAPWrite(cno,buf,strlen(buf),TRUE,&wcomp)) < 0)
		{
		alert(int_cmdline.printer, TRUE, "atalk interface: hide_printer(): PAPWrite() failed while changing AppleTalk type, ret=%d", ret);
		int_exit(EXIT_PRNERR);
		}

	do	{						/* Wait for the response, */
		abSleep(4,TRUE);		/* throwing it away. */
		} while(! receive(cno, FALSE));

	DODEBUG(("hide_printer() done"));
	} /* end of hide_printer() */

/*
** This routine opens a printer, hiding it and re-opening
** it if necessary.
*/
int open_printer(const char address[], int *wlen)
	{
	int cno;					/* PAP connexion number */
	char unrenamed[100];		/* space for building printer name */
	int namelen;
	#ifdef GNUC_HAPPY
	int typeoffset = 0;
	int typelen = 0;
	int zoneoffset = 0;
	#else
	int typeoffset, typelen, zoneoffset;
	#endif
	int renamed = FALSE;		/* set to TRUE if we rename the printer */

	DODEBUG(("open_printer(\"%s\")", address));

	/* Split up the name now as a syntax check. */
	if( ! ( (namelen=strcspn(address, ":"))
				&& (namelen <= 32)
				&& (address[namelen] == ':')
				&& (typelen=strcspn(&address[(typeoffset=(namelen+1))], "@"))
				&& (typelen <= 32)
				&& (address[(zoneoffset=(namelen+2+typelen))-1] == '@')
				&& (strlen(&address[zoneoffset]) <= 32)
				) )
		{
		alert(int_cmdline.printer, TRUE, "Syntax error in printer address.");
		int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
		}

	while(TRUE)			/* really, just a goto */
		{
		/* First, try with the name as is. */
		if((cno = basic_open_printer(address,wlen)) >= 0)
			return cno;

		/*
		** If we were looking for a printer of type "LaserWriter", there is
		** nothing more we can do.  Also, if we have renamed a printer
		** and it has not re-appeared yet, there is no point in trying
		** to rename it again.
		*/
		if((strncmp(&address[typeoffset],"LaserWriter",typelen)==0) || renamed || !is_laserwriter)
			{
			alert(int_cmdline.printer, TRUE, "Printer \"%s\" not found.", address);
			int_exit(EXIT_PRNERR_NOT_RESPONDING);
			}

		/*
		** Previously we were looking for a hidden printer.  We didn't
		** find it, so now try with the type changed to "LaserWriter".
		*/
		snprintf(unrenamed, sizeof(unrenamed), "%.*s:LaserWriter@%s", namelen, address, &address[zoneoffset]);
		if((cno = basic_open_printer(unrenamed,wlen)) < 0)
			{
			alert(int_cmdline.printer, TRUE, "Printer \"%s\" not found,", address);
			alert(int_cmdline.printer, FALSE, "nor is \"%s\".", unrenamed);
			int_exit(EXIT_PRNERR_NOT_RESPONDING);
			}

		/* now, send the code to the printer to hide it. */
		hide_printer(cno,&address[typeoffset],typelen);
		renamed = TRUE;

		/* Close the connection, we can do nothing more with it. */
		PAPClose(cno);

		/* Give the printer time to recover. */
		sleep(10);
		} /* end of while(TRUE) */

	} /* end of open_printer() */

/*
** Receive any data the printer has for us.
**
** Return TRUE if an end of job indicator is received.
**
** If the flag is TRUE, the data is copied to stdout,
** otherwise it is discarded.
*/
int receive(int cno, int flag)
	{
	static int rcomp=0;			/* read complete flag, must be static */
	static int rlen=0;			/* bytes read, must be static */
	static int eoj=0;
	static char rbuf[LocalQuantum * 512];
	int paperr;

	DODEBUG(("receive(%d,%d)", cno, flag));

	if(rcomp > 0)						/* If PAPread() not done, return. */
		{
		DODEBUG(("PAPRead() not done"));
		return FALSE;
		}

	if(rcomp < 0)						/* If there was an error, */
		{								/* then make it a printer alert. */
		alert(int_cmdline.printer, TRUE, "atalk interface: PAPRead() completion error: %d", rcomp);
		int_exit(EXIT_PRNERR);
		}

	#ifdef DEBUG
	if(rlen)
		{
		debug("%d bytes received:", rlen);
		debug("%.*s", rlen, rbuf);
		}
	#endif

	if(rlen && flag)					/* If we have something, */
		{								/* and we are not hiding a printer, */
		write(1, rbuf, rlen);			/* send it to stdout. */
		}

	if(eoj)								/* If the read that just completed */
		{								/* detected EOJ, return TRUE. */
		DODEBUG(("EOJ from printer"));
		rlen = 0;
		eoj = 0;
		return TRUE;
		}

	/* Start a new PAPRead(). */
	DODEBUG(("Starting new PAPRead()"));
	if((paperr = PAPRead(cno,rbuf,&rlen,&eoj,&rcomp)) < 0)
		{
		alert(int_cmdline.printer, TRUE, "atalk interface: PAPRead() returned %d", paperr);
		int_exit(EXIT_PRNERR);
		}

	DODEBUG(("receive() done"));

	return FALSE;						/* not server EOF */
	} /* end of receive() */

/*
** Copy the job from stdin to the printer.
** Return TRUE if no more jobs.
*/
int copy_job(int cno, int wlen)
	{
	int wcomp = 0;				/* PAPWrite() completion flag */
	char tobuf[wlen];			/* Buffer to read blocks form pipe and write them to pritner */
	int tolen = 0;				/* number of bytes to write to printer (initial zero is for GNU-C) */
	int paperr;					/* return value of PAPWrite() */
	int server_eoj = FALSE;

	DODEBUG(("copy_job(%d,%d)", cno, wlen));

	do	{
		if( ! server_eoj )		/* We must always receive first */
			server_eoj = receive(cno, TRUE);

		if(wcomp <= 0)			/* If PAPWrite() done or error, */
			{
			if(wcomp < 0)		/* If error, */
				{
				alert(int_cmdline.printer, TRUE, "atalk interface: copy_job(): PAPWrite() completion error: %d",wcomp);
				int_exit(EXIT_PRNERR);
				}

			/*
			** Since PAPWrite() is done, read next block from the
			** pipe on stdin.  We must not let read() block for
			** very long because then the AppleTalk stack would
			** stall.
			*/
			sigprocmask(SIG_UNBLOCK, &sigset_sigusr1, (sigset_t*)NULL);
			sigalrm_caught = FALSE;
			alarm(1);
			while((tolen=read(0, tobuf, wlen)) == -1)
				{
				if(errno == EAGAIN)		/* POSIX response for no data. */
					{
					tolen = 0;
					break;
					}

				if(errno != EINTR)		/* If not interupted system call, */
					{
					alert(int_cmdline.printer, TRUE, "Pipe read error, errno=%d (%s)", errno, gu_strerror(errno));
					int_exit(EXIT_PRNERR);
					}

				if(sigalrm_caught)		/* If read() timed out, */
					{
					/* sigalrm_caught = FALSE; */
					tolen = -1;			/* set special flag value */
					break;
					}
				}
			alarm(0);
			sigprocmask(SIG_BLOCK, &sigset_sigusr1, (sigset_t*)NULL);

			DODEBUG(("%d bytes from pipe", tolen));

			/*
			** If we got a block from stdin, even if it is of length zero,
			** dispatch it to the printer.
			*/
			if(tolen != -1 && (paperr = PAPWrite(cno, tobuf, tolen, tolen == 0 ? TRUE : FALSE, &wcomp)) < 0)
				{
				alert(int_cmdline.printer, TRUE, "atalk interface: copy_job(): PAPWrite() returned %d", paperr);
				int_exit(EXIT_PRNERR);
				}
			} /* end of if write done or error completing PAPwrite() */

		abSleep(4, TRUE);		/* give atalk time to work */

		DODEBUG(("wcomp=%d", wcomp));
		} while(tolen || wcomp);

	DODEBUG(("end of job sent, waiting for response"));

	/*
	** If we are sending to a LaserWriter compatible printer, wait for
	** the rest of the response.  (That is, wait until the server
	** (that is, the printer) sends EOJ.)
	*/
	if(is_laserwriter)
		{
		do	{
			abSleep(4, TRUE);
			} while( ! server_eoj && ! (server_eoj = receive(cno, TRUE)) );
		}

	DODEBUG(("copy_job() done"));

	return sigusr1_caught;		/* return TRUE if more jobs coming */
	} /* end of copy_job() */

/*
** Main (Program entry point)
*/
int main(int argc, char *argv[])
	{
	int printer_cno;			/* connection number */
	int wlen;

	/* Initialize international messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_INTERFACES, LOCALEDIR);
	textdomain(PACKAGE_INTERFACES);
	#endif

	/* Parse the command line and possibly substitute environment
	   variables into the paramater structure for convienence. */
	int_cmdline_set(argc, argv);

	/* Check for --probe. */
	if(int_cmdline.probe)
		{
		fprintf(stderr, _("The interface program \"%s\" does not support probing.\n"), int_cmdline.int_basename);
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
			if((is_laserwriter = gu_torf(value)) == ANSWER_UNKNOWN)
				{
				o.error = N_("value must be \"true\" or \"false\"");
				retval = -1;
				break;
				}
			}
		else if(strcmp(name, "open_retries") == 0)
			{
			open_retries = atoi(value);
			if(open_retries < -1 || open_retries == 0)
				{
				o.error = N_("value must be a positive integer or -1 (infinity)");
				retval = -1;
				break;
				}
			}
		else if(strcmp(name, "lookup_retries") == 0)
			{
			lookup_retries = atoi(value);
			if(lookup_retries < 1)
				{
				o.error = N_("value must be a positive integer");
				retval = -1;
				break;
				}
			}
		else if(strcmp(name, "lookup_interval") == 0)
			{
			lookup_interval = atoi(value);
			if(lookup_interval < 1)
				{
				o.error = N_("value must be a positive integer");
				retval = -1;
				break;
				}
			}
		else if(strcmp(name, "idle_status_interval") == 0)
			{
			if((idle_status_interval = atoi(value)) < 0)
				{
				o.error = N_("value must be a positive integer or zero");
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
		alert(int_cmdline.printer, TRUE, _("Option parsing error:  %s"), gettext(o.error));
		alert(int_cmdline.printer, FALSE, "%s", o.options);
		alert(int_cmdline.printer, FALSE, "%*s^ %s", o.index, "", _("right here"));
		int_exit(EXIT_PRNERR_NORETRY_BAD_SETTINGS);
		}
	}

	/*
	** Define a signal set which contains SIGUSR1
	** we will use this for blocking and unblocking it.
	*/
	sigemptyset(&sigset_sigusr1);
	sigaddset(&sigset_sigusr1, SIGUSR1);

	/*
	** Install the USR1 signal handler, block SIGUSR1,
	** and let pprdrv know we are ready to receive
	** SIGUSR1 end of job notifications.
	**
	** (If the jobbreak method is "none" we will not do this.
	** This condition exists so that ppad may invoke the
	** interface to make queries during setup without fear
	** of being hit by a signal.  As of version 1.32, this feature
	** of ppad is not implemented yet.)
	*/
	signal(SIGUSR1, sigusr1_handler);
	sigprocmask(SIG_BLOCK, &sigset_sigusr1, (sigset_t*)NULL);
	if(int_cmdline.jobbreak == JOBBREAK_SIGNAL || int_cmdline.jobbreak == JOBBREAK_SIGNAL_PJL)
		kill(getppid(),SIGUSR1);

	/*
	** Install the handler for SIGALRM which we will be
	** using to cause read() to timeout in copy_job() so
	** we can call receive().
	*/
	signal(SIGALRM,sigalrm_handler);

	abInit(FALSE);		/* initialize AppleTalk (no display) */
	nbpInit();			/* initialize Name Binding Protocol */
	PAPInit();			/* initialize Printer Access Protocol */

	/* open the connection to the printer */
	gu_write_string(1, "%%[ PPR connecting ]%%\n");
	printer_cno = open_printer(int_cmdline.address, &wlen);
	gu_write_string(1, "%%[ PPR connected ]%%\n");

	/* copy the data */
	while( copy_job(printer_cno,wlen) ) /* continue printing jobs */
		{								/* while we get SIGUSR1 job seperators. */
		gu_nonblock(0, FALSE);			/* Clear the non-blocking flag. */
		sigusr1_caught=FALSE;			/* Clear EOJ indicator. */
		kill(getppid(),SIGUSR1);		/* Tell pprdrv that we understood EOJ. */
		}

	/* close the connection to the printer */
	PAPClose(printer_cno);

	return EXIT_PRINTED;
	} /* end of main() */

/* end of file */

