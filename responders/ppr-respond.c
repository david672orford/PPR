/*
** mouse:~ppr/src/pprd/ppr-respond.c
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
** Last modified 31 March 2005.
*/

/*
** This program is called by ppr, pprd, and pprdrv when they want to send a
** message to a user or system administrator.
**
** This program receives most of its input as command-line parameters in the
** form name=value.  Most of them are passed on the the responder program
** unmodified.  The principal task of this program is to create the additional
** localized parameters subject=, short_message=, and long_message=.
**
** Since the program does its job and then exits, it blithely allocates
** memory without freeing it.
*/

#include "config.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "pprd.h"
#include "respond.h"

struct RESPONSE_INFO
	{
	struct QEntryFile qentry;	/* contents of queue file */
	char *job;					/* destname-id of job */
	char *destination;
	char *printer;
	int response_code;			/* what happened to the job? */
	char *extra;				/* extra parameter supplementing response_code */
	char *commentary_cooked;
	char *commentary_raw1;
	char *commentary_raw2;
	int commentary_duration;
	int commentary_severity;
	int commentary_seq_number;
	int pages;
	int charge_per_duplex;
	int charge_per_simplex;
	struct COMPUTED_CHARGE charge;
	char *reason;
	int commentary_duration_threshold;
	int commentary_severity_threshold;
	} ;

/*
 * We call this function when we want to print a message expressing a
 * period of elapsed time in days, hours, and minutes.
 *
 * It is not yet clear if this will do for all languages.  The Gettext
 * plural mechanisms doesn't handle two numbers in the same message.
 */
static char *elapsed_time_description(int elapsed_time)
	{
	int days, hours, minutes;
	char *message = NULL;
	char *part2;

	minutes = elapsed_time % 60;
	elapsed_time /= 60;
	hours = elapsed_time % 24;
	days = elapsed_time / 60;
						
	if(days >= 1)
		{
		if(minutes >= 30)	/* round off hours */
			hours++;
		if(hours == 24)		/* handle carry */
			{
			days++;
			hours = 0;
			}
		gu_asprintf(&part2, ngettext("%d hour", "%d hours", hours), hours);
		/* Translators: %s is replaced with "%d minutes" */
		gu_asprintf(&message, ngettext("%d day %s", "%d days %s", days), days, part2);
		gu_free(part2);
		}
	else if(hours >= 1)
		{
		gu_asprintf(&part2, ngettext("%d minute", "%d minutes", minutes), minutes);
		/* Translators: %s is replaced with "%d minutes" */
		gu_asprintf(&message, ngettext("%d hour %s", "%d hours %s", hours), hours, part2);
		gu_free(part2);
		}
	else
		{
		gu_asprintf(&message, ngettext("%d minute", "%d minutes", minutes), minutes);
		}

	return message;
	}

