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
** Last modified 25 March 2005.
*/

/*
** This program is called by ppr, pprd, and pprdrv when they want to send a
** message to a user or system administrator.
**
** For messages about jobs, this ** program receives the job id (or if there
** isn't one yet, just the destination id), the responder code number, and an
** additional parameter (the meaning of which depends on the code number).  If
** there is a queue file, it is opened and information is read from it.  A well
** formed message is generated and then the responder program is run to
** transmit the message to the user.
**
** Note that the calling conventions of this program will change as PPR
** changes.  One goal of this program is to hide those calling convention
** changes by keeping the interface to the responder scripts as unchanging
** as possible.
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
	int commentary_severity;
	} ;

static char *build_subject(struct RESPONSE_INFO *rinfo)
	{
	void *message;

	message = gu_pcs_new_cstr("short_message=");
	
	switch(rinfo->response_code)
		{
		case RESP_FINISHED:
			gu_pcs_append_sprintf(&message,
				_("%s printed on %s"),
				rinfo->job, rinfo->printer);
			break;

		case RESP_ARRESTED:
			gu_pcs_append_sprintf(&message,
				_("%s arrested while printing on %s"),
				rinfo->job, rinfo->printer);
			break;

		case RESP_CANCELED:
			gu_pcs_append_sprintf(&message,
				_("%s canceled"),
				rinfo->job);
			break;

		case RESP_CANCELED_PRINTING:
			gu_pcs_append_sprintf(&message,
				_("%s canceled while printing on %s"),
				rinfo->job, rinfo->printer);
			break;

		case RESP_CANCELED_BADDEST:
			gu_pcs_append_sprintf(&message,
				_("%s canceled, destination unknown"),
				rinfo->job);
			break;

		case RESP_CANCELED_REJECTING:
			gu_pcs_append_sprintf(&message,
				_("%s canceled, %s not acceping requests"),
				rinfo->job, rinfo->destination);
			break;

		case RESP_STRANDED_PRINTER_INCAPABLE:
			gu_pcs_append_sprintf(&message,
				_("%s stranded because %s can't print it"),
				rinfo->job, rinfo->printer);
			break;

		case RESP_STRANDED_GROUP_INCAPABLE:
			gu_pcs_append_sprintf(&message,
				_("%s stranded because no member of %s can print it"),
				rinfo->job, rinfo->destination);
			break;

		case RESP_CANCELED_NOCHARGEACCT:
			gu_pcs_append_sprintf(&message,
				_("job for %s rejected because %s doesn't have a charge account"),
				rinfo->job, rinfo->extra);
			break;

		case RESP_CANCELED_OVERDRAWN:
			gu_pcs_append_sprintf(&message,
				_("job for %s rejected because account is overdrawn"),
				rinfo->job);
			break;

		case RESP_CANCELED_NONCONFORMING:
			gu_pcs_append_sprintf(&message,
				_("job for %s rejected due to lack of page demarcation"),
				rinfo->job);
			break;

		case RESP_NOFILTER:
			gu_pcs_append_sprintf(&message,
				_("job for %s rejected because no filter for %s"),
				rinfo->job, rinfo->extra);
			break;

		case RESP_FATAL:
			gu_pcs_append_sprintf(&message,
				_("job for %s rejected: %s"),
				rinfo->job, rinfo->extra);
			break;

		case RESP_NOSPOOLER:
			gu_pcs_append_sprintf(&message,
				_("job for %s because pprd not running"),
				rinfo->job);
			break;

		case RESP_BADMEDIA:
			gu_pcs_append_sprintf(&message,
				_("job for %s rejected because mediam not available"),
				rinfo->job);
			break;

		case RESP_BADPJLLANG:
			gu_pcs_append_sprintf(&message,
				_("job for %s rejected: PDL %s not recognized"),
				rinfo->job, rinfo->extra);
			break;

		case RESP_FATAL_SYNTAX:
			gu_pcs_append_sprintf(&message,
				_("job for %s rejected: %s"),
				rinfo->job, rinfo->extra);
			break;

		case RESP_CANCELED_NOPAGES:
			gu_pcs_append_sprintf(&message,
				_("job for %s rejected: can't select pages from this job"), 
				rinfo->job);
			break;

		case RESP_CANCELED_ACL:
			gu_pcs_append_sprintf(&message,
				_("job for %s rejected: ACL forbids %s access"),
				rinfo->job, rinfo->extra);
			break;

		case RESP_TYPE_COMMENTARY | COM_PRINTER_ERROR:
			gu_pcs_append_sprintf(&message,
				_("error on %s: %s"),
				rinfo->printer, rinfo->commentary_cooked);	
			break;

		case RESP_TYPE_COMMENTARY | COM_PRINTER_STATUS:
			gu_pcs_append_sprintf(&message,
				_("status of %s: %s"),
				rinfo->printer, rinfo->commentary_cooked);	
			break;

		case RESP_TYPE_COMMENTARY | COM_STALL:
			gu_pcs_append_sprintf(&message,
				_("%s: %s"),
				rinfo->printer, rinfo->commentary_cooked);	
			break;

		case RESP_TYPE_COMMENTARY | COM_EXIT:
			gu_pcs_append_sprintf(&message,
				_("%s: %s"),
				rinfo->printer, rinfo->commentary_cooked);	
			break;

		default:
			gu_pcs_append_sprintf(&message,
				_("Undefined response code %d for job \"%s\"."),
				rinfo->response_code, rinfo->job);
			break;
		}

	return gu_pcs_free_keep_cstr(&message);
	}

/*
** Build a message in final_str, basing it upon the code "response" and inserting
** particuliar information from the other arguments.
*/
static char *build_short_message(struct RESPONSE_INFO *rinfo)
	{
	void *message;

	message = gu_pcs_new_cstr("short_message=");
	
	switch(rinfo->response_code)
		{
		case RESP_FINISHED:
			gu_pcs_append_sprintf(&message,
				_("Your print job \"%s\" has been printed on \"%s\"."),
				rinfo->job, rinfo->printer);
			break;

		case RESP_ARRESTED:
			gu_pcs_append_sprintf(&message,
				_("Your print job \"%s\" was arrested after an attempt\n"
				"to print it on \"%s\" resulted in a job error."),
				rinfo->job, rinfo->printer);
			break;

		case RESP_CANCELED:
			gu_pcs_append_sprintf(&message,
				_("Your print job \"%s\" has been canceled."),
				rinfo->job);
			break;

		case RESP_CANCELED_PRINTING:
			gu_pcs_append_sprintf(&message,
				_("Your print job \"%s\" was canceled while printing on \"%s\"."),
				rinfo->job, rinfo->printer);
			break;

		case RESP_CANCELED_BADDEST:
			gu_pcs_append_sprintf(&message,
				_("Your print job \"%s\" was canceled because\n"
				"\"%s\" is not a known destination."),
				rinfo->job, rinfo->destination);
			break;

		case RESP_CANCELED_REJECTING:
			gu_pcs_append_sprintf(&message,
				_("Your print job \"%s\" was canceled because\n"
				"the destination \"%s\" is not acceping requests."),
				rinfo->job, rinfo->destination);
			break;

		case RESP_STRANDED_PRINTER_INCAPABLE:
			gu_pcs_append_sprintf(&message,
				_("Your print job \"%s\" is stranded because\n"
				"the printer \"%s\" is incapable of printing it."),
				rinfo->job, rinfo->printer);
			break;

		case RESP_STRANDED_GROUP_INCAPABLE:
			gu_pcs_append_sprintf(&message,
				_("Your print job \"%s\" is stranded because no\n"
				"member of the group \"%s\" is capable of printing it."),
				rinfo->job, rinfo->destination);
			break;

		case RESP_CANCELED_NOCHARGEACCT:
			gu_pcs_append_sprintf(&message,
				_("Your new print job for \"%s\" was rejected because\n"
				"\"%s\" does not have a charge account."),
				rinfo->job, rinfo->extra);
			break;

		case RESP_CANCELED_OVERDRAWN:
			gu_pcs_append_sprintf(&message,
				_("Your new print job for \"%s\" was rejected because\n"
				"your account is overdrawn."),
				rinfo->job);
			break;

		case RESP_CANCELED_NONCONFORMING:
			gu_pcs_append_sprintf(&message,
				_("Your new print job for \"%s\" was rejected because\n"
				"it does not contain DSC page division information."),
				rinfo->job);
			break;

		case RESP_NOFILTER:
			gu_pcs_append_sprintf(&message,
				_("Your new print job for \"%s\" has been rejected because no filter\n"
				"is available which can convert %s to PostScript."),
				rinfo->job, rinfo->extra);
			break;

		case RESP_FATAL:
			gu_pcs_append_sprintf(&message,
				_("Your new print job for \"%s\" has been rejected by PPR because of a\n"
				"fatal error: %s."),
				rinfo->job, rinfo->extra);
			break;

		case RESP_NOSPOOLER:
			gu_pcs_append_sprintf(&message,
				_("Your new print job for \"%s\" has been lost because PPRD is not running."),
				rinfo->job);
			break;

		case RESP_BADMEDIA:
			gu_pcs_append_sprintf(&message,
				_("Your new print job for \"%s\" has been rejected because it requires\n"
				"a size and type of medium (paper) which is not available."),
				rinfo->job);
			break;

		case RESP_BADPJLLANG:
			gu_pcs_append_sprintf(&message,
				_("Your new print job for \"%s\" has been rejected because the\n"
				"PJL header requests an unrecognized printer language \"%s\"."),
				rinfo->job, rinfo->extra);
			break;

		case RESP_FATAL_SYNTAX:
			gu_pcs_append_sprintf(&message,
				_("Your new print job for \"%s\" has been rejected because\n"
				"the ppr command line contains an error:\n"
				"\n"
				"%s."),
				rinfo->job, rinfo->extra);
			break;

		case RESP_CANCELED_NOPAGES:
			gu_pcs_append_sprintf(&message,
				_("Your new print job for \"%s\" has been rejected because\n"
				"you requested printing of only selected pages but the pages\n"
				"are not marked by DSC comments."),
				rinfo->job);
			break;

		case RESP_CANCELED_ACL:
			gu_pcs_append_sprintf(&message,
				_("Your print job for \"%s\" has been rejected because the\n"
				"PPR access control lists do not grant \"%s\" access to\n"
				"that destination."),
				rinfo->job, rinfo->extra);
			break;

		case RESP_TYPE_COMMENTARY | COM_PRINTER_ERROR:
			gu_pcs_append_sprintf(&message,
				_("The printer \"%s\" which is printing \"%s\" reports error\n"
				"\"%s\"."),
				rinfo->printer, rinfo->job, rinfo->commentary_cooked);	
			break;

		case RESP_TYPE_COMMENTARY | COM_PRINTER_STATUS:
			gu_pcs_append_sprintf(&message,
				_("The printer \"%s\" which is printing \"%s\" reports status\n"
				"\"%s\"."),
				rinfo->printer, rinfo->job, rinfo->commentary_cooked);	
			break;

		case RESP_TYPE_COMMENTARY | COM_STALL:
			gu_pcs_append_sprintf(&message,
				_("The printer \"%s\" which is printing \"%s\" %s\n"),
				rinfo->printer, rinfo->job, rinfo->commentary_cooked);	
			break;

		case RESP_TYPE_COMMENTARY | COM_EXIT:
			gu_pcs_append_sprintf(&message,
				_("The printer \"%s\" which is printing \"%s\" %s."),
				rinfo->printer, rinfo->job, rinfo->commentary_cooked);	
			break;

		default:
			gu_pcs_append_sprintf(&message,
				_("Undefined response code %d for job \"%s\"."),
				rinfo->response_code, rinfo->job);
			break;
		}

	return gu_pcs_free_keep_cstr(&message);
	} /* end of respond_build_message() */

