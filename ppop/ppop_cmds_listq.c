/*
** mouse:~ppr/src/ppop/ppop_cmds_listq.c
** Copyright 1995--2005, Trinity College Computing Center.
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
** Last modified 28 September 2005.
*/

/*
** This module contains the code for the ppop queue
** listing sub-commands such as "ppop list",
** "ppop lpq" and "ppop qquery".
*/

#include "config.h"
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "ppop.h"
#include "util_exits.h"

/*
 * This routine converts a Unix time to a string.
 *
 * If the job was submitted within the last 24 hours, display the
 * time of day.  Otherwise, display the date.  The date and time
 * formats can be customized in ppr.conf.
 */
static char *format_time(char *timestr, size_t timestr_len, time_t time_to_format)
	{
	struct tm *tm_time;
	time_t time_now;
	static gu_boolean format_loaded = FALSE;
	static const char *format_date = NULL;
	static const char *format_time = NULL;

	if(!format_loaded)
		{
		FILE *cf;
		if((cf = fopen(PPR_CONF, "r")))
			{
			struct GU_INI_ENTRY *section;
			if((section = gu_ini_section_load(cf, "internationalization")))
				{
				const struct GU_INI_ENTRY *value;
				if((value = gu_ini_section_get_value(section, "ppopdateformat")))
					{
					format_date = gu_ini_value_index(value, 0, NULL);
					format_time = gu_ini_value_index(value, 1, NULL);
					if(format_date)
						format_date = gu_strdup(format_date);
					if(format_time)
						format_time = gu_strdup(format_time);
					}
				gu_ini_section_free(section);
				}
			fclose(cf);
			}
		format_loaded = TRUE;
		}

	tm_time = localtime(&time_to_format);
	time_now = time((time_t*)NULL);

	/* If within last 24 hours, */
	if(difftime(time_now, time_to_format) >= (24*60*60))
		{
		/* If a date format is defined in ppr.conf, try it. */
		if(!format_date || gu_utf8_strftime(timestr, timestr_len, format_date, tm_time) == 0)
			{
			/* It didn't work, do it our way. */
			gu_utf8_strftime(timestr, timestr_len, "%d-%b-%y", tm_time);
			}
		}

	/* If more than 24 hours ago, */
	else
		{
		/* If a time (clock) format is defined in ppr.conf, try it. */
		if(!format_time || gu_utf8_strftime(timestr, timestr_len, format_time, tm_time) == 0)
			{
			/* It didn't work.  Use a default which depends on whether 
			 * AM and PM are acceptable in the current locale.
			 */
			if(gu_utf8_strftime(timestr, timestr_len, "%p", tm_time) == 0)
				gu_utf8_strftime(timestr, timestr_len, "%H:%M", tm_time);
			else
				gu_utf8_strftime(timestr, timestr_len, "%I:%M%p", tm_time);
			}
		}

	return timestr;
	} /* format_time() */

/*
** Those subcommands which allow PPRDEST to be used as the default argument
** call this.  They pass in their argv[] and get either the same one back
** or a new one.  If the one they pass in is empty, and PPRDEST is defined,
** they get a new one consisting only of the value of PPRDEST.
*/
static char **allow_PPRDEST(char *argv[])
	{
	static char *PPRDEST_argv[2];

	if(!argv[0] && (PPRDEST_argv[0] = getenv("PPRDEST")))
		{
		PPRDEST_argv[1] = (char*)NULL;
		argv = PPRDEST_argv;
		}

	return argv;
	}

/*
** Routine for printing the job status.
**
** It is passed the status code and the pointer to the file structure
** for the communications file since if the job is printing we will have
** to read the name of the printer as the next record in the comfile.
**
** It is also passed the ``handle'' of the queue file so it can read
** the required media names.  Lastly, it is passed the indent to use when
** printing continuation lines.
*/
static void job_status(const struct QEntry *qentry, const struct QEntryFile *qentryfile,
		 const char *onprinter, FILE *vfile, int indent, int reason_indent)
	{
	/* Print a message appropriate to the status. */
	switch(qentry->status)
		{
		case STATUS_WAITING:					/* <--- waiting for printer */
			{
			int x;
			printf("waiting for printer\n");
			if(qentry->never)					/* If one or more counted out, */
				{								/* assume it is a group */
				for(x=0; x < indent; x++)		/* and explain. */
					putchar(' ');
				printf("(Not all \"%s\"\n", qentryfile->destname);
				for(x=0; x < indent; x++)
					putchar(' ');
				printf("members are suitable.)\n");
				}
			}
			break;

		case STATUS_WAITING4MEDIA:				/* <--- waiting for media */
			if(vfile)							/* If we have an open queue file to work with, */
				{
				char *line = NULL;
				int line_available = 80;

				printf("waiting for media:\n");

				while((line = gu_getline(line, &line_available, vfile)) && strcmp(line, CHOPT_QF_ENDTAG1))
					{
					if(strncmp(line, "Media: ",7) == 0)
						{
						int x;
						for(x=0; x < indent; x++)		/* indent */
							putchar(' ');
						for(x=7; line[x] != '\0' && line[x] != ' '; x++)
							putchar(line[x]);
						putchar('\n');
						}
					}

				if(line) gu_free(line);
				}
			else						/* Caller wants to do it itself. */
				{
				printf("waiting for media\n");
				}
			break;

		case STATUS_HELD:				/* <--- job is held */
			printf("held\n");
			break;

		case STATUS_STRANDED:			/* <--- job is stranded */
			printf("stranded\n");
			goto stranded_arrested;

		case STATUS_ARRESTED:			/* <--- job is arrested */
			printf("arrested\n");
			goto stranded_arrested;

		stranded_arrested:
			if(vfile)
			  {
			  char *line = NULL;
			  int line_available = 80;

			  /* Scan the rest of the queue file. */
			  while((line = gu_getline(line, &line_available, vfile)) && strcmp(line, CHOPT_QF_ENDTAG1) )
				{
				/* Look for "Reason:" lines */
				if(strncmp(line, "Reason: ", 8) == 0)
					{
					/* print indented, wrapping at commas */
					char *ptr;
					int x;
					for(ptr = &line[8]; *ptr; )
						{
						for(x=0; x < reason_indent; x++) /* indent */
							putchar(' ');

						putchar('(');

						do	{
							if(*ptr == '|')				/* break at | but */
								{						/* don't print the | */
								ptr++;
								break;
								}
							putchar(*ptr);
							} while( *(ptr++) != ',' && *ptr );

						gu_puts(")\n");
						}
					break;		/* print only the first reason */
					} /* end of if this is a "Reason:" line */
				} /* end of queue file line loop */

			  if(line) gu_free(line);
			  } /* end if if there is an open queue file */
			break;

		case STATUS_CANCEL:				/* <--- job is being canceled */
			printf("being canceled\n");
			break;

		case STATUS_SEIZING:
			printf("being seized\n");
			break;

		case STATUS_FINISHED:
			printf("finished\n");
			break;

		default:						/* <--- job is being printed */
			if(qentry->status >= 0)		/* if greater than -1, is prnid */
				{						/* read the name of the printer */
				int found = FALSE;
				long bytes_sent;
				int pages_started;
				int pages_printed;

				printf("printing on %s\n", onprinter);

				/*
				** If we are handed the open queue file, get the values
				** and print from the last "Progress:" line.
				*/
				if(vfile)
					{
					char *line = NULL;
					int line_available = 80;
					while((line = gu_getline(line, &line_available, vfile)) && strcmp(line, CHOPT_QF_ENDTAG1))
						{
						if(gu_sscanf(line, "Progress: %ld %d %d", &bytes_sent, &pages_started, &pages_printed) == 3)
							found = TRUE;
						}
					if(line) gu_free(line);
					}

				if(found)										/* If "Progress:" line was found, */
					{
					int x;
					for(x=0; x < reason_indent; x++)			/* indent */
						fputc(' ', stdout);

					/* Print percent of bytes sent. */
					{
					int total_bytes =
						qentryfile->PassThruPDL
								? qentryfile->attr.input_bytes
								: qentryfile->attr.postscript_bytes;

					if(total_bytes == 0)						/* !!! a bug somewhere !!! */
						printf("bug%%");
					else
						printf("%d%%", (int)(((double)bytes_sent / (double)total_bytes) * 100.0 + 0.5));
					}

					/* Print number of pages started. */
					if(qentryfile->attr.pages > 0 || pages_started > 0 || pages_printed)
						printf(", page %d", pages_started);

					/* How many pages does the printer report are completed? */
					if(pages_printed > 0)
						printf(" (%d)", pages_printed);

					fputc('\n', stdout);
					}

				}
			else		/* if hit default case but not a positive status */
				{
				fputs("unknown status\n", stdout);
				}
		} /* end of switch */

	} /* end of job_status() */

