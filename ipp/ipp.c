/*
** mouse:~ppr/src/ipp/ipp.c
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
** Last modified 1 March 2005.
*/

#include "config.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "ipp_constants.h"
#include "ipp_utils.h"

#if 1
#define DEBUG(a) debug a
#else
#define DEBUG(a)
#endif

/* Are we using a real FIFO or just an append to a file? */
#ifdef HAVE_MKFIFO
#define FIFO_OPEN_FLAGS (O_WRONLY | O_NONBLOCK)
#else
#define FIFO_OPEN_FLAGS (O_WRONLY | O_APPEND)
#endif

static const char *uri_basename(const char uri[])
	{
	const char *p;
	if((p = strrchr(uri, '/')))
		return p + 1;
	else
		gu_Throw("URI \"%s\" has no basename", uri);
	}

static void do_print_job(struct IPP *ipp)
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
	args[iii++] = "-f";
	args[iii++] = for_whom;
	args[iii++] = "--proxy-for";
	args[iii++] = for_whom;

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

			execv(PPR_PATH, args);
	
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
			DEBUG(("Got %d bytes", read_len));
			while(read_len > 0)
				{
				if((write_len = write(toppr_fds[1], p, read_len)) < 0)
					gu_Throw("write() failed, errno=%d (%s)", errno, gu_strerror(errno));
				DEBUG(("Wrote %d bytes", write_len));
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
		ipp_add_string(ipp, IPP_TAG_JOB, IPP_TAG_NAME, "job-state", "pending", FALSE);
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
	char fname_dir[MAX_PPR_PATH];
	char fname_in[MAX_PPR_PATH];
	char fname_out[MAX_PPR_PATH];
	long int pid;
	
	DEBUG(("do_passthru()"));

	pid = (long int)getpid();
	gu_snprintf(fname_dir, sizeof(fname_dir), "%s/ppr-ipp", TEMPDIR);
	mkdir(fname_dir, UNIX_770);
	gu_snprintf(fname_in,  sizeof(fname_in),  "%s/%ld-in",  fname_dir, pid);
	gu_snprintf(fname_out, sizeof(fname_out), "%s/%ld-out", fname_dir, pid);

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
	FILE *f;
	gu_boolean found = FALSE;

	if((f = fopen(ALIASCONF"/default", "r")))
		{
		char *line = NULL;
		int line_len = 80;
		char *p;
		while((line = gu_getline(line, &line_len, f)))
			{
			if((p = lmatchp(line, "ForWhat:")))
				{
				ipp_add_string(ipp, IPP_TAG_PRINTER, IPP_TAG_NAME, "printer-name", gu_strdup(p), TRUE);
				found = TRUE;
				break;
				}
			}
		if(line)
			gu_free(line);
		fclose(f);
		}

	if(!found)
		ipp->response_code = IPP_NOT_FOUND;
	}

int main(int argc, char *argv[])
	{
	struct IPP *ipp = NULL;
	char *root = NULL;
	void (*p_handler)(struct IPP *ipp);
	
	/* Initialize international messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	gu_Try {
		char *p, *path_info;
		int content_length;

		/*
		** This program is setuid ppr.  It would be better if it inherited all 
		** of its privledges from ppr-httpd (which runs as pprwww:ppr), but 
		** pprd can't send it SIGUSR1 from pprd unless at least the saved UID 
		** is ppr.  So it is setuid ppr, but we restore the EUID here so that
		** it can't open the pprd FIFO if an ordinary user runs it.
		*/
		if(seteuid(getuid()) == -1)
			gu_Throw("seteuid() failed");
	
		/* Do basic input validation */
		if(!(p = getenv("REQUEST_METHOD")) || strcmp(p, "POST") != 0)
			gu_Throw("REQUEST_METHOD is not POST");
		if(!(p = getenv("CONTENT_TYPE")) || strcmp(p, "application/ipp") != 0)
			gu_Throw("CONTENT_TYPE is not application/ipp");
		if(!(path_info = getenv("PATH_INFO")) || strlen(path_info) < 1)
			gu_Throw("PATH_INFO is missing");
		if(!(p = getenv("CONTENT_LENGTH")) || (content_length = atoi(p)) < 0)
			gu_Throw("CONTENT_LENGTH is missing or invalid");

		/* This is the full URL of this script. */ 
		{
		char *server, *port, *script;

		if(!(server = getenv("SERVER_NAME")))
			gu_Throw("SERVER_NAME is not defined");
		if(!(port = getenv("SERVER_PORT")))
			gu_Throw("SERVER_PORT is not defined");
		if(!(script = getenv("SCRIPT_NAME")))
			gu_Throw("SCRIPT_NAME is not defined");

		if(strcmp(script, "/") == 0)
			script = "";

		if(strcmp(port, "631") == 0)
			gu_asprintf(&root, "ipp://%s%s", server, script);
		else
			gu_asprintf(&root, "http://%s:%s%s", server, port, script);
		}
	
		/* Wrap all of this information up in an IPP object. */
		ipp = ipp_new(root, path_info, content_length, 0, 1);

		if((p = getenv("REMOTE_USER")) && *p)	/* defined and not empty */
			ipp_set_remote_user(ipp, p);
		if((p = getenv("REMOTE_ADDR")))
			ipp_set_remote_addr(ipp, p);

		ipp_parse_request_header(ipp);

		DEBUG(("dispatching operation 0x%.2x (%s)", ipp->operation_id, ipp_operation_to_str(ipp->operation_id)));
		switch(ipp->operation_id)
			{
			case IPP_PRINT_JOB:
				p_handler = do_print_job;
				break;
			case CUPS_GET_DEFAULT:
				p_handler = do_get_default;
				break;
			default:
				p_handler = NULL;
				break;
			}

		if(p_handler)		/* if we found a handler function, */
			{
			DEBUG(("handler is internal"));
			ipp_parse_request_body(ipp);

			if(ipp_validate_request(ipp))
				{
				(*p_handler)(ipp);
				if(ipp->response_code == IPP_OK)
					ipp_add_string(ipp, IPP_TAG_OPERATION, IPP_TAG_TEXT, "status-message", "successful-ok", FALSE);
				}
			}
		else
			{
			DEBUG(("passing thru to pprd"));
			do_passthru(ipp);
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
