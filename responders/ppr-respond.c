/*
** mouse:~ppr/src/pprd/ppr-respond.c
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
** Last modified 12 February 2004.
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
	char *jobname;					/* destname-id of job */
	int response_code;				/* what happened to the job? */
	char *extra;					/* extra parameter supplementing response_code */
	struct QFileEntry job;
	int pages_printed;
	struct COMPUTED_CHARGE charge;	/* the amount of money to charge */
	} ;

/*
** Build a message in final_str, basing it upon the code "response" and inserting
** particuliar information from the other arguments.
*/
static void respond_build_message(char *response_str, size_t space_available, struct RESPONSE_INFO *rinfo)
	{
	switch(rinfo->response_code)
		{
		case RESP_FINISHED:
			snprintf(response_str, space_available,
				_("Your print job \"%s\" has been printed on \"%s\"."),
				rinfo->jobname, rinfo->extra);
			break;

		case RESP_ARRESTED:
			snprintf(response_str, space_available,
				_("Your print job \"%s\" was arrested after an attempt\n"
				"to print it on \"%s\" resulted in a job error."),
				rinfo->jobname, rinfo->extra);
			break;

		case RESP_CANCELED:
			snprintf(response_str, space_available,
				_("Your print job \"%s\" has been canceled."),
				rinfo->jobname);
			break;

		case RESP_CANCELED_PRINTING:
			snprintf(response_str, space_available,
				_("Your print job \"%s\" was canceled while printing on \"%s\"."),
				rinfo->jobname, rinfo->extra);
			break;

		case RESP_CANCELED_BADDEST:
			snprintf(response_str, space_available,
				_("Your print job \"%s\" was canceled because\n"
				"\"%s\" is not a known destination."),
				rinfo->jobname, rinfo->extra);
			break;

		case RESP_CANCELED_REJECTING:
			snprintf(response_str, space_available,
				_("Your print job \"%s\" was canceled because\n"
				"the destination \"%s\" is not acceping requests."),
				rinfo->jobname, rinfo->extra);
			break;

		case RESP_STRANDED_PRINTER_INCAPABLE:
			snprintf(response_str, space_available,
				_("Your print job \"%s\" is stranded because\n"
				"the printer \"%s\" is incapable of printing it."),
				rinfo->jobname, rinfo->extra);
			break;

		case RESP_STRANDED_GROUP_INCAPABLE:
			snprintf(response_str, space_available,
				_("Your print job \"%s\" is stranded because no\n"
				"member of the group \"%s\" is capable of printing it."),
				rinfo->jobname, rinfo->extra);
			break;

		case RESP_CANCELED_NOCHARGEACCT:
			snprintf(response_str, space_available,
				_("Your new print job for \"%s\" was rejected because\n"
				"\"%s\" does not have a charge account."),
				rinfo->jobname, rinfo->extra);
			break;

		case RESP_CANCELED_BADAUTH:
			snprintf(response_str, space_available,
				_("Your new print job for \"%s\" was rejected because\n"
				"you did not enter %s's authorization code."),
				rinfo->jobname, rinfo->extra);
			break;

		case RESP_CANCELED_OVERDRAWN:
			snprintf(response_str, space_available,
				_("Your new print job for \"%s\" was rejected because\n"
				"your account is overdrawn."),
				rinfo->jobname);
			break;

		case RESP_CANCELED_NONCONFORMING:
			snprintf(response_str, space_available,
				_("Your new print job for \"%s\" was rejected because\n"
				"it does not contain DSC page division information."),
				rinfo->jobname);
			break;

		case RESP_NOFILTER:
			snprintf(response_str, space_available,
				_("Your new print job for \"%s\" has been rejected because no filter\n"
				"is available which can convert %s to PostScript."),
				rinfo->jobname, rinfo->extra);
			break;

		case RESP_FATAL:
			snprintf(response_str, space_available,
				_("Your new print job for \"%s\" has been rejected by PPR because of a\n"
				"fatal error: %s."),
				rinfo->jobname, rinfo->extra);
			break;

		case RESP_NOSPOOLER:
			snprintf(response_str, space_available,
				_("Your new print job for \"%s\" has been lost because PPRD is not running."),
				rinfo->jobname);
			break;

		case RESP_BADMEDIA:
			snprintf(response_str, space_available,
				_("Your new print job for \"%s\" has been rejected because it requires\n"
				"a size and type of medium (paper) which is not available."),
				rinfo->jobname);
			break;

		case RESP_BADPJLLANG:
			snprintf(response_str, space_available,
				_("Your new print job for \"%s\" has been rejected because the\n"
				"PJL header requests an unrecognized printer language \"%s\"."),
				rinfo->jobname, rinfo->extra);
			break;

		case RESP_FATAL_SYNTAX:
			snprintf(response_str, space_available,
				_("Your new print job for \"%s\" has been rejected because\n"
				"the ppr command line contains an error:\n"
				"\n"
				"%s."),
				rinfo->jobname, rinfo->extra);
			break;

		case RESP_CANCELED_NOPAGES:
			snprintf(response_str, space_available,
				_("Your new print job for \"%s\" has been rejected because\n"
				"you requested printing of only selected pages but the pages\n"
				"are not marked by DSC comments."),
				rinfo->jobname);
			break;

		case RESP_CANCELED_ACL:
			snprintf(response_str, space_available,
				_("Your print job for \"%s\" has been rejected because the\n"
				"PPR access control lists do not grant \"%s\" access to\n"
				"that destination."),
				rinfo->jobname, rinfo->extra);
			break;

		default:
			snprintf(response_str, space_available,
				_("Undefined response code %d for your job \"%s\"."),
				rinfo->response_code, rinfo->jobname);
			break;
		}
	} /* end of respond_build_message() */

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
						if(strlen(responder->options) > 0)		/* if any from job too, */
							{
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
 * Handle "job fate" messages from pprd and ppr
 */
static int job_message(int argc, char *argv[])
	{
	struct RESPONSE_INFO rinfo;
	struct RESPONDER *actual_responder;
	char response_message[256];
	char responder_fname[MAX_PPR_PATH];
	char time_in_ascii[16];
	char numpages_in_ascii[16];

	/* all formats have these parameters in common */
	rinfo.jobname = argv[2];
	rinfo.response_code = atoi(argv[3]);
	rinfo.extra = argv[4];

	/* pprd provides 2 additional parameters and a queue file on fd 3. */
	if(strcmp(argv[1], "pprd") == 0)
		{
		int charge_per_duplex, charge_per_simplex;
		FILE *f;

		charge_per_duplex = atoi(argv[5]);
		charge_per_simplex = atoi(argv[6]);

		if(!(f = fdopen(3, "r")))
			{
			fprintf(stderr, "%s: fdopen(%d, \"r\") failed, errno=%d (%s)\n", argv[0], 3, errno, gu_strerror(errno));
			return -1;
			}
	
		read_struct_QFileEntry(f, &rinfo.job);
	
		fclose(f);
		}

	/* ppr provides no queue file but it makes up for it in parameters. */
	else if(strcmp(argv[1], "ppr") == 0)
		{
		rinfo.job.responder.name = argv[5];
		rinfo.job.responder.address = argv[6];
		rinfo.job.responder.options = argv[7];
		rinfo.job.For = argv[8];
		rinfo.job.Title = argv[9];
		rinfo.job.lc_messages = argv[10];
		rinfo.job.time = 0;
		}

	else
		{
		gu_Throw("can't happen");
		}

	rinfo.pages_printed = -1;
	rinfo.charge.total = 0;

	/* Sanity check */
	if(!rinfo.job.responder.name && !rinfo.job.responder.address)
		gu_Throw("responder name or address missing");

	/* If no response is possible, then we are done. */
	if(!(actual_responder = followme(&rinfo.job.responder)))
		return 0;

	/* Initialize international messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, rinfo.job.lc_messages);
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	/* Construct the message string. */
	respond_build_message(response_message, sizeof(response_message), &rinfo);

	/* Convert some other stuff to strings. */
	snprintf(time_in_ascii, sizeof(time_in_ascii), "%ld", rinfo.job.time);
	snprintf(numpages_in_ascii, sizeof(numpages_in_ascii), "%d", rinfo.pages_printed);

	/* Build path to responder. */
	ppr_fnamef(responder_fname, "%s/%s", RESPONDERDIR, actual_responder->name);

	/* Replace this child program with the responder. */
	execl(
		responder_fname, actual_responder->name,
		actual_responder->address,
		actual_responder->options,
		(char*)NULL
		);

	/* Catch execl() failures. */
	gu_Throw("execl(\"%s\", ...) failed, errno=%d (%s)", responder_fname, errno, gu_strerror(errno));
	}

/*
 * Handle "commentary" messages from pprdrv
 */
static int commentary_message(int argc, char *argv[])
	{
	char canned_message[1024];
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
	}

/*
** The command line interface of this program is not documented.  It will
** quite likely change with each version of PPR.  Different parameters
** are expected based upon which program invokes it (as indicated by
** argv[1]).
*/
int main(int argc, char *argv[])
	{
	if(argc < 2)
		gu_Throw("too few arguments");
	if(strcmp(argv[1], "pprd") == 0)
		job_message(argc, argv);
	else if(strcmp(argv[1], "ppr") == 0)
		job_message(argc, argv);
	else if(strcmp(argv[1], "pprdrv_commentary") == 0)
		commentary_message(argc, argv);
	else
		gu_Throw("Unrecognized category: %s", argv[1]);
	return 1;
	}

/* end of file */
