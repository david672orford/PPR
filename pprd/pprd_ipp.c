/*
** mouse:~ppr/src/pprd/pprd_ipp.c
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
** Last modified 2 March 2005.
*/

/*
 * In this module we handle Internet Printing Protocol requests which have
 * been received by ppr-httpd and passed on to us by the ipp CGI program.
 *
 * These requests arrive as an "IPP" line which specifies the PID of
 * the ipp CGI and a list of HTTP headers and CGI variables.  The POSTed
 * data is in a temporary file (whose name can be derived from the PID)
 * and we send the response document to another temporary file.  Then
 * we send SIGUSR1 to the CGI program "ipp" to tell it that the response 
 * is ready.
 */

#include "config.h"
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "ipp_constants.h"
#include "ipp_utils.h"
#include "pprd.h"
#include "pprd.auto_h"

struct OPERATION {
	void *requested_attributes;
	gu_boolean requested_attributes_all;
	char *printer_uri;
	struct URI *printer_uri_obj;
	char *printer_name;
	};

/** Create an object which can tell us if an attribute was requested. */
static struct OPERATION *request_new(struct IPP *ipp)
	{
	struct OPERATION *this;
	ipp_attribute_t *attr;

	this = gu_alloc(1, sizeof(struct OPERATION));
	this->requested_attributes = gu_pch_new(25);
	this->requested_attributes_all = FALSE;
	this->printer_uri = NULL;
	this->printer_uri_obj = NULL;
	this->printer_name = NULL;

	/* Traverse the request's operation attributes. */
	for(attr = ipp->request_attrs; attr; attr = attr->next)
		{
		if(attr->group_tag != IPP_TAG_OPERATION)
			continue;
		if(attr->value_tag == IPP_TAG_KEYWORD && strcmp(attr->name, "requested-attributes") == 0)
			{
			int iii;
			for(iii=0; iii<attr->num_values; iii++)
				gu_pch_set(this->requested_attributes, attr->values[iii].string.text, "TRUE");
			}
		else if(attr->value_tag == IPP_TAG_URI && strcmp(attr->name, "printer-uri") == 0)
			this->printer_uri = attr->values[0].string.text;
		else if(attr->value_tag == IPP_TAG_URI && strcmp(attr->name, "printer-name") == 0)
			this->printer_name = attr->values[0].string.text;
		else
			ipp_copy_attribute(ipp, IPP_TAG_UNSUPPORTED, attr);
		}

	/* If no attributes were requested or "all" were requested, */
	if(gu_pch_size(this->requested_attributes) == 0 || gu_pch_get(this->requested_attributes, "all"))
		this->requested_attributes_all = TRUE;

	if(this->printer_uri)
		this->printer_uri_obj = gu_uri_new(this->printer_uri);

	return this;
	}

/** Destroy a requested attributes object.
 *
 * Remember that we mustn't try to free printer_uri or printer_name since
 * they are simply pointers to the values allocated in the IPP object.
 */ 
static void request_free(struct OPERATION *this)
	{
	char *name, *value;
	for(gu_pch_rewind(this->requested_attributes); (name = gu_pch_nextkey(this->requested_attributes, (void*)&value)); )
		{
		if(strcmp(value, "TOUCHED") != 0)
			{
			debug("requested attribute \"%s\" not implemented", name);
			}
		}
	gu_pch_free(this->requested_attributes);
	if(this->printer_uri_obj)
		gu_uri_free(this->printer_uri_obj);
	gu_free(this);
	}

/** Return true if the indicate attributes was requested. */
static gu_boolean request_attr_requested(struct OPERATION *this, char name[])
	{
	if(this->requested_attributes_all)
		return TRUE;
	if(gu_pch_get(this->requested_attributes, name))
		{
		gu_pch_set(this->requested_attributes, name, "TOUCHED");
		return TRUE;
		}
	return FALSE;
	}

/** Return the requested queue name or NULL if none requested. */
static char *request_destname(struct OPERATION *this)
	{
	if(this->printer_name)
		return this->printer_name;
	if(this->printer_uri)
		return this->printer_uri_obj->basename;		/* possibly null */
	return NULL;
	}