/*
** There are two possible flag pages, so we
** need to execute this code twice.  Theirfor
** it is a subroutine.
*/
static const char *describe_flag_page_setting(int setting)
	{
	switch(setting)
		{
		case BANNER_DONTCARE:
			return "Unspecified";
		case BANNER_YESPLEASE:
			return "Yes";
		case BANNER_NOTHANKYOU:
			return "No";
		default:
			return "INVALID";
		}
	} /* end of describe_flag_page_setting() */

static const char *describe_proofmode(int proofmode)
	{
	switch(proofmode)
		{
		case PROOFMODE_NOTIFYME:
			return "NotifyMe";
		case PROOFMODE_SUBSTITUTE:
			return "Substitute";
		case PROOFMODE_TRUSTME:
			return "TrustMe";
		default:
			return "INVALID";
		}
	} /* end of describe_proofmode() */

static const char *describe_pageorder(int pageorder)
	{
	switch(pageorder)
		{
		case PAGEORDER_ASCEND:
			return "Ascend";
		case PAGEORDER_DESCEND:
			return "Descend";
		case PAGEORDER_SPECIAL:
			return "Special";
		default:
			return "INVALID";
		}
	} /* end of describe_pageorder() */

static const char *describe_sigpart(int sigpart)
	{
	switch(sigpart)
		{
		case SIG_FRONTS:
			return "Fronts";
		case SIG_BACKS:
			return "Backs";
		case SIG_BOTH:
			return "Both";
		default:
			return "INVALID";
		}
	} /* end of describe_sigpart() */

static const char *describe_orientation(int orientation)
	{
	switch(orientation)
		{
		case ORIENTATION_PORTRAIT:
			return "Portrait";
		case ORIENTATION_LANDSCAPE:
			return "Landscape";
		case ORIENTATION_UNKNOWN:
			return "Unknown";
		default:
			return "INVALID";
		}
	} /* end of describe_orienation() */

/*====================================================================
** This routine is passed pointers to functions with which
** to print a queue listing.
**
** This routine all by itself does not implement a command
**
** This uses the "l" command too.
**
** One, (*help)() is called if the wrong number of arguments
** is passed in.
**
** The second, (*banner) is called to print the column headings.
**
** The third, (*item) is called once for each queue entry.
**		The first argument is the ASCIIZ name of the destination.
**		The second argument is the pprd queue structure.
**		The Third argument is a structure containing the information
**		from the queue file.
**		The third argument is the handle of the response file.  This
**		is provided so that (*item)() may pass it to job_status().
**		The fourth argument is the handle of the queue file which
**		should be passed to job_status().
**		If the fifth argument is non-zero, then the header will be
**		suppressed when there are no files in the queue.
====================================================================*/
int custom_list(char *argv[],
		void(*help)(void),		/* function for syntax error */
		void(*banner)(void),	/* function to print column labels */
		int(*item)(const struct QEntry *qentry,
			const struct QEntryFile*,
			const char *onprinter,
			FILE *qstream),		/* file to read Media: from */
		int suppress,			/* True if should suppress header on empty queue */
		int arrested_drop_time)
	{
	int arg_index;
	FILE *FIFO, *reply_file;
	const struct Jobname *job;			/* Split up job name */
	struct QEntryFile qentryfile;		/* full queue entry */
	struct QEntry qentry;
	int header_printed = FALSE;
	char *line = NULL;
	int line_available = 80;
	time_t time_now;
	int stop = FALSE;

	char *destname;
	int id;
	int subid;
	int priority;
	int status;
	int never;
	int notnow;
	int pass;
	long int arrest_time;
	char *onprinter;

	if(! argv[0])
		{						/* If no destinations specified, */
		(*help)();				/* invoke the help function */
		return EXIT_SYNTAX;		/* and exit */
		}

	if(arrested_drop_time >= 0)			/* maybe get current time to compare to arrest time */
		time(&time_now);				/* of arrested jobs */

	if(!suppress)						/* If we should always print header, */
		{								/* print it now */
		if( ! opt_machine_readable )		/* unless a program is reading our output. */
			(*banner)();
		header_printed = TRUE;
		}

	/* Loop to process each argument: */
	for(arg_index=0; argv[arg_index]; arg_index++)
		{
		/* Separate the components of the job name. */
		if(!(job = parse_jobname(argv[arg_index])))
			return EXIT_SYNTAX;

		/* Prepare to communicate with pprd. */
		FIFO = get_ready();

		/* Send command. */
		fprintf(FIFO, "l %s %d %d\n", job->destname, job->id, job->subid);
		fflush(FIFO);

		/* Wait for pprd to reply. */
		if((reply_file = wait_for_pprd(TRUE)) == (FILE*)NULL)
			return print_reply();

		/*
		** Loop to process each queue entry.
		*/
		while( ! stop && (line = gu_getline(line, &line_available, reply_file)))
			{
			destname = onprinter = (char*)NULL;
			never = notnow = pass = 0;

			if(gu_sscanf(line,"%S %d %d %d %d %S %d %d %d %ld",
					&destname, &id, &subid,
					&priority, &status, &onprinter, &never, &notnow, &pass, &arrest_time) < 6)
				{
				printf("Invalid response line: %s", line);
				if(destname)
					gu_free(destname);
				if(onprinter)
					gu_free(onprinter);
				continue;
				}

			/*
			** Read additional information from the queue file included
			** in the reply file.
			*/
			qentryfile_clear(&qentryfile);
			if(qentryfile_load(&qentryfile, reply_file) == -1)
				printf("Invalid queue entry:\n");

			/* Copy everything into a QEntry structure for easy parameter passing. */
			qentry.id = id;
			qentry.subid = subid;
			qentry.priority = priority;
			qentry.status = status;
			qentry.never = never;
			qentry.notnow = notnow;
			qentry.pass = pass;

			/* And into the QEntryFile structure too, this will take care of deallocation. */
			qentryfile.destname = destname;
			qentryfile.id = id;
			qentryfile.subid = subid;

			/*
			** Normally we print the column headings if they have not
			** been printed already and the -M switch was not used.
			** Then, we call the routine which the caller has specified
			** as the one that should be called for each entry.
			**
			** Notice that this whole block of code is skipt if
			** arrest_dropout_time is non-negative and the job is arrested and
			** the time in seconds since it was arrested is greater than
			** arrest_dropout_time.
			*/
			if(status != STATUS_ARRESTED || arrested_drop_time < 0 || (time_now - arrest_time) <= arrested_drop_time)
				{
				if(!header_printed)
					{
					if( ! opt_machine_readable )
						(*banner)();
					header_printed = TRUE;
					}

				stop = (*item)(&qentry, &qentryfile, onprinter, reply_file);
				}

			/*
			** Eat up any remaining part of the queue file.
			*/
			while((line = gu_getline(line, &line_available, reply_file)) && strcmp(line, CHOPT_QF_ENDTAG2)) ;

			/*
			** Free memory in the qentryfile.
			** This will free destname as well.
			*/
			qentryfile_free(&qentryfile);
			gu_free(onprinter);
			} /* end of while, loop back for next job */

		fclose(reply_file);
		} /* end of loop for each argument */

	if(line) gu_free(line);

	return EXIT_OK;						/* no errors */
	} /* end of custom_list() */

