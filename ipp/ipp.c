/*
** mouse:~ppr/src/ipp/ipp.c
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
** Last modified 11 February 2004.
*/

#include "before_system.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "ipp_constants.h"
#include "ipp_utils.h"

/* Are we using a real FIFO or just an append to a file? */
#ifdef HAVE_MKFIFO
#define FIFO_OPEN_FLAGS (O_WRONLY | O_NONBLOCK)
#else
#define FIFO_OPEN_FLAGS (O_WRONLY | O_APPEND)
#endif

static const char *printer_uri(struct IPP *ipp)
	{
	ipp_attribute_t *p;
	if(!(p = ipp_find_attribute(ipp, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri")))
		gu_Throw("no printer-uri");
	return p->values[0].string.text;
	}

static const char *printer_uri_basename(struct IPP *ipp)
	{
	const char *p = printer_uri(ipp);
	if(p && (p = strrchr(p, '/')))
		return p + 1;
	return NULL;
	}

static void do_print_job(struct IPP *ipp)
	{
	const char *printer;
	pid_t pid;
	int toppr_fds[2];
	int jobid_fds[2];
		
	if(!(printer = printer_uri_basename(ipp)))
		gu_Throw("no printer name");

	if(pipe(toppr_fds) == -1)
		gu_Throw("pipe() failed");

	gu_Try
		{
		if(pipe(jobid_fds) == -1)
			gu_Throw("pipe() failed");
		gu_Try
			{
			if((pid = fork()) == -1)
				{
				gu_Throw("fork() failed, errno=%d (%s)", errno, gu_strerror(errno));
				}
			}
		gu_Catch
			{
			close(jobid_fds[0]);
			close(jobid_fds[1]);
			gu_ReThrow();
			}
		}
	gu_Catch
		{
		close(toppr_fds[0]);
		close(toppr_fds[1]);
		gu_ReThrow();
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

		execl(PPR_PATH, PPR_PATH, "-d", printer, "--print-id-to-fd", fd_str, NULL);

		_exit(242);
		}

	/* parent */
	{
	int len, write_len;
	char *ptr;
	char buf[10];
	int jobid;

	close(toppr_fds[0]);
	close(jobid_fds[1]);
	
	gu_Try
		{
		gu_Try
			{
			/* Copy the job data to ppr. */
			while((len = ipp_get_block(ipp, &ptr)) > 0)
				{
				debug("Got %d bytes", len);
				while(len > 0)
					{
					if((write_len = write(toppr_fds[1], ptr, len)) < 0)
						gu_Throw("write() failed, errno=%d (%s)", errno, gu_strerror(errno));
					debug("Wrote %d bytes", write_len);
					len -= write_len;
					ptr += write_len;
					}
				}

			debug("Done sending job data to ppr");
			}
		gu_Final
			{
			close(toppr_fds[1]);
			}
		gu_Catch
			{
			gu_ReThrow();
			}

		/* If the job was sucessful, ppr will have printed the jobid to our return pipe. */
		if((len = read(jobid_fds[0], buf, sizeof(buf))) == -1)
			gu_Throw("read() failed, errno=%d (%s)", errno, gu_strerror(errno));
		debug("read %d bytes as jobid", len);
	
		buf[len < sizeof(buf) ? len : sizeof(buf) - 1] = '\0';
		jobid = atoi(buf);
		debug("jobid is %d", jobid);
		}
	gu_Final
		{
		close(jobid_fds[0]);
		}
	gu_Catch
		{
		gu_ReThrow();
		}
		
	if(jobid > 0)
		{	
		/* Include the job id, both in numberic form and in URI form. */
		ipp_add_integer(ipp, IPP_TAG_JOB, IPP_TAG_INTEGER, "job-id", jobid);
		{
		char *p;
		gu_asprintf(&p, "%s/%d", printer_uri(ipp), jobid);
		ipp_add_string(ipp, IPP_TAG_JOB, IPP_TAG_URI, "job-uri", p);
		}
	
		ipp_add_string(ipp, IPP_TAG_JOB, IPP_TAG_NAME, "job-state", "pending");
		}
	else
		{
		ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_TEXT, "status-message", "successful-ok");
		}

	}
	} /* end of do_print_job() */

static volatile gu_boolean sigcaught;
static void user_sighandler(int sig)
	{
	sigcaught = TRUE;
	}

static volatile gu_boolean timeout;
static void alarm_sighandler(int sig)
	{
	timeout = TRUE;
	}

static void do_passthru(struct IPP *ipp)
	{
	int fifo = -1;
	int fd = -1;
	sigset_t set, oset;
	char fname_in[MAX_PPR_PATH];
	char fname_out[MAX_PPR_PATH];
	long int pid;
	
	debug("do_passthru()");

	pid = (long int)getpid();
	gu_snprintf(fname_in,  sizeof(fname_in),  "%s/ppr-ipp-%ld-in",  TEMPDIR, pid);
	gu_snprintf(fname_out, sizeof(fname_out), "%s/ppr-ipp-%ld-out", TEMPDIR, pid);

	gu_Try {
		if((fifo = open(FIFO_NAME, FIFO_OPEN_FLAGS)) < 0)
			gu_Throw(_("can't open FIFO, pprd is probably not running."));

		if((fd = open(fname_in, O_WRONLY | O_CREAT | O_EXCL, UNIX_660)) == -1)
			gu_Throw(_("can't create \"%s\", errno=%d (%s)"), fname_in, errno, gu_strerror(errno));

		ipp_request_to_fd(ipp, fd);

		close(fd);
		fd = -1;
		
		signal(SIGUSR1, user_sighandler);
		signal(SIGALRM, alarm_sighandler);

		sigemptyset(&set);
		sigaddset(&set, SIGUSR1);
		sigaddset(&set, SIGALRM);
		sigprocmask(SIG_BLOCK, &set, &oset);
		
		sigcaught = FALSE;
		timeout = FALSE;
	
		{
		char command[256];
		int command_len;
		int ret;
		gu_snprintf(command, sizeof(command),
			"IPP %ld ROOT=%s PATH_INFO=%s REMOTE_USER=%s REMOTE_ADDR=%s\n",
			pid,
			ipp->root,
			ipp->path_info,
			ipp->remote_user ? ipp->remote_user : "",
			ipp->remote_addr ? ipp->remote_addr : ""
			);
		command_len = strlen(command);

		if((ret = write(fifo, command, command_len)) == -1)
			gu_Throw("write to FIFO failed, errno=%d (%s)", errno, gu_strerror(errno));
		}
		
		alarm(60);

		while(!sigcaught && !timeout)
			sigsuspend(&oset);
	
		alarm(0);
	
		if(timeout)
			gu_Throw("timeout waiting for pprd");

		if((fd = open(fname_out, O_RDONLY)) == -1)
			gu_Throw("can't open \"%s\", errno=%d (%s)", fname_out, errno, gu_strerror(errno));

		ipp_reply_from_fd(ipp, fd);
		fd = -1;
		}

	gu_Final {
		sigprocmask(SIG_SETMASK, &oset, (sigset_t*)NULL);

		unlink(fname_in);
		unlink(fname_out);

		if(fd != -1)
			close(fd);
		
		if(fifo != -1)
			close(fifo);
		}

	gu_Catch {
		gu_ReThrow();
		}
	} /* end of do_passthru() */

static void do_get_default(struct IPP *ipp)
	{
	ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_NAME, "printer-name", "default");
	}

int main(int argc, char *argv[])
	{
	struct IPP *ipp = NULL;
	char *root = NULL;

	/* Leave only the saved UID as "ppr". */
	seteuid(getuid());
	
	/* Initialize international messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	gu_Try {
		char *p, *path_info;
		int content_length;

		/* Do basic input validation */
		if(!(p = getenv("REQUEST_METHOD")) || strcmp(p, "POST") != 0)
			gu_Throw("REQUEST_METHOD is not POST");
		if(!(p = getenv("CONTENT_TYPE")) || strcmp(p, "application/ipp") != 0)
			gu_Throw("CONTENT_TYPE is not application/ipp");
		if(!(path_info = getenv("PATH_INFO")) || strlen(path_info) < 1)
			gu_Throw("PATH_INFO is missing");
		if(!(p = getenv("CONTENT_LENGTH")) || (content_length = atoi(p)) < 0)
			gu_Throw("CONTENT_LENGTH is missing or invalid");

		gu_asprintf(&root, "http://%s:%s/%s",
			(p = getenv("SERVER_NAME")) ? p : "localhost",
			(p = getenv("SERVER_PORT")) ? p : "15010",
			(p = getenv("SCRIPT_NAME")) ? p : "ipp"
			);
		
		ipp = ipp_new(root, path_info, content_length, 0, 1);

		if((p = getenv("REMOTE_USER")))
			ipp_set_remote_user(ipp, p);
		if((p = getenv("REMOTE_ADDR")))
			ipp_set_remote_addr(ipp, p);

		ipp_parse_request_header(ipp);

		debug("dispatching operation 0x%.2x", ipp->operation_id);
		switch(ipp->operation_id)
			{
			case CUPS_GET_CLASSES:
			case CUPS_GET_PRINTERS:
			case IPP_GET_PRINTER_ATTRIBUTES:
			case IPP_GET_JOBS:
				do_passthru(ipp);
				break;

			default:
				ipp_parse_request_body(ipp);

				/* For now, English is all we are capable of. */
				ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_CHARSET, "attributes-charset", "utf-8");
				ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE, "natural-language", "en");

				switch(ipp->operation_id)
					{
					case IPP_PRINT_JOB:
						do_print_job(ipp);
						break;
					case CUPS_GET_DEFAULT:
						do_get_default(ipp);
						break;
					default:
						gu_Throw("unsupported operation");
						break;
					}

				if(ipp->response_code == IPP_OK)
					ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_TEXT, "status-message", "successful-ok");

				break;
			}

		ipp_send_reply(ipp, TRUE);
		}

	gu_Final {
		if(ipp)
			ipp_delete(ipp);
		if(root)
			gu_free(root);
		}

	gu_Catch
		{
		printf("Content-Type: text/plain\n");
		printf("Status: 500\n");
		printf("\n");
		printf("ipp: exception caught: %s\n", gu_exception);
		fprintf(stderr, "ipp: exception caught: %s\n", gu_exception);
		return 1;
		}

	return 0;
	}

/** Send a debug message to the HTTP server's error log

This function sends a message to stderr.  Messages sent to stderr end up in
the HTTP server's error log.  The function takes a printf() style format
string and argument list.  The marker "ipp: " is prepended to the message.

*/
void debug(const char message[], ...)
	{
	va_list va;
	va_start(va, message);
	fputs("ipp: ", stderr);
	vfprintf(stderr, message, va);
	fputc('\n', stderr);
	va_end(va);
	} /* end of debug() */

/* end of file */
