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
** Last modified 4 February 2004.
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
#include "pool.h"
#include "pstring.h"
#include "ipp_constants.h"
#include "ipp_utils.h"
#include "pprd.h"
#include "pprd.auto_h"

void ipp_dispatch(const char command[])
	{
	const char function[] = "ipp_dispatch";
	char fname[MAX_PPR_PATH];
	const char *p;
	long int ipp_cgi_pid;
	int in_fd, out_fd;
	struct stat statbuf;
	pool subpool;
	struct IPP *ipp = NULL;

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

	in_fd = out_fd = -1;
	gu_Try
		{
		ppr_fnamef(fname, "%s/ppr-ipp-%ld-in", TEMPDIR, ipp_cgi_pid);
		if((in_fd = open(fname, O_RDONLY)) == -1)
			gu_Throw("can't open \"%s\", errno=%d (%s)", fname, errno, gu_strerror(errno));
		gu_set_cloexec(in_fd);
		if(fstat(in_fd, &statbuf) == -1)
			gu_Throw("fstat(%d, &statbuf) failed, errno=%d (%s)", in_fd, errno, gu_strerror(errno));
		ppr_fnamef(fname, "%s/ppr-ipp-%ld-out", TEMPDIR, ipp_cgi_pid);
		if((out_fd = open(fname, O_WRONLY | O_EXCL | O_CREAT, S_IRUSR | S_IWUSR, UNIX_600)) == -1)
			gu_Throw("can't create \"%s\", errno=%d (%s)", fname, errno, gu_strerror(errno));
		gu_set_cloexec(out_fd);
		}
	gu_Catch
		{
		if(in_fd != -1)
			close(in_fd);
		if(out_fd != -1)
			close(out_fd);
		error("%s(): %s", function, gu_exception);
		return;
		}

	subpool = new_subpool(global_pool);

	gu_Try {
		char *path_info = NULL;
		{
		char *opts = pstrdup(subpool, p);
		char *name, *value;
		while((name = gu_strsep(&opts, " ")))
			{
			if(!(value = strchr(name, '=')))
				gu_Throw("parse error");
			*(value++) = '\0';

			if(strcmp(name, "PATH_INFO") == 0)
				path_info = value;
			else
				debug("%s(): unknown parameter %s=\"%s\"", function, name, value);
			}
		}

		/* Create an IPP object and read the request from the temporary file. */
		ipp = ipp_new(path_info, statbuf.st_size, in_fd, out_fd);
		ipp_parse_request(ipp);
		
		/* For now, English is all we are capable of. */
		ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_CHARSET, "attributes-charset", "utf-8");
		ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE, "natural-language", "en");

		debug("%s(): dispatching operation 0x%.2x", function, ipp->operation_id);
		switch(ipp->operation_id)
			{




			default:
				gu_Throw("unsupported operation");
				break;
			}

		if(ipp->response_code == IPP_OK)
			ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_TEXT, "status-message", "successful-ok");

		ipp_send_reply(ipp);
		}
	gu_Final {
		/* Close the output file and tell the ipp CGI to take it away. */
		close(out_fd);
		if(kill((pid_t)ipp_cgi_pid, SIGUSR1) == -1)
			{
			debug("%s(): kill(%ld, SIGUSR1) failed, errno=%d (%s), deleting reply file", function, (long)ipp_cgi_pid, errno, gu_strerror(errno));
			unlink(fname);
			}

		/* Close and deallocate everything else. */
		close(in_fd);
		delete_pool(subpool);
		if(ipp)
			ipp_delete(ipp);
		}
	gu_Catch {
		error("%s(): %s", function, gu_exception);
		}
	} /* end of ipp_dispatch() */

/* end of file */
