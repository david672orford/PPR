/*
** mouse:~ppr/src/ipp/ipp_print.c
** Copyright 1995--2006, Trinity College Computing Center.
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
** Last modified 12 April 2006.
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

/*
 * Given a URI, return the base filename without path.  We use this as a
 * crude way of extracting the queue name from a URI.
 */
static const char *uri_basename(const char uri[])
	{
	const char *p;
	if((p = strrchr(uri, '/')))
		return p + 1;
	else
		gu_Throw("URI \"%s\" has no basename", uri);
	}

/*
 * Handle IPP_PRINT_JOB 
 */
void ipp_print_job(struct IPP *ipp)
	{
	const char *printer_uri = NULL;
	const char *username = NULL;
	const char *args[100];			/* ppr command line */
	char for_whom[64];
	int iii = 0;
		
	{
	ipp_attribute_t *attr;
	for(attr = ipp->request_attrs; attr; attr = attr->next)
		{
		if(attr->group_tag != IPP_TAG_OPERATION)
			continue;
		
		if(attr->value_tag == IPP_TAG_URI && strcmp(attr->name, "printer-uri") == 0)
			printer_uri = attr->values[0].string.text;
		else if(attr->value_tag == IPP_TAG_NAME && strcmp(attr->name, "requesting-user-name") == 0)
			username = attr->values[0].string.text;
		else
			ipp_copy_attribute(ipp, IPP_TAG_UNSUPPORTED, attr);
		}
	}

	if(!printer_uri)
		{
		ipp->response_code = IPP_BAD_REQUEST;
		return;
		}
	
	snprintf(for_whom, sizeof(for_whom),
		"%s@%s",
		ipp->remote_user ? ipp->remote_user : username, 
		ipp->remote_addr ? ipp->remote_addr : "?"
		);

	args[iii++] = PPR_PATH;
	args[iii++] = "-d";
	args[iii++] = uri_basename(printer_uri);
	args[iii++] = "-u";
	args[iii++] = for_whom;
	args[iii++] = "--responder";
	args[iii++] = "followme";
	args[iii++] = "--responder-address";
	args[iii++] = ipp->remote_user ? ipp->remote_user : username;

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

			args[iii++] = "--print-id-to-fd";
			args[iii++] = fd_str;
			args[iii++] = NULL;

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
		ipp_add_printf(ipp, IPP_TAG_JOB, IPP_TAG_URI, "job-uri", "%s/%d", printer_uri, jobid);
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
