/*
** mouse:~ppr/src/ppop/ppop_cmds_other.c
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
** Last modified 21 October 2005.
*/

/*
** PPOP queue listing commands.
**
** Routines which implement other user commands.
*/

#include "config.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <wchar.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "ppop.h"
#include "util_exits.h"
#include "interface.h"

/*===================================================================
** Routine to display the number of jobs canceled.
** This is used for "ppop cancel", "ppop purge", and "ppop clean".
** The second argument is TRUE when this is called from
** ppop_cancel_byuser().
===================================================================*/
static void say_canceled(int count, int mine_only)
	{
	if(count == 0)
		{
		if(mine_only)
			gu_utf8_printf(_("You had no jobs to cancel.\n"));
		else
			gu_utf8_printf(_("There were no jobs to cancel.\n"));
		}
	else
		{
		gu_utf8_printf(ngettext("%d job was canceled.\n", "%d jobs were canceled.\n", count), count);
		}
	} /* end of say_canceled() */

/*===================================================================
** 
===================================================================*/

/*
** Translate an exit code into a fault type decription.  If the exit code
** does not indicate a fault, return NULL.
*/
static const char *fault_translate(int code)
	{
	switch(code)
		{
		case EXIT_PRNERR:
		case EXIT_PRNERR_NORETRY:
			return _("Use \"ppop alerts\" to see details.");
			break;
		case EXIT_PRNERR_NORETRY_ACCESS_DENIED:
			return _("Printer refuses to allow the spooler access.");
			break;
		case EXIT_PRNERR_NOT_RESPONDING:
			return _("Printer isn't responding.");
			break;
		case EXIT_PRNERR_NORETRY_BAD_SETTINGS:
			return _("Bad settings, use \"ppop alerts\" to see details.");
			break;
		case EXIT_PRNERR_NO_SUCH_ADDRESS:
		case EXIT_PRNERR_NORETRY_NO_SUCH_ADDRESS:
			return _("Printer address doesn't exist.");
			break;
		default:
			return NULL;
		}
	} /* end of fault_translate() */