/*================================================================
** ppop list {destination}
**
** Normal queue listing.
** This uses custom_list().
================================================================*/
static void ppop_list_help(void)
	{
	fputs(_("Usage: ppop list {all, <printer>, <group> , <job>} ...\n\n"
		"This command will print information about the specified jobs.\n"
		"Jobs may be specified by job id, by queue, or \"all\" may\n"
		"be specified.  Multiple specifications may be used with this\n"
		"command.\n"), errors);
	}

static void ppop_list_banner(void)
	{
/*		123456789012345678901234567890123456789012345678901234567890 */
gu_utf8_puts(_("Queue ID        For                      Time      Pgs Status\n"));
gu_utf8_puts(  "----------------------------------------------------------------------------\n");
/*		  12345678-xxxx.y David Chappell           21 May 79 999 waiting for printer
		  glunkish-1004   Joseph Smith             11:31pm   ??? printing on glunkish
		  melab_deskjet-1004
		  Joseph Andrews                           11:35pm   001 waiting for media
		                                                         letterhead
*/
	}

static int ppop_list_item(const struct QEntry *qentry,
		const struct QEntryFile *qentryfile,
		const char *onprinter,
		FILE *qstream)
	{
	char timestr[64];		/* 9 chars expected, but large buffer for utf-8 */
	char pagesstr[4];
	const char *jobname;

	/* Print the job name with line wrapping if it overflows its column. */
	jobname = jobid(qentryfile->destname, qentry->id, qentry->subid);
	if(strlen(jobname) > 15)
		gu_utf8_printf("%s\n               ", jobname);
	else
		gu_utf8_printf("%-15.15s", jobname);


	/* Convert the number of pages to ASCII with unknown values represented as ???
	 * and values higher than 999 as >1k. */
	if(qentryfile->attr.pages < 0)
		strlcpy(pagesstr, "???", sizeof(pagesstr));
	else if(qentryfile->attr.pages > 999)
		strlcpy(pagesstr, ">1k", sizeof(pagesstr));
	else
		snprintf(pagesstr, sizeof(pagesstr), "%03d", qentryfile->attr.pages);

	gu_utf8_printf(" %-24.24s %-9.9s %s ",
		qentryfile->For ? qentryfile->For : "???",
		format_time(timestr, sizeof(timestr), (time_t)qentryfile->time),
		pagesstr
		);

	job_status(qentry, qentryfile, onprinter, qstream,
		55,		/* media indent */
		55		/* reason indent */
		);

	return FALSE;
	} /* end of ppop_list_item() */

int ppop_list(char *argv[], int suppress)
	{
	argv = allow_PPRDEST(argv);
	return custom_list(argv, ppop_list_help, ppop_list_banner, ppop_list_item, suppress, opt_arrest_interest_interval);
	}

/*================================================================
** ppop lpq {destination}
**
** LPQ style listing.  This command exists to support lprsrv(8).
** It uses custom_list().  Please notice that the -M (machine
** readable format) switch should have no effect on this command.
** This is because this command exists primarily to provide output
** which will be read by other programs.  A separate format for
** reading by programs would be absurd, especially as ppop has
** other subcommands such as "ppop qquery" which are much better
** suited to that sort of thing.
================================================================*/

static int lpqlist_rank;
static int lpqlist_banner_called;
static const char *lpqlist_destname;
static char **lpqlist_argv;

static const char *count_suffix(int count)
	{
	switch(count % 10)
		{
		case 1:
			return "st";
		case 2:
			return "nd";
		case 3:
			return "rd";
		default:
			return "th";
		}
	}

/*
** custom_list() calls this if it can't parse the destination name
*/
static void ppop_lpq_help(void)
	{
	fputs(_("Usage: ppop lpq {all, <printer>, <group>} [<user>...] [<id>...]\n\n"
		"The \"ppop lpq\" subcommand prints a queue listing in a format\n"
		"similiar to that of the Berkeley Unix \"lpq\" command.\n"), errors);
	}

/*
** ppop_lpq_banner() calls this to print the progress of
** the job a printer is working on.  This routine writes
** to stdout.  It prints a line fragment of either the form:
**
** " (0% sent)"
**	or
** " (101% sent, 2 of 7 pages completed)"
**
** !!! This routine must be kept up to date with the queue file format. !!!
** !!! This routine is not ready for distributed printing. !!!
*/
static void ppop_lpq_banner_progress(const char *printer_job_destname, int printer_job_id, int printer_job_subid)
	{
	char fname[MAX_PPR_PATH];
	FILE *f;

	ppr_fnamef(fname, "%s/%s-%d.%d", QUEUEDIR, printer_job_destname, printer_job_id, printer_job_subid);
	if((f = fopen(fname, "r")) != (FILE*)NULL)
		{
		long int postscript_bytes, input_bytes, bytes_tosend, bytes_sent;
		int pages, pages_printed, N, copies;
		gu_boolean barbar = FALSE;

		bytes_sent = pages_printed = pages = postscript_bytes = 0;
		copies = N = 1;

		{
		char *line = NULL;
		int line_available = 80;
		while((line = gu_getline(line, &line_available, f)))
			{
			switch(*line)
				{
				case 'A':
					if(gu_sscanf(line, "Attr-ByteCounts: %ld %ld", &input_bytes, &postscript_bytes))
						continue;
					if(gu_sscanf(line, "Attr-Pages: %d", &pages))
						continue;
					break;
				case 'N':
					if(gu_sscanf(line, "N_Up: %d", &N))
						continue;
					break;
				case 'O':
					if(gu_sscanf(line, "Opts: %*d %d", &copies))
						continue;
					break;
				case 'P':
					if(lmatch(line, "PassThruPDL:"))
						{
						barbar = TRUE;
						continue;
						}
					if(gu_sscanf(line, "Progress: %ld %*d %d", &bytes_sent, &pages_printed))
						continue;
					break;
				}
			}
		}

		if(copies == -1)
			copies = 1;
		pages = (pages + N - 1) / N * copies;
		bytes_tosend = barbar ? input_bytes : postscript_bytes;

		if(bytes_tosend == 0)
			fatal(EXIT_INTERNAL, "ppop_lpq_banner_progress(): assertion failed");

		printf(" (%d%% sent", (int)((bytes_sent*(long)100) / bytes_tosend));

		if(pages_printed > 0 && pages > 0)
			printf(", %d of %d pages completed", pages_printed, pages);

		fputc(')', stdout);

		fclose(f);
		}
	} /* end of ppop_lpq_banner_progress() */

