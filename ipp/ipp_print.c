/*
** mouse:~ppr/src/ipp/ipp_print.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 26 April 2006.
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
	int value_tag;			/* type of IPP attribute value */
	char *name;				/* name of IPP attribute */
	char *ppr_option;		/* ppr(1) option to implement it */
	char *constraints;		/* rules for validating or mapping values */
	};

/* For operation attributes. */
struct IPP_TO_PPR xlate_operation[] =
	{
	{IPP_TAG_NAME, "job-name", "--title", NULL},
	{IPP_TAG_NAME, "document-name", "--lpq-filename", NULL},
	/* compression */
	/* document-format */
	{IPP_TAG_ZERO}
	};

/* For job template attributes. */
struct IPP_TO_PPR xlate_job_template[] =
	{
	{IPP_TAG_INTEGER, "job-priority", "--ipp-priority", "1 100"},
	{IPP_TAG_INTEGER, "copies", "--copies", "1 "},
	{IPP_TAG_KEYWORD, "sides", "--feature",		/* feature from PPD file */
							"one-sided\0Duplex=None\0"
							"two-sided-long-edge\0Duplex=DuplexNoTumble\0"
						   	"two-sided-short-edge\0Duplex=DuplexTumble\0"
							},
	{IPP_TAG_INTEGER, "number-up", "-N", "1 16"},
	{IPP_TAG_ZERO}
	};

/*
 * Driver function for above tables.  It reads IPP attributes and appends
 * options to the ppr(1) command line.
 */
static ipp_attribute_t* convert_attributes(
		struct IPP *ipp,
	   	ipp_attribute_t *attr,			/* attribute to start with */
	   	int group_tag,					/* process only attributes of this type */
	   	struct IPP_TO_PPR *template,	/* conversion template table */
	   	const char ***args, int *args_i, int *args_space
		)
	{
	struct IPP_TO_PPR *tp;		/* markes our place in the template table */

	/* Step thru IPP attributes. */	
	for( ; attr; attr = attr->next)
		{
		/* If another attribute group is starting, we are done. */
		if(attr->group_tag != group_tag)
			break;

		/* Stop thru this templates looking for a match for this attribute. */
		for(tp=template; tp->value_tag != IPP_TAG_ZERO; tp++)
			{
			if(strcmp(tp->name, attr->name) != 0)
				continue;

			/* Check for incorrect value tag or other than a single value. 
			 * Per RFC 3196 3.1.2.1.5 we reject the whole request if any
			 * of these tests fails. */
			if(attr->value_tag != tp->value_tag || attr->num_values != 1)
				{
				debug("attribute %s bad", attr->name);
				ipp->response_code = IPP_BAD_REQUEST;
				/* Suppress unsupported processing.  Otherwise any attributes
				 * which we have not yet consumed will be listed as unsupported! */
				ipp->request_attrs = NULL;
				continue;
				}

			/* Value validation and mapping. */
			{
			const char *value = NULL;	/* store value hither after validation and string conversion */
			switch(tp->value_tag)		/* validate according to value type */
				{
				case IPP_TAG_INTEGER:	/* integer within inclusive range */
					{
					int lower_bound, upper_bound;
					int matches = gu_sscanf(tp->constraints, "%d %d", &lower_bound, &upper_bound);
					if((matches >= 1 && attr->values[0].integer >= lower_bound)
							&&
						(matches == 2 && attr->values[0].integer <= upper_bound)
						)
						{
						char *temp;		/* value is const, but gu_asprintf() requires non-const */
						gu_asprintf(&temp, "%d", attr->values[0].integer);
						value = temp;
						}
					}
					break;
				case IPP_TAG_NAME:
					value = attr->values[0].string.text;
					break;
				case IPP_TAG_KEYWORD:	/* if keyword listed, take mapped value */
					{
					const char *p, *keyword, *cooresponding_value;
					/* Step thru the keywords listed in the constraints pattern
					 * of the matched template.  The end of the list is marked
					 * by an empty keyword. */
					for(p=tp->constraints; *p; )
						{
						keyword = p;
						p += strlen(p) + 1;		/* step over keyword and its terminating null */
						cooresponding_value = p;
						p += strlen(p) + 1;
						if(strcmp(keyword, attr->values[0].string.text) == 0)
							{
							value = cooresponding_value;	/* accept mapped value */
							break;
							}
						}
					}
					break;
				}

			/* If value failed constraint tests, then it will not have been 
			 * converted to a string. */
			if(!value)
				{
				/* We change the group tag but leave the value tag alone.
				 * This indicates to the client that we support the attribute
				 * but not the selected value. */
				attr->group_tag = IPP_TAG_UNSUPPORTED;
				ipp_insert_attribute(ipp, attr);	/* move attrib to response */
				break;
				}

			/* If we are out of space in the ppr arguments array, enlarge it. */
			if((*args_i + 2) >= *args_space)
				{
				*args_space += 64;
				*args = gu_realloc(*args, *args_space, sizeof(char*));
				}
	
			/* Append the ppr option supplied in the template and the 
			 * validated (and possibly mapped) value to the ppr command 
			 * line. */
			*args[(*args_i)++] = tp->ppr_option;
			*args[(*args_i)++] = value;
			}

			/* We found it, no need to examine furthur templates. */
			break;
			} /* template loop */

		/* If the template search loop failed to find this attribute, 
		 * we do not support it at all (as opposed to just not supporting
		 * the chosen value). */
		if(!tp)
			{
			attr->group_tag = IPP_TAG_UNSUPPORTED;
			attr->value_tag = IPP_TAG_UNSUPPORTED_VALUE;
			attr->num_values = 0;
			ipp_insert_attribute(ipp, attr);
			}
		} /* attribute loop */

	return attr;
	} /* convert_attributes() */

/*
 * Handle IPP_PRINT_JOB or IPP_VALIDATE_JOB
 */
void ipp_print_job(struct IPP *ipp)
	{
	struct URI *printer_uri;
	const char *destname;
	const char *user_at_host;
	const char **args;			/* ppr command line */
	int args_i;
	int args_space;
		
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

	user_at_host = extract_identity(ipp, FALSE);

	/* Allocate some space for the PPR arguments.  At least 14 spaces
	 * will be needed. */
	args_space = 64;
	args = gu_realloc(NULL, args_space, sizeof(char*));

	/* Basic arguments */
	args_i = 0;
	args[args_i++] = PPR_PATH;
	args[args_i++] = "-d"; args[args_i++] = destname;
	args[args_i++] = "-u"; args[args_i++] = user_at_host;
	args[args_i++] = "-f"; args[args_i++] = user_at_host;
	args[args_i++] = "--responder"; args[args_i++] = "followme";
	/*args[args_i++] = "--responder-address"; args[args_i++] = user_at_host;*/

	/* Convert IPP operation and job template attributes to PPR options. 
	 * Move ipp->request_attrs ahead as we go in order to keep track of
	 * what has been handled so far. */
	ipp->request_attrs = convert_attributes(ipp, ipp->request_attrs, IPP_TAG_OPERATION, xlate_operation, &args, &args_i, &args_space);
	ipp->request_attrs  = convert_attributes(ipp, ipp->request_attrs, IPP_TAG_JOB, xlate_job_template, &args, &args_i, &args_space);

	/* If this isn't an actual print job request, but just a dry run,
	 * we have done all we need to do, bail out. */
	if(ipp->operation_id == IPP_VALIDATE_JOB)
		return;

	/* Set up a pipe and launch PPR on the end of it. */
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
			
			gu_snprintf(fd_str, sizeof(fd_str), "%d", jobid_fds[1]);

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