/*===========================================================================
** Take a "ppop status" auxiliary status line and convert it to human-
** readable form.  Depending on the settings of --machine-readable and
** --opt_verbose, this function may choose not to print certain information.
** It will return non-zero if it prints something.
===========================================================================*/
int print_aux_status(char *line, int printer_status, const char sep[])
	{
	char *p;

	/* This has something to do with elaborating on fault conditions. */
	if((p = lmatchp(line, "exit:")))
		{
		const char *cp;
		if((cp = fault_translate(atoi(p))))
			{
			gu_utf8_puts(sep);
			if(opt_machine_readable)
				gu_utf8_printf("fault: %s", cp);
			else
				gu_utf8_printf(_("(%s)"), cp);
			return 1;
			}
		return 0;
		}

	/* Unified SNMP-style status of the printer */
	if((p = lmatchp(line, "status:")))
		{
		int device_status_code = -1, printer_status_code = -1;
		const char *description;

		gu_sscanf(p, "%d %d", &device_status_code, &printer_status_code);
		translate_snmp_status(device_status_code, printer_status_code, &description, NULL, NULL);

		if(opt_machine_readable)
			{
			gu_utf8_puts(sep);
			gu_utf8_printf("status: %s", description);
			return 1;
			}
		else if(opt_verbose || printer_status != PRNSTATUS_IDLE)
			{
			gu_utf8_puts(sep);
			gu_utf8_printf(_("Printer Status: %s"), gettext(description));
			return 1;
			}

		return 0;
		}

	/* Unified SNMP-style printer problem list item */
	if((p = lmatchp(line, "errorstate:")))
		{
		int bit = -1, start = 0, last = 0, last_commentary = 0;
		char *details = NULL;
		int start_minutes_ago, last_minutes_ago;
		gu_boolean print_howlong, print_ago;
		time_t time_now = time(NULL);

		gu_sscanf(p, "%d %d %d %d %T", &bit, &start, &last, &last_commentary, &details);

		/* Convert absolute times to times relative to the current time. */
		start_minutes_ago = (int)((time_now - start + 30) / 60);
		last_minutes_ago = (int)((time_now - last + 30) / 60);

		/* Decide about "for X minutes" and "(as of X minutes ago)". */
		print_ago = last_minutes_ago > 15;
		print_howlong = !print_ago && start_minutes_ago > 5;

		/* Skip things unconfirmed for more than 4 hours unless --opt_verbose is used. */
		if(last_minutes_ago > (4 * 60) && !opt_verbose)
			return 0;

		gu_utf8_puts(sep);

		{
		const char *description;
		translate_snmp_error(bit, &description, NULL, NULL);
		if(opt_machine_readable)
			gu_utf8_printf("errorstate: %s", description);
		else
			gu_utf8_printf(_("Printer Problem: \"%s\""), gettext(description));
		}

		/* If there are details beyond the SNMP fault bit available, print them. */
		if(details)
			{
			gu_utf8_printf(" (%s)", details);
			gu_free(details);
			}

		/* If the condition didn't arise within the last few minutes, say how long it has persisted. */
		if(print_howlong || opt_verbose)
			{
			if(start_minutes_ago < 120)
				{
				gu_utf8_printf(ngettext(" for %d minute", " for %d minutes", start_minutes_ago), start_minutes_ago);
				}
			else
				{
				int start_hours_ago = (start_minutes_ago + 30) / 60;
				gu_utf8_printf(ngettext(" for %d hour", " for %d hours", start_hours_ago), start_hours_ago);
				}
			}

		/* If the last notification was more than 15 minutes ago, say so. */
		if(print_ago || opt_verbose)
			{
			if(last_minutes_ago < 120)
				{
				gu_utf8_printf(ngettext(" (as of %d minute ago)", " (as of %d minutes ago)", last_minutes_ago), last_minutes_ago);
				}
			else
				{
				int last_hours_ago = (last_minutes_ago + 30) / 60;
				gu_utf8_printf(ngettext(" (as of %d hour ago)", " (as of %d hours ago)", last_hours_ago), last_hours_ago);
				}
			}

		return 1;
		}

	/* What pprdrv is doing right now */
	if((p = lmatchp(line, "operation:")))
		{
		char operation[16];
		int minutes = 0;

		if(gu_sscanf(p, "%@s %d", sizeof(operation), operation, &minutes) == 2)
			{
			if(strcmp(operation, "LOOKUP") == 0)
				p = _("looking up address");
			if(strcmp(operation, "CONNECT") == 0)
				p = _("connecting");
			else if(strcmp(operation, "WRITE") == 0)
				p = _("sending data");
			else if(strcmp(operation, "CLOSE") == 0)
				p = _("closing connection");
			else if(strcmp(operation, "QUERY") == 0)
				p = _("querying printer");
			else if(strcmp(operation, "WAIT_PJL_START") == 0)
				p = _("syncing PJL");
			else if(strncmp(operation, "WAIT_", 5) == 0)
				p = _("waiting for printer to finish");
			else if(strcmp(operation, "RIP_CLOSE") == 0)
				p = _("waiting for RIP to finish");
			else if(strcmp(operation, "COM_WAIT") == 0)
				p = _("waiting for responders to finish");
			else
				p = operation;
			}

		gu_utf8_puts(sep);

		if(opt_machine_readable)
			gu_utf8_puts("operation: ");
		else
			gu_utf8_puts(_("Operation: "));

		if(minutes >= 2)
			gu_utf8_printf(ngettext("%s, stalled for %d minute", "%s, stalled for %d minutes", minutes), p, minutes);
		else
			gu_utf8_printf(_("%s..."), p);

		return 1;
		}

	/* Last %%[ status: ]%% or %%[ PrinterError: ]%% */
	if((p = lmatchp(line, "lw-status:")))
		{
		int important = atoi(p);
		p += strspn(p, "0123456789");
		p += strspn(p, " \t");
		if(opt_machine_readable)
			{
			gu_utf8_puts(sep);
			gu_utf8_printf("lw-status: %d ", important ? 1 : 0);
			puts_detabbed(p);
			return 1;
			}
		else if(opt_verbose || important)
			{
			gu_utf8_puts(sep);
			gu_utf8_printf(_("Raw LW Status: \"%s\""), p);
			return 1;
			}
		return 0;
		}

	/* Last PJL status message received from printer */
	if((p = lmatchp(line, "pjl-status:")))
		{
		int important = atoi(p);
		p += strspn(p, "0123456789");
		p += strspn(p, " \t");
		if(opt_machine_readable)
			{
			gu_utf8_puts(sep);
			gu_utf8_printf("pjl-status: %d ", important ? 1 : 0);
			puts_detabbed(p);
			return 1;
			}
		else if(opt_verbose || important)
			{
			gu_utf8_puts(sep);
			gu_utf8_printf(_("Raw PJL Status: %s"), p);
			return 1;
			}
		return 0;
		}

	/* Last SNMP status retrieved from the printer */
	if((p = lmatchp(line, "snmp-status:")))
		{
		int important = atoi(p);
		p += strspn(p, "0123456789");
		p += strspn(p, " \t");
		if(opt_machine_readable || opt_verbose || important)
			{
			char *f1, *f2, *fx;

			gu_utf8_puts(sep);

			if(opt_machine_readable)
				gu_utf8_printf("snmp-status: %d ", important ? 1 : 0);
			else
				gu_utf8_printf(_("Raw SNMP Status: "));

			if(!(f1 = gu_strsep(&p, " ")) || !(f2 = gu_strsep(&p, " ")))
				{
				gu_utf8_printf("[can't parse]");
				}
			else
				{
				int i;
				const char *raw1;
				translate_snmp_status(atoi(f1), atoi(f2), NULL, &raw1, NULL);
				gu_utf8_printf("%s", raw1);
				for(i=0; (fx = gu_strsep(&p, " ")); i++)
					{
					translate_snmp_error(atoi(fx), NULL, &raw1, NULL);
					gu_utf8_printf("%c %s", i==0 ? ';' : ',', raw1);
					}
				}

			return 1;
			}
		return 0;
		}

	/* The number of seconds on the page clock and (if it is running) at what
	   time (wall clock) the clock was observed to have that many seconds
	   on it.
	   */
	if((p = lmatchp(line, "page:")))
		{
		int seconds, asof;

		gu_utf8_puts(sep);

		if(opt_machine_readable)
			gu_utf8_puts("page: ");
		else
			gu_utf8_puts(_("Page Clock: "));

		switch(gu_sscanf(p, "%d %d", &seconds, &asof))
			{
			case 1:
				gu_utf8_printf(ngettext("%d second (clock stopt)", "%d seconds (clock stopt)", seconds), seconds);
				break;
			case 2:
				{
				int computed = time(NULL);
				computed -= asof;
				computed += seconds;
				gu_utf8_printf(ngettext("%d second", "%d seconds", computed), computed);
				}
				break;
			default:
				gu_utf8_puts(line);
				break;
			}
		return 1;
		}

	/* The name of the job the printer is printing, if it isn't our's. */
	if((p = lmatchp(line, "job:")))
		{
		gu_utf8_puts(sep);
		if(opt_machine_readable)
			{
			gu_utf8_puts("job: ");
			puts_detabbed(p);
			}
		else
			{
			gu_utf8_printf(_("Job on Printer: %s"), p);
			}
		return 1;
		}

	/* new and unknown kind of aux status line */
	gu_utf8_puts(sep);
	puts_detabbed(line);
	return 1;
	} /* end of print_aux_status() */