/*
** This is called once, just before the first queue entry is printed.
*/
static void ppop_lpq_banner(void)
	{
	FILE *FIFO, *reply_file;
	char *line = NULL; int line_len = 128;
	char *printer_name;
	char *printer_job_destname;
	int printer_job_id, printer_job_subid;
	int printer_status;
	int printer_next_retry;
	int printer_countdown;

	/* Ask for the status of all the printers: */
	FIFO = get_ready();
	fprintf(FIFO, "s %s\n", lpqlist_destname);
	fflush(FIFO);

	if((reply_file = wait_for_pprd(TRUE)) == (FILE*)NULL)
		{
		print_reply();
		return;
		}

	/*
	** The printer status comes back as a line of space
	** separated data and a line with the last status message
	** on it.
	*/
	while((line = gu_getline(line, &line_len, reply_file)))
		{
		printer_name = (char*)NULL;
		printer_job_destname = (char*)NULL;

		if(gu_sscanf(line, "%S %d %d %d %S %d %d",
				&printer_name, &printer_status,
				&printer_next_retry, &printer_countdown,
				&printer_job_destname, &printer_job_id, &printer_job_subid
				) != 7)
			{
			printf("Malformed response: \"%s\"", line);
			if(printer_name)
				gu_free(printer_name);
			if(printer_job_destname) 
				gu_free(printer_job_destname);
			continue;
			}

		/*
		** If there is more than one printer we will prefix the message
		** about each with its name.
		*/
		if(strcmp(lpqlist_destname, printer_name))
			printf("%s: ", printer_name);

		/*
		** Print the status of this printer.  These messages should
		** be compatible with the Samba parsing code.
		**
		** Samba expects:
		** OK: "enabled", "online", "idle", "no entries", "free", "ready"
		** STOPPED: "offline", "disabled", "down", "off", "waiting", "no daemon"
		** ERROR: "jam", "paper", "error", "responding", "not accepting", "not running", "turned off"
		*/
		switch(printer_status)
			{
			case PRNSTATUS_IDLE:
				printf("idle");
				break;
			case PRNSTATUS_PRINTING:
				if(printer_next_retry)
					printf("printing %s (%d%s retry)", jobid(printer_job_destname,printer_job_id,printer_job_subid), printer_next_retry, count_suffix(printer_next_retry));
				else
					printf("printing %s", jobid(printer_job_destname,printer_job_id,printer_job_subid));

				ppop_lpq_banner_progress(printer_job_destname, printer_job_id, printer_job_subid);

				break;
			case PRNSTATUS_CANCELING:
				fputs("canceling active job", stdout);
				break;
			case PRNSTATUS_SEIZING:
				fputs("seizing active job", stdout);
				break;
			case PRNSTATUS_STOPPING:
				printf("stopping (still printing %s)", jobid(printer_job_destname,printer_job_id,printer_job_subid));
				break;
			case PRNSTATUS_STOPT:
				printf("printing disabled");
				break;
			case PRNSTATUS_HALTING:
				printf("halting (still printing %s)", jobid(printer_job_destname,printer_job_id,printer_job_subid));
				break;
			case PRNSTATUS_FAULT:
				if(printer_next_retry)
					printf("error, %d%s retry in %d seconds", printer_next_retry, count_suffix(printer_next_retry), printer_countdown);
				else
					printf("error, no auto retry");
				break;
			case PRNSTATUS_ENGAGED:
				printf("otherwise engaged or offline");
				break;
			case PRNSTATUS_STARVED:
				printf("waiting for resource ration");
				break;
			default:
				printf("unknown status\n");
			}

		while((line = gu_getline(line, &line_len, reply_file)) && strcmp(line, "."))
			{
			/* print_aux_status(line, printer_status, "; "); */
			print_aux_status(line, printer_status, "\n\t");
			}

		putchar('\n');

		gu_free(printer_name);
		gu_free(printer_job_destname);
		} /* end of loop for each printer */

	fclose(reply_file);

	/*
	** Finally, print the banner:
	**                           1234567890123456789012345678901234567890
	**    1st    chappell   8021 entropy.tex                           100801 bytes
	*/
	puts("Rank   Owner      Job  Files                                 Total Size");

	/* Suppress "no entries": */
	lpqlist_banner_called = TRUE;
	} /* end of ppop_lpq_banner() */

/*
** This is called for each job in the queue.
*/
static int ppop_lpq_item(const struct QEntry *qentry,
		const struct QEntryFile *qentryfile,
		const char *onprinter,
		FILE *qstream)
	{
	char sizestr[16];
	char rankstr[8];
	#define fixed_for_MAXLENGTH 10
	char fixed_for[fixed_for_MAXLENGTH + 1];
	#define fixed_name_MAXLENGTH 37
	char fixed_name[fixed_name_MAXLENGTH + 1];
	char fixed_name_len;

	/* Advance the rank number to the next value: */
	lpqlist_rank++;

	/*
	** See if this file should be excluded because of not matching
	** the pattern set by the extra arguments.
	*/
	if(lpqlist_argv[0])
		{
		int x;
		int ok = FALSE;
		const char *user;
		#ifdef GNUC_HAPPY
		size_t user_len = 0;
		#else
		size_t user_len;
		#endif

		if(qentryfile->For)				/* probably never false */
			{
			user = qentryfile->For;
			user_len = strlen(qentryfile->For);
			}
		else							/* probably never invoked */
			{
			user = qentryfile->user;
			user_len = strlen(user);
			}

		for(x=0; lpqlist_argv[x]; x++)
			{
			if( (strlen(lpqlist_argv[x]) == user_len && strncmp(lpqlist_argv[x], user, user_len) == 0)
						|| qentry->id == atoi(lpqlist_argv[x]) )
				{
				ok = TRUE;
				break;
				}
			}
		if(!ok) return FALSE;	/* skip but don't stop */
		}

	/* Build a gramaticaly correct size string. */
	{
	long int bytes = qentryfile->PassThruPDL ? qentryfile->attr.input_bytes : qentryfile->attr.postscript_bytes;

	if(bytes == 1)
		strlcpy(sizestr, "1 byte", sizeof(sizestr));
	else
		snprintf(sizestr, sizeof(sizestr), "%ld bytes", bytes);
	}

	/*
	** If the job is printing, its "Rank" is "active",
	** otherwise it is "1st", "2nd", etc.
	*/
	if(qentry->status >= 0)				/* if printing */
		{
		strlcpy(rankstr, "active", sizeof(rankstr));
		}
	else								/* if not printing */
		{
		snprintf(rankstr, sizeof(rankstr), "%d%s", lpqlist_rank, count_suffix(lpqlist_rank));
		}

	/*
	** Change spaces in the user name to underscores because some programs
	** which parse LPQ output, such as Samba get confused by them.
	*/
	if(qentryfile->For)
		{
		const char *ptr = qentryfile->For;
		int x;

		for(x=0; ptr[x] && x < fixed_for_MAXLENGTH; x++)
			{
			if(isspace(ptr[x]))
				fixed_for[x] = '_';
			else
				fixed_for[x] = ptr[x];
			}

		fixed_for[x] = '\0';
		}
	else
		{
		strlcpy(fixed_for, "(unknown)", sizeof(fixed_for));
		}

	/*
	** Select a string for the LPQ filename field.  We would prefer the
	** actual file name but will accept the title.
	**
	** Change spaces in the file name to underscores because some programs
	** which parse LPQ output, such as Samba get confused by them.
	**
	** We also change all slashes to hyphens if the name doesn't
	** begin with a slash because Samba tries to extract the basename
	** and gets confused by dates.
	*/
	{
	const char *ptr;
	int x;
	if(!(ptr = qentryfile->lpqFileName))
		if(!(ptr = qentryfile->Title))
			ptr = "stdin";

	for(x=0; ptr[x] && x < fixed_name_MAXLENGTH; x++)
		{
		if(isspace(ptr[x]))
				fixed_name[x] = '_';
		else if(ptr[x] == '/' && ptr[0] != '/')
				fixed_name[x] = '-';
		else
				fixed_name[x] = ptr[x];
		}

	fixed_name[x] = '\0';
	fixed_name_len = x;
	}

	/*
	** If the jobs is held or arrested, modify the end
	** of the fixed_name to say so.
	*/
	{
	const char *note;

	switch(qentry->status)
		{
		case STATUS_ARRESTED:
			note = "arrested";
			break;

		case STATUS_HELD:
			note = "held";
			break;

		case STATUS_STRANDED:
			note = "stranded";
			break;

		default:
			note = NULL;
			break;
		}

	/* If there is a note, put it in the last columns of fixed_name[]. */
	if(note)
		{
		int x;
		int note_start = fixed_name_MAXLENGTH - strlen(note);
		strcpy(fixed_name + note_start, note);	/* strlcpy() doesn't help here */

		x = note_start - 2;
		if(x > fixed_name_len) x = fixed_name_len;

		fixed_name[x++] = '<';

		while(x < note_start)
			fixed_name[x++] = '-';
		}

	}

	printf("%-6.6s %-10.10s %-4d %-37.37s %s\n", rankstr, fixed_for, qentry->id, fixed_name, sizestr);

	return FALSE;
	} /* end of ppop_lpq_item() */

