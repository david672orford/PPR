/*
** mouse:~ppr/src/pprdrv/pprdrv_feedback.c
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
** Last modified 4 June 2004.
*/

/*===========================================================================
** This module handles data coming back from the interface.  This will 
** generally be messages sent by this printer.  These messages include 
** notification of PostScript errors, printer status messages, and anything 
** the PostScript job writes to stdout.
===========================================================================*/

#include "before_system.h"
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "pprdrv.h"
#include "interface.h"

/* size of buffer for feedback_reader() */
#define FEEDBACK_BUFSIZE 4096

/* For representing unprintables: */
static const char hexchars[] = "0123456789ABCDEF";

static int intfd = -1;					/* fd to read from */
static gu_boolean posterror = FALSE;	/* true if a PostScript error is detected */
static gu_boolean ghosterror = FALSE;	/* true if a non-PostScript Ghostscript error is detected */

enum PJL_STATE
		{
		PJL_INITIAL,
		PJL_USTATUS_PAGE,
		PJL_USTATUS_JOB,
		PJL_USTATUS_JOB_START,
		PJL_USTATUS_JOB_END,
		PJL_USTATUS_DEVICE
		};

static gu_boolean pjl_seen;						/* at least one @PJL USTATUS JOB seen */
static gu_boolean pjl_job_active;				/* in job according to PJL messages */
static gu_boolean tbcp_from_printer;			/* ^AM received */
static enum PJL_STATE pjl_state;				/* PJL multi-line response parsing state */
static gu_boolean pjl_pagedrop;					/* Set on every page message */
static int pjl_pagecount;						/* Number of "PAGE" messages received */
static int pjl_chargable_pagecount;				/* Total of EOJ PAGES= responses */

/* These hold the last PJL USTATUS message. */
static enum PJL_ONLINE pjl_online;
static int pjl_code;
static char pjl_display[80];
static int pjl_code2;
static char pjl_display2[80];

/* These are prototypes for internal functions to which there are forward references. */
static void pjl_ustatus_clear(void);

/*===================================================================
** Get the feedback signal handler set up
** and turn on signals.
===================================================================*/
void feedback_setup(int fd)
	{
	FUNCTION4DEBUG("feedback_setup")

	DODEBUG_FEEDBACK(("%s(fd=%d)", function, fd));

	intfd = fd;									/* save the file descriptor */

	pjl_seen = FALSE;
	pjl_job_active = FALSE;
	tbcp_from_printer = FALSE;

	pjl_state = PJL_INITIAL;
	pjl_pagedrop = FALSE;
	pjl_pagecount = 0;
	pjl_chargable_pagecount = -1;				/* none received */

	pjl_ustatus_clear();

	} /* end of feedback_setup() */

/* This function clears the variables that store the last
   PJL USTATUS message.  It is called from feedback_setup()
   and at the start of each PJL USTATUS message. */
static void pjl_ustatus_clear(void)
	{
	pjl_online = PJL_ONLINE_UNKNOWN;
	pjl_code = 0;
	pjl_display[0] = '\0';
	pjl_code2 = 0;
	pjl_display2[0] = '\0';
	}