static char *build_subject(struct RESPONSE_INFO *rinfo)
	{
	void *message;
	const char *title;

	message = gu_pcs_new_cstr("subject=");
	
	if(!(title = rinfo->qentry.Title))
		title = _("untitled");

	switch(rinfo->response_code)
		{
		case RESP_FINISHED:
			gu_pcs_append_sprintf(&message,
				_("%s (%s) printed on %s"),
				rinfo->job, title, rinfo->printer);
			break;

		case RESP_ARRESTED:
			gu_pcs_append_sprintf(&message,
				_("%s (%s) arrested while printing on %s"),
				rinfo->job, title, rinfo->printer);
			break;

		case RESP_CANCELED:
			gu_pcs_append_sprintf(&message,
				_("%s (%s) canceled"),
				rinfo->job, title);
			break;

		case RESP_CANCELED_PRINTING:
			gu_pcs_append_sprintf(&message,
				_("%s (%s) canceled while printing on %s"),
				rinfo->job, title, rinfo->printer);
			break;

		case RESP_CANCELED_BADDEST:
			gu_pcs_append_sprintf(&message,
				_("%s (%s) canceled, destination unknown"),
				rinfo->job, title);
			break;

		case RESP_CANCELED_REJECTING:
			gu_pcs_append_sprintf(&message,
				_("%s (%s) rejected: %s set to reject"),
				rinfo->job, title, rinfo->destination);
			break;

		case RESP_STRANDED_PRINTER_INCAPABLE:
			gu_pcs_append_sprintf(&message,
				_("%s (%s) stranded: %s can't print it"),
				rinfo->job, title, rinfo->printer);
			break;

		case RESP_STRANDED_GROUP_INCAPABLE:
			gu_pcs_append_sprintf(&message,
				_("%s (%s) stranded: no member of %s can print it"),
				rinfo->job, title, rinfo->destination);
			break;

		case RESP_CANCELED_NOCHARGEACCT:
			gu_pcs_append_sprintf(&message,
				_("job for %s rejected: %s doesn't have a charge account"),
				rinfo->destination, rinfo->extra);
			break;

		case RESP_CANCELED_OVERDRAWN:
			gu_pcs_append_sprintf(&message,
				_("job for %s rejected: account %s overdrawn"),
				rinfo->destination, rinfo->extra);
			break;

		case RESP_CANCELED_NONCONFORMING:
			gu_pcs_append_sprintf(&message,
				_("job for %s rejected: no page demarcation"),
				rinfo->destination);
			break;

		case RESP_NOFILTER:
			gu_pcs_append_sprintf(&message,
				_("job for %s rejected: no filter for %s"),
				rinfo->destination, rinfo->extra);
			break;

		case RESP_FATAL:
			gu_pcs_append_sprintf(&message,
				_("job for %s rejected: %s"),
				rinfo->destination, rinfo->extra);
			break;

		case RESP_NOSPOOLER:
			gu_pcs_append_sprintf(&message,
				_("job for %s: pprd not running"),
				rinfo->destination);
			break;

		case RESP_BADMEDIA:
			gu_pcs_append_sprintf(&message,
				_("job for %s rejected: unrecognized medium"),
				rinfo->destination);
			break;

		case RESP_BADPJLLANG:
			gu_pcs_append_sprintf(&message,
				_("job for %s rejected: PDL %s not recognized"),
				rinfo->destination, rinfo->extra);
			break;

		case RESP_FATAL_SYNTAX:
			gu_pcs_append_sprintf(&message,
				_("job for %s rejected: %s"),
				rinfo->destination, rinfo->extra);
			break;

		case RESP_CANCELED_NOPAGES:
			gu_pcs_append_sprintf(&message,
				_("job for %s rejected: can't select pages from this job"), 
				rinfo->destination);
			break;

		case RESP_CANCELED_ACL:
			gu_pcs_append_sprintf(&message,
				_("job for %s rejected: ACL does not allow %s access"),
				rinfo->destination, rinfo->extra);
			break;

		case RESP_TYPE_COMMENTARY | COM_PRINTER_ERROR:
		case RESP_TYPE_COMMENTARY | COM_PRINTER_STATUS:
			gu_pcs_append_sprintf(&message,
				_("status of %s: %s while printing %s (%s)"),
				rinfo->printer, gettext(rinfo->commentary_cooked), rinfo->job, title);	
			break;

		case RESP_TYPE_COMMENTARY | COM_STALL:
			gu_pcs_append_sprintf(&message,
				_("problem on %s: %s while printing %s (%s)\n"),
				rinfo->printer, gettext(rinfo->commentary_cooked), rinfo->job, title);	
			break;

		case RESP_TYPE_COMMENTARY | COM_EXIT:
			gu_pcs_append_sprintf(&message,
				_("%s: %s while printing %s (%s)"),
				rinfo->printer, gettext(rinfo->commentary_cooked), rinfo->job, title);	
			break;

		default:
			gu_pcs_append_sprintf(&message,
				_("undefined response code %d while printing %s (%s)"),
				rinfo->response_code, rinfo->job, title);
			break;
		}

	return gu_pcs_free_keep_cstr(&message);
	} /* build_subject() */

