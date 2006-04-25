/*
** mouse:~ppr/src/ipp/ipp_print.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 24 April 2006.
*/

/*
 * This is PPR's IPP (Internet Printer Protocol) server.
 * This module contains routines for accepting print jobs.
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "ipp_constants.h"
#include "ipp_utils.h"
#include "queueinfo.h"
#include "ipp.h"

struct IPP_TO_PPR {
	int value_tag;
	char *name;
	char *ppr_option;
	char *constraints;
	};

struct IPP_TO_PPR xlate_operation[] =
	{
	{IPP_TAG_NAME, "job-name", "--title", NULL},
	{IPP_TAG_NAME, "document-name", "--lpq-filename", NULL},
	{IPP_TAG_ZERO}
	};

struct IPP_TO_PPR xlate_job_template[] =
	{
	{IPP_TAG_INTEGER, "job-priority", "--ipp-priority", "1 100"},
	{IPP_TAG_INTEGER, "copies", "-n", "1 "},
	{IPP_TAG_KEYWORD, "sides", "--feature",
							"one-sided\0Duplex=None\0"
							"two-sided-long-edge\0Duplex=DuplexNoTumble\0"
						   	"two-sided-short-edge\0Duplex=DuplexTumble\0"},
	{IPP_TAG_INTEGER, "number-up", "-N", "1 16"},
	{IPP_TAG_ZERO}
	};

static char **convert_attributes(struct IPP *ipp, int group_tag, struct IPP_TO_PPR *template, char **args, int args_i, int args_space)
	{
	ipp_attribute_t *attr;
	struct IPP_TO_PPR *tp;
	for(attr = ipp->request_attrs; attr; attr = attr->next)
		{
		if(attr->group_tag != group_tag)
			break;
		for(tp=template; tp->value_tag != IPP_TAG_ZERO; tp++)
			{
			if(strcmp(tp->name, attr->name) == 0)
				{
				if(attr->value_tag != tp->value_tag || attr->num_values != 1)
					{
					debug("attribute %s bad", attr->name);
					ipp->response_code = IPP_BAD_REQUEST;
					ipp->request_attrs = NULL;		/* suppress unsupported processing */
					}
				else
					{
					switch(tp->value_tag)
						{
						case IPP_TAG_INTEGER:
							break;
						case IPP_TAG_NAME:
							break;
						case IPP_TAG_KEYWORD:
							break;
						}
					}
				break;
				}
			}
		if(!tp)
			{
			attr->group_tag = IPP_TAG_UNSUPPORTED;
			attr->value_tag = IPP_TAG_UNSUPPORTED_VALUE;
			attr->num_values = 0;
			ipp_insert_attribute(ipp, attr);
			}
		}	
	}

/*
 * Handle IPP_PRINT_JOB 
 */