/*===================================================================
** ppop status {printer}
**
** Display the status of printers.  We will send the "s" command
** over the FIFO to pprd.
===================================================================*/
int ppop_status(char *argv[])
	{
	const char *destname;
	FILE *FIFO, *reply_file;
	char *line = NULL;
	int line_len = 128;
	char *printer_name;
	char *job_destname;
	int job_id,job_subid;
	int status;
	int next_retry;
	int countdown;

	if(!argv[0])
		{
		gu_utf8_fprintf(errors, _("Usage: ppop status {<printer>, <group>, all}\n"));
		return EXIT_SYNTAX;
		}

	if(!(destname = parse_destname(argv[0], FALSE)))
		return EXIT_SYNTAX;

	FIFO = get_ready();
	fprintf(FIFO, "s %s\n", destname);
	fflush(FIFO);

	if( ! opt_machine_readable )
		{
		gu_utf8_printf("%s          %s\n", _("Printer"), _("Status"));
		gu_utf8_puts("------------------------------------------------------------\n");
		}

	if(!(reply_file = wait_for_pprd(TRUE)))
		return print_reply();

	while((line = gu_getline(line, &line_len, reply_file)))
		{
		printer_name = job_destname = (char*)NULL;

		if(gu_sscanf(line, "%S %d %d %d %S %d %d",
				&printer_name, &status,
				&next_retry, &countdown, &job_destname, &job_id, &job_subid
				) != 7)
			{
			gu_utf8_printf(_("Malformed response: %s\n"), line);
			gu_free_if(printer_name);
			gu_free_if(job_destname);
			continue;
			}

		if(! opt_machine_readable)			/* human readable */
			gu_utf8_printf("%-16s ", printer_name);
		else
			gu_utf8_printf("%s\t", printer_name);

		if(!opt_machine_readable)			/* human readable */
			{
			switch(status)
				{
				case PRNSTATUS_IDLE:
					gu_utf8_puts(_("idle"));
					break;
				case PRNSTATUS_PRINTING:
					if(next_retry)
						gu_utf8_printf(_("printing %s (retry %d)"), jobid(job_destname,job_id,job_subid), next_retry);
					else
						gu_utf8_printf(_("printing %s"), jobid(job_destname,job_id,job_subid));
					break;
				case PRNSTATUS_CANCELING:
					gu_utf8_printf(_("canceling %s"), jobid(job_destname, job_id, job_subid));
					break;
				case PRNSTATUS_SEIZING:			/* Spelling "seizing" is standard! */
					gu_utf8_printf(_("seizing %s"), jobid(job_destname, job_id, job_subid));
					break;
				case PRNSTATUS_STOPPING:
					gu_utf8_printf(_("stopping (printing %s)"), jobid(job_destname,job_id,job_subid));
					break;
				case PRNSTATUS_HALTING:
					gu_utf8_printf(_("halting (printing %s)"), jobid(job_destname, job_id, job_subid));
					break;
				case PRNSTATUS_STOPT:
					gu_utf8_puts(_("stopt"));
					break;
				case PRNSTATUS_FAULT:
					if(next_retry)
						gu_utf8_printf(ngettext(
							"fault, retry %d in %d second",
							"fault, retry %d in %d seconds",
							countdown), next_retry, countdown);
					else
						gu_utf8_puts(_("fault, no auto retry"));
					break;
				case PRNSTATUS_ENGAGED:
					gu_utf8_printf(ngettext(
						"otherwise engaged or off-line, retry %d in %d second",
						"otherwise engaged or off-line, retry %d in %d seconds",
						countdown), next_retry, countdown);
					break;
				case PRNSTATUS_STARVED:
					gu_utf8_puts(_("waiting for resource ration"));
					break;
				default:
					gu_utf8_puts(X_("unknown status"));
					break;
				}
			}
		else							/* If to be machine readable, */
			{							/* Note: don't translate these strings! */
			switch(status)
				{
				case PRNSTATUS_IDLE:
					gu_utf8_puts("idle");
					break;
				case PRNSTATUS_PRINTING:
					gu_utf8_printf("printing %s %d", jobid(job_destname,job_id,job_subid), next_retry);
					break;
				case PRNSTATUS_CANCELING:
					gu_utf8_printf("canceling %s %d", jobid(job_destname,job_id,job_subid), next_retry);
					break;
				case PRNSTATUS_SEIZING:
					gu_utf8_printf("seizing %s %d", jobid(job_destname,job_id,job_subid), next_retry);
					break;
				case PRNSTATUS_STOPPING:
					gu_utf8_printf("stopping %s %d", jobid(job_destname,job_id,job_subid), next_retry);
					break;
				case PRNSTATUS_STOPT:
					gu_utf8_puts("stopt");
					break;
				case PRNSTATUS_HALTING:
					gu_utf8_printf("halting %s %d", jobid(job_destname,job_id,job_subid), next_retry);
					break;
				case PRNSTATUS_FAULT:
					gu_utf8_printf("fault %d %d", next_retry, countdown);
					break;
				case PRNSTATUS_ENGAGED:
					gu_utf8_printf("engaged %d %d", next_retry, countdown);
					break;
				case PRNSTATUS_STARVED:
					gu_utf8_puts("starved");
					break;
				default:
					gu_utf8_puts("unknown");
					break;
				}
			}

		/*
		** Read and print auxiliary status lines until we receive a line with just a period.
		** Notice that we pass the lines thru print_aux_status() to get them translated
		** to human readable form the the user's natural language.
		*/
		while((line = gu_getline(line, &line_len, reply_file)) && strcmp(line, "."))
			{
			print_aux_status(line, status,
				opt_machine_readable ? "\t" : "\n                 ");
			}

		putchar('\n');

		/* leave these frees in since they are allocated on each iteration */
		gu_free(printer_name);
		gu_free(job_destname);
		} /* end of loop for each printer */

	fclose(reply_file);
	return EXIT_OK;
	} /* end of ppop_status() */

/*=====================================================================
** ppop message {printer}
** Retrieve the auxiliary status messages from a certain printer.
=====================================================================*/
int ppop_message(char *argv[])
	{
	const char *destname;
	char fname[MAX_PPR_PATH];
	FILE *statfile;

	if(!argv[0] || argv[1])
		{
		fputs(_("Usage: ppop message <printer>\n"), errors);
		return EXIT_SYNTAX;
		}

	if(!(destname = parse_destname(argv[0], FALSE)))
		return EXIT_SYNTAX;

	ppr_fnamef(fname, "%s/%s/status", PRINTERS_CACHEDIR, destname);
	if((statfile = fopen(fname, "r")))
		{
		char *line = NULL; int line_len = 40;
		while((line = gu_getline(line, &line_len, statfile)))
			{
			if(print_aux_status(line, PRNSTATUS_PRINTING, ""))
				putchar('\n');
			}
		fclose(statfile);
		}

	return EXIT_OK;
	} /* end of ppop_message() */