static char *build_message(struct RESPONSE_INFO *rinfo, gu_boolean long_format)
	{
	void *message;
	const char *title;

	message = gu_pcs_new_cstr(long_format ? "long_message=" : "short_message=");

	if(!(title = rinfo->qentry.Title))
		title = _("untitled");
	
	switch(rinfo->response_code)
		{
		case RESP_FINISHED:
			gu_pcs_append_sprintf(&message,
				_("Your print job %s (%s) has been printed on the printer %s."),
				rinfo->job, title, rinfo->printer);
			if(long_format)
				{
				if(rinfo->pages >= 0)
					{
					gu_pcs_append_char(&message, ' ');
					gu_pcs_append_sprintf(&message,
						ngettext(
							"The job is %d page long.",
							"The job is %d pages long.",
							rinfo->pages
							),
						rinfo->pages
						);
					}
				if(rinfo->qentry.opts.copies > 1)
					{
					gu_pcs_append_char(&message, ' ');
					gu_pcs_append_sprintf(&message,
						_("%d copies were printed."),
						rinfo->qentry.opts.copies
						);
					}
				if(rinfo->charge_per_duplex > 0 || rinfo->charge_per_simplex > 0)
					{
					gu_pcs_append_char(&message, ' ');
					gu_pcs_append_sprintf(&message,
						_("The charge for printing this job is %s."),
						money(rinfo->charge.total)
						);
					}
				}
			break;

		case RESP_ARRESTED:
			gu_pcs_append_sprintf(&message,
				_("Your print job %s (%s) was arrested after an attempt\n"
				"to print it on %s resulted in a job error."),
				rinfo->job, title, rinfo->printer);
			break;

		case RESP_CANCELED:
			gu_pcs_append_sprintf(&message,
				_("Your print job %s (%s) has been canceled."),
				rinfo->job, title);
			break;

		case RESP_CANCELED_PRINTING:
			gu_pcs_append_sprintf(&message,
				_("Your print job %s (%s) was canceled while printing on %s."),
				rinfo->job, title, rinfo->printer);
			break;

		case RESP_CANCELED_BADDEST:
			gu_pcs_append_sprintf(&message,
				_("Your print job %s (%s) was canceled because\n"
				"\"%s\" is not a known destination."),
				rinfo->job, title, rinfo->destination);
			break;

		case RESP_CANCELED_REJECTING:
			gu_pcs_append_sprintf(&message,
				_("Your print job %s (%s) was canceled because\n"
				"the destination %s is not acceping requests."),
				rinfo->job, title, rinfo->destination);
			break;

		case RESP_STRANDED_PRINTER_INCAPABLE:
			gu_pcs_append_sprintf(&message,
				_("Your print job %s (%s) is stranded because\n"
				"the printer %s is incapable of printing it."),
				rinfo->job, title, rinfo->printer);
			break;

		case RESP_STRANDED_GROUP_INCAPABLE:
			gu_pcs_append_sprintf(&message,
				_("Your print job %s (%s) is stranded because no\n"
				"member of the group %s is capable of printing it."),
				rinfo->job, title, rinfo->destination);
			break;

		case RESP_CANCELED_NOCHARGEACCT:
			gu_pcs_append_sprintf(&message,
				_("The print job which you just now sent to %s was rejected because\n"
				"the user %s does not have a charge account."),
				rinfo->destination, rinfo->extra);
			break;

		case RESP_CANCELED_OVERDRAWN:
			gu_pcs_append_sprintf(&message,
				_("The print job which you just now sent to %s was rejected because\n"
				"the account %s is overdrawn."),
				rinfo->job, rinfo->extra);
			break;

		case RESP_CANCELED_NONCONFORMING:
			gu_pcs_append_sprintf(&message,
				_("The print job which you just now sent to %s was rejected because\n"
				"it does not contain page boundary markers."),
				rinfo->destination);
			break;

		case RESP_NOFILTER:
			gu_pcs_append_sprintf(&message,
				_("The print job which you just now sent to %s was rejected because\n"
				"no filter is available which can convert %s to PostScript."),
				rinfo->destination, rinfo->extra);
			break;

		case RESP_FATAL:
			gu_pcs_append_sprintf(&message,
				_("The print job which you just now sent to %s was rejected because\n"
				"an error occured: %s."),
				rinfo->destination, rinfo->extra);
			break;

		case RESP_NOSPOOLER:
			gu_pcs_append_sprintf(&message,
				_("The print job which you just now sent to %s was lost because pprd is not running."),
				rinfo->destination);
			break;

		case RESP_BADMEDIA:
			gu_pcs_append_sprintf(&message,
				_("The print job which you just now sent to %s was rejected because\n"
				"it requires a medium (generally paper) of an unrecognized size or type."),
				rinfo->destination);
			break;

		case RESP_BADPJLLANG:
			gu_pcs_append_sprintf(&message,
				_("The print job which you just now sent to %s was rejected because\n"
				"the PJL header requests an unrecognized printer language called \"%s\"."),
				rinfo->destination, rinfo->extra);
			break;

		case RESP_FATAL_SYNTAX:
			gu_pcs_append_sprintf(&message,
				_("The print job which you just now sent to %s was rejected because\n"
				"the ppr command line contains an error:\n"
				"\n"
				"%s."),
				rinfo->destination, rinfo->extra);
			break;

		case RESP_CANCELED_NOPAGES:
			gu_pcs_append_sprintf(&message,
				_("The print job which you just now sent to %s was rejected because\n"
				"you requested printing of only selected pages but the job does not\n"
				"contain page-boundary markers."),
				rinfo->job);
			break;

		case RESP_CANCELED_ACL:
			gu_pcs_append_sprintf(&message,
				_("The print job which you just now sent to %s was rejected because\n"
				"the access control list for %s does not grant %s access."),
				rinfo->destination, rinfo->destination, rinfo->extra);
			break;

		case RESP_TYPE_COMMENTARY | COM_PRINTER_ERROR:
		case RESP_TYPE_COMMENTARY | COM_PRINTER_STATUS:
			if(rinfo->commentary_seq_number == 1 && rinfo->commentary_severity > 5)
				{
				gu_pcs_append_sprintf(&message,
					_("The printer %s which could print your job %s (%s) cannot at present\n"
					"due to the error condition \"%s\"."),
					rinfo->printer, rinfo->job, title, gettext(rinfo->commentary_cooked));
				}
			else
				{
				gu_pcs_append_sprintf(&message,
					_("The printer %s which is printing your job %s (%s) reports error\n"
					"\"%s\"."),
					rinfo->printer, rinfo->job, title, gettext(rinfo->commentary_cooked));
				if(long_format)
					{
					gu_pcs_append_char(&message, ' ');
					if(rinfo->commentary_severity >= 7)
						gu_pcs_append_cstr(&message, _("Printing has probably come to a halt."));
					else
						gu_pcs_append_cstr(&message, _("It is advisable to correct this condition before it worsens."));

					if(rinfo->commentary_duration >= 300)	/* if longer than 5 minutes */
						{
						char *temp = elapsed_time_description(rinfo->commentary_duration / 60);
						gu_pcs_append_sprintf(&message, _("This condition has persisted for %s."), temp);
						gu_free(temp);
						}
					}
				}
			break;

		case RESP_TYPE_COMMENTARY | COM_STALL:
			if(strcmp(rinfo->commentary_cooked, "STALLED") == 0)
				{
				switch(rinfo->commentary_duration / 60)
					{
					case 1:
					case 2:
						gu_pcs_append_sprintf(&message,
							_("The printer %s which is printing your job %s (%s) may be stalled."),
							rinfo->printer, rinfo->job, title);
						break;
					case 3:
					case 4:
						gu_pcs_append_sprintf(&message,
							_("The printer %s which is printing your job %s (%s) is probably stalled."),
							rinfo->printer, rinfo->job, title);
						break;
					default:
						gu_pcs_append_sprintf(&message,
							_("The printer %s which is printing your job %s (%s) has been stalled for %d minutes."),
							rinfo->printer, rinfo->job, title, rinfo->commentary_duration / 60);
						break;
					}
				}
			else if(strcmp(rinfo->commentary_cooked, "UNSTALLED") == 0)
				{
				switch(rinfo->commentary_duration / 60)
					{
					case 1:
					case 2:
					case 3:
					case 4:
						gu_pcs_append_sprintf(&message,
							_("The printer %s which is printing your job %s (%s) was not actually stalled."),
							rinfo->printer, rinfo->job, title);
						break;
					default:
						gu_pcs_append_sprintf(&message,
							_("The printer %s which is printing your job %s (%s) is no longer stalled."),
							rinfo->printer, rinfo->job, title);
						break;
					}
				}
			break;

		case RESP_TYPE_COMMENTARY | COM_EXIT:
			if(rinfo->commentary_severity > 5)
				{
				gu_pcs_append_sprintf(&message,
					_("The printer %s, which could print your job %s (%s) cannot at present: %s."),
					rinfo->printer, rinfo->job, title, rinfo->commentary_cooked);	
				}
			else
				{
				gu_pcs_append_sprintf(&message,
					_("The printer %s, completed an attempt to print your job %s (%s).  The result was: %s."),
					rinfo->printer, rinfo->job, title, rinfo->commentary_cooked);	
				}
			break;

		default:
			gu_pcs_append_sprintf(&message,
				"Invalid response_code %d for job %s.",		/* don't translate */
				rinfo->response_code, rinfo->job);
			break;
		}

	if(long_format && rinfo->reason)
		{
		gu_pcs_append_char(&message, ' ');
		gu_pcs_append_sprintf(&message,
			_("Probable cause: %s."),
			rinfo->reason
			);
		}

	if(long_format && rinfo->qentry.time != 0)
		{
		long elapsed_time = (time(NULL) - rinfo->qentry.time);
		if(elapsed_time > rinfo->commentary_duration_threshold)
			{
			char *temp = elapsed_time_description(elapsed_time / 60);
			gu_pcs_append_sprintf(&message, _("This print job was submitted %s ago."), temp);
			gu_free(temp);
			}
		}
	
	return gu_pcs_free_keep_cstr(&message);
	}