int ppop_lpq(char *argv[])
	{
	int retval;
	const struct Jobname *job;
	#define new_argv_SIZE 21
	char *new_argv[new_argv_SIZE];
	int x;

	/* If no argument, use PPRDEST: */
	argv = allow_PPRDEST(argv);

	/* If first parameter is empty or it is a job id rather than a destination id, */
	if(argv[0] == (char*)NULL || (job = parse_jobname(argv[0])) || job->id != WILDCARD_JOBID)
		{
		ppop_lpq_help();
		exit(EXIT_SYNTAX);
		}

	lpqlist_rank = 0;					/* reset number for "Rank" column */
	lpqlist_banner_called = FALSE;		/* not called yet! */
	lpqlist_destname = job->destname;
	lpqlist_argv = new_argv;

	for(x=0; x < (new_argv_SIZE - 1) && argv[x+1] != (char*)NULL; x++)
		{
		new_argv[x] = argv[x+1];
		argv[x+1] = (char*)NULL;
		}
	new_argv[x] = (char*)NULL;

	retval = custom_list(argv, ppop_lpq_help, ppop_lpq_banner, ppop_lpq_item, TRUE, opt_arrest_interest_interval);

	if( !lpqlist_banner_called && retval != EXIT_SYNTAX )
		puts("no entries");

	return retval;
	} /* end of ppop_lpq() */

/*================================================================
** ppop details {destination}
**
** Full debuging listing of the queue.
** This uses custom_list().
================================================================*/
static void ppop_details_help(void)
	{
	fputs(_("Usage: ppop details {all, <group>, <printer>, <job>}\n\n"
		"This command displays a detailed description of all jobs,\n"
		"the jobs queued for the indicated group, the jobs queued\n"
		"for the indicated printer, or the indicated job.\n"), errors);
	}

static void ppop_details_banner(void)
	{
	/* This kind of queue listing does not have column headings. */
	}

static int ppop_details_item(const struct QEntry *qentry,
		const struct QEntryFile *qentryfile,
		const char *onprinter,
		FILE *qstream)
	{
	/* print job name */
	printf("Job ID: %s\n", jobid(qentryfile->destname,qentry->id,qentry->subid));

	/* Say which part of the whole this is. */
	if(qentryfile->attr.parts == 1)
		printf("Part: 1 of 1\n");
	else
		printf("Part: %d of %d\n", qentry->subid,qentryfile->attr.parts);

	/* Give the input filter chain description */
	printf("Filters: %s\n", qentryfile->Filters ? qentryfile->Filters : "");

	/* Print submitting user id. */
	printf("User: %s\n", qentryfile->user);

	/* Who is it for? */
	printf("For: %s\n",qentryfile->For ? qentryfile->For : "(unknown)" );

	/* print the date it was submitted */
	printf("Submission Time: %s", ctime((time_t*)&qentryfile->time) );

	/* print priority */
	printf("Current Priority: %d\n", qentry->priority);
	printf("Origional Priority: %d\n", qentryfile->priority);

	/* What was the file name? */
	printf("lpq filename: %s\n", qentryfile->lpqFileName ? qentryfile->lpqFileName : "");

	/* What is the title? */
	printf("Title: %s\n", qentryfile->Title ? qentryfile->Title : "");

	/* Who or what is the creator? */
	printf("Creator: %s\n", qentryfile->Creator ? qentryfile->Creator : "");

	/* what are the routing instructions */
	printf("Routing: %s\n", qentryfile->Routing ? qentryfile->Routing : "");

	/* Flag pages */
	printf("Banner: %s\n",describe_flag_page_setting(qentryfile->do_banner));
	printf("Trailer: %s\n",describe_flag_page_setting(qentryfile->do_trailer));

	/* response methode and address */
	printf("Respond by: %s \"%s\"\n", qentryfile->responder.name, qentryfile->responder.address);
	if(qentryfile->responder.options)
		printf("Responder options: %s\n", qentryfile->responder.options);

	if(qentryfile->commentary)
		printf("Commentary: %d\n", qentryfile->commentary);

	/* attributes */
	printf("Required Language Level: %d\n", qentryfile->attr.langlevel);
	printf("Required Extensions: %d\n", qentryfile->attr.extensions);
	printf("DSC Level: %f\n", qentryfile->attr.DSClevel);
	if(qentryfile->attr.pages >= 0)
		printf("Pages: %d\n", qentryfile->attr.pages);
	else
		printf("Pages: ?\n");
	printf("Page Order: %s\n", describe_pageorder(qentryfile->attr.pageorder));
	gu_puts("Page List: ");
	pagemask_print(qentryfile);
	putchar('\n');
	printf("Prolog Present: %s\n", qentryfile->attr.prolog ? "True" : "False");
	printf("DocSetup Present: %s\n", qentryfile->attr.docsetup ? "True" : "False");
	printf("Script Present: %s\n", qentryfile->attr.script ? "True" : "False");
	printf("ProofMode: %s\n", describe_proofmode(qentryfile->attr.proofmode));
	printf("Orientation: %s\n", describe_orientation(qentryfile->attr.orientation));
	printf("Pages Per Sheet: %d\n", qentryfile->attr.pagefactor);
	printf("Unfiltered Size: %ld\n", qentryfile->attr.input_bytes);
	printf("PostScript Size: %ld\n", qentryfile->attr.postscript_bytes);
	printf("DocumentData: ");
	switch(qentryfile->attr.docdata)
		{
		case CODES_UNKNOWN:
			printf("UNKNOWN");
			break;
		case CODES_Clean7Bit:
			printf("Clean7Bit");
			break;
		case CODES_Clean8Bit:
			printf("Clean8Bit");
			break;
		case CODES_Binary:
			printf("Binary");
			break;
		default:
			printf("<invalid>");
			break;
		}
	printf("\n");

	/* N-Up */
	printf("N-Up N: %d\n", qentryfile->N_Up.N);
	printf("N-Up Borders: %s\n", qentryfile->N_Up.borders ? "True" : "False");
	printf("Signiture Sheets: %d\n", qentryfile->N_Up.sigsheets);
	printf("Signiture Part: %s\n", describe_sigpart(qentryfile->N_Up.sigpart));

	/* options */
	if(qentryfile->opts.copies < 0)
		fputs("Copies: ?\n", stdout);
	else
		printf("Copies: %d\n", qentryfile->opts.copies);
	printf("Collate: %s\n", qentryfile->opts.collate ? "True" : "False");
	printf("Auto Bin Select: %s\n", qentryfile->opts.binselect ? "True" : "False");
	printf("Keep Bad Features: %s\n", qentryfile->opts.keep_badfeatures ? "True" : "False");

	/* "Draft" notice */
	printf("Draft Notice: %s\n", qentryfile->draft_notice ? qentryfile->draft_notice : "");

	/* Print the job status */
	printf("Status: ");
	job_status(qentry, qentryfile, onprinter, (FILE*)NULL, 8, 8);

	/* show the never and notnow masks */
	printf("Never mask: %d\n", qentry->never);
	printf("NotNow mask: %d\n", qentry->notnow);

	/* Copy the tail end of the queue file to stdout. */
	{
	char *line = NULL;
	int line_max = 80;

	while((line = gu_getline(line, &line_max, qstream)) && strcmp(line, CHOPT_QF_ENDTAG1))
		{
		if(strncmp(line, "End", 3) == 0)		/* skip delimiters */
			continue;
		printf("%s\n", line);
		}

	if(line) gu_free(line);
	}

	/* Put a blank line after this entry. */
	putchar('\n');

	return FALSE;				/* don't stop */
	} /* end of ppop_details_item() */