/*=====================================================================
** ppop media {printer}
**
** Show the media which are mounted on a specific printer
** or all printers in a group.
** Use the "f" command.
=====================================================================*/
int ppop_media(char *argv[])
	{
	const char function[] = "ppop_media";
	int retcode;
	const char *destname;
	FILE *FIFO, *reply_file;

	if(!argv[0])
		{
		gu_utf8_fputs(_("Usage: ppop media {<printer>, <group>, all}\n"), errors);
		return EXIT_SYNTAX;
		}

	if(!(destname = parse_destname(argv[0], FALSE)))
		return EXIT_SYNTAX;

	FIFO = get_ready();
	fprintf(FIFO, "f %s\n", destname);
	fflush(FIFO);

	if(!opt_machine_readable)
		{
		gu_utf8_printf(_("Printer                  Bin             Medium\n"));
		gu_utf8_printf("---------------------------------------------------------------\n");
		}

	if(!(reply_file = wait_for_pprd(TRUE)))
		return print_reply();

	{
	retcode = EXIT_OK;
	char *line = NULL;
	int line_space = 80;
	while(retcode == EXIT_OK && (line = gu_getline(line, &line_space, reply_file)))
		{
		char *printer, *bin, *medium;
		int nbins;
		int pos = 0;
		if(gu_sscanf(line, "%S %d", &printer, &nbins) != 2)
			{
			retcode = EXIT_INTERNAL;
			break;
			}
		if(!opt_machine_readable)
			{
			gu_utf8_printf("%-24s ", printer);
			pos += 25;
			if(nbins == 0)
				gu_utf8_printf(_("(no bins defined)\n"));
			}
		while(nbins-- > 0)
			{
			medium = NULL;
			if(!(line = gu_getline(line, &line_space, reply_file)) || gu_sscanf(line, "%S %S", &bin, &medium) < 1)
				{
				retcode = EXIT_INTERNAL;
				break;
				}
			if(!opt_machine_readable)
				{
				while(pos++ < 25)
					gu_fputwc(' ', stdout);
				gu_utf8_printf("%-15s %s\n", bin, medium ? medium : "");
				}
			else
				gu_utf8_printf("%s\t%s\t%s\n", printer, bin, medium ? medium : "");
			gu_free(bin);
			gu_free_if(medium);
			pos = 0;
			}
		gu_free(printer);
		}
	}

	if(retcode == EXIT_INTERNAL)
		gu_utf8_fprintf(errors, "%s(): bad response from pprd.\n", function);

	return retcode;
	} /* end of ppop_media() */

/*==========================================================================
** ppop mount {printer} {tray} {media}
**
** Mount a specific media type on a specific location.
** We do this with the "M" (Mount) command.
==========================================================================*/
int ppop_mount(char *argv[])
	{
	int retcode = EXIT_OK;
	const char *destname, *binname, *mediumname;
	FILE *FIFO;

	if(!argv[0] || !argv[1])
		{
		gu_utf8_fprintf(errors, _("Usage: ppop mount <printer> <bin> <medium>\n"));
		return EXIT_SYNTAX;
		}

	if(!assert_am_operator())
		return EXIT_DENIED;

	if(!(destname = parse_destname(argv[0], FALSE)))
		return EXIT_SYNTAX;

	binname = argv[1];
	mediumname = argv[2] ? argv[2] : "";

	/* If the mediumname is not empty, make sure it exists. */
	if(mediumname[0])
		{
		do	{
			char padded_mediumname[MAX_MEDIANAME];
			FILE *mfile;							/* media file */
			struct Media ms;
			ASCIIZ_to_padded(padded_mediumname,mediumname,sizeof(padded_mediumname));
	
			if(!(mfile = fopen(MEDIAFILE, "rb")))
				{
				gu_utf8_fprintf(errors, _("Can't open \"%s\", errno=%d (%s)."), MEDIAFILE, errno, gu_strerror(errno));
				retcode = EXIT_INTERNAL;
				break;
				}
	
			do	{
				if(fread(&ms,sizeof(struct Media), 1, mfile) == 0)
					{
					gu_utf8_fprintf(errors, _("Medium \"%s\" is unknown.\n"), mediumname);
					retcode = EXIT_ERROR;
					break;
					}
				} while(memcmp(padded_mediumname, ms.medianame, sizeof(ms.medianame)) != 0);
	
			fclose(mfile);
			} while(FALSE);
		}

	/*
	** If there was no error detected above, get ready for the 
	** response from pprd and send the mount command.
	*/
	if(retcode == EXIT_OK)
		{
		FIFO = get_ready();
		fprintf(FIFO, "M %s %s %s\n", destname, binname, mediumname);
		fflush(FIFO);
		wait_for_pprd(TRUE);
		retcode = print_reply();
		}

	return retcode;
	} /* end of ppop_mount() */

/*==========================================================================
** ppop start {printer}
**
** start a printer which was stopt
** We do this with the "t" command.
**
** ppop [w]stop {printer}
**
** Stop a printer gently, let current job finish.
** We do this with the "p" command.
** If the second parameter is true, we will wait for
** the printer to stop.
**
** ppop halt {printer}
**
** Halt, stop a printer now, terminating printing of the current job.
** We do this with the "b" (stop with a bang) command.
** "h" is used by hold.
==========================================================================*/
int ppop_start_stop_wstop_halt(char *argv[], int variation)
	{
	int x;
	int result = 0;
	const char *destname;
	FILE *FIFO;

	if(!argv[0])
		{
		switch(variation)
			{
			case 0:
				fputs(_("Usage: ppop start <printer> ...\n\n"
						"This command starts a previously stopt printer.\n"), errors);
				break;
			case 1:
			case 2:
				fputs(_("Usage: ppop stop <printer> ...\n"
						"Usage: ppop wstop {printer}\n\n"
						"This command stops a printer from printing.  If a job is being\n"
						"printed when this command is issued, the printer does not\n"
						"actually stop until the job is done.\n\n"
						"The second form, \"ppop wstop\" does not return until the printer\n"
						"has actually stopt.\n"), errors);
				break;
			case 3:
				fputs(_("Usage: ppop halt <printer> ...\n\n"
						"This command stops the printer immediately.  If a job is printing,\n"
						"the job is returned to the queue for later printing.\n"), errors);
				break;
			}

		return EXIT_SYNTAX;
		}

	if(!assert_am_operator())			/* only allow operator to do this */
		return EXIT_DENIED;

	for(x=0; argv[x]; x++)
		{
		if(!(destname = parse_destname(argv[x], FALSE)))
			{
			result = EXIT_SYNTAX;
			break;
			}

		FIFO = get_ready();

		switch(variation)
			{
			case 0:				/* start */
				fprintf(FIFO, "t %s\n", destname);
				break;
			case 1:				/* stop */
				fprintf(FIFO, "p %s\n", destname);
				break;
			case 2:				/* wstop */
				fprintf(FIFO, "P %s\n", destname);
				break;
			case 3:				/* halt */
				fprintf(FIFO, "b %s\n", destname);
				break;
			}

		fflush(FIFO);					/* send the command */

		wait_for_pprd(variation != 2);	/* no timeout for wstop */

		if((result = print_reply()))
			break;
		}

	return result;
	} /* end of ppop_start() */