/*
 * handle the followme meta responder
 */
static struct RESPONDER *followme(struct RESPONDER *responder)
	{
	if(strcmp(responder->name, "followme") == 0)
		{
		gu_boolean found = FALSE;
		char fname[MAX_PPR_PATH];
		FILE *f;
		ppr_fnamef(fname, "%s/followme.db/%s", VAR_SPOOL_PPR, responder->address);
		if((f = fopen(fname, "r")))
			{
			int line_available = 80;
			char *line = NULL;
			if((line = gu_getline(line, &line_available, f)))
				{
				char *responder_name, *responder_address, *responder_options;
				if(gu_sscanf(line, "%S %S %Q", &responder_name, &responder_address, &responder_options) == 3)
					{
					found = TRUE;

					/* Yes, we leak memory here when run by pprd, but it doesn't matter. */
					responder->name = responder_name;
					responder->address = responder_address;

					/* If both the job and followme supply responder options, 
					   we will use the ones from the job followed by those 
					   from followme. */
					if(strlen(responder_options) > 0)			/* if any from followme, */
						{
						if(responder->options && strlen(responder->options) > 0)
							{		/* if any from job too, */
							char *p;
							gu_asprintf(&p, "%s %s", responder_options, responder->options);
							gu_free(responder_options);
							responder->options = p;
							}
						else									/* if just from followme, */
							{
							responder->options = responder_options;
							}
						}
					}
				gu_free(line);
				}
			fclose(f);
			}
		if(!found)
			responder->name = "write";
		}

	/* maybe no messages are desired? */
	if(strcmp(responder->name, "none") == 0)
		return NULL;

	return responder;
	}