int ppop_details(char *argv[])
	{
	return custom_list(argv, ppop_details_help, ppop_details_banner, ppop_details_item, FALSE, opt_arrest_interest_interval);
	} /* end of ppop_details() */

/*================================================================
** ppop qquery {destination}
**
** This command is for use by queue display programs which
** run ppop to get their data.
================================================================*/

#define MAX_QQUERY_ITEMS 64
static int qquery_query[MAX_QQUERY_ITEMS];
static int qquery_query_count;

#define MAX_QQUERY_ADDON_ITEMS 16
struct ADDON
	{
	char *name;
	char *value;
	};
static struct ADDON addon[MAX_QQUERY_ADDON_ITEMS];
static int addon_count;

static void ppop_qquery_help(void)
	{
	fputs("Wrong syntax!\n",errors);
	} /* end of ppop_qquery_help() */

static void ppop_qquery_banner(void)
	{ /* empty */ }

static int ppop_qquery_item(const struct QEntry *qentry,
		const struct QEntryFile *qentryfile,
		const char *onprinter,
		FILE *qstream)
	{
	const char *status;							/* status string */
	char status_scratch[128];					/* printing on _____ */
	char explain[256];							/* decoded "Reason:" line */
	char media[256];							/* space separated list of required media */

	/* Establish a line buffer */
	{
	char *line = NULL;
	int line_available = 80;

	/*
	** In this loop we read the Addon section of the queue file and save the
	** values of any lines the user has asked for.
	*/
	{
	char *p;
	int x;
	while((line = gu_getline(line, &line_available, qstream)) && strcmp(line, "EndAddon"))
		{
		if(!(p = strchr(line, ':')))
			{
			fprintf(errors, _("Invalid Addon line: %s\n"), line);
			continue;
			}

		*p = '\0';
		p++;
		while(isspace(*p))
			p++;

		for(x=0; x < addon_count; x++)
			{
			if(strcmp(addon[x].name, line) == 0)
				{
				if(addon[x].value)
					gu_free(addon[x].value);
				addon[x].value = gu_strdup(p);
				break;
				}
			}
		}
	}

	/*
	** In this loop, we read the remainder of the queue file and save the values
	** a a few of the lines.
	*/
	{
	int mi = 0;									/* start empty */
	explain[0] = '\0';							/* start empty */
	while((line = gu_getline(line, &line_available, qstream)) && strcmp(line, CHOPT_QF_ENDTAG1) )
		{
		if(strncmp(line, "Media: ", 7) == 0)
			{
			char *ptr = &line[7];
			int len = strcspn(ptr, " ");

			if((mi + 1 + len + 1) < sizeof(media))
				{
				if(mi)
					media[mi++] = ' ';
				while(len--)
					media[mi++] = *(ptr++);
				}
			}
		else if(strncmp(line, "Reason: ", 8) == 0)
			{
			char *ptr = &line[8];
			int di;

			/* Copy, truncate, and decode. */
			for(di=0; di < (sizeof(explain) - 1) && *ptr != '\n'; ptr++)
				{
				switch(*ptr)
					{
					case ',':
						explain[di++] = ',';
					case '|':
						explain[di++] = ' ';
						break;
					default:
						explain[di++] = *ptr;
						break;
					}
				}

			explain[di] = '\0';
			}
		}
	if(line) gu_free(line);
	media[mi] = '\0';
	}

	/* We are done with the line buffer */
	}

	/* Determine the status. */
	switch(qentry->status)
		{
		case STATUS_WAITING:			/* <--- waiting for printer */
			status = "waiting for printer";
			if(qentry->never)			/* If one or more counted out, */
				snprintf(explain, sizeof(explain), "Not all \"%s\" members are suitable", qentryfile->destname);
			else
				explain[0] = '\0';
			break;
		case STATUS_WAITING4MEDIA:		/* <--- waiting for media */
			status = "waiting for media";
			strlcpy(explain, media, sizeof(explain));
			break;
		case STATUS_HELD:				/* <--- job is held */
			status = "held";
			break;
		case STATUS_ARRESTED:			/* <--- job is arrested */
			status = "arrested";
			break;
		case STATUS_STRANDED:			/* <--- job is stranded */
			status = "stranded";
			break;
		case STATUS_CANCEL:				/* <--- job is being canceled */
			status = "being canceled";
			break;
		case STATUS_SEIZING:
			status = "being seized";
			break;
		default:						/* <--- job is being printed */
			if(qentry->status >= 0)		/* if greater than -1, is prnid */
				{						/* read the name of the printer */
				snprintf(status_scratch, sizeof(status_scratch), "printing on %s", onprinter);
				status = status_scratch;
				}
			else		/* if hit default case but not a positive status */
				{
				status = "unknown status";
				}
		} /* end of switch(qentry->status) */

	/* Print the requested fields. */
	{
	int x;
	for(x=0; x < qquery_query_count; x++)
		{
		if(x)
			fputc('\t', stdout);

		switch(qquery_query[x])
			{
			case 0:						/* jobname */
				gu_puts(jobid(qentryfile->destname,qentryfile->id,qentryfile->subid));
				break;
			case 1:						/* for */
				puts_detabbed(qentryfile->For ? qentryfile->For : "?");
				break;
			case 2:						/* title */
				puts_detabbed(qentryfile->Title ? qentryfile->Title : "");
				break;
			case 3:						/* status */
				gu_puts(status);
				break;
			case 4:						/* explain */
				puts_detabbed(explain);
				break;
			case 5:						/* copies */
				if(qentryfile->opts.copies >= 0)
					printf("%d", qentryfile->opts.copies);
				break;
			case 6:						/* copiescollate */
				if(qentryfile->opts.collate)
					gu_puts("true");
				else
					gu_puts("false");
				break;
			case 7:						/* pagefactor */
				printf("%d", qentryfile->attr.pagefactor);
				break;
			case 8:						/* routing */
				if(qentryfile->Routing)
					puts_detabbed(qentryfile->Routing);
				break;
			case 9:						/* creator */
				if(qentryfile->Creator)
					puts_detabbed(qentryfile->Creator);
				break;
			case 10:					/* nupn */
				printf("%d", qentryfile->N_Up.N);
				break;
			case 11:					/* nupborders */
				fputs( qentryfile->N_Up.borders ? "true" : "false", stdout);
				break;
			case 12:					/* sigsheets */
				printf("%d", qentryfile->N_Up.sigsheets);
				break;
			case 13:					/* sigpart */
				fputs(describe_sigpart(qentryfile->N_Up.sigpart), stdout);
				break;
			case 14:					/* pageorder */
				fputs(describe_pageorder(qentryfile->attr.pageorder), stdout);
				break;
			case 15:					/* proofmode */
				fputs(describe_proofmode(qentryfile->attr.proofmode), stdout);
				break;
			case 16:					/* priority */
				printf("%d", qentry->priority);
				break;
			case 17:					/* opriority */
				printf("%d", qentryfile->priority);
				break;
			case 18:					/* banner */
				fputs(describe_flag_page_setting(qentryfile->do_banner), stdout);
				break;
			case 19:					/* trailer */
				fputs(describe_flag_page_setting(qentryfile->do_trailer), stdout);
				break;
			case 20:					/* inputbytes */
				printf("%ld", qentryfile->attr.input_bytes);
				break;
			case 21:					/* postscriptbytes */
				printf("%ld", qentryfile->attr.postscript_bytes);
				break;
			case 22:					/* prolog */
				fputs( (qentryfile->attr.prolog ? "yes" : "no"), stdout);
				break;
			case 23:					/* docsetup */
				fputs( (qentryfile->attr.docsetup ? "yes" : "no"), stdout);
				break;
			case 24:					/* script */
				fputs( (qentryfile->attr.script ? "yes" : "no"), stdout);
				break;
			case 25:					/* orientation */
				fputs( describe_orientation(qentryfile->attr.orientation), stdout);
				break;
			case 26:					/* draft-notice */
				if(qentryfile->draft_notice)
					puts_detabbed(qentryfile->draft_notice);
				break;
			case 27:					/* username */
				fputs(qentryfile->user, stdout);
				break;
			case 28:					/* userid */
				/* removed */
				break;
			case 29:					/* proxy-for */
				/* removed */
				break;
			case 30:					/* longsubtime */
				{
				const char *t = ctime((time_t*)&qentryfile->time);
				printf("%.*s", (int)strcspn(t, "\n"), t);
				}
				break;
			case 31:					/* subtime */
				{
				char timestr[64];		/* 9 chars expected, but large buffer for utf-8 */
				fputs(format_time(timestr, sizeof(timestr), (time_t)qentryfile->time), stdout);
				}
				break;
			case 32:					/* pages */
				if(qentryfile->attr.pages >= 0)
					printf("%d", pagemask_count(qentryfile));
				else
					fputs("?", stdout);
				break;
			case 33:					/* lpqfilename */
				if(qentryfile->lpqFileName)
					puts_detabbed(qentryfile->lpqFileName);
				else if(qentryfile->Title)
					puts_detabbed(qentryfile->Title);
				break;
			case 34:					/* totalpages */
				{
				int total = qentryfile->attr.pages;
				if(qentryfile->opts.copies > 1) total *= qentryfile->opts.copies;
				if(total >= 0)
					printf("%d", total);
				else
					fputs("?", stdout);
				}
				break;
			case 35:					/* totalsides */
				{
				int total = qentryfile->attr.pages;
				if(qentryfile->opts.copies > 1)
					total *= qentryfile->opts.copies;
				total = (total + qentryfile->N_Up.N - 1) / qentryfile->N_Up.N;
				if(total >= 0)
					printf("%d", total);
				else
					fputs("?", stdout);
				}
				break;
			case 36:					/* totalsheets */
				{
				int total = qentryfile->attr.pages;
				if(qentryfile->opts.copies > 1)
					total *= qentryfile->opts.copies;
				total = (total + qentryfile->attr.pagefactor - 1) / qentryfile->attr.pagefactor;
				if(total >= 0)
					printf("%d", total);
				else
					gu_puts("?");
				}
				break;
			case 37:					/* fulljobname */
				printf("%s-%d.%d", qentryfile->destname, qentryfile->id, qentryfile->subid);
				break;
			case 38:					/* intype */
				if(qentryfile->Filters)
					gu_puts(qentryfile->Filters);
				break;
			case 39:					/* commentary */
				printf("%d", qentryfile->commentary);
				break;

			case 43:					/* destname */
				gu_puts(qentryfile->destname);
				break;
			case 44:					/* responder */
				if(qentryfile->responder.name)
					gu_puts(qentryfile->responder.name);
				break;
			case 45:					/* responder-address */
				if(qentryfile->responder.address);
					gu_puts(qentryfile->responder.address);
				break;
			case 46:					/* responder-options */
				if(qentryfile->responder.options)
					gu_puts(qentryfile->responder.options);
				break;
			case 47:					/* status/explain */
				gu_puts(status);
				if(explain[0])
					{
					gu_puts(" (");
					puts_detabbed(explain);
					gu_puts(")");
					}
				break;
			case 48:					/* pagesxcopies */
				if(qentryfile->attr.pages >= 0)
					printf("%d", qentryfile->attr.pages);
				else
					printf("?");
				if(qentryfile->opts.copies > 1)
					printf("x%d", qentryfile->opts.copies);
				break;
			case 49:					/* page-list */
				pagemask_print(qentryfile);
				break;

			default:
				if(qquery_query[x] >= 1000)
					{
					struct ADDON *ap = &addon[qquery_query[x] - 1000];
					if(ap->value)
						puts_detabbed(ap->value);
					}
				break;
			} /* end of switch */
		} /* end of for loop */
	}

	/* Free the Addon lines. */
	{
	int x;
	for(x=0; x < addon_count; x++)
		{
		if(addon[x].value)
			{
			gu_free(addon[x].value);
			addon[x].value = NULL;
			}
		}
	}

	putchar('\n');

	return FALSE;		/* don't stop */
	} /* end of ppop_qquery_item() */