/*========================================================================
** ppop cancel {<job>, <destination>, all}
**
** Cancel a job or all jobs on a destination.
** We do these things with the "c"ancel command.
**
** The function ppop_cancel_byuser() may be called by ppop_cancel().
** It does the work for ppop cancel in cases where the argument is only
** a destination name.  It is used to cancel all of a user's own
** jobs.  It uses custom_list() from ppop_cmds_listq.c to determine
** which jobs belong to this user.
========================================================================*/
static int ppop_cancel_byuser_total;
static int ppop_cancel_byuser_inform;

static void ppop_cancel_byuser_help(void)
	{ fputs("Syntax error.\n", errors); }

static void ppop_cancel_byuser_banner(void)
	{ }

static int ppop_cancel_byuser_item(const struct QEntry *qentry,
		const struct QEntryFile *qentryfile,
		const char *onprinter,
		FILE *qstream)
	{
	FILE *FIFO, *reply_file;
	int count;

	if( ! is_my_job(qentry, qentryfile) )
		return FALSE;	/* don't stop */

	FIFO = get_ready();
	fprintf(FIFO, "c %s %d %d %d\n", qentryfile->destname, qentry->id, qentry->subid, ppop_cancel_byuser_inform);
	fflush(FIFO);

	if(!(reply_file = wait_for_pprd(TRUE)))
		{
		print_reply();
		return TRUE;	/* stop */
		}

	count = gu_fgetint(reply_file);
	ppop_cancel_byuser_total += count;

	fclose(reply_file);

	return FALSE;		/* don't stop */
	} /* end of ppop_cancel_byuser_item() */

/* This function is called by ppop_cancel() when no specific job is specified (I think). */
static int ppop_cancel_byuser(char *destname, int inform)
	{
	char *list[2];
	int ret;

	ppop_cancel_byuser_inform = inform;
	ppop_cancel_byuser_total = 0;

	list[0] = destname;			/* custom_list() wants an argument list, */
	list[1] = (char*)NULL;		/* construct one. */

	if((ret = custom_list(list, ppop_cancel_byuser_help, ppop_cancel_byuser_banner, ppop_cancel_byuser_item, FALSE, -1)) )
		return ret;

	say_canceled(ppop_cancel_byuser_total, TRUE);

	return EXIT_OK;
	} /* end of ppop_cancel_byuser() */

int ppop_cancel(char *argv[], int inform)
	{
	int x;
	const struct Jobname *job;
	FILE *FIFO, *reply_file;
	int count;

	if(!argv[0])
		{
		fputs(_("Usage: ppop cancel {<job>, <destination>}\n\n"
			  "This command cancels a job or all of your jobs queued for the\n"
			  "specified destination.\n"), errors);

		return EXIT_SYNTAX;
		}

	for(x=0; argv[x]; x++)
		{
		if(!(job = parse_jobname(argv[x])))
			return EXIT_SYNTAX;

		if(job->id == WILDCARD_JOBID)	/* If it is to be all jobs owned by this user, */
			{							/* then use special routine. */
			int ret;
			if((ret = ppop_cancel_byuser(argv[x], inform)))
				return ret;
			continue;
			}

		/* Make sure the user has permission to cancel this job. */
		if(!job_permission_check(job))
			return EXIT_DENIED;

		/* Ask pprd to cancel it. */
		FIFO = get_ready();
		fprintf(FIFO, "c %s %d %d %d\n", job->destname, job->id, job->subid, inform);
		fflush(FIFO);

		if(!(reply_file = wait_for_pprd(TRUE)))
			return print_reply();

		count = gu_fgetint(reply_file);

		say_canceled(count, FALSE);

		fclose(reply_file);

		if(count == 0)
			return EXIT_NOTFOUND;
		}

	return EXIT_OK;
	} /* end of ppop_cancel() */

/*========================================================================
** ppop purge _destination_
**
** Cancel all jobs on a destination.  Only an operator may do this.
** We do it with the "c"ancel command and -1 for the job id and subid.
========================================================================*/
int ppop_purge(char *argv[], int inform)
	{
	int x;
	const char *destname;
	FILE *FIFO, *reply_file;
	int count;

	if(argv[0] == (char*)NULL)
		{
		fputs(_("Usage: ppop purge <destination> ...\n\n"
				"This command cancels all jobs queued for a particular destination.\n"
				"Only an operator may use this command.  Extra arguments are\n"
				"interpreted as the names of extra destinations to purge.\n"), errors);

		return EXIT_SYNTAX;
		}

	if(!assert_am_operator())
		return EXIT_DENIED;

	for(x=0; argv[x]; x++)
		{
		if(!(destname = parse_destname(argv[x], FALSE)))
			return EXIT_SYNTAX;

		FIFO = get_ready();
		fprintf(FIFO, "c %s -1 -1 %d\n", destname, inform);
		fflush(FIFO);

		if(!(reply_file = wait_for_pprd(TRUE)))
			return print_reply();

		count = gu_fgetint(reply_file);

		say_canceled(count, FALSE);		/* say how many were canceled */

		fclose(reply_file);
		}

	return EXIT_OK;
	} /* end of ppop_purge() */

/*=======================================================================
** ppop clean _destination_
**
** Delete all the arrested jobs on a destination.
=======================================================================*/
static int ppop_clean_total;

static void ppop_clean_help(void)
	{
	fputs(_("Syntax error.\n"), errors);
	}

static void ppop_clean_banner(void)
	{
	}

static int ppop_clean_item(const struct QEntry *qentry,
		const struct QEntryFile *qentryfile,
		const char *onprinter,
		FILE *qstream)
	{
	FILE *FIFO, *reply_file;
	int count;

	if(qentry->status != STATUS_ARRESTED)
		return FALSE;	/* don't stop */

	/*
	** Send a cancel command to pprd.  The value for the 7th field (inform)
	** does not matter since the user is never informed
	** of the deletion of arrested jobs.
	*/
	FIFO = get_ready();
	fprintf(FIFO, "c %s %d %d 1\n", qentryfile->destname, qentry->id, qentry->subid);
	fflush(FIFO);

	if((reply_file = wait_for_pprd(TRUE)) == (FILE*)NULL)
		{
		print_reply();
		return TRUE;	/* stop */
		}

	count = gu_fgetint(reply_file);
	ppop_clean_total += count;

	fclose(reply_file);

	return FALSE;		/* don't stop */
	} /* end of ppop_clean_item() */

