/*
** mouse:~ppr/src/pprd/pprd_ipp.c
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
** Last modified 10 December 2004.
*/

/*
 * In this module we handle Internet Printing Protocol requests which have
 * been received by ppr-httpd and passed on to us by the ipp CGI program.
 *
 * These requests arrive as an "IPP" line which specifies the PID of
 * the ipp CGI and a list of HTTP headers and CGI variables.  The POSTed
 * data is in a temporary file (whose name can be derived from the PID)
 * and we send the response document to another temporary file.  Then
 * we send SIGUSR1 to the ipp CGI to tell it that the response is ready.
 */

#include "before_system.h"
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
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

static void printer_add_status(struct IPP *ipp, int printer)
	{
	const char function[] = "printer_add_status";
	switch(printers[printer].status)
		{
		case PRNSTATUS_IDLE:
			ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
				"printer-state", IPP_PRINTER_IDLE);
			break;
		case PRNSTATUS_PRINTING:
			ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
				"printer-state", IPP_PRINTER_PROCESSING);
			ipp_add_printf(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
				"printer-state-message", _("printing %s"), "???");
			break;
		case PRNSTATUS_CANCELING:
			ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
				"printer-state", IPP_PRINTER_PROCESSING);
			ipp_add_printf(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
				"printer-state-message", _("canceling %s"), "???");
			break;
		case PRNSTATUS_SEIZING:
			ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
				"printer-state", IPP_PRINTER_PROCESSING);
			ipp_add_printf(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
				"printer-state-message", _("seizing %s"), "???");
			break;
		case PRNSTATUS_STOPPING:
			ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
				"printer-state", IPP_PRINTER_PROCESSING);
			ipp_add_printf(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
				"printer-state-message", _("stopping (printing %s)"), "???");
			break;
		case PRNSTATUS_HALTING:
			ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
				"printer-state", IPP_PRINTER_PROCESSING);
			ipp_add_printf(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
				"printer-state-message", _("halting (printing %s)"), "???");
			break;
		case PRNSTATUS_STOPT:
			ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
				"printer-state", IPP_PRINTER_STOPPED);
			ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
				"printer-state-message", _("stopt"), FALSE);
			break;
		case PRNSTATUS_FAULT:
			ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
				"printer-state", IPP_PRINTER_STOPPED);
			if(1)
				{
				ipp_add_printf(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
					"printer-state-message", _("fault, retry %d in %d seconds"), 1, 1);
				}
			else
				{
				ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
					"printer-state-message", _("fault, no auto retry"), FALSE);
				}
			break;
		case PRNSTATUS_ENGAGED:
			ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
				"printer-state", IPP_PRINTER_STOPPED);
			ipp_add_printf(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
				"printer-state-message", _("otherwise engaged or off-line, retry %d in %d seconds"), 1, 2);
			break;
		case PRNSTATUS_STARVED:
			ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM,
				"printer-state", IPP_PRINTER_STOPPED);
			ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_TEXT,
				"printer-state-message", _("waiting for resource ration"), FALSE);
			break;

		default:
			error("%s(): invalid printer_status %d", function, printers[printer].status);
			break;
		}
	}