int ppop_qquery(char *argv[])
	{
	const char function[] = "ppop_qquery";
	int x;
	char *ptr;
	int retval;

	addon_count = 0;

	for(x=0; (ptr = argv[x+1]); x++)
		{
		argv[x+1] = (char*)NULL;		/* Remove it so as not to disturb custom_list() later. */

		if(x >= MAX_QQUERY_ITEMS)
			{
			fprintf(errors, X_("%s(): MAX_QQUERY_ITEMS exceeded\n"), function);
			return EXIT_SYNTAX;
			}

		if(strncmp(ptr, "addon:", 6) == 0)
			{
			if(addon_count >= MAX_QQUERY_ADDON_ITEMS)
				{
				fprintf(errors, X_("%s(): MAX_QQUERY_ADDON_ITEMS exceeded\n"), function);
				return EXIT_SYNTAX;
				}
			addon[addon_count].name = (ptr + 6);
			addon[addon_count].value = NULL;
			qquery_query[x] = (addon_count + 1000);
			addon_count++;
			}

		/*
		** If you change this list, don't forget to change
		** ../docs/refman/ppad.8.pod too.
		*/
		else if(strcmp(ptr,"jobname")==0)
			qquery_query[x] = 0;
		else if(strcmp(ptr,"for")==0)
			qquery_query[x] = 1;
		else if(strcmp(ptr,"title")==0)
			qquery_query[x] = 2;
		else if(strcmp(ptr,"status")==0)
			qquery_query[x] = 3;
		else if(strcmp(ptr,"explain")==0)
			qquery_query[x] = 4;
		else if(strcmp(ptr,"copies")==0)
			qquery_query[x] = 5;
		else if(strcmp(ptr,"copiescollate")==0)
			qquery_query[x] = 6;
		else if(strcmp(ptr,"pagefactor")==0)
			qquery_query[x] = 7;
		else if(strcmp(ptr,"routing")==0)
			qquery_query[x] = 8;
		else if(strcmp(ptr,"creator")==0)
			qquery_query[x] = 9;
		else if(strcmp(ptr,"nupn")==0)
			qquery_query[x] = 10;
		else if(strcmp(ptr,"nupborders")==0)
			qquery_query[x] = 11;
		else if(strcmp(ptr,"sigsheets")==0)
			qquery_query[x] = 12;
		else if(strcmp(ptr,"sigpart")==0)
			qquery_query[x] = 13;
		else if(strcmp(ptr,"pageorder")==0)
			qquery_query[x] = 14;
		else if(strcmp(ptr,"proofmode")==0)
			qquery_query[x] = 15;
		else if(strcmp(ptr,"priority")==0)
			qquery_query[x] = 16;
		else if(strcmp(ptr,"opriority")==0)
			qquery_query[x] = 17;
		else if(strcmp(ptr,"banner")==0)
			qquery_query[x] = 18;
		else if(strcmp(ptr,"trailer")==0)
			qquery_query[x] = 19;
		else if(strcmp(ptr,"inputbytes")==0)
			qquery_query[x] = 20;
		else if(strcmp(ptr,"postscriptbytes")==0)
			qquery_query[x] = 21;
		else if(strcmp(ptr,"prolog")==0)
			qquery_query[x] = 22;
		else if(strcmp(ptr,"docsetup")==0)
			qquery_query[x] = 23;
		else if(strcmp(ptr,"script")==0)
			qquery_query[x] = 24;
		else if(strcmp(ptr,"orientation")==0)
			qquery_query[x] = 25;
		else if(strcmp(ptr,"draft-notice")==0)
			qquery_query[x] = 26;
		else if(strcmp(ptr,"username")==0)
			qquery_query[x] = 27;
		/* 28 was userid, can be reused */
		/* 29 was proxy-for, can be reused */
		else if(strcmp(ptr, "longsubtime") == 0)
			qquery_query[x] = 30;
		else if(strcmp(ptr, "subtime") == 0)
			qquery_query[x] = 31;
		else if(strcmp(ptr, "pages") == 0)
			qquery_query[x] = 32;
		else if(strcmp(ptr, "lpqfilename") == 0)
			qquery_query[x] = 33;
		else if(strcmp(ptr, "totalpages") == 0)
			qquery_query[x] = 34;
		else if(strcmp(ptr, "totalsides") == 0)
			qquery_query[x] = 35;
		else if(strcmp(ptr, "totalsheets") == 0)
			qquery_query[x] = 36;
		else if(strcmp(ptr, "fulljobname") == 0)
			qquery_query[x] = 37;
		else if(strcmp(ptr, "filters") == 0)
			qquery_query[x] = 38;
		else if(strcmp(ptr, "commentary") == 0)
			qquery_query[x] = 39;

		else if(strcmp(ptr, "destname") == 0)
			qquery_query[x] = 43;
		else if(strcmp(ptr, "responder") == 0)
			qquery_query[x] = 44;
		else if(strcmp(ptr, "responder-address") == 0)
			qquery_query[x] = 45;
		else if(strcmp(ptr, "responder-options") == 0)
			qquery_query[x] = 46;
		else if(strcmp(ptr, "status/explain") == 0)
			qquery_query[x] = 47;
		else if(strcmp(ptr, "pagesxcopies") == 0)
			qquery_query[x] = 48;
		else if(strcmp(ptr, "page-list") == 0)
			qquery_query[x] = 49;

		else
			{
			fprintf(errors, "Name of field number %d unrecognized: \"%s\"\n", x+1, ptr);
			return EXIT_SYNTAX;
			}
		}

	qquery_query_count = x;

	retval = custom_list(argv, ppop_qquery_help, ppop_qquery_banner, ppop_qquery_item, FALSE, opt_arrest_interest_interval);

	return retval;
	} /* end of ppop_qquery() */