void ipp_print_job(struct IPP *ipp)
	{
	struct URI *printer_uri;
	const char *destname;
	const char *requesting_user_name;
	const char *user_at_host;
	const char *args[100];			/* ppr command line */
	int args_i;
		
	if(!(printer_uri = ipp_claim_uri(ipp, IPP_TAG_OPERATION, "printer-uri")))
		{
		ipp->response_code = IPP_BAD_REQUEST;
		return;
		}
	if(!(destname = printer_uri_validate(printer_uri, NULL)))
		{
		ipp->response_code = IPP_NOT_FOUND;
		return;
		}

	requesting_user_name = ipp_claim_string(ipp, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name");
	user_at_host = ipp_user_at_host(ipp, requesting_user_name);

	/* Basic arguments */
	args_i = 0;
	args[args_i++] = PPR_PATH;
	args[args_i++] = "-d"; args[args_i++] = destname;
	args[args_i++] = "-u"; args[args_i++] = user_at_host;
	args[args_i++] = "-f"; args[args_i++] = ipp->remote_user ? ipp->remote_user : requesting_user_name;
	args[args_i++] = "--responder"; args[args_i++] = "followme";
	args[args_i++] = "--responder-address"; args[args_i++] = user_at_host;

	/* Additional arguments at the user's request. */
	gu_Try
		{
		ipp_attribute_t *attr;
		for(attr = ipp->request_attrs; attr; attr = attr->next)
			{
			if(attr->group_tag != IPP_TAG_OPERATION)
				break;
			if(strcmp(attr->name, "job-name") == 0)
				{
				if(attr->value_tag != IPP_TAG_NAME || attr->num_values != 1)
					gu_Throw(attr->name);
				args[args_i++] = "--title";
				args[args_i++] = attr->values[0].string.text;
				continue;
				}
			if(strcmp(attr->name, "document-name") == 0)
				{
				if(attr->value_tag != IPP_TAG_NAME || attr->num_values != 1)
					gu_Throw(attr->name);
				args[args_i++] = "--lpq-filename";
				args[args_i++] = attr->values[0].string.text;
				continue;
				}
			if(strcmp(attr->name, "compression") == 0)
				{

				continue;
				}
			if(strcmp(attr->name, "document-format") == 0)
				{

				continue;
				}
			attr->group_tag = IPP_TAG_UNSUPPORTED;
			attr->value_tag = IPP_TAG_UNSUPPORTED_VALUE;
			attr->num_values = 0;
			ipp_insert_attribute(ipp, attr);
			}
		for(attr = ipp->request_attrs; attr; attr = attr->next)
			{
			if(attr->group_tag != IPP_TAG_JOB)
				break;
			if(strcmp(attr->name, "job-priority") == 0)
				{
				if(attr->value_tag != IPP_TAG_INTEGER || attr->num_values != 1)
					gu_Throw(attr->name);
				if(attr->values[0].integer < 1 || attr->values[0].integer > 100)
					{
					attr->group_tag = IPP_TAG_UNSUPPORTED;
					ipp_insert_attribute(ipp, attr);
					}
				else
					{
					char *temp;		/* !!! memory leak !!! */
					gu_asprintf(&temp, "%d", attr->values[0].integer);
					args[args_i++] = "--ipp-priority";
					args[args_i++] = temp;
					}
				continue;
				}
			if(strcmp(attr->name, "copies") == 0)
				{
				if(attr->value_tag != IPP_TAG_INTEGER || attr->num_values != 1)
					gu_Throw(attr->name);
				if(attr->values[0].integer < 1)
					{
					attr->group_tag = IPP_TAG_UNSUPPORTED;
					ipp_insert_attribute(ipp, attr);
					}
				else
					{
					char *temp;		/* !!! memory leak !!! */
					gu_asprintf(&temp, "%d", attr->values[0].integer);
					args[args_i++] = "--copies";
					args[args_i++] = temp;
					}
				continue;
				}
			if(strcmp(attr->name, "sides") == 0)
				{
				if(attr->value_tag != IPP_TAG_KEYWORD || attr->num_values != 1)
					gu_Throw(attr->name);
				if(strcmp("one-sided", attr->values[0].string.text) == 0)
					{
					args[args_i++] = "--feature";
					args[args_i++] = "Duplex=None";
					}
				else if(strcmp("two-sided-long-edge", attr->values[0].string.text) == 0)
					{
					args[args_i++] = "--feature";
					args[args_i++] = "Duplex=DuplexNoTumble";
					}
				else if(strcmp("two-sided-short-edge", attr->values[0].string.text) == 0)
					{
					args[args_i++] = "--feature";
					args[args_i++] = "Duplex=DuplexTumble";
					}
				else
					{
					attr->group_tag = IPP_TAG_UNSUPPORTED;
					ipp_insert_attribute(ipp, attr);
					}
				continue;
				}
			if(strcmp(attr->name, "number-up") == 0)
				{
				if(attr->value_tag != IPP_TAG_INTEGER || attr->num_values != 1)
					gu_Throw(attr->name);
				if(attr->values[0].integer < 1 || attr->values[0].integer > 16)
					{
					attr->group_tag = IPP_TAG_UNSUPPORTED;
					ipp_insert_attribute(ipp, attr);
					}
				else
					{
					char *temp;		/* !!! memory leak !!! */
					gu_asprintf(&temp, "%d", attr->values[0].integer);
					args[args_i++] = "-N";
					args[args_i++] = temp;
					}
				continue;
				}
			attr->group_tag = IPP_TAG_UNSUPPORTED;
			attr->value_tag = IPP_TAG_UNSUPPORTED_VALUE;
			attr->num_values = 0;
			ipp_insert_attribute(ipp, attr);
			}
		/* Delete all processed attributes from the request. */
		ipp->request_attrs = attr;
		}
	gu_Catch
		{
		debug("attribute %s bad", gu_exception);
		ipp->response_code = IPP_BAD_REQUEST;
		ipp->request_attrs = NULL;		/* suppress unsupported processing */
		return;
		}

	{
	int toppr_fds[2] = {-1, -1};	/* for sending print data to ppr */
	int jobid_fds[2] = {-1, -1};	/* for ppr to send us jobid */
	gu_Try {
		pid_t pid;
		int read_len, write_len;
		char *p;
		char jobid_buf[10];
		int jobid;

		if(pipe(toppr_fds) == -1)
			gu_Throw("pipe() failed");
	
		if(pipe(jobid_fds) == -1)
			gu_Throw("pipe() failed, errno=%d (%s)", errno, gu_strerror(errno));
	
		if((pid = fork()) == -1)
			gu_Throw("fork() failed, errno=%d (%s)", errno, gu_strerror(errno));
	
		if(pid == 0)		/* child */
			{
			char fd_str[10];
	
			close(toppr_fds[1]);
			close(jobid_fds[0]);
			dup2(toppr_fds[0], 0);
			close(toppr_fds[0]);
			dup2(2, 1);
			
			snprintf(fd_str, sizeof(fd_str), "%d", jobid_fds[1]);

			args[args_i++] = "--print-id-to-fd";
			args[args_i++] = fd_str;
			args[args_i++] = NULL;

			execv(PPR_PATH, (char**)args);
	
			_exit(242);
			}
	
		/* These are the child ends.  If we don't close them here, we won't know
		 * when the child closes them.  We set them to -1 so that they won't
		 * be closed again in the gu_Final clause.
		 */
		close(toppr_fds[0]);
		toppr_fds[0] = -1;
		close(jobid_fds[1]);
		jobid_fds[1] = -1;
	
		/* Copy the job data to ppr. */
		while((read_len = ipp_get_block(ipp, &p)) > 0)
			{
			/*DEBUG(("Got %d bytes", read_len));*/
			while(read_len > 0)
				{
				if((write_len = write(toppr_fds[1], p, read_len)) < 0)
					gu_Throw("write() failed, errno=%d (%s)", errno, gu_strerror(errno));
				/*DEBUG(("Wrote %d bytes", write_len));*/
				read_len -= write_len;
				p += write_len;
				}
			}
	
		DEBUG(("Done sending job data to ppr"));

		close(toppr_fds[1]);
		toppr_fds[1] = -1;
	
		/* If the job was sucessful, ppr will have printed the jobid to our return pipe. */
		if((read_len = read(jobid_fds[0], jobid_buf, sizeof(jobid_buf))) == -1)
			gu_Throw("read() failed, errno=%d (%s)", errno, gu_strerror(errno));
		if(read_len <= 0)
			gu_Throw("read %d bytes as jobid", read_len);
		jobid_buf[read_len < sizeof(jobid_buf) ? read_len : sizeof(jobid_buf) - 1] = '\0';
		jobid = atoi(jobid_buf);
		DEBUG(("jobid is %d", jobid));
		
		/* Include the job id, both in numberic form and in URI form. */
		ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_INTEGER, "job-id", jobid);
		ipp_add_template(ipp, IPP_TAG_JOB, IPP_TAG_URI, "job-uri", "/jobs/%d", jobid);

		ipp_add_string(ipp, IPP_TAG_JOB, IPP_TAG_NAME, "job-state", "pending");
		}
	gu_Final
		{
		if(toppr_fds[0] != -1)
			close(toppr_fds[0]);
		if(toppr_fds[1] != -1)
			close(toppr_fds[1]);
		if(jobid_fds[0] != -1)
			close(jobid_fds[0]);
		if(jobid_fds[1] != -1)
			close(jobid_fds[1]);
		}
	gu_Catch
		{
		gu_ReThrow();
		}
	}
	
	} /* ipp_print_job() */

/* end of file */