/* IPP_GET_PRINTER_ATTRIBUTES */
static void ipp_get_printer_attributes(struct IPP *ipp)
    {
	ipp_attribute_t *printer_uri;
	const char *printer_uri_value;

	if(!(printer_uri = ipp_find_attribute(ipp, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri")))
		{
		ipp->response_code = IPP_NOT_FOUND;
		return;
		}
	printer_uri_value = printer_uri->values[0].string.text;
		
	lock();

	do	{
		char *p;
		if((p = strstr(printer_uri_value, "/printers/")))
			{
			const char *queue_name = p + sizeof("/printers/") - 1;
			int i;
	
			for(i=0; printer_count; i++)
				{
				if(strcmp(printers[i].name, queue_name) == 0)
					break;
				}
	
			if(i == printer_count)
				{
				ipp->response_code = IPP_NOT_FOUND;
				break;
				}

			ipp_add_template(ipp, IPP_TAG_PRINTER, IPP_TAG_URI, "printer-uri", "/printers/%s", queue_name);
			/* ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM, "printer-state", 4); */
			/* ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_KEYWORD, "printer-state-reasons", "glug", FALSE); */
			ipp_add_boolean(ipp, IPP_TAG_PRINTER, IPP_TAG_BOOLEAN, "printer-is-accepting-jobs", groups[i].accepting);
			/* ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_MIMETYPE, "document-format-supported", "text/plain", FALSE); */
			}
		else
			{
			ipp->response_code = IPP_NOT_FOUND;
			}
		} while(FALSE);

	unlock();
    }

/* IPP_GET_JOBS */
static void ipp_get_jobs(struct IPP *ipp)
	{
	const char function[] = "ipp_get_jobs";
	int i;
	char fname[MAX_PPR_PATH];
	FILE *qfile;
	struct QFileEntry qfileentry;

	lock();

	/* Loop over the queue entries. */
	for(i=0; i < queue_entries; i++)
		{
		if(queue[i].destnode_id != nodeid_local())		/* delete in 2.x */
			continue;

		/* Read and parse the queue file. */
		ppr_fnamef(fname, "%s/%s:%s-%d.%d(%s)", QUEUEDIR,
			nodeid_to_name(queue[i].destnode_id),
			destid_to_name(queue[i].destnode_id, queue[i].destid),
			queue[i].id,
			queue[i].subid,
			nodeid_to_name(queue[i].homenode_id));
		if(!(qfile = fopen(fname, "r")))
			{
			error("%s(): can't open \"%s\", errno=%d (%s)", function, fname, errno, gu_strerror(errno) );
			continue;
			}
		if(read_struct_QFileEntry(qfile, &qfileentry) == -1)
			{
			error("%s(): invalid queue file: %s", function, fname);
			continue;
			}

		ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_INTEGER, "job-id", queue[i].id);
		ipp_add_template(ipp, IPP_TAG_JOB, IPP_TAG_URI, "job-printer-uri", "/printers/%s", destid_to_name(queue[i].destnode_id, queue[i].destid));
		ipp_add_template(ipp, IPP_TAG_JOB, IPP_TAG_URI, "job-uri", "/jobs/%d", queue[i].id);

		/* Derived from "ppop lpq" */
		{
		const char *ptr;
		if(!(ptr = qfileentry.lpqFileName))
			ptr = qfileentry.Title;
		if(ptr)
			ipp_add_string(ipp, IPP_TAG_JOB, IPP_TAG_NAME, "job-name", gu_strdup(ptr), TRUE);
		}

		/* Derived from "ppop lpq" */
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
		{
		long int bytes = qfileentry.PassThruPDL ? qfileentry.attr.input_bytes : qfileentry.attr.postscript_bytes;
		ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_INTEGER, "job-k-octets", (bytes + 512) / 1024);
		}

		ipp_add_end(ipp, IPP_TAG_JOB);

		destroy_struct_QFileEntry(&qfileentry);
		}

	unlock();
	}

/* CUPS_GET_PRINTERS */
static void cups_get_printers(struct IPP *ipp)
	{
	int i;
	lock();
	for(i=0; i < printer_count; i++)
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_NAME,
			"printer-name", printers[i].name, FALSE);
		ipp_add_template(ipp, IPP_TAG_PRINTER, IPP_TAG_URI,
			"printer-uri", "/printers/%s", printers[i].name);

		printer_add_status(ipp, i);

		ipp_add_boolean(ipp, IPP_TAG_PRINTER, IPP_TAG_BOOLEAN,
			"printer-is-accepting-jobs", printers[i].accepting);

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
					
			ipp_add_end(ipp, IPP_TAG_PRINTER);
			}
		}

	unlock();
	}
	
/* CUPS_GET_CLASSES */
static void cups_get_classes(struct IPP *ipp)
	{
	int i, i2;
	const char *members[MAX_GROUPSIZE];
	lock();
	for(i=0; i < group_count; i++)
		{
		ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_NAME, "printer-name", groups[i].name, FALSE);
		ipp_add_template(ipp, IPP_TAG_PRINTER, IPP_TAG_URI, "printer-uri", "/classes/%s", groups[i].name);
		ipp_add_boolean(ipp, IPP_TAG_PRINTER, IPP_TAG_BOOLEAN, "printer-is-accepting-jobs", groups[i].accepting);
		for(i2=0; i2 < groups[i].members; i2++)
			members[i2] = printers[groups[i].printers[i2]].name;
		ipp_add_strings(ipp, IPP_TAG_PRINTER, IPP_TAG_NAME, "member-names", groups[i].members, members, FALSE);
		ipp_add_end(ipp, IPP_TAG_PRINTER);
		}
	unlock();
	}

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
		ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_CHARSET, "attributes-charset", "utf-8", FALSE);
		ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE, "natural-language", "en", FALSE);

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