int ppop_clean(char *argv[])
	{
	int ret;

	if(argv[0] == (char*)NULL)
		{
		fputs(_("Usage: ppop clean <destination> ...\n\n"
				"This command will delete all of the arrested jobs\n"
				"queued for the indicated destination or destinations\n"), errors);
		exit(EXIT_SYNTAX);
		}

	if(!assert_am_operator())
		return EXIT_DENIED;

	ppop_clean_total = 0;

	if((ret = custom_list(argv, ppop_clean_help, ppop_clean_banner, ppop_clean_item, FALSE, -1)))
		return ret;

	say_canceled(ppop_clean_total, FALSE);

	return EXIT_OK;
	} /* end of ppop_clean() */

/*=======================================================================
** ppop cancel-active <destination>
** ppop cancel-my-active <destination>
**
** Delete the active job for the destination.
=======================================================================*/
static int ppop_cancel_active_total;
static int ppop_cancel_active_my;
static int ppop_cancel_active_inform;

static void ppop_cancel_active_help(void)
	{
	fputs("Syntax error.\n", errors);
	}

static void ppop_cancel_active_banner(void)
	{
	}

static int ppop_cancel_active_item(const struct QEntry *qentry,
		const struct QEntryFile *qentryfile,
		const char *onprinter,
		FILE *qstream)
	{
	FILE *FIFO, *reply_file;
	int count;

	if(qentry->status < 0)
		return FALSE;

	if(ppop_cancel_active_my && !is_my_job(qentry, qentryfile))
		return TRUE;

	FIFO = get_ready();
	fprintf(FIFO, "c %s %d %d %d\n", qentryfile->destname, qentry->id, qentry->subid, ppop_cancel_active_inform);
	fflush(FIFO);

	if((reply_file = wait_for_pprd(TRUE)) == (FILE*)NULL)
		{
		print_reply();
		return TRUE;
		}

	count = gu_fgetint(reply_file);
	ppop_cancel_active_total += count;

	fclose(reply_file);

	return FALSE;
	} /* end of ppop_cancel_active_item() */

int ppop_cancel_active(char *argv[], int my, int inform)
	{
	int ret;
	const struct Jobname *job;

	/* If parameter missing or it is a job id rather than a destination id, */
	if(!argv[0] || !(job = parse_jobname(argv[0])) || job->id != WILDCARD_JOBID)
		{
		gu_utf8_fprintf(errors, _("Usage: ppop cancel-%sactive <destination> ...\n\n"
				"This command will delete all of the arrested jobs\n"
				"queued for the indicated destination or destinations.\n"), my ? "my-" : "");
		exit(EXIT_SYNTAX);
		}

	if(!my && !assert_am_operator())
		return EXIT_DENIED;

	ppop_cancel_active_my = my;
	ppop_cancel_active_inform = inform;

	ppop_cancel_active_total = 0;

	if((ret = custom_list(argv, ppop_cancel_active_help, ppop_cancel_active_banner, ppop_cancel_active_item, FALSE, -1)))
		return ret;

	if(ppop_cancel_active_total == 0)
		{
		if(my)
			gu_utf8_puts(_("You have no active jobs to cancel.\n"));
		else
			gu_utf8_puts(_("There are no active jobs to cancel.\n"));
		}
	else
		{
		gu_utf8_printf(ngettext("%d active jobs were canceled.\n", "%d active jobs were canceled.\n", ppop_cancel_active_total), ppop_cancel_active_total);
		}

	return EXIT_OK;
	} /* end of ppop_cancel_active() */

/*========================================================================
** ppop move {destination|job} {destination}
**
** Move a job to a new destination or
** move all jobs from one destination to another.
** We perform these functions with the "m" command.
========================================================================*/
int ppop_move(char *argv[])
	{
	const struct Jobname *job;
	const char *new_destname;
	FILE *FIFO;

	if(!argv[0] || !argv[1])
		{
		fputs(_("Usage: ppop move <job> <destination>\n"
			  "     ppop move <old_destionation> <new_destination>\n\n"
			  "This command moves a job or jobs to a different queue.\n"), errors);
		return EXIT_SYNTAX;
		}

	if(!(job = parse_jobname(argv[0])))
		return EXIT_SYNTAX;

	if(job->id == WILDCARD_JOBID)				/* all jobs */
		{
		if(!assert_am_operator())
			return EXIT_DENIED;
		}
	else if(!job_permission_check(job))			/* one job */
		return EXIT_DENIED;

	if(!(new_destname = parse_destname(argv[1], FALSE)))
		return EXIT_SYNTAX;

	FIFO = get_ready();
	fprintf(FIFO, "m %s %d %d %s\n",
			job->destname, job->id, job->subid,
			new_destname);
	fflush(FIFO);							/* force the buffer contents out */

	wait_for_pprd(TRUE);

	return print_reply();
	} /* end of ppop_move() */

/*===========================================================================
** ppop rush {job}
**
** Move a job to the head of the queue.
**
** ppop last {job}
**
** Move a job to the end of the queue.
===========================================================================*/
int ppop_rush(char *argv[], int newpos)
	{
	int x;
	const struct Jobname *job;
	FILE *FIFO;
	int result = 0;

	if( argv[0]==(char*)NULL )
		{
		if(newpos == 0)
			{
			fputs(_("Usage: ppop rush <job> ...\n\n"
				"This command moves the specified jobs to the head of the queue.\n"), errors);
			}
		else
			{
			fputs(_("Usage: ppop last <job> ...\n\n"
				"This command moves the specified jobs to the end of the queue.\n"), errors);
			}

		return EXIT_SYNTAX;
		}

	if(!assert_am_operator())			/* only operator may rush jobs */
		return EXIT_DENIED;

	for(x=0; argv[x]; x++)
		{
		if(!(job = parse_jobname(argv[x])))
			return EXIT_SYNTAX;

		FIFO = get_ready();
		fprintf(FIFO, "U %s %d %d %d\n", job->destname, job->id, job->subid, newpos);
		fflush(FIFO);
		wait_for_pprd(TRUE);

		if((result = print_reply()))
			break;
		}

	return result;
	} /* end of ppop_rush() */