/*==========================================================================
** This function is called periodically to read messages coming back
** from the printer over the pipe to the interface program.  Sometimes
** the file descriptor will be in non-blocking mode.
**
** This data will be output of the PostScript "print" command, PostScript
** error messages, printer error messages such as "out of paper" and
** possibly other things.
==========================================================================*/
int feedback_reader(void)
	{
	const char *function = "feedback_reader";
	static int already_here = 0;
	static unsigned char buf[FEEDBACK_BUFSIZE + 1];

	int total_bytes = 0;
	int last_read_len;					/* bytes from last read() */
	int buflen;							/* bytes now in buffer */
	int linelen;						/* length of current line */
	int x;								/* used for stepping thru the buffer */
	unsigned char *ptr;					/* pointer to next line in buffer */
	char *ptr2;

	DODEBUG_FEEDBACK(("%s()", function));

	/*
	** Loop to read the data from the pipe.
	*/
	while(TRUE)
		{
		DODEBUG_FEEDBACK(("%s(): top of loop, already_here=%d", function, already_here));

		/* Read from interface's stdout/stderr. */
		while((last_read_len = read(intfd, buf+already_here, 4096-already_here)) < 0)
			{
			if(errno == EAGAIN)			/* Handle the result of POSIX O_NONBLOCK.*/
				{						/* (System V O_NDELAY mode causes read() */
				last_read_len = 0;		/* to return zero, O_NONBLOCK causes EAGAIN.) */
				break;
				}

			if(errno == EINTR)
				error("%s(): read() interupted", function);

			fatal(EXIT_PRNERR, "%s(): error reading from interface, errno=%d (%s)", function, errno, gu_strerror(errno));
			}

		if(last_read_len == 0)
			break;

		DODEBUG_FEEDBACK(("%s(): read %d new byte%s", function, last_read_len, last_read_len != 1 ? "s" : ""));
		DODEBUG_FEEDBACK(("%s(): \"%.*s\"", function, last_read_len, buf+already_here));

		total_bytes += last_read_len;
		buflen = last_read_len + already_here;

		/*
		** Convert line endings.  This includes converting carriage
		** return termination to line feed termination and converting
		** CRLF termination to LF termination.
		**
		** This also removes the control-D's that some protocols
		** introduce into the the data stream to indicate end
		** of job.
		**
		** We do not remove the form-feeds what HP PJL uses to indicate
		** the end of a record.
		*/
		{
		int y, c;
		static int lastc = -1;
		for(x=y=already_here; x < buflen; )
			{
			c = buf[x++];

			switch(c)
				{
				case '\r':				/* Carriage returns get */
					buf[y++] = '\n';	/* converted to line feeds. */
					break;

				case '\n':				/* Line feeds get discarded unless */
					if(lastc != '\r')	/* they don't follow CR's. */
						buf[y++] = c;
					break;

				case 0x04:				/* Control-D's should be removed */
					DODEBUG_FEEDBACK(("%s(): control-d received", function));
					control_d_count--;	/* after we decrement the count */
					break;

				case 0x00:				/* This would confuse later code */
					buf[y++] = '?';
					break;

				case 0x01:				/* Control-A is for TBCP */
					if(x < buflen)
						{
						c = buf[x++];
						if(!tbcp_from_printer && c == 'M')
							{
							DODEBUG_FEEDBACK(("%s() start TBCP from printer", function));
							tbcp_from_printer = TRUE;
							break;
							}
						else if(tbcp_from_printer)
							{
							c ^= 0x40;			/* unencode */

							if(c == '\r')		/* catch strange HP behavior */
								{				/* and discard it */
								DODEBUG_FEEDBACK(("%s(): printer sent a TBCP encoded CR!", function));
								break;
								}
							}
						}
					/* fall thru to default case */

				default:				/* everything else, */
					buf[y++] = c;		/* gets copied. */
					break;
				}

			lastc = c;
			}

		#ifdef DEBUG_FEEDBACK
		if(buflen != y)
			debug("CR/LF xlate has changed buffer size from %d to %d bytes", buflen, y);
		#endif

		buflen = y;						/* buffer may have been shortened */
		buf[buflen] = '\0';				/* Terminate buffer as though a string. */
		}

		already_here = 0;

		DODEBUG_FEEDBACK(("%d bytes now in buffer", buflen));

		/*
		** Handle the whole buffer one line at a time.
		** ptr always equals buf[x]
		**
		** Within this loop, ptr will point to the current line
		** and its length will be stored in linelen.
		*/
		for(x=0,ptr=buf; x<buflen; x+=linelen,ptr+=linelen)
			{
			/* Form feeds are used to end PJL responses.  We will
			   treat them as one character lines. */
			if(*ptr == '\f')
				{
				DODEBUG_FEEDBACK(("%s(): FF ends PJL block", function));

				/* Dispatch actions which should be taken when leaving certain states. */
				switch(pjl_state)
					{
					case PJL_USTATUS_JOB_START: /* end of job start message? */
						pjl_job_active = TRUE;
						pjl_seen = TRUE;		/* entire job start sequence seen */
						break;
					case PJL_USTATUS_JOB_END:	/* end of job end message? */
						pjl_job_active = FALSE;
						break;
					case PJL_USTATUS_DEVICE:	/* end of device status message? */
						handle_ustatus_device(pjl_online, pjl_code, pjl_display, pjl_code2, pjl_display2);
						break;
					default:
						break;
					}

				pjl_state = PJL_INITIAL;		/* return to normal state */

				linelen = 1;					/* swallow one character */
				continue;
				}

			/* Some HP printers echo the Universal Exit
			   Language command. */
			if(strncmp((char*)ptr, "\33%-12345X", 9) == 0)
				{
				DODEBUG_FEEDBACK(("%s(): UEL! yum! yum!", function));
				linelen = 9;
				continue;
				}

			/*
			** Determine the length of the next line by
			** scanning for the next line feed.  The
			** length computed includes the line feed.
			*/
			{
			char *p2;
			if((p2 = strchr((char*)ptr, '\n')))			/* if line terminated, */
				{
				linelen = (p2 + 1 - (char*)ptr);
				}
			else										/* line is not terminated */
				{
				if((buflen - x) < FEEDBACK_BUFSIZE)		/* if buffer will not be */
					{									/* left full, defer. */
					already_here = buflen - x;
					if(x > 0) memmove(buf, ptr, already_here);
					DODEBUG_FEEDBACK(("%s(): defering processing of partial line", function));
					break;
					}
				else									/* no room to defer */
					{
					linelen = buflen - x;
					}
				}
			}

			#ifdef DEBUG_FEEDBACK_LINES
			debug("considering line: \"%.*s\"", (int)strcspn((char*)ptr, "\n"), ptr);
			#endif

			/*
			** Handle various @PJL job status messages.  We will try
			** to `swallow' all those we can by executing an continue.
			*/
			if(pjl_state == PJL_USTATUS_PAGE)
				{
				/* We just received a page number.  If we are not printing a
				   banner page right now, count this as one side of one sheet. */
				if(doing_primary_job)
					progress_pages_truly_printed(job.N_Up.N);

				/* This is so feedback_pjl_wait() can reset the alarm clock
				   which goes off if too much time elapses between page
				   ejections: */
				pjl_pagedrop = TRUE;

				/* This is used for the printer's display. */
				pjl_pagecount++;

				/* Inform writemon, in case it is keeping track of how long pages take. */
				writemon_pagedrop();

				continue;
				}
			if(pjl_state == PJL_USTATUS_JOB)					/* This state heralds a transition to */
				{												/* a more specific state. */
				if(strncmp((char*)ptr, "START", 5) == 0)
					pjl_state = PJL_USTATUS_JOB_START;
				else if(strncmp((char*)ptr, "END", 3) == 0)
					pjl_state = PJL_USTATUS_JOB_END;
				continue;
				}
			if(pjl_state == PJL_USTATUS_JOB_END)
				{
				/* total the page counts from all the "primary" jobs. */
				if(strncmp((char*)ptr, "PAGES=", 6) == 0 && doing_primary_job)
					{
					if(pjl_chargable_pagecount == -1) pjl_chargable_pagecount = 0;
					pjl_chargable_pagecount += atoi((char*)&ptr[6]);
					}
				continue;
				}
			if(pjl_state == PJL_USTATUS_DEVICE)
				{
				if((ptr2 = lmatchp((char*)ptr, "CODE=")))
					pjl_code = atoi(ptr2);
				else if((ptr2 = lmatchp((char*)ptr, "CODE2=")))
					pjl_code2 = atoi(ptr2);
				else if(strncmp((char*)ptr, "ONLINE=TRUE", 11) == 0)
					pjl_online = PJL_ONLINE_TRUE;
				else if(strncmp((char*)ptr, "ONLINE=FALSE", 12) == 0)
					pjl_online = PJL_ONLINE_FALSE;
				else if((ptr2 = lmatchp((char*)ptr, "DISPLAY=\"")))
					snprintf(pjl_display, sizeof(pjl_display), "%.*s", (int)strcspn(ptr2, "\""), ptr2);
				else if((ptr2 = lmatchp((char*)ptr, "DISPLAY2=\"")))
					snprintf(pjl_display2, sizeof(pjl_display2), "%.*s", (int)strcspn(ptr2, "\""), ptr2);
				continue;
				}
			if(strncmp((char*)ptr, "@PJL USTATUS PAGE", 17) == 0)
				{
				pjl_state = PJL_USTATUS_PAGE;
				continue;
				}
			if(strncmp((char*)ptr, "@PJL USTATUS JOB", 16) == 0)
				{
				pjl_state = PJL_USTATUS_JOB;
				continue;
				}
			if(strncmp((char*)ptr, "@PJL USTATUS DEVICE", 19) == 0
						|| strncmp((char*)ptr, "@PJL INFO STATUS", 16) == 0)
				{
				pjl_state = PJL_USTATUS_DEVICE;
				pjl_ustatus_clear();
				continue;
				}

			/*
			** Handle lines in the form "%%[ PrinterError: xxx ]%%"
			** Write printer errors to the printer's status file.
			**
			** These messages are generated by certain PostScript interpreters.
			*/
			if(strncmp((char*)ptr, "%%[ PrinterError: ", 18) == 0)
				{
				char *subptr;			/* will be pointer to closing `bracket' */

				/* If it has the closing bracket stuff too, then call the hook
				   to "ppop status", commentators, and the progress file.
				   */
				if((subptr = strstr((char*)ptr, " ]%%")))
					{
					*subptr = '\0';

					if((subptr = strstr(ptr, "; source:")))		/* Remove "; source: AppleTalk" and */
						*subptr = '\0';							/* stuff like that. */

					handle_lw_status((char*)&ptr[4], NULL);
					}

				continue;		/* keep line out of the job log */
				}

			/*
			** Handle messages of the form:
			**	%%[ status: xxx ]%%
			**
			** These messages are generated by certain PostScript interpreters.
			*/
			if(strncmp((char*)ptr, "%%[ status: ", 12) == 0)
				{
				char *subptr;

				/* If it has the closing bracket stuff too, call the hook function. */
				if((subptr = strstr((char*)ptr, " ]%%")))
					{
					*subptr = '\0';

					if((subptr = strstr(ptr, "; source:")))		/* Remove "; source: AppleTalk" and */
						*subptr = '\0';							/* stuff like that. */

					handle_lw_status((char*)&ptr[12], NULL);
					}

				continue;		/* keep this line out of the job log */
				}

			/*
			** Handle messages of the form:
			**	%%[ job: xxx; status: yyy ]%%
			**
			** These messages are generated by certain PostScript interpreters.
			*/
			if(strncmp((char*)ptr, "%%[ job: ", 9) == 0)
				{
				char *subptr;
				char *status = NULL;

				/* If it has the closing bracket stuff too, call the hook function. */
				if((subptr = strstr((char*)ptr, " ]%%")))
					{
					*subptr = '\0';

					if((subptr = strstr(ptr, "; source:")))		/* Remove "; source: AppleTalk" and */
						*subptr = '\0';							/* stuff like that. */

					if((subptr = strstr(ptr, "; status: ")))
						{
						*subptr = '\0';
						status = (subptr + 10);
						}

					handle_lw_status(status, (char*)&ptr[9]);
					}

				continue;		/* keep this line out of the job log */
				}

			/*
			** Handle messages of the form:
			**	%%[ PPR SNMP N1 N2 N3 ]%%
			** Where:
			**	 N1 = hrDeviceStatus (in signed decimal)
			**	 N2 = hrPrinterStatus (in signed decimal)
			**	 N3 = hrPrinterDetectedErrorState (in unsigned 32 bit hexadecimal)
			**
			** These messages are generated by PPR's interface programs.  They convey
			** the SNMP status of the printer.
			*/
			if((ptr2 = lmatchp((char*)ptr, "%%[ PPR SNMP: ")) && strstr(ptr2, " ]%%"))
				{
				int n1, n2;
				unsigned int n3;
				if(sscanf(ptr2, "%d %d %x", &n1, &n2, &n3) != 3)
					error("%s(): can't parse SNMP: %*s", function, (strstr(ptr2, " ]%%") - ptr2), ptr2);
				handle_snmp_status(n1, n2, n3);
				continue;
				}

			/*
			** Handle messages of the form:
			**	%%[ PPR hrPrinterDetectedErrorState: XXXXXXXX ]%%
			**
			** These messages are generated by PPR's interface programs.  They convey
			** the SNMP status of the printer.
			*/
			if((ptr2 = lmatchp((char*)ptr, "%%[ PPR hrPrinterDetectedErrorState: ")) && strstr(ptr2, " ]%%"))
				{
				unsigned int temp;
				if(sscanf(ptr2, "%x", &temp) != 1)
					error("%s(): can't parse hex number: %*s", function, (int)strcspn(ptr2, " "), ptr2);
				continue;
				}

			/*
			** These lines are used to give a positive indication of whether
			** the connexion to the printer has gone through or not.
			**
			** These message are generated by some of the PPR interface programs.
			*/
			if(linelen == 27 && strncmp((char*)ptr, "%%[ PPR address lookup ]%%\n", linelen) == 0)
				{
				ppop_status_connecting("LOOKUP");
				continue;
				}
			if(linelen == 23 && strncmp((char*)ptr, "%%[ PPR connecting ]%%\n", linelen) == 0)
				{
				ppop_status_connecting("CONNECT");
				continue;
				}
			if(linelen == 22 && strncmp((char*)ptr, "%%[ PPR connected ]%%\n", linelen) == 0)
				{
				ppop_status_connecting(NULL);
				continue;
				}

			/*
			** If it is a PostScript error, set a flag so that the job will be
			** arrested as soon as it is done and call a routine which
			** analyzes the error message and attempts to write an intelligent
			** description of it into the job's log file.
			*/
			if((ptr2 = lmatchp((char*)ptr, "%%[ Error:")))
				{
				char *error, *command;
				char *end_error, *end_command;

				/* Set the flag which indicates a PostScript error: */
				posterror = TRUE;

				/* Find the error string. */
				error = ptr2;

				/* Find the offending command in the error message: */
				if((command = strstr((char*)ptr, "OffendingCommand: ")))
					command += 18;

				/* Terminate each one with a NULL */
				if((end_error = strchr(error, ';')))
					*end_error = '\0';
				end_command = (char*)NULL;
				if(command && (end_command = strchr(command, ' ')))
					*end_command = '\0';

				/* Write an introduction to the PostScript error in the job log 
				   and a "Reason:" line into the queue file.
				   */
				describe_postscript_error(job.Creator, error, command);

				/* If they existed, put the origional characters back so that
				   the message can be logged. */
				if(end_error)
					*end_error = ';';
				if(end_command)
					*end_command = ' ';
				} /* drop thru so that line will be entered in job log */

			/*
			** How about a Ghostscript style PostScript error message?  If 
			** Ghostscript is run without -dSHORTERRORS, it uses its own
			** error message format.
			**
			** Normaly, the error message is in this style:
			**
			** Error: /undefined in x
			**
			** but for "unstoppable errors" they look like:
			**
			** Unrecoverable error: undefined in x
			*/
			{
			char *error, *command;
			if(		(
					(error = lmatchp((char*)ptr, "Error:"))
					||
					(error = lmatchp((char*)ptr, "Unrecoverable error:"))
					)
					&&
					(command = strstr(error, " in "))
					)
				{
				int end_command_len;
				char end_command_char;

				/* Skip the slash that appears in front of the error name in the
				   "Error:" format. */
				if(*error == '/')
					error++;

				/* Temporily terminate the error. */
				error[strcspn(error, " ")] = '\0';
				
				/* Move past " in ". */
				command += 4;

				end_command_len = strcspn(command , "-\n");
				end_command_char = command[end_command_len];
				command[end_command_len] = '\0';

				if(strcmp(command, ".outputpage") == 0)
					{
					alert(printer.Name, TRUE, "Ghostscript RIP failed due to a problem with its output pipe.");
					ghosterror = TRUE;
					/*continue;*/
					}
				else
					{
					/* Hand the facts to our `inteligent' routine. */
					describe_postscript_error(job.Creator, error, command);
					posterror = TRUE;
					}

				/* If they existed, put the origional characters back. */
				error[strlen(error)] = ' ';
				command[end_command_len] = end_command_char;
				} /* drop thru so that line will be entered in job log */
			}

			/*
			** If we see Ghostscript's error message for an unknown device,
			** put an alert in the printer's alert log and set a flag so that
			** when Ghostscript dies we will not it is not the job's fault.
			*/
			if((ptr2 = lmatchp((char*)ptr, "Unknown device:")))
				{
				ptr2[strcspn(ptr2, "\n")] = '\0';
				alert(printer.Name, TRUE, "Ghostscript does not have a driver for the device \"%s\".", ptr2);
				ghosterror = TRUE;
				continue;
				}

			/*
			** Catch other signs that Ghostscript is having trouble with its 
			** command line.  This error is considered non-fatal
			** (by Ghostscript), but we want to be prepared if it does bomb 
			** out.
			*/
			if((ptr2 = lmatchp((char*)ptr, "Unknown switch -")))
				{
				ptr2[strcspn(ptr2, "\n")] = '\0';
				alert(printer.Name, TRUE, "Unrecognized option passed to Ghostscript RIP: %s", ptr2);
				ghosterror = TRUE;
				continue;
				}

			/*
			** Catch messages from the Ghostscript wrapper.
			*/
			if((ptr2 = lmatchp((char*)ptr, "RIP:")))
				{
				ptr2[strcspn(ptr2, "\n")] = '\0';
				alert(printer.Name, TRUE, "RIP error: %s", ptr2);
				ghosterror = TRUE;
				continue;
				}

			/*
			** Catch messages from CUPS filters.
			*/
			if((ptr2 = lmatchp((char*)ptr, "ERROR:")))
				{
				ptr2[strcspn(ptr2, "\n")] = '\0';
				alert(printer.Name, TRUE, "RIP error in CUPS filter:\n%s", ptr2);
				ghosterror = TRUE;
				continue;
				}

			if(lmatchp((char*)ptr, "DEBUG:"))
				continue;
			if(lmatchp((char*)ptr, "DEBUG2:"))
				continue;

			/*
			** If we get this far, allow the *PatchFile query
			** to get a look at the line.  If the line is a reply
			** to a query it will ask us to keep it out of the
			** log file.
			*/
			if(patchfile_query_callback((char*)ptr))
				continue;

			/*
			** Allow the persistent download query code to review the line.
			*/
			if(persistent_query_callback((char*)ptr))
				continue;

			/*
			** Allow the pagecount query code to have a look.
			*/
			if(pagecount_query_callback((char*)ptr))
				continue;

			/*
			** If we get this far, we should write the message
			** to the job's log file.
			*/
			DODEBUG_FEEDBACK(("%s(): no match (?), writing to job log", function));
			{
			int i;

			for(i=0; i < linelen; i++)
				{
				if(isprint(ptr[i]) || isspace(ptr[i]))
					{
					log_putc(ptr[i]);
					}
				else			/* no printable, represent with hexadecimal */
					{
					log_putc('<');
					log_putc(hexchars[ptr[i] / 16]);
					log_putc(hexchars[ptr[i] % 16]);
					log_putc('>');
					}
				}
			} /* end of write to log block */

			} /* End of loop which considers the lines from the printer. */
		} /* end of read() loop */

	DODEBUG_FEEDBACK(("%s(): done, %d bytes read", function, total_bytes));
	return total_bytes;
	} /* end of feedback_reader() */