/** Convert PPR printer status to IPP status */
static void printer_add_status(struct IPP *ipp, int prnid, struct OPERATION *req)
	{
	const char function[] = "printer_add_status";
	switch(printers[prnid].status)
		{
		case PRNSTATUS_IDLE:
			if(request_attr_requested(req, "printer-state"))
				{
				ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
					"printer-state", IPP_PRINTER_IDLE);
				}
			break;
		case PRNSTATUS_PRINTING:
			if(request_attr_requested(req, "printer-state"))
				{
				ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
					"printer-state", IPP_PRINTER_PROCESSING);
				}
			if(request_attr_requested(req, "printer-state-message"))
				{
				ipp_add_printf(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
					"printer-state-message", _("printing %s"), jobid(destid_to_name(prnid), printers[prnid].id, printers[prnid].subid));
				}
			break;
		case PRNSTATUS_CANCELING:
			if(request_attr_requested(req, "printer-state"))
				{
				ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
					"printer-state", IPP_PRINTER_PROCESSING);
				}
			if(request_attr_requested(req, "printer-state-message"))
				{
				ipp_add_printf(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
					"printer-state-message", _("canceling %s"), jobid(destid_to_name(prnid), printers[prnid].id, printers[prnid].subid));
				}
			break;
		case PRNSTATUS_SEIZING:
			if(request_attr_requested(req, "printer-state"))
				{
				ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
					"printer-state", IPP_PRINTER_PROCESSING);
				}
			if(request_attr_requested(req, "printer-state-message"))
				{
				ipp_add_printf(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
					"printer-state-message", _("seizing %s"), jobid(destid_to_name(prnid), printers[prnid].id, printers[prnid].subid));
				}
			break;
		case PRNSTATUS_STOPPING:
			if(request_attr_requested(req, "printer-state"))
				{
				ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
					"printer-state", IPP_PRINTER_PROCESSING);
				}
			if(request_attr_requested(req, "printer-state-message"))
				{
				ipp_add_printf(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
					"printer-state-message", _("stopping (printing %s)"), jobid(destid_to_name(prnid), printers[prnid].id, printers[prnid].subid));
				}
			break;
		case PRNSTATUS_HALTING:
			if(request_attr_requested(req, "printer-state"))
				{
				ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
					"printer-state", IPP_PRINTER_PROCESSING);
				}
			if(request_attr_requested(req, "printer-state-message"))
				{
				ipp_add_printf(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
					"printer-state-message", _("halting (printing %s)"), jobid(destid_to_name(prnid), printers[prnid].id, printers[prnid].subid));
				}
			break;
		case PRNSTATUS_STOPT:
			if(request_attr_requested(req, "printer-state"))
				{
				ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
					"printer-state", IPP_PRINTER_STOPPED);
				}
			if(request_attr_requested(req, "printer-state-message"))
				{
				ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
					"printer-state-message", _("stopt"), FALSE);
				}
			break;
		case PRNSTATUS_FAULT:
			if(request_attr_requested(req, "printer-state"))
				{
				ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
					"printer-state", IPP_PRINTER_STOPPED);
				}
			if(request_attr_requested(req, "printer-state-message"))
				{
				if(printers[prnid].next_error_retry)
					{
					ipp_add_printf(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
						"printer-state-message", _("fault, retry %d in %d seconds"), printers[prnid].next_error_retry, printers[prnid].countdown);
					}
				else
					{
					ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
						"printer-state-message", _("fault, no auto retry"), FALSE);
					}
				}
			break;
		case PRNSTATUS_ENGAGED:
			if(request_attr_requested(req, "printer-state"))
				{
				ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
					"printer-state", IPP_PRINTER_STOPPED);
				}
			if(request_attr_requested(req, "printer-state-message"))
				{
				ipp_add_printf(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
					"printer-state-message", _("otherwise engaged or off-line, retry %d in %d seconds"), printers[prnid].next_engaged_retry, printers[prnid].countdown);
				}
			break;
		case PRNSTATUS_STARVED:
			if(request_attr_requested(req, "printer-state"))
				{
				ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
					"printer-state", IPP_PRINTER_STOPPED);
				}
			if(request_attr_requested(req, "printer-state-message"))
				{
				ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
					"printer-state-message", _("waiting for resource ration"), FALSE);
				}
			break;

		default:
			error("%s(): invalid printer_status %d", function, printers[prnid].status);
			break;
		}
	}