/*=========================================================================
** ppop hold {job}
**
** Place a specific job on hold.
** We do this by sending pprd the "h" command.
**
** ppop release {job}
**
** Release a previously held or arrested job.
** We do this by sending pprd the "r" command.
=========================================================================*/
int ppop_hold_release(char *argv[], int release)
	{
	int x;
	const struct Jobname *job;
	FILE *FIFO;
	int result = 0;

	if(!argv[0])
		{
		if(! release)
			{
			fputs(_("Usage: ppop hold <job> ...\n\n"
				"This causes jobs to be placed in the held state.  A job\n"
				"which is held will not be printed until it is released.\n"), errors);
			}
		else
			{
			fputs(_("Usage: ppop release <job> ...\n\n"
				"This command releases previously held or arrested jobs.\n"), errors);
			}
		return EXIT_SYNTAX;
		}

	for(x=0; argv[x]; x++)
		{
		if(!(job = parse_jobname(argv[x])))
			return EXIT_SYNTAX;

		if(job->id == WILDCARD_JOBID)
			{
			fputs(_("You must indicate a specific job.\n"), errors);
			return EXIT_SYNTAX;
			}

		if(!job_permission_check(job))
			return EXIT_DENIED;

		FIFO = get_ready();
		fprintf(FIFO, "%c %s %d %d\n", release ? 'r' : 'h',
				job->destname, job->id, job->subid);
		fflush(FIFO);
		wait_for_pprd(TRUE);

		if((result = print_reply()))
			break;
		}

	return result;
	} /* end of ppop_hold_release() */

/*==========================================================================
** ppop accept {destination}
** ppop reject {destination}
**
** Set a printer or group to accept or reject print jobs.
==========================================================================*/
int ppop_accept_reject(char *argv[], int reject)
	{
	const char *destname;
	FILE *FIFO;

	if(!argv[0])
		{
		if(! reject)
			fputs(_("Usage: ppop accept <destionation>\n"), errors);
		else
			fputs(_("Usage: ppop reject <destination>\n"), errors);

		fputs(_("\n\tThis command sets the status of a destination.\n"
				"\tThe status of a destination may be displayed with\n"
				"\tthe \"ppop destination\" command.\n"), errors);

		return EXIT_SYNTAX;
		}

	if(!assert_am_operator())
		return EXIT_DENIED;

	if(!(destname = parse_destname(argv[0], FALSE)))
		return EXIT_SYNTAX;

	FIFO = get_ready();

	if(! reject)
		fprintf(FIFO, "A %s\n", destname);
	else
		fprintf(FIFO, "R %s\n", destname);

	fflush(FIFO);

	wait_for_pprd(TRUE);

	return print_reply();
	} /* end of ppop_accept_reject() */

/*===========================================================================
** ppop dest {destination}
**
** Show the type and status of one or more destinations.
===========================================================================*/
int ppop_destination(char *argv[], int info_level)
	{
	const char function[] = "ppop_destination";
	const char *search_destname;
	FILE *FIFO, *reply_file;
	const char *format;
	int is_group, is_accepting, is_charge;
	char *line = NULL;
	int line_len = 128;
	char *destname, *comment, *interface, *address;

	if(!argv[0])
		{
		fputs(_("Usage: ppop dest[ination] {<destionation>, all}\n"
				"       ppop ldest {<destionation>, all}\n"
				"       ppop dest-comment-address {<destination>, all}\n"
				"\n"
				"\tThis command displays the status of a print destination.  A print\n"
				"\tdestination is a printer or a group of printers.  A destination\n"
				"\tmay be be set to either accept or reject print jobs sent to it.  If\n"
				"\ta print job is rejected, it is canceled and the user is so informed.\n"
				"\n"
				"\tThe \"ppop ldest\" form of this command also displays the comment\n"
				"\tattached to the printer or group.  The \"ppop dest-comment-address\n"
				"\tform displays the command and the interface and address.\n"), errors);

		return EXIT_ERROR;
		}

	if(!(search_destname = parse_destname(argv[0], FALSE)))
		return EXIT_SYNTAX;

	/* Send a request to pprd. */
	FIFO = get_ready();
	fprintf(FIFO, "D %s\n", search_destname);
	fflush(FIFO);

	/* If a human is reading our output, give column names. */
	if(opt_machine_readable)
		{
		switch(info_level)
			{
			case 0:
				format = "%s\t%s\t%s\t%s\n";
				break;
			case 1:
				format = "%s\t%s\t%s\t%s\t%s\n";
				break;
			case 2:
				format = "%s\t%s\t%s\t%s\t%s\t%s %s\n";
				break;
			default:
				format = "missing format\n";
			}
		}
	else
		{
		int rule_width;

		switch(info_level)
			{
			case 0:
				format = "%-16s %-9s %-11s %-9s\n";
				rule_width = 75;
				break;
			case 1:
				format = "%-16s %-9s %-11s %-9s %s\n";
				rule_width = 75;
				break;
			case 2:
				format = "%-16s %-7s %-10s %-9s %-32s %s %s\n";
				rule_width = 131;
				break;
			default:
				format = "missing format\n";
				rule_width = 20;
				break;
			}

		gu_utf8_printf(format, _("Destination"), _("Type"), _("Status"), _("Charge"), _("Comment"), _("Address"), "");
		while(rule_width--)
			putchar('-');
		putchar('\n');
		}

	/* Wait for a reply from pprd. */
	if(!(reply_file = wait_for_pprd(TRUE)))
		return print_reply();			/* if error, print it */

	/* Process all of the responses from pprd. */
	while((line = gu_getline(line, &line_len, reply_file)))
		{
		destname = comment = interface = address = (char*)NULL;
		if(gu_sscanf(line, "%S %d %d %d", &destname, &is_group, &is_accepting, &is_charge) != 4)
			{
			gu_utf8_printf("Malformed response: %s", line);
			gu_free_if(destname);
			continue;
			}

		if(info_level > 0)						/* If we should display the */
			{									/* destination comment */
			char fname[MAX_PPR_PATH];
			FILE *f;

			ppr_fnamef(fname, "%s/%s", is_group ? GRCONF : PRCONF, destname);
			if((f = fopen(fname, "r")))
				{
				char *pconf_line = NULL;
				int pconf_line_len = 128;
				while((pconf_line = gu_getline(pconf_line, &pconf_line_len, f)))
					{
					if(gu_sscanf(pconf_line, "Comment: %T", &comment) == 1)
						continue;
					if(info_level > 1 && !is_group)
						{
						if(gu_sscanf(pconf_line, "Interface: %S", &interface) == 1)
							continue;
						if(gu_sscanf(pconf_line, "Address: %A", &address) == 1)
							continue;
						}
					}
				gu_free_if(pconf_line);
				fclose(f);
				}
			}

		gu_utf8_printf(format,
				destname,
				opt_machine_readable ? (is_group ? "group" : "printer") : (is_group ? _("group") : _("printer")),
				opt_machine_readable ? (is_accepting ? "accepting" : "rejecting") : (is_accepting ? _("accepting") : _("rejecting")),
				opt_machine_readable ? (is_charge ? "yes" : "no") : (is_charge ? _("yes") : _("no")),
				(comment ? comment : ""),
				(interface ? interface : ""),
				(address ? address : "")
				);

		/* Do not remove these */
		gu_free(destname);
		gu_free_if(comment);
		gu_free_if(interface);
		gu_free_if(address);
		} /* end of loop which processes responses from pprd. */

	fclose(reply_file);

	/* We have to do the aliases ourselves. */
	{
	DIR *dir;
	struct dirent *direntp;
	int len;

	if(!(dir = opendir(ALIASCONF)))
		fatal(EXIT_INTERNAL, "%s(): opendir(\"%s\") failed, errno=%d (%s)", function, ALIASCONF, errno, gu_strerror(errno));

	while((direntp = readdir(dir)))
		{
		/* Skip . and .. and hidden files. */
		if(direntp->d_name[0] == '.')
			continue;

		/* Skip Emacs style backup files. */
		len = strlen(direntp->d_name);
		if(len > 0 && direntp->d_name[len-1] == '~')
			continue;

		if(strcmp(search_destname, "all") == 0 || strcmp(search_destname, direntp->d_name) == 0)
			{
			gu_utf8_printf(format,
				direntp->d_name,
				opt_machine_readable ? "alias" : _("alias"),
				"?",			/* accepting */
				"?",			/* charge */
				"comment",
				"",				/* interface */
				""				/* address */
				);
			}
		}

	closedir(dir);
	}

	return EXIT_OK;
	} /* end of ppop_destination() */