/*===================================================================
** Wait until we receive the next glob of data to be read from
** the interface program.
**
** If timeout is greater than zero and more than that many seconds
** elapse since we called writemon_start(), return -1.
**
** Note that it is ABSOLUTELY NECESSARY to call writemon_start()
** before calling this function!
**
** If return_on_signal is TRUE, then if select() is interupted by
** a signal, we return right away even if the timeout hasn't
** expired.  This feature is used by signal_jobbreak().
===================================================================*/
int feedback_wait(int timeout, gu_boolean return_on_signal)
	{
	const char *function = "feedback_wait";
	fd_set rfds;
	struct timeval sleep_time;
	int readyfds;
	int retval = -1;

	DODEBUG_FEEDBACK(("%s(timeout=%d, return_on_signal=%s)", function, timeout, return_on_signal ? "TRUE" : "FALSE"));

	while(writemon_sleep_time(&sleep_time, timeout))
		{
		FD_ZERO(&rfds);
		FD_SET(intfd, &rfds);

		DODEBUG_FEEDBACK(("%s(): intfd=%d, sleep_time=%d.%06d", function, intfd, (int)sleep_time.tv_sec, (int)sleep_time.tv_usec));
		if((readyfds = select(intfd + 1, &rfds, NULL, NULL, &sleep_time)) < 0)
			{
			if(errno == EINTR)
				{
				fault_check();
				if(return_on_signal)
					break;
				continue;
				}
			fatal(EXIT_PRNERR, "%s(): select() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
			}

		if(readyfds > 0)
			{
			if(!FD_ISSET(intstdout, &rfds))
				fatal(EXIT_PRNERR_NORETRY, "%s(): assertion failed", function);

			fault_check();

			DODEBUG_FEEDBACK(("%s(): data glob ready", function));
			if(feedback_reader() > 0)
				retval = 0;
			#ifdef DEBUG_FEEDBACK
			else
				debug("%s(): zero length data glob indicates end of feedback", function);
			#endif

			break;
			}
		}

	DODEBUG_FEEDBACK(("%s(): done, returning %d", function, retval));

	return retval;
	} /* end of feedback_wait() */

/*===================================================================
** This routine may be called from printer_cleanup().
** If we are called, we can assume that PJL handshaking
** is in use.
**
** If we saw a PJL job start indication, wait for a PJL job
** end indication.  If there was no evidence that PJL job
** start/stop indications are turned on, do nothing.
===================================================================*/
void feedback_pjl_wait(void)
	{
	const char *function = "feedback_pjl_wait";
	DODEBUG_INTERFACE(("%s()", function));

	if(printer.Jobbreak != JOBBREAK_PJL && printer.Jobbreak != JOBBREAK_SIGNAL_PJL)
		fatal(EXIT_PRNERR_NORETRY, "%s: assertion failed", function);

	/*
	** If the interface is capable of receiving data from the printer,
	** then wait for a PJL response indicating that the job is done.
	*/
	if(printer.Feedback)
		{
		int t = 0;

		/*
		** Be careful!  It may be that the first PJL message hasn't arrived
		** yet.  We wouldn't want to conclude that the jobs was done just
		** because we weren't between the start and end messages.
		*/
		if(!pjl_seen)
			{
			writemon_start("WAIT_PJL_START");
			while(!pjl_seen)
				{
				DODEBUG_INTERFACE(("%s(): waiting for PJL job start", function));
				feedback_wait(0, FALSE);
				}
			writemon_unstalled("WAIT_PJL_START");
			}

		/*
		** Inform the progress monitoring code that we are entering the
		** waiting-for-job-to-end-monitored-by-PJL phase of operations.
		*/
		writemon_start("WAIT_PJL");

		/*
		** Notice that we do not continue to wait if a PostScript
		** error has been reported.  Under normal circumstances this
		** is not strictly necessary, but under certain odd circumstances
		** an HP4M+ with JetDirect has been known to interpret the
		** Universal Exit Language sequence as a PostScript error.
		*/
		while(pjl_job_active && !posterror)
			{
			DODEBUG_INTERFACE(("%s(): waiting for EOJ", function));

			/* Look for broken printer as indicated by a 50xxx PJL code. */
			if((pjl_code / 1000) == 50 || strstr(pjl_display, "PRESS SELECT"))
				{
				error("%s(): bailing out due to code %d (%s)", function, pjl_code, pjl_display);
				break;
				}

			/* If a page has been dropt into the output tray,
			   cancel any stall commentary and start the
			   clock ticking again. */
			if(pjl_pagedrop)
				{
				writemon_unstalled("WAIT_PJL"); /* possibly announce no longer stalled */
				writemon_start("WAIT_PJL");		/* measure stall from this time */
				pjl_pagedrop = FALSE;			/* reset flag so we can detect next page drop */
				t = 0;							/* writemon_start() started new timeout */
				}

			/* Move the timeout to a minute after the last one.
			   Remember that this timeout time is relative to the
			   time of the first call to feedback_wait() after
			   the call to writemon_start(). */
			t += 60;

			/* Wait for the next chunk from the printer. */
			feedback_wait(t, FALSE);

			/*
			** Update the progress indication.
			**
			** This code doesn't work.  For some reason the printer queues
			** the message changes until the job is done!
			**
			** Though the message setting code is disabled, the skeleton
			** has been left since the traffic it generates serves to
			** reset the printer's communications timeout.	(Remember that
			** the interfaces have a non-zero default for idle_status_interval
			** only if the jobbreak method is "control-d".)
			*/
			printer_universal_exit_language();
			#if 0
			switch((t / increment) % 3)
				{
				case 0:
					if(job.For)
						printer_display_printf("%s", job.For);
					break;
				case 1:
					printer_display_printf("Page Time: %d:%02d", t/60, t%60);
					break;
				case 2:
					printer_display_printf("Pages: %d", pjl_pagecount);
					break;
				}
			#else
			printer_puts("@PJL\n");
			#endif

			/* Flush the commands to the printer but remove those bytes from the total transmitted count. */

			progress_bytes_sent_correction(printer_flush());
			}

		writemon_unstalled("WAIT_PJL");
		}

	pjl_seen = FALSE;	/* Forget this line and we won't work next time! */

	DODEBUG_INTERFACE(("%s(): done", function));
	} /* end of feedback_pjl_wait() */

/*===================================================================
** Read final output from the interface program.  This is called
** from reapchild() and interface_close() after intstdin has been
** closed.
===================================================================*/
void feedback_drain(void)
	{
	FUNCTION4DEBUG("feedback_drain")
	DODEBUG_FEEDBACK(("%s()", function));

	/* Read chunks with no timeout, until EOF. */
	writemon_start("CLOSE");
	while(feedback_wait(0, FALSE) != -1) ;
	writemon_unstalled("CLOSE");

	DODEBUG_FEEDBACK(("%s(): done", function));
	} /* end of feedback_drain() */

/*===================================================================
** Determine if feedback_reader() has detected a messages indicating
** a PostScript error.
===================================================================*/
gu_boolean feedback_posterror(void)
	{
	return posterror;
	}

/*===================================================================
** Determine if feedback_reader() has detected a messages indicating
** a non-PostScript error in the Ghostscript RIP.
===================================================================*/
gu_boolean feedback_ghosterror(void)
	{
	return ghosterror;
	}

/*===================================================================
** Return the number of PJL PAGE messages received by
** feedback_reader().
===================================================================*/
int feedback_pjl_chargable_pagecount(void)
	{
	return pjl_chargable_pagecount;
	}

/* end of file */