/** Handler for IPP_GET_PRINTER_ATTRIBUTES */
static void ipp_get_printer_attributes(struct IPP *ipp)
    {
	FUNCTION4DEBUG("ipp_get_printer_attributes")
	const char *destname;
	int destid;
	struct OPERATION *req;

	req = request_new(ipp);

	if(!(destname = request_destname(req)) || (destid = destid_by_name(destname)) == -1)
		{
		request_free(req);
		ipp->response_code = IPP_NOT_FOUND;
		return;
		}
		
	if(request_attr_requested(req, "printer-name"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_NAME,
			"printer-name", gu_strdup(destname), TRUE);
		}
	if(request_attr_requested(req, "printer-uri-supported"))
		{
		ipp_add_template(ipp, IPP_TAG_PRINTER, IPP_TAG_URI,
			"printer-uri-supported", destid_is_group ? "/classes/%s" : "/printers/%s", destname);
		}

	if(request_attr_requested(req, "uri_security_supported"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_KEYWORD,
			"uri-security-supported", "none", FALSE);
		}

	if(request_attr_requested(req, "printer-make-and-model"))
		{	/* dummy code */
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
			"printer-make-and-model", "HP LaserJet 4200", FALSE);
		}

	if(destid_is_group(destid))
		{
		if(request_attr_requested(req, "printer-is-accepting-jobs"))
			{
			ipp_add_boolean(ipp, IPP_TAG_PRINTER, IPP_TAG_BOOLEAN,
				"printer-is-accepting-jobs", groups[destid_to_gindex(destid)].accepting);
			}
		}
	else
		{
		if(request_attr_requested(req, "printer-is-accepting-jobs"))
			{
			ipp_add_boolean(ipp, IPP_TAG_PRINTER, IPP_TAG_BOOLEAN,
				"printer-is-accepting-jobs", printers[destid].accepting);
			}
		printer_add_status(ipp, destid, req);
		}

	if(request_attr_requested(req, "printer-state-reasons"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_KEYWORD,
			"printer-state-reasons", "none", FALSE);
		}

	/* Which operations are supported for this printer object? */
	if(request_attr_requested(req, "operations-supported"))
		{
		int supported[] =
			{
			IPP_PRINT_JOB,
			/* IPP_PRINT_URI, */
			/* IPP_VALIDATE_JOB, */
			/* IPP_CREATE_JOB, */
			/* IPP_SEND_DOCUMENT, */
			/* IPP_SEND_URI, */
			IPP_CANCEL_JOB,
			/* IPP_GET_JOB_ATTRIBUTES, */
			IPP_GET_JOBS,
			IPP_GET_PRINTER_ATTRIBUTES,
			CUPS_GET_PRINTERS,
			CUPS_GET_CLASSES
			};
		ipp_add_integers(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
			"operations-supported", sizeof(supported) / sizeof(supported[0]), supported);
		}

	if(request_attr_requested(req, "charset-configured"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_CHARSET, 
			"charset-configured", "utf-8", FALSE);
		}
	if(request_attr_requested(req, "charset-supported"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_CHARSET, 
			"charset-supported", "utf-8", FALSE);
		}
	
	if(request_attr_requested(req, "natural-language-configured"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_LANGUAGE, 
			"natural-language-configured", "en-us", FALSE);
		}
	if(request_attr_requested(req, "generated-natural-language-supported"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_LANGUAGE, 
			"generated-natural-language-supported", "en-us", FALSE);
		}

	if(request_attr_requested(req, "document-format-default"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_MIMETYPE,
			"document-format-default", "text/plain", FALSE);
		}

	if(request_attr_requested(req, "document-format-supported"))
		{
		const char *list[] = {
			"text/plain",
			"application/postscript",
			"application/octet-stream"
			};
		ipp_add_strings(ipp, IPP_TAG_PRINTER, IPP_TAG_MIMETYPE,
			"document-format-supported", sizeof(list) / sizeof(list[0]), list, FALSE);
		}

	/* On request, PPR will attempt to override job options
	 * already selected in the job body. */
	if(request_attr_requested(req, "pdl-override-supported"))
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_KEYWORD,
			"pdl-override-supported", "attempted", FALSE);
		}
	
	/* measured in seconds */
	if(request_attr_requested(req, "printer-uptime"))
		{
		ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_INTEGER,
			"printer-uptime", time(NULL) - daemon_start_time);
		}

	request_free(req);
    } /* ipp_get_printer_attributes() */

