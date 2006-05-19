/*
** mouse:~ppr/src/ppop/ppop_cmds_other.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 19 May 2006.
*/

/*
** This module contains code for the ppop subcommands other than those
** which list the queue or modify a job.
<helptopic>
	<name>printers</name>
	<desc>controlling printers</desc>
</helptopic>
<helptopic>
	<name>groups</name>
	<desc>controlling groups of printers</desc>
</helptopic>
<helptopic>
	<name>jobs</name>
	<desc>controlling jobs</desc>
</helptopic>
<helptopic>
	<name>media</name>
	<desc>mounting media</desc>
</helptopic>
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
#include "dispatch_table.h"

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

/*
<command>
	<name><word>status</word></name>
	<desc>display the status of printers</desc>
	<args>
		<arg><name>printers</name><desc>printer to show or \"all\"</desc></arg>
	</args>
</command>
*/
int command_status(const char *argv[])
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

/*
** ppop message {printer}
** Retrieve the auxiliary status messages from a certain printer.
<command>
	<name><word>message</word></name>
	<desc>display display auxiliary status of printer</desc>
	<args>
		<arg><name>printers</name><desc>printer to show</desc></arg>
	</args>
</command>
*/
int command_message(const char *argv[])
	{
	const char *destname;
	char fname[MAX_PPR_PATH];
	FILE *statfile;

	if(!(destname = parse_destname(argv[0], FALSE)))
		return EXIT_SYNTAX;

	ppr_fnamef(fname, "%s/%s/device_status", PRINTERS_PURGABLE_STATEDIR, destname);
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

/*
<command helptopics="media,printers">
	<name><word>media</word></name>
	<desc>show the media mounted on specificified printers</desc>
	<args>
		<arg><name>destination</name><desc>printer or group to show or \"all\"</desc></arg>
	</args>
</command>
*/
int command_media(const char *argv[])
	{
	const char function[] = "ppop_media";
	int retcode;
	const char *destname;
	FILE *FIFO, *reply_file;

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
	char *line = NULL;
	int line_space = 80;
	for(retcode = EXIT_OK; retcode == EXIT_OK && (line = gu_getline(line, &line_space, reply_file)); )
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
		gu_utf8_fprintf(stderr, "%s(): bad response from pprd.\n", function);

	return retcode;
	} /* end of ppop_media() */

/*
<command acl="ppop" helptopics="media,printers">
	<name><word>mount</word></name>
	<desc>mount specified medium on specified tray of specified printer</desc>
	<args>
		<arg><name>printer</name><desc>printer on which to mount <arg>medium</arg></desc></arg>
		<arg><name>tray</name><desc>tray of <arg>printer</arg> on which to mount <arg>medium</arg></desc></arg>
		<arg><name>medium</name><desc>medium to mount on <arg>tray</arg> or <arg>printer</arg></desc></arg>
	</args>
</command>
*/
int command_mount(const char *argv[])
	{
	int retcode = EXIT_OK;
	const char *destname, *binname, *mediumname;
	FILE *FIFO;

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
				gu_utf8_fprintf(stderr, _("Can't open \"%s\", errno=%d (%s)."), MEDIAFILE, errno, gu_strerror(errno));
				retcode = EXIT_INTERNAL;
				break;
				}
	
			do	{
				if(fread(&ms,sizeof(struct Media), 1, mfile) == 0)
					{
					gu_utf8_fprintf(stderr, _("Medium \"%s\" is unknown.\n"), mediumname);
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

static int ppop_cancel_byuser_item(
		int rank,
		const struct QEntry *qentry,
		const struct QEntryFile *qentryfile,
		const char *onprinter,
		FILE *qstream
		)
	{
	FILE *FIFO, *reply_file;
	int count;

	if( ! is_my_job(qentry, qentryfile) )
		return FALSE;	/* don't stop */

	FIFO = get_ready();
	fprintf(FIFO, "c %s %d %d %d\n", qentryfile->jobname.destname, qentryfile->jobname.id, qentryfile->jobname.subid, ppop_cancel_byuser_inform);
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
static int ppop_cancel_byuser(const char *destname, int inform)
	{
	const char *list[2];
	int ret;

	ppop_cancel_byuser_inform = inform;
	ppop_cancel_byuser_total = 0;

	list[0] = destname;			/* custom_list() wants an argument list, */
	list[1] = (char*)NULL;		/* construct one. */

	if((ret = custom_list(list, NULL, ppop_cancel_byuser_item, FALSE, -1)) )
		return ret;

	say_canceled(ppop_cancel_byuser_total, TRUE);

	return EXIT_OK;
	} /* end of ppop_cancel_byuser() */

static int ppop_cancel(const char *argv[], int inform)
	{
	int x;
	const struct Jobname *job;
	FILE *FIFO, *reply_file;
	int count;

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

/*
<command helptopics="jobs">
	<name><word>cancel</word></name>
	<desc>cancel print jobs and inform user</desc>
	<args>
		<arg flags="repeat"><name>what</name><desc>job to be canceled or queue to purge of user's jobs</desc></arg>
	</args>
</command>
*/
int command_cancel(const char *argv[])
	{
	return ppop_cancel(argv, 1);
	}

/*
<command helptopics="jobs">
	<name><word>scancel</word></name>
	<desc>cancel print jobs but don't inform user</desc>
	<args>
		<arg flags="repeat"><name>what</name><desc>job to be canceled or queue to purge of user's jobs</desc></arg>
	</args>
</command>
*/
int command_scancel(const char *argv[])
	{
	return ppop_cancel(argv, 0);
	}

/*========================================================================
** ppop purge _destination_
**
** Cancel all jobs on a destination.  Only an operator may do this.
** We do it with the "c"ancel command and -1 for the job id and subid.
========================================================================*/
static int ppop_purge(const char *argv[], int inform)
	{
	int x;
	const char *destname;
	FILE *FIFO, *reply_file;
	int count;

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

/*
<command acl="ppop" helptopics="jobs,printers,groups">
	<name><word>purge</word></name>
	<desc>cancel all print jobs from the indicated queues</desc>
	<args>
		<arg flags="repeat"><name>destionation</name><desc>queue to be purged</desc></arg>
	</args>
</command>
*/
int command_purge(const char *argv[])
	{
	return ppop_purge(argv, 1);
	}

/*
<command acl="ppop" helptopics="jobs,printers,groups">
	<name><word>spurge</word></name>
	<desc>cancel all print jobs from the indicated queues but don't inform users</desc>
	<args>
		<arg flags="repeat"><name>destionation</name><desc>queue to be purged</desc></arg>
	</args>
</command>
*/
int command_spurge(const char *argv[])
	{
	return ppop_purge(argv, 0);
	}

/*=======================================================================
** ppop clean _destination_
** Delete all the arrested jobs on a destination.
=======================================================================*/
static int ppop_clean_total;

static int ppop_clean_item(
		int rank,
		const struct QEntry *qentry,
		const struct QEntryFile *qentryfile,
		const char *onprinter,
		FILE *qstream
		)
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
	fprintf(FIFO, "c %s %d %d 1\n", qentryfile->jobname.destname, qentryfile->jobname.id, qentryfile->jobname.subid);
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

/*
<command acl="ppop" helptopics="jobs,printers,groups">
	<name><word>clean</word></name>
	<desc>remove arrested jobs from queues</desc>
	<args>
		<arg flags="repeat"><name>destionation</name><desc>queue to be cleaned</desc></arg>
	</args>
</command>
*/
int command_clean(const char *argv[])
	{
	int ret;
	ppop_clean_total = 0;
	if((ret = custom_list(argv, NULL, ppop_clean_item, FALSE, -1)))
		return ret;
	say_canceled(ppop_clean_total, FALSE);
	return EXIT_OK;
	} /* end of command_clean() */

/*=======================================================================
** ppop cancel-active <destination>
** ppop cancel-my-active <destination>
** Delete the active job for the destination.
=======================================================================*/
static int ppop_cancel_active_total;
static int ppop_cancel_active_my;
static int ppop_cancel_active_inform;

static int ppop_cancel_active_item(
		int rank,
		const struct QEntry *qentry,
		const struct QEntryFile *qentryfile,
		const char *onprinter,
		FILE *qstream
		)
	{
	FILE *FIFO, *reply_file;
	int count;

	if(qentry->status < 0)
		return FALSE;

	if(ppop_cancel_active_my && !is_my_job(qentry, qentryfile))
		return TRUE;

	FIFO = get_ready();
	fprintf(FIFO, "c %s %d %d %d\n", qentryfile->jobname.destname, qentryfile->jobname.id, qentryfile->jobname.subid, ppop_cancel_active_inform);
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

static int ppop_cancel_active(const char *argv[], int my, int inform)
	{
	int ret;

	ppop_cancel_active_my = my;
	ppop_cancel_active_inform = inform;
	ppop_cancel_active_total = 0;

	if((ret = custom_list(argv, NULL, ppop_cancel_active_item, FALSE, -1)))
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

/*
<command acl="ppop" helptopics="jobs">
	<name><word>cancel-active</word></name>
	<desc>cancel any jobs that are being printed from indicated queues</desc>
	<args>
		<arg flags="repeat"><name>destionation</name><desc>queue from which to cancel active job</desc></arg>
	</args>
</command>
*/
int command_cancel_active(const char *argv[])
	{
	return ppop_cancel_active(argv, FALSE, 1);
	}

/*
<command acl="ppop" helptopics="jobs">
	<name><word>scancel-active</word></name>
	<desc>silently cancel any jobs that are being printed from indicated queues</desc>
	<args>
		<arg flags="repeat"><name>destionation</name><desc>queue from which to cancel active job</desc></arg>
	</args>
</command>
*/
int command_scancel_active(const char *argv[])
	{
	return ppop_cancel_active(argv, FALSE, 0);
	}

/*
<command helptopics="jobs">
	<name><word>cancel-my-active</word></name>
	<desc>cancel any of user's jobs that are being printed from indicated queues</desc>
	<args>
		<arg flags="repeat"><name>destionation</name><desc>queue from which to cancel active user's active job</desc></arg>
	</args>
</command>
*/
int command_cancel_my_active(const char *argv[])
	{
	return ppop_cancel_active(argv, TRUE, 1);
	}

/*
<command helptopics="jobs">
	<name><word>scancel-my-active</word></name>
	<desc>cancel any of user's jobs that are being printed from indicated queues</desc>
	<args>
		<arg flags="repeat"><name>destionation</name><desc>queue from which to cancel user's active job</desc></arg>
	</args>
</command>
*/
int command_scancel_my_active(const char *argv[])
	{
	return ppop_cancel_active(argv, TRUE, 0);
	}

/*
<command helptopics="jobs">
	<name><word>move</word></name>
	<desc>move a jobs or all jobs on a given destionation to a new destination</desc>
	<args>
		<arg><name>from</name><desc>job to move or queue from which to remove</desc></arg>
		<arg><name>to</name><desc>destionation to which to move the job or jobs</desc></arg>
	</args>
</command>
*/
int command_move(const char *argv[])
	{
	const struct Jobname *job;
	const char *new_destname;
	FILE *FIFO;

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
	} /* end of command_move() */

/*===========================================================================
 * Change a job's position in the queue.
===========================================================================*/
static int ppop_rush(const char *argv[], int newpos)
	{
	int x;
	const struct Jobname *job;
	FILE *FIFO;
	int result = 0;

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

/*
<command acl="ppop" helptopics="jobs">
	<name><word>rush</word></name>
	<desc>move a job to the head of the queue</desc>
	<args>
		<arg flags="repeat"><name>job</name><desc>job to move to head of queue</desc></arg>
	</args>
</command>
*/
int command_rush(const char *argv[])
	{
	return ppop_rush(argv, 0);
	}

/*
<command acl="ppop" helptopics="jobs">
	<name><word>last</word></name>
	<desc>move a job to the end of the queue</desc>
	<args>
		<arg flags="repeat"><name>job</name><desc>job to move to end of queue</desc></arg>
	</args>
</command>
*/
int command_last(const char *argv[])
	{
	return ppop_rush(argv, 10000);
	}

/*=========================================================================
** ppop hold {job}
** ppop release {job}
=========================================================================*/
static int ppop_hold_release(const char *argv[], int release)
	{
	int x;
	const struct Jobname *job;
	FILE *FIFO;
	int result = 0;

	for(x=0; argv[x]; x++)
		{
		if(!(job = parse_jobname(argv[x])))
			return EXIT_SYNTAX;

		if(job->id == WILDCARD_JOBID)
			{
			gu_utf8_fputs(_("You must indicate a specific job.\n"), stderr);
			return EXIT_SYNTAX;
			}

		if(!job_permission_check(job))
			return EXIT_DENIED;

		FIFO = get_ready();
		fprintf(FIFO, "%c %s %d %d\n",
				release ? 'r' : 'h',
				job->destname, job->id, job->subid
				);
		fflush(FIFO);
		wait_for_pprd(TRUE);

		if((result = print_reply()))
			break;
		}

	return result;
	} /* end of ppop_hold_release() */

/*
<command helptopics="jobs">
	<name><word>hold</word></name>
	<desc>place a specific job on hold</desc>
	<args>
		<arg flags="repeat"><name>job</name><desc>job to hold</desc></arg>
	</args>
</command>
*/
int command_hold(const char *argv[])
	{
	return ppop_hold_release(argv, 0);
	}

/*
<command helptopics="jobs">
	<name><word>release</word></name>
	<desc>release a job that is held or arrested</desc>
	<args>
		<arg flags="repeat"><name>job</name><desc>job to release</desc></arg>
	</args>
</command>
*/
int command_release(const char *argv[])
	{
	return ppop_hold_release(argv, 1);
	}

/*==========================================================================
** This is a bunch of simple commands which operate on print destinations.
** They all work by sending a command to pprd, so we have gathered the
** real work into a function.
==========================================================================*/
static int ppop_destination_command(const char *argv[], int command)
	{
	int x;
	const char *destname;
	int result = 0;
	FILE *FIFO;
	for(x=0; argv[x]; x++)
		{
		if(!(destname = parse_destname(argv[x], FALSE)))
			return EXIT_SYNTAX;
		FIFO = get_ready();
		fprintf(FIFO, "%c %s\n", command, destname);
		fflush(FIFO);
		wait_for_pprd(TRUE);
		if((result = print_reply()))
			break;
		}
	return result;
	} /* end of ppop_destination_command() */

/*
<command acl="ppop" helptopics="printers">
	<name><word>start</word></name>
	<desc>start printers which were stopped or faulted</desc>
	<args>
		<arg flags="repeat"><name>printer</name><desc>printer to be started</desc></arg>
	</args>
</command>
*/
int command_start(const char *argv[])
	{
	return ppop_destination_command(argv, 't');
	} /* end of command_start() */

/*
<command acl="ppop" helptopics="printers">
	<name><word>stop</word></name>
	<desc>stop printers at end of next job</desc>
	<args>
		<arg flags="repeat"><name>printer</name><desc>printer to be stopt</desc></arg>
	</args>
</command>
*/
int command_stop(const char *argv[])
	{
	return ppop_destination_command(argv, 'p');
	} /* end of command_stop() */

/*
<command acl="ppop" helptopics="printers">
	<name><word>wstop</word></name>
	<desc>stop printers gently, wait until stopt</desc>
	<args>
		<arg flags="repeat"><name>printer</name><desc>printer to be stopt</desc></arg>
	</args>
</command>
*/
int command_wstop(const char *argv[])
	{
	return ppop_destination_command(argv, 'P');
	} /* end of command_wstop() */

/*
<command acl="ppop" helptopics="printers">
	<name><word>halt</word></name>
	<desc>stop printers immediately</desc>
	<args>
		<arg flags="repeat"><name>printer</name><desc>printer to be stopt</desc></arg>
	</args>
</command>
*/
int command_halt(const char *argv[])
	{
	return ppop_destination_command(argv, 'b');
	} /* end of command_halt() */

/*
<command acl="ppop" helptopics="printers,groups">
	<name><word>accept</word></name>
	<desc>allow a destination to accept new jobs</desc>
	<args>
		<arg flags="repeat"><name>destination</name><desc>name of printer or group or alias for same</desc></arg>
	</args>
</command>
*/
int command_accept(const char *argv[])
	{
	return ppop_destination_command(argv, 'A');
	}

/*
<command acl="ppop" helptopics="printers,groups">
	<name><word>reject</word></name>
	<desc>forbid a destination to accept new jobs</desc>
	<args>
		<arg flags="repeat"><name>destination</name><desc>name of printer or group or alias for same</desc></arg>
	</args>
</command>
*/
int command_reject(const char *argv[])
	{
	return ppop_destination_command(argv, 'R');
	}

/*===========================================================================
** ppop dest {destination}
** Show the type and status of one or more destinations.
===========================================================================*/
static int ppop_destination(const char *argv[], int info_level)
	{
	const char function[] = "ppop_destination";
	int x;
	const char *search_destname;
	FILE *FIFO, *reply_file;
	const char *format;
	int is_group, is_accepting, is_charge;
	char *line = NULL;
	int line_len = 128;
	char *destname, *comment, *interface, *address;

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

		gu_utf8_printf(format,
			_("Destination"),
			_("Type"),
			_("Status"),
			_("Charge"),
			_("Comment"),
			_("Address"),
			""
			);
		while(rule_width--)
			putchar('-');
		putchar('\n');
		}

	for(x=0; argv[x]; x++)
		{
		if(!(search_destname = parse_destname(argv[x], FALSE)))
			return EXIT_SYNTAX;

		/* Send a request to pprd. */
		FIFO = get_ready();
		fprintf(FIFO, "D %s\n", search_destname);
		fflush(FIFO);
	
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
		}

	/* We have to do the aliases ourselves. */
	if(strcmp(argv[0], "all") == 0)
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
	
		closedir(dir);
		}

	return EXIT_OK;
	} /* end of ppop_destination() */

/*
<command helptopics="printers,groups">
	<name><word>destination</word></name>
	<desc>show the type and status of one or more print destinations</desc>
	<args>
		<arg flags="repeat"><name>destination</name><desc>name of printer or group or alias for same</desc></arg>
	</args>
</command>
*/
int command_destination(const char *argv[])
	{
	return ppop_destination(argv, 0);
	}

/*
<command helptopics="printers,groups">
	<name><word>dest</word></name>
	<desc>abbreviation for ppop destination</desc>
	<args>
		<arg flags="repeat"><name>destination</name><desc>name of printer or group or alias for same</desc></arg>
	</args>
</command>
*/
int command_dest(const char *argv[])
	{
	return ppop_destination(argv, 0);
	}

/*
<command helptopics="printers,groups">
	<name><word>destination-comment</word></name>
	<desc>show the type, status, and comment of one or more print destinations</desc>
	<args>
		<arg flags="repeat"><name>destination</name><desc>name of printer or group or alias for same</desc></arg>
	</args>
</command>
*/
int command_destination_comment(const char *argv[])
	{
	return ppop_destination(argv, 1);
	}

/*
<command helptopics="printers,groups">
	<name><word>ldest</word></name>
	<desc>abbreviation for ppop destination-comment</desc>
	<args>
		<arg flags="repeat"><name>destination</name><desc>name of printer or group or alias for same</desc></arg>
	</args>
</command>
*/
int command_ldest(const char *argv[])
	{
	return ppop_destination(argv, 1);
	}

/*
<command helptopics="printers,groups">
	<name><word>destination-comment-address</word></name>
	<desc>show the type, status, comment, and address of one or more print destinations</desc>
	<args>
		<arg flags="repeat"><name>destination</name><desc>name of printer or group or alias for same</desc></arg>
	</args>
</command>
*/
int command_destination_comment_address(const char *argv[])
	{
	return ppop_destination(argv, 1);
	}

/*
** ppop alerts {printer}
** Show the alert messages for a specific printer.
<command helptopics="printers">
	<name><word>alerts</word></name>
	<desc>show alert messages for a specific printer</desc>
	<args>
		<arg><name>printer</name><desc>name of printer to show</desc></arg>
	</args>
</command>
*/
int command_alerts(const char *argv[])
	{
	const char *destname;
	char fname[MAX_PPR_PATH];
	struct stat statbuf;
	FILE *f;
	int c;

	if(!(destname = parse_destname(argv[0], FALSE)))
		return EXIT_SYNTAX;

	/* See if the printer configuration file exists. */
	ppr_fnamef(fname, "%s/%s", PRCONF, destname);
	if(stat(fname, &statbuf))
		{
		gu_utf8_fprintf(stderr, _("Printer \"%s\" does not exist.\n"), destname);
		return EXIT_ERROR;
		}

	/* Try to open the alerts file. */
	ppr_fnamef(fname, "%s/%s/alerts", PRINTERS_PURGABLE_STATEDIR, destname);
	if(!(f = fopen(fname, "r")))
		{
		if(errno == ENOENT)
			{
			gu_utf8_fprintf(stderr, _("No alerts for printer \"%s\".\n"), destname);
			return EXIT_OK;
			}
		else
			{
			gu_utf8_fprintf(stderr, _("Can't open \"%s\", errno=%d (%s).\n"), fname, errno, gu_strerror(errno));
			return EXIT_INTERNAL;
			}
		}

	/* copy the alerts to stdout */
	while((c = fgetc(f)) != EOF)
		putchar(c);

	fclose(f);

	return EXIT_OK;
	} /* end of ppop_alerts() */

/*
<command helptopics="jobs">
	<name><word>log</word></name>
	<desc>show the log for a specific print job</desc>
	<args>
		<arg><name>job</name><desc>jobid of job to show</desc></arg>
	</args>
</command>
*/
int command_log(const char *argv[])
	{
	const char function[] = "ppop_log";
	const struct Jobname *job;
	int subid;
	char fname[MAX_PPR_PATH];
	struct stat statbuf;
	FILE *f;
	wchar_t wc;

	if(!(job = parse_jobname(argv[0])))
		{
		gu_utf8_fprintf(stderr, _("Invalid job id: %s\n"), argv[0]);
		return EXIT_SYNTAX;
		}

	if(job->id == WILDCARD_JOBID)
		{
		gu_utf8_fputs(_("You must indicate a specific job.\n"), stderr);
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
		gu_utf8_fprintf(stderr, _("Job \"%s\" does not exist.\n"), jobid(job->destname, job->id, subid));
		return EXIT_ERROR;
		}

	/* construct the name of the log file */
	ppr_fnamef(fname, "%s/%s-%d.%d-log", DATADIR, job->destname, job->id, subid);

	/* open the log file */
	if((f = fopen(fname, "r")) == (FILE*)NULL)
		{
		gu_utf8_fprintf(stderr, _("There is no log for job \"%s\".\n"), jobid(job->destname, job->id, subid));
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