static char *build_long_message(struct RESPONSE_INFO *rinfo)
	{
	void *message;

	message = gu_pcs_new_cstr("short_message=");
	
	switch(rinfo->response_code)
		{
		case RESP_FINISHED:
			gu_pcs_append_sprintf(&message,
				_("Your print job \"%s\" has been printed on \"%s\"."),
				rinfo->job, rinfo->printer);
			break;

		case RESP_ARRESTED:
			gu_pcs_append_sprintf(&message,
				_("Your print job \"%s\" was arrested after an attempt\n"
				"to print it on \"%s\" resulted in a job error."),
				rinfo->job, rinfo->printer);
			break;

		case RESP_CANCELED:
			gu_pcs_append_sprintf(&message,
				_("Your print job \"%s\" has been canceled."),
				rinfo->job);
			break;

		case RESP_CANCELED_PRINTING:
			gu_pcs_append_sprintf(&message,
				_("Your print job \"%s\" was canceled while printing on \"%s\"."),
				rinfo->job, rinfo->printer);
			break;

		case RESP_CANCELED_BADDEST:
			gu_pcs_append_sprintf(&message,
				_("Your print job \"%s\" was canceled because\n"
				"\"%s\" is not a known destination."),
				rinfo->job, rinfo->destination);
			break;

		case RESP_CANCELED_REJECTING:
			gu_pcs_append_sprintf(&message,
				_("Your print job \"%s\" was canceled because\n"
				"the destination \"%s\" is not acceping requests."),
				rinfo->job, rinfo->destination);
			break;

		case RESP_STRANDED_PRINTER_INCAPABLE:
			gu_pcs_append_sprintf(&message,
				_("Your print job \"%s\" is stranded because\n"
				"the printer \"%s\" is incapable of printing it."),
				rinfo->job, rinfo->printer);
			break;

		case RESP_STRANDED_GROUP_INCAPABLE:
			gu_pcs_append_sprintf(&message,
				_("Your print job \"%s\" is stranded because no\n"
				"member of the group \"%s\" is capable of printing it."),
				rinfo->job, rinfo->destination);
			break;

		case RESP_CANCELED_NOCHARGEACCT:
			gu_pcs_append_sprintf(&message,
				_("Your new print job for \"%s\" was rejected because\n"
				"\"%s\" does not have a charge account."),
				rinfo->job, rinfo->extra);
			break;

		case RESP_CANCELED_OVERDRAWN:
			gu_pcs_append_sprintf(&message,
				_("Your new print job for \"%s\" was rejected because\n"
				"your account is overdrawn."),
				rinfo->job);
			break;

		case RESP_CANCELED_NONCONFORMING:
			gu_pcs_append_sprintf(&message,
				_("Your new print job for \"%s\" was rejected because\n"
				"it does not contain DSC page division information."),
				rinfo->job);
			break;

		case RESP_NOFILTER:
			gu_pcs_append_sprintf(&message,
				_("Your new print job for \"%s\" has been rejected because no filter\n"
				"is available which can convert %s to PostScript."),
				rinfo->job, rinfo->extra);
			break;

		case RESP_FATAL:
			gu_pcs_append_sprintf(&message,
				_("Your new print job for \"%s\" has been rejected by PPR because of a\n"
				"fatal error: %s."),
				rinfo->job, rinfo->extra);
			break;

		case RESP_NOSPOOLER:
			gu_pcs_append_sprintf(&message,
				_("Your new print job for \"%s\" has been lost because PPRD is not running."),
				rinfo->job);
			break;

		case RESP_BADMEDIA:
			gu_pcs_append_sprintf(&message,
				_("Your new print job for \"%s\" has been rejected because it requires\n"
				"a size and type of medium (paper) which is not available."),
				rinfo->job);
			break;

		case RESP_BADPJLLANG:
			gu_pcs_append_sprintf(&message,
				_("Your new print job for \"%s\" has been rejected because the\n"
				"PJL header requests an unrecognized printer language \"%s\"."),
				rinfo->job, rinfo->extra);
			break;

		case RESP_FATAL_SYNTAX:
			gu_pcs_append_sprintf(&message,
				_("Your new print job for \"%s\" has been rejected because\n"
				"the ppr command line contains an error:\n"
				"\n"
				"%s."),
				rinfo->job, rinfo->extra);
			break;

		case RESP_CANCELED_NOPAGES:
			gu_pcs_append_sprintf(&message,
				_("Your new print job for \"%s\" has been rejected because\n"
				"you requested printing of only selected pages but the pages\n"
				"are not marked by DSC comments."),
				rinfo->job);
			break;

		case RESP_CANCELED_ACL:
			gu_pcs_append_sprintf(&message,
				_("Your print job for \"%s\" has been rejected because the\n"
				"PPR access control lists do not grant \"%s\" access to\n"
				"that destination."),
				rinfo->job, rinfo->extra);
			break;

		case RESP_TYPE_COMMENTARY | COM_PRINTER_ERROR:
			gu_pcs_append_sprintf(&message,
				_("The printer \"%s\" which is printing \"%s\" reports error\n"
				"\"%s\"."),
				rinfo->printer, rinfo->job, rinfo->commentary_cooked);	
			break;

		case RESP_TYPE_COMMENTARY | COM_PRINTER_STATUS:
			gu_pcs_append_sprintf(&message,
				_("The printer \"%s\" which is printing \"%s\" reports status\n"
				"\"%s\"."),
				rinfo->printer, rinfo->job, rinfo->commentary_cooked);	
			break;

		case RESP_TYPE_COMMENTARY | COM_STALL:
			gu_pcs_append_sprintf(&message,
				_("The printer \"%s\" which is printing \"%s\" %s\n"),
				rinfo->printer, rinfo->job, rinfo->commentary_cooked);	
			break;

		case RESP_TYPE_COMMENTARY | COM_EXIT:
			gu_pcs_append_sprintf(&message,
				_("The printer \"%s\" which is printing \"%s\" %s."),
				rinfo->printer, rinfo->job, rinfo->commentary_cooked);	
			break;

		default:
			gu_pcs_append_sprintf(&message,
				_("Undefined response code %d for job \"%s\"."),
				rinfo->response_code, rinfo->job);
			break;
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


#if 0
	/*
	** Prepare the canned message.
	*/
	{
	const char *job_title = job.Title ? job.Title : job.lpqFileName ? job.lpqFileName : "untitled";
	int duration = raw2 ? atoi(raw2) : 0;
	canned_message[0] = '\0';

	if(category & COM_PRINTER_ERROR)
		{
		snprintf(canned_message, sizeof(canned_message),
				severity > 5
					?
					commentary_seq_number == 1
						?
						_("The printer \"%s\", which could print your job \"%s-%d\" (%s), "
						"reports the error \"%s%s%s%s\".  This condition must be "
						"corrected before your job can start.")
						:
						_("The printer \"%s\", which is printing your job \"%s-%d\" (%s), "
						"reports the error \"%s%s%s%s\".  This condition must be "
						"corrected before your job can continue.")
					:
					_("The printer \"%s\", which is printing your job \"%s-%d\" (%s), "
					"reports the error \"%s%s%s%s\".  It may be to your benefit "
					"to correct this condition."),
				printer.Name,
				job.destname, job.id,
				job_title,
				cooked,
				raw1 ? " (" : "",
				raw1 ? raw1 : "",
				raw1 ? ")" : ""
				);
		if(duration > 1)
			{
			gu_snprintfcat(canned_message, sizeof(canned_message),
				_("  This condition has persisted for %d minutes."),
				duration);
			}
		}

	if(category & COM_EXIT && severity > 5)
		{
		snprintf(canned_message, sizeof(canned_message),
				_("The printer \"%s\" cannot start your job \"%s-%d\" (%s) "
				"because of the error condition \"%s\"."),
			printer.Name,
			job.destname, job.id,
			job_title,
			cooked
			);
		}

	if(category & COM_STALL)
		{
		snprintf(canned_message, sizeof(canned_message),
				_("The printer \"%s\", which is printing your job \"%s-%d\" (%s), "
				"%s."),
			printer.Name,
			job.destname, job.id,
			job_title,
			cooked
			);
		}

	}
#endif

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
	rinfo.commentary_severity = -1;

	for(iii=1; iii < argc; iii++)
		{
		if(strcmp(argv[iii], "qfile_fd3") == 0)
			{
			FILE *f = fdopen(3, "r");
			qentryfile_load(&rinfo.qentry, f);
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
			gu_pca_push(command, argv[iii]);
			}
		else if((p = gu_name_matchp(argv[iii], "title")))
			{
			rinfo.qentry.Title = p;
			gu_pca_push(command, argv[iii]);
			}
		else if((p = gu_name_matchp(argv[iii], "lc_messages")))
			{
			rinfo.qentry.lc_messages = p;
			gu_pca_push(command, argv[iii]);
			}
		else if((p = gu_name_matchp(argv[iii], "charge_per_duplex")))
			{
			gu_pca_push(command, argv[iii]);
			}
		else if((p = gu_name_matchp(argv[iii], "charge_per_simplex")))
			{
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
		else if((p = gu_name_matchp(argv[iii], "commentary_severity")))
			{
			rinfo.commentary_severity = atoi(p);
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

	/* Deteremine if this message should actually be sent. */
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
			}
		gu_free(temp);
		}
	
	/* Add the (possibly modified) responder options to the command line. */
	gu_pca_push(command, gu_name_str_value("responder_name", actual_responder->name));
	gu_pca_push(command, gu_name_str_value("responder_address", actual_responder->address));
	gu_pca_push(command, gu_name_str_value("responder_options", actual_responder->options));

	/* Initialize international messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, rinfo.qentry.lc_messages);
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	/* Add suggested messages */
	gu_pca_push(command, build_subject(&rinfo));
	gu_pca_push(command, build_short_message(&rinfo));
	gu_pca_push(command, build_long_message(&rinfo));

	/* Add job information from queue file */
	if(rinfo.qentry.time > 0)
		gu_pca_push(command, gu_name_long_value("time", rinfo.qentry.time));
	if(rinfo.qentry.attr.pages >= 0)
		{
		int pages = rinfo.qentry.attr.pages;
		if(rinfo.qentry.page_list.count >= 0)
			pages = rinfo.qentry.page_list.count;
		pages = (pages + rinfo.qentry.N_Up.N - 1) / rinfo.qentry.N_Up.N;
		if(rinfo.qentry.opts.copies >= 0)
			pages *= rinfo.qentry.opts.copies;
		gu_pca_push(command, gu_name_int_value("pages", pages));
		}

	gu_asprintf(&p, "%s/%s", RESPONDERDIR, actual_responder->name);
	gu_pca_unshift(command, p);		/* argv[0] */
	execv(p, gu_pca_ptr(command));

	return 1;
	}

/* end of file */