/*===========================================================================
** ppop alerts {printer}
**
** Show the alert messages for a specific printer.
===========================================================================*/
int ppop_alerts(char *argv[])
	{
	const char *destname;
	char fname[MAX_PPR_PATH];
	struct stat statbuf;
	FILE *f;
	int c;

	if(! argv[0])
		{
		fputs(_("Usage: ppop alerts _printer_\n"), errors);
		return EXIT_SYNTAX;
		}

	if(!(destname = parse_destname(argv[0], FALSE)))
		return EXIT_SYNTAX;

	/* See if the printer configuration file exists. */
	ppr_fnamef(fname, "%s/%s", PRCONF, destname);
	if(stat(fname, &statbuf))
		{
		gu_utf8_fprintf(errors, _("Printer \"%s\" does not exist.\n"), destname);
		return EXIT_ERROR;
		}

	/* Try to open the alerts file. */
	ppr_fnamef(fname, "%s/%s/alerts", PRINTERS_CACHEDIR, destname);
	if(!(f = fopen(fname, "r")))
		{
		if(errno == ENOENT)
			{
			gu_utf8_fprintf(errors, _("No alerts for printer \"%s\".\n"), destname);
			return EXIT_OK;
			}
		else
			{
			gu_utf8_fprintf(errors, _("Can't open \"%s\", errno=%d (%s).\n"), fname, errno, gu_strerror(errno));
			return EXIT_INTERNAL;
			}
		}

	/* copy the alerts to stdout */
	while((c = fgetc(f)) != EOF)
		putchar(c);

	fclose(f);

	return EXIT_OK;
	} /* end of ppop_alerts() */

/*==========================================================================
** ppop log {job}
**
** Show the log for a specific print job.
==========================================================================*/
int ppop_log(char *argv[])
	{
	const char function[] = "ppop_log";
	const struct Jobname *job;
	int subid;
	char fname[MAX_PPR_PATH];
	struct stat statbuf;
	FILE *f;
	wchar_t wc;

	if(!argv[0])
		{
		fputs(_("Usage: ppop log <job>\n"), errors);
		return EXIT_SYNTAX;
		}

	if(!(job = parse_jobname(argv[0])))
		{
		gu_utf8_fprintf(errors, _("Invalid job id: %s\n"), argv[0]);
		return EXIT_SYNTAX;
		}

	if(job->id == WILDCARD_JOBID)
		{
		fputs(_("You must indicate a specific job.\n"), errors);
		return EXIT_SYNTAX;
		}

	/* For now we will substitute the subid 0 if the subid wasn't specified. */
	if((subid = job->subid) == WILDCARD_SUBID)
		subid = 0;

	/* construct the name of the queue file */
	ppr_fnamef(fname, "%s/%s-%d.%d",
		QUEUEDIR, job->destname, job->id, subid);

	/* make sure the queue file is there */
	if(stat(fname,&statbuf))
		{
		gu_utf8_fprintf(errors, _("Job \"%s\" does not exist.\n"), jobid(job->destname, job->id, subid));
		return EXIT_ERROR;
		}

	/* construct the name of the log file */
	ppr_fnamef(fname, "%s/%s-%d.%d-log", DATADIR, job->destname, job->id, subid);

	/* open the log file */
	if((f = fopen(fname, "r")) == (FILE*)NULL)
		{
		gu_utf8_fprintf(errors, _("There is no log for job \"%s\".\n"), jobid(job->destname, job->id, subid));
		return EXIT_OK;
		}

	/* In machine readable mode we say when the log file was last modified. */
	if(opt_machine_readable)
		{
		struct stat statbuf;
		if(fstat(fileno(f), &statbuf) == -1)
			fatal(EXIT_INTERNAL, "%s(): fstat() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
		gu_utf8_printf("mtime: %ld\n", (long)statbuf.st_mtime);
		}

	/* Copy the log file to stdout. */
	while((wc = gu_utf8_fgetwc(f)) != WEOF)
		gu_fputwc(wc, stdout);

	fclose(f);
	return EXIT_OK;
	} /* end of ppop_log() */

/* end of file */