/** Handler for IPP_GET_JOBS */
static void ipp_get_jobs(struct IPP *ipp)
	{
	const char function[] = "ipp_get_jobs";
	const char *destname = NULL;
	int destname_id = -1;
	int i;
	char fname[MAX_PPR_PATH];
	FILE *qfile;
	struct QFileEntry qfileentry;
	struct OPERATION *req;

	req = request_new(ipp);

	if((destname = request_destname(req)))
		{
		if((destname_id = destid_by_name(destname)) == -1)
			{
			request_free(req);
			ipp->response_code = IPP_NOT_FOUND;
			return;
			}
		}

	lock();

	/* Loop over the queue entries. */
	for(i=0; i < queue_entries; i++)
		{
		if(destname_id != -1 && queue[i].destid != destname_id)
			continue;

		/* Read and parse the queue file. */
		ppr_fnamef(fname, "%s/%s-%d.%d", QUEUEDIR, destid_to_name(queue[i].destid), queue[i].id, queue[i].subid);
		if(!(qfile = fopen(fname, "r")))
			{
			error("%s(): can't open \"%s\", errno=%d (%s)", function, fname, errno, gu_strerror(errno) );
			continue;
			}
		{
		int ret = read_struct_QFileEntry(qfile, &qfileentry);
		fclose(qfile);
		if(ret == -1)
			{
			error("%s(): invalid queue file: %s", function, fname);
			continue;
			}
		}

		if(request_attr_requested(req, "job-id"))
			{
			ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_INTEGER,
				"job-id", queue[i].id);
			}
		if(request_attr_requested(req, "job-printer-uri"))
			{
			ipp_add_template(ipp, IPP_TAG_JOB, IPP_TAG_URI,
				"job-printer-uri", "/printers/%s", destid_to_name(queue[i].destid));
			}
		if(request_attr_requested(req, "job-uri"))
			{
			ipp_add_template(ipp, IPP_TAG_JOB, IPP_TAG_URI,
				"job-uri", "/jobs/%d", queue[i].id);
			}

		/* Derived from "ppop lpq" */
		if(request_attr_requested(req, "job-name"))
			{
			const char *ptr;
			if(!(ptr = qfileentry.lpqFileName))
				ptr = qfileentry.Title;
			if(ptr)
				ipp_add_string(ipp, IPP_TAG_JOB, IPP_TAG_NAME, "job-name", gu_strdup(ptr), TRUE);
			}

		/* Derived from "ppop lpq" */
		if(request_attr_requested(req, "job-originating-user-name"))
			{
			const char *user;
			if(qfileentry.proxy_for)
				user = qfileentry.proxy_for;
			else if(qfileentry.For)				/* probably never false */
				user = qfileentry.For;
			else								/* probably never invoked */
				user = qfileentry.username;
			ipp_add_string(ipp, IPP_TAG_JOB, IPP_TAG_NAME, "job-originating-user-name", gu_strdup(user), TRUE);
			}

		/* Derived from "ppop lpq" */
		if(request_attr_requested(req, "job-k-octets"))
			{
			long int bytes = qfileentry.PassThruPDL ? qfileentry.attr.input_bytes : qfileentry.attr.postscript_bytes;
			ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_INTEGER, "job-k-octets", (bytes + 512) / 1024);
			}

		ipp_add_end(ipp, IPP_TAG_JOB);

		destroy_struct_QFileEntry(&qfileentry);
		}

	unlock();

	request_free(req);
	} /* ipp_get_jobs() */

