/*
** mouse:~ppr/src/ipp/ipp.c
** Copyright 1995--2003, Trinity College Computing Center.
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
** Last modified 30 July 2003.
*/

#include "before_system.h"
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "ipp_constants.h"
#include "ipp_except.h"
#include "ipp_utils.h"

struct exception_context the_exception_context[1];

static void do_print_job(struct IPP *ipp)
	{
	pid_t pid;
	int toppr_fds[2];
	int jobid_fds[2];
	const char *printer_uri;
	int jobid;
		
	{
	ipp_attribute_t *p;
	if(!(p = ipp_find_attribute(ipp, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri")))
		Throw("no printer-uri");
	printer_uri = p->values[0].string.text;
	}

	if(pipe(toppr_fds) == -1)
		Throw("pipe() failed");

	if(pipe(jobid_fds) == -1)
		Throw("pipe() failed");

	if((pid = fork()) == -1)
		{
		Throw("fork() failed");
		}

	if(pid == 0)		/* child */
		{
		char fd_str[10];

		close(toppr_fds[1]);
		close(jobid_fds[0]);
		dup2(toppr_fds[0], 0);
		close(toppr_fds[0]);
		dup2(2, 1);

		snprintf(fd_str, sizeof(fd_str), "%d", jobid_fds[1]);

		execl(PPR_PATH, PPR_PATH, "-d", "dummy", "--print-id-to-fd", fd_str, NULL);

		_exit(242);
		}

	/* parent */
	{
	int len, write_len;
	char *ptr;

	close(toppr_fds[0]);
	close(jobid_fds[1]);
	
	while((len = ipp_get_block(ipp, &ptr)) > 0)
		{
		debug("Got %d bytes", len);
		while(len > 0)
			{
			if((write_len = write(toppr_fds[1], ptr, len)) < 0)
				Throw("write() failed");
			debug("Wrote %d bytes", write_len);
			len -= write_len;
			ptr += write_len;
			}
		}
	}

	close(toppr_fds[1]);

	/* If the job was sucessful, ppr will have printed the jobid to our return pipe. */
	{
	int len;
	char buf[10];
	if((len = read(jobid_fds[0], buf, sizeof(buf))) == -1)
		Throw("read() failed");
	debug("read %d bytes as jobid", len);

	buf[len < sizeof(buf) ? len : sizeof(buf) - 1] = '\0';
	jobid = atoi(buf);
	debug("jobid is %d", jobid);

	close(jobid_fds[0]);
	}


	ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_CHARSET, "attributes-charset", "utf-8");
	ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE, "natural-language", "en");

	if(jobid > 0)
		{	
		/* Include the job id, both in numberic form and in URI form. */
		ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_INTEGER, "job-id", jobid);
		{
		char *p;
		gu_asprintf(&p, "%s/%d", printer_uri, jobid);
		ipp_add_string(ipp, IPP_TAG_JOB, IPP_TAG_URI, "job-uri", p);
		}
	
		ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_TEXT, "status-message", "successful-ok");
		ipp_add_string(ipp, IPP_TAG_JOB, IPP_TAG_NAME, "job-state", "pending");
		}
	
	} /* end of do_print_job() */

static void do_printer_attributes(struct IPP *ipp)
    {
	ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_CHARSET, "attributes-charset", "utf-8");
	ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE, "natural-language", "en");
	ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_TEXT, "status-message", "successful-ok");

	ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_URI, "printer-uri", "http://localhost:15010/cgi-bin/ipp/test");
	ipp_add_integer(ipp, IPP_TAG_PRINTER, IPP_TAG_ENUM, "printer-state", 4);
	ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_KEYWORD, "printer-state-reasons", "glug");

	ipp_add_boolean(ipp, IPP_TAG_PRINTER, IPP_TAG_BOOLEAN, "printer-is-accepting-jobs", TRUE);
	ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_MIMETYPE, "document-format-supported", "text/plain");

    }

int main(int argc, char *argv[])
	{
	const char *e;
	struct IPP *ipp;

	Try {
		ipp = ipp_new();
		ipp_parse_request(ipp);

		debug("dispatching");
		switch(ipp->operation_id)
			{
			case IPP_PRINT_JOB:
				do_print_job(ipp);
				break;
				
			case IPP_GET_PRINTER_ATTRIBUTES:
				do_printer_attributes(ipp);
				break;

			default:
				Throw("unsupported operation");
				break;
			}
		
		ipp_send_reply(ipp);
		ipp_delete(ipp);
		}

	Catch(e)
		{
		printf("Content-Type: text/plain\n");
		printf("Status: 500\n");
		printf("\n");
		printf("ipp: exception caught: %s\n", e);
		fprintf(stderr, "ipp: exception caught: %s\n", e);
		return 1;
		}

	return 0;
	}

/* end of file */