/*===================================================================
** ppop progress {job}
**
** Display the progress of a job.  This command is intended for use
** by frontends.  It emmits three integers separated by tabs.
** They are the percentage of the job's bytes transmitted, the number
** of "%%Page:" comments sent, and the number of pages which the
** printer claims to have printed.	(Many printers make no such claims.
** Currently this is only supported for HP PJL printers.)
**
** Strings in this command are not marked for translation because
** this command is in a state of flux!
===================================================================*/

static void ppop_progress_help(void)
	{
	fputs(_("Usage: ppop progress <job>\n\n"
		"This command shows the progress made printing the\n"
		"indicated job.\n"), errors);
	}

static void ppop_progress_banner(void)
	{
	printf("Job									 |Bytes					   |Pages\n");
	printf("----------------+--------------------+----------+----------+---+----+----+----\n");
	printf("Queue ID		|Submitter			 |Sent		|Total	   |%%	|Std |Done|Tot\n");
	printf("----------------+--------------------+----------+----------+---+----+----+----\n");
	}

static int ppop_progress_item(const struct QEntry *qentry,
		const struct QEntryFile *qentryfile,
		const char *onprinter,
		FILE *qstream)
	{
	const char function[] = "ppop_progress_item";
	const char *jobname;
	long int bytes_total;
	long bytes_sent = 0;
	int pages_started = 0;
	int pages_printed = 0;
	int percent_sent;
	int total_pages;

	/* We have been passed a handle to the part of the queue file that
	   hasn't been parsed yet.  We will read it to find the last
	   "Progress:" line. */
	{
	char *line = NULL;
	int line_available = 80;
	while((line = gu_getline(line, &line_available, qstream)) && strcmp(line, QF_ENDTAG1))
		{
		gu_sscanf(line, "Progress: %ld %d %d", &bytes_sent, &pages_started, &pages_printed);
		}
	if(line) gu_free(line);
	}

	/* Format the job name. */
	jobname = jobid(qentryfile->destname, qentry->id, qentry->subid);

	/* Compute the percentage of progress.  We do this by figuring out
	   how many bytes have to be sent and the comparing it to the
	   number that have been sent. */
	bytes_total =
		qentryfile->PassThruPDL
				? qentryfile->attr.input_bytes
				: qentryfile->attr.postscript_bytes;

	if(bytes_total == 0)
		fatal(EXIT_INTERNAL, "%s(): assertion failed", function);

	percent_sent = (int)(((double)bytes_sent / (double)bytes_total * 100.0 + 0.5));

	/* Compute total number of pages.  This is the number
	   of page descriptions times the page count. */
	total_pages = qentryfile->attr.pages;
	if(qentryfile->opts.copies > 1)
		total_pages *= qentryfile->opts.copies;

	/* Print our assembled results. */
	if(opt_machine_readable)
		{
		printf("%s\t%s\t%ld\t%ld\t%d\t%d\t%d\t%d\n",
				jobname,
				qentryfile->For ? qentryfile->For : "",
				bytes_sent,
				bytes_total,
				percent_sent,
				pages_started,
				pages_printed,
				total_pages);
		}
	else
		{
		printf("%-16s|%-20s|%10ld|%10ld|%3d|%4d|%4d|%4d\n",
				jobname, qentryfile->For ? qentryfile->For : "",
				bytes_sent,
				bytes_total,
				percent_sent,
				pages_started,
				pages_printed,
				total_pages);
		}

	return FALSE;		/* don't stop */
	} /* end of ppop_progress_item() */

int ppop_progress(char *argv[])
	{
	return custom_list(argv, ppop_progress_help, ppop_progress_banner, ppop_progress_item, FALSE, -1);
	} /* end of ppop_progress() */

/* end of file */