/** Handler for CUPS_GET_PRINTERS */
static void cups_get_printers(struct IPP *ipp)
	{
	struct OPERATION *req;
	int i;

	req = request_new(ipp);
	
	lock();
	for(i=0; i < printer_count; i++)
		{
		if(request_attr_requested(req, "printer-name"))
			{
			ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_NAME,
				"printer-name", printers[i].name, FALSE);
			}

		if(request_attr_requested(req, "printer-uri"))
			{
			ipp_add_template(ipp, IPP_TAG_PRINTER, IPP_TAG_URI,	
				"printer-uri", "/printers/%s", printers[i].name);
			}

		printer_add_status(ipp, i, req);

		if(request_attr_requested(req, "printer-is-accepting-jobs"))
			{
			ipp_add_boolean(ipp, IPP_TAG_PRINTER, IPP_TAG_BOOLEAN,
				"printer-is-accepting-jobs", printers[i].accepting);
			}

		if(request_attr_requested(req, "device-uri"))
			{
			char fname[MAX_PPR_PATH];
			FILE *f;
			char *line = NULL;
			int line_len = 80;
			char *p;
			char *interface = NULL;
			char *address = NULL;
	
			ppr_fnamef(fname, "%s/%s", PRCONF, printers[i].name);
			if((f = fopen(fname, "r")))
				{
				while((line = gu_getline(line, &line_len, f)))
					{
					if(gu_sscanf(line, "Interface: %S", &p) == 1)
						{
						if(interface)
							gu_free(interface);
						interface = p;
						}
					if(gu_sscanf(line, "Address: %A", &p) == 1)
						{
						if(address)
							gu_free(address);
						address = p;
						}
					}
				fclose(f);
		
				if(interface && address)
					{
					ipp_add_printf(ipp, IPP_TAG_PRINTER, IPP_TAG_URI, "device-uri",
						"%s:%s",
						address[0] == '/' ? "file" : interface,		/* for benefit of CUPS lpc */
						address);
					}
		
				if(interface)
					gu_free(interface);
				if(address)
					gu_free(address);
				}
			} /* device-uri */

		ipp_add_end(ipp, IPP_TAG_PRINTER);
		}
	unlock();

	request_free(req);
	}
	
/* CUPS_GET_CLASSES */
static void cups_get_classes(struct IPP *ipp)
	{
	int i, i2;
	const char *members[MAX_GROUPSIZE];
	struct OPERATION *req;
	req = request_new(ipp);
	lock();
	for(i=0; i < group_count; i++)
		{
		if(request_attr_requested(req, "printer-name"))
			{
			ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_NAME,
				"printer-name", groups[i].name, FALSE);
			}
		if(request_attr_requested(req, "printer-uri"))
			{
			ipp_add_template(ipp, IPP_TAG_PRINTER, IPP_TAG_URI,
				"printer-uri", "/classes/%s", groups[i].name);
			}
		if(request_attr_requested(req, "printer-is-accepting-jobs"))
			{
			ipp_add_boolean(ipp, IPP_TAG_PRINTER, IPP_TAG_BOOLEAN,
				"printer-is-accepting-jobs", groups[i].accepting);
			}
		if(request_attr_requested(req, "member-names"))
			{
			for(i2=0; i2 < groups[i].members; i2++)
				members[i2] = printers[groups[i].printers[i2]].name;
			ipp_add_strings(ipp, IPP_TAG_PRINTER, IPP_TAG_NAME, "member-names", groups[i].members, members, FALSE);
			}
		ipp_add_end(ipp, IPP_TAG_PRINTER);
		}
	unlock();
	request_free(req);
	} /* cups_get_classes() */