/*
** The command line interface of this program is not documented.  It will
** quite likely change with each version of PPR.  Different parameters
** are expected based upon which program invokes it (as indicated by
** argv[1]).
*/
int main(int argc, char *argv[])
	{
	struct RESPONSE_INFO rinfo;
	struct RESPONDER *actual_responder;
	int iii;
	void *command;
	char *p;

	command = gu_pca_new(20,20);

	qentryfile_clear(&rinfo.qentry);
	rinfo.job = NULL;
	rinfo.destination = NULL;
	rinfo.printer = NULL;
	rinfo.response_code = -1;
	rinfo.extra = NULL;
	rinfo.commentary_cooked = NULL;
	rinfo.commentary_raw1 = NULL;
	rinfo.commentary_raw2 = NULL;
	rinfo.commentary_duration = 0;
	rinfo.commentary_severity = -1;
	rinfo.commentary_severity = -1;
	rinfo.pages = -1;
	rinfo.charge_per_duplex = 0;
	rinfo.charge_per_simplex = 0;
	rinfo.reason = NULL;
	rinfo.commentary_duration_threshold = 0;
	rinfo.commentary_severity_threshold = 5;

	for(iii=1; iii < argc; iii++)
		{
		if(strcmp(argv[iii], "qfile_fd3") == 0)
			{
			FILE *f = fdopen(3, "r");
			char *line = NULL;
			int line_len = 80;
			char *p;
			qentryfile_load(&rinfo.qentry, f);
			while((line = gu_getline(line, &line_len, f)))
				{
				if((p = lmatchp(line, "Reason:")))
					{
					rinfo.reason = gu_strdup(p);
					continue;
					}
				}
			fclose(f);
			}
		else if((p = gu_name_matchp(argv[iii], "responder_name")))
			{
			rinfo.qentry.responder.name = p;
			}
		else if((p = gu_name_matchp(argv[iii], "responder_address")))
			{
			rinfo.qentry.responder.address = p;
			}
		else if((p = gu_name_matchp(argv[iii], "responder_options")))
			{
			rinfo.qentry.responder.options = p;
			}
		else if((p = gu_name_matchp(argv[iii], "job")))
			{
			rinfo.job = p;
			gu_pca_push(command, argv[iii]);
			}
		else if((p = gu_name_matchp(argv[iii], "destination")))
			{
			rinfo.destination = p;
			gu_pca_push(command, argv[iii]);
			}
		else if((p = gu_name_matchp(argv[iii], "printer")))
			{
			rinfo.printer = p;
			gu_pca_push(command, argv[iii]);
			}
		else if((p = gu_name_matchp(argv[iii], "response_code")))
			{
			rinfo.response_code = atoi(p);
			gu_pca_push(command, argv[iii]);
			}
		else if((p = gu_name_matchp(argv[iii], "extra")))
			{
			rinfo.extra = p;
			gu_pca_push(command, argv[iii]);
			}
		else if((p = gu_name_matchp(argv[iii], "for")))
			{
			rinfo.qentry.For = p;
			}
		else if((p = gu_name_matchp(argv[iii], "title")))
			{
			if(*p)	/* value is optional */
				rinfo.qentry.Title = p;
			}
		else if((p = gu_name_matchp(argv[iii], "lc_messages")))
			{
			if(*p)	/* value is optional */
				rinfo.qentry.lc_messages = p;
			}
		else if((p = gu_name_matchp(argv[iii], "charge_per_duplex")))
			{
			rinfo.charge_per_duplex = atoi(p);
			gu_pca_push(command, argv[iii]);
			}
		else if((p = gu_name_matchp(argv[iii], "charge_per_simplex")))
			{
			rinfo.charge_per_simplex = atoi(p);
			gu_pca_push(command, argv[iii]);
			}
		else if((p = gu_name_matchp(argv[iii], "commentary_cooked")))
			{
			rinfo.commentary_cooked = p;
			gu_pca_push(command, argv[iii]);
			}
		else if((p = gu_name_matchp(argv[iii], "commentary_raw1")))
			{
			rinfo.commentary_raw1 = p;
			gu_pca_push(command, argv[iii]);
			}
		else if((p = gu_name_matchp(argv[iii], "commentary_raw2")))
			{
			rinfo.commentary_raw2 = p;
			gu_pca_push(command, argv[iii]);
			}
		else if((p = gu_name_matchp(argv[iii], "commentary_duration")))
			{
			rinfo.commentary_duration = atoi(p);
			gu_pca_push(command, argv[iii]);
			}
		else if((p = gu_name_matchp(argv[iii], "commentary_severity")))
			{
			rinfo.commentary_severity = atoi(p);
			gu_pca_push(command, argv[iii]);
			}
		else if((p = gu_name_matchp(argv[iii], "commentary_seq_number")))
			{
			rinfo.commentary_seq_number = atoi(p);
			gu_pca_push(command, argv[iii]);
			}
		else
			{
			gu_pca_push(command, argv[iii]);
			}
		}

	/* Make sure mandatory responder information is present. */
	if(!rinfo.qentry.responder.name)
		gu_Throw("reponder_name is missing");
	if(!rinfo.qentry.responder.address)
		gu_Throw("reponder_address is missing");
	
	/* If no response is possible, then we are done. */
	if(!(actual_responder = followme(&rinfo.qentry.responder)))
		return 0;

	/* Parse the responder options and deteremine if this message should actually be sent. */
	if(actual_responder->options && strlen(actual_responder->options) > 0)
		{
		char *temp = gu_strdup(actual_responder->options);
		char *item, *value;
		gu_boolean yes;
		for(p = temp; (item = gu_strsep(&p, " \t")); )
			{
			if((value = gu_name_matchp(item, "printed")))
				{
				if(gu_torf_setBOOL(&yes, value) != -1 && !yes && rinfo.response_code==RESP_FINISHED)
					return 0;
				}
			else if((value = gu_name_matchp(item, "canceled")))
				{
				if(gu_torf_setBOOL(&yes, value) != -1 && !yes && (rinfo.response_code==RESP_CANCELED || rinfo.response_code==RESP_CANCELED_PRINTING))
					return 0;
				}
			else if((value = gu_name_matchp(item, "commentary_severity_threshold")))
				{
				rinfo.commentary_severity_threshold = atoi(p);
				if(rinfo.commentary_severity >= 0)		/* if defined */
					{
					int temp = atoi(p);
					if(temp < rinfo.commentary_severity)
						return 0;
					}
				}
			else if((value = gu_name_matchp(item, "commentary_duration_threshold")))
				{
				rinfo.commentary_duration_threshold = atoi(p);
				}
			}
		gu_free(temp);
		}

	if(rinfo.response_code & RESP_TYPE_COMMENTARY && rinfo.commentary_severity < rinfo.commentary_severity_threshold)
		return 0;
	
	/* Initialize international messages library. */
	#ifdef INTERNATIONAL
	if(rinfo.qentry.lc_messages)
		{
		setlocale(LC_ALL, rinfo.qentry.lc_messages);
		bindtextdomain(PACKAGE, LOCALEDIR);
		textdomain(PACKAGE);
		}
	#endif

	/* Add job information from queue file */
	if(rinfo.qentry.time > 0)
		gu_pca_push(command, gu_name_long_value("time", rinfo.qentry.time));
	if(rinfo.qentry.For)
		gu_pca_push(command, gu_name_str_value("for", rinfo.qentry.For));
	if(rinfo.qentry.Title)
		gu_pca_push(command, gu_name_str_value("title", rinfo.qentry.Title));
	if(rinfo.qentry.lc_messages)
		gu_pca_push(command, gu_name_str_value("lc_messages", rinfo.qentry.lc_messages));
	if(rinfo.reason)
		gu_pca_push(command, gu_name_str_value("reason", rinfo.reason));

	/* Extract page count and number of copies.  If a charge is in effect,
	 * compute it.
	 */
	if(rinfo.qentry.attr.pages >= 0)
		{
		rinfo.pages = rinfo.qentry.attr.pages;
		if(rinfo.qentry.page_list.count >= 0)
			rinfo.pages = rinfo.qentry.page_list.count; 
		rinfo.pages = (rinfo.pages + rinfo.qentry.N_Up.N - 1) / rinfo.qentry.N_Up.N;
		gu_pca_push(command, gu_name_int_value("pages", rinfo.pages));

		if(rinfo.qentry.opts.copies >= 0)
			gu_pca_push(command, gu_name_int_value("copies", rinfo.qentry.opts.copies));

		if(rinfo.charge_per_duplex > 0 || rinfo.charge_per_simplex > 0)
			{
			compute_charge(&rinfo.charge,
				rinfo.charge_per_duplex,
				rinfo.charge_per_simplex,
				rinfo.pages,
				rinfo.qentry.N_Up.N,
				rinfo.qentry.attr.pagefactor,
				rinfo.qentry.N_Up.sigsheets,
				rinfo.qentry.N_Up.sigpart,
				rinfo.qentry.opts.copies
				);
			gu_pca_push(command, gu_name_int_value("charge_duplex_sheets", rinfo.charge.duplex_sheets));
			gu_pca_push(command, gu_name_int_value("charge_simplex_sheets", rinfo.charge.simplex_sheets));
			gu_pca_push(command, gu_name_str_value("charge_total", money(rinfo.charge.total)));
			}
		}

	/* Add suggested messages */
	gu_pca_push(command, build_subject(&rinfo));
	gu_pca_push(command, build_message(&rinfo, FALSE));		/* short_message */
	gu_pca_push(command, build_message(&rinfo, TRUE));		/* long_message */

	/* Add the (possibly modified) responder options to the command line. */
	gu_pca_push(command, gu_name_str_value("responder_name", actual_responder->name));
	gu_pca_push(command, gu_name_str_value("responder_address", actual_responder->address));
	gu_pca_push(command, gu_name_str_value("responder_options", actual_responder->options));

	gu_asprintf(&p, "%s/%s", RESPONDERDIR, actual_responder->name);
	gu_pca_unshift(command, p);		/* argv[0] */
	fflush(stdout);
	execv(p, gu_pca_ptr(command));

	gu_Throw(_("%s: execv(\"%s\", ...) failed, errno=%d (%s)"), argv[0], p, errno, gu_strerror(errno));
	}

/* end of file */