void ipp_dispatch(const char command[])
	{
	const char function[] = "ipp_dispatch";
	char fname_in[MAX_PPR_PATH];
	char fname_out[MAX_PPR_PATH];
	const char *p;
	long int ipp_cgi_pid;
	int in_fd, out_fd;
	struct stat statbuf;
	char *command_scratch = NULL;
	struct IPP *ipp = NULL;

	debug("%s(): %s", function, command);
	
	if(!(p = lmatchsp(command, "IPP")))
		{
		error("%s(): command missing", function);
		return;
		}

	if((ipp_cgi_pid = atol(p)) == 0)
		{
		error("%s(): no PID for reply", function);
		return;
		}
	p += strspn(p, "0123456789");
	p += strspn(p, " ");

	in_fd = out_fd = -1;
	gu_Try
		{
		ppr_fnamef(fname_in, "%s/ppr-ipp/%ld-in", TEMPDIR, ipp_cgi_pid);
		if((in_fd = open(fname_in, O_RDONLY)) == -1)
			gu_Throw("can't open \"%s\", errno=%d (%s)", fname_in, errno, gu_strerror(errno));
		gu_set_cloexec(in_fd);
		if(fstat(in_fd, &statbuf) == -1)
			gu_Throw("fstat(%d, &statbuf) failed, errno=%d (%s)", in_fd, errno, gu_strerror(errno));

		ppr_fnamef(fname_out, "%s/ppr-ipp/%ld-out", TEMPDIR, ipp_cgi_pid);
		if((out_fd = open(fname_out, O_WRONLY | O_EXCL | O_CREAT, UNIX_660)) == -1)
			gu_Throw("can't create \"%s\", errno=%d (%s)", fname_out, errno, gu_strerror(errno));
		gu_set_cloexec(out_fd);

		{
		char *root=NULL, *path_info=NULL, *remote_user=NULL, *remote_addr=NULL;
		char *opts = command_scratch = gu_strdup(p);
		char *name, *value;
		while((name = gu_strsep(&opts, " ")) && *name)
			{
			if(!(value = strchr(name, '=')))
				gu_Throw("parse error, no = in \"%s\"", name);
			*(value++) = '\0';

			if(strcmp(name, "ROOT") == 0)
				root = value;
			else if(strcmp(name, "PATH_INFO") == 0)
				path_info = value;
			else if(strcmp(name, "REMOTE_USER") == 0)
				remote_user = value;
			else if(strcmp(name, "REMOTE_ADDR") == 0)
				remote_addr = value;
			else
				debug("%s(): unknown parameter %s=\"%s\"", function, name, value);
			}

		/* Create an IPP object and read the request from the temporary file. */
		ipp = ipp_new(root, path_info, statbuf.st_size, in_fd, out_fd);
		if(remote_user)
			ipp_set_remote_user(ipp, remote_user);
		if(remote_addr)
			ipp_set_remote_addr(ipp, remote_addr);
		ipp_parse_request_header(ipp);
		ipp_parse_request_body(ipp);
		}
		
		/* For now, English is all we are capable of. */
		if(ipp_validate_request(ipp))
			{
			debug("%s(): dispatching operation 0x%.2x", function, ipp->operation_id);
			switch(ipp->operation_id)
				{
				case IPP_GET_PRINTER_ATTRIBUTES:
					ipp_get_printer_attributes(ipp);
					break;
				case IPP_GET_JOBS:
					ipp_get_jobs(ipp);
					break;
				case CUPS_GET_CLASSES:
					cups_get_classes(ipp);
					break;
				case CUPS_GET_PRINTERS:
					cups_get_printers(ipp);
					break;
				default:
					gu_Throw("unsupported operation");
					break;
				}
			}

		if(ipp->response_code == IPP_OK)
			ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_TEXT, "status-message", "successful-ok", FALSE);

		ipp_send_reply(ipp, FALSE);
		}
	gu_Final {
		if(ipp)
			ipp_delete(ipp);
		if(command_scratch)
			gu_free(command_scratch);
		if(in_fd != -1)
			close(in_fd);
		if(out_fd != -1)
			close(out_fd);

		/* Close the output file and tell the IPP CGI program to take it away. */
		debug("Sending signal to IPP CGI...");
		if(kill((pid_t)ipp_cgi_pid, SIGUSR1) == -1)
			{
			debug("%s(): kill(%ld, SIGUSR1) failed, errno=%d (%s), deleting reply file", function, (long)ipp_cgi_pid, errno, gu_strerror(errno));
			unlink(fname_in);
			unlink(fname_out);
			}
		}
	gu_Catch {
		error("%s(): %s", function, gu_exception);
		}

	debug("%s(): done", function);
	} /* end of ipp_dispatch() */

/* end of file */
