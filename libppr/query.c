/*
** mouse:~ppr/src/libppr/query.c
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
** Last modified 16 October 2003.
*/

#include "before_system.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <sys/wait.h>
#include "gu.h"
#include "global_defines.h"
#include "interface.h"
#include "libppr_query.h"

/*! \file
	\brief print query routines

This file contains routines for sending queries to printers.

*/

/** create a query object with interface and address

*/
struct QUERY *query_new_byaddress(const char interface[], const char address[])
	{
	struct QUERY *q;

	if(!interface)
		gu_Throw("No interface specified");
	if(!address)
		gu_Throw("No address specified");

	q = gu_alloc(1, sizeof(struct QUERY));

	q->interface = interface;
	q->address = address;
	q->line = NULL;
	q->line_len = 128;
	q->connected = FALSE;

	q->control_d = (strcmp(interface, "atalk") == 0) ? FALSE : TRUE;

	return q;
	}

/** create a query object from an existing printer

*/
struct QUERY *query_new_byprinter(const char printer[])
	{
	char fname[MAX_PPR_PATH];
	FILE *f;
	char *line = NULL;
	int line_len = 128;
	char *interface = NULL;
	char *address = NULL;
	char *tptr;

	if(!printer)
		gu_Throw("No printer specified");

	ppr_fnamef(fname, "%s/%s", PRCONF, printer);
	if(!(f = fopen(fname, "r")))
		gu_Throw("Can't open printer configuration");

	while((line = gu_getline(line, &line_len, f)))
		{
		if(gu_sscanf(line, "Interface: %S", &tptr) == 1)
			{
			if(interface) gu_free(interface);
			interface = tptr;
			}
		else if(gu_sscanf(line, "Address: %A", &tptr) == 1)
			{
			if(address) gu_free(address);
			address = tptr;
			}
		}

	fclose(f);		/* close printer configuration file */

	gu_Try {
		if(!interface)
			gu_Throw("interface is NULL");
		if(!address)
			gu_Throw("address is NULL");
		}
	gu_Catch
		{
		if(interface)
			gu_free(interface);
		if(address)
			gu_free(address);
		gu_ReThrow();
		}

	return query_new_byaddress(interface, address);
	}

/** destroy a query object

*/
void query_delete(struct QUERY *q)
	{
	if(!q)
		gu_Throw("query_delete(NULL)!");
	if(q->connected)
		query_disconnect(q);
	if(q->line)
		gu_free(q->line);
	gu_free(q);
	} /* end of query_delete() */

/** connect to the printer

*/
void query_connect(struct QUERY *q, gu_boolean probe)
	{
	if(q->connected)
		gu_Throw("already connected");

	q->buf_stdin_len = q->buf_stdout_len = q->buf_stderr_len = 0;
	q->buf_stdout_eaten = q->buf_stderr_eaten = 0;
	q->eof_stdout = q->eof_stderr = FALSE;
	q->job_started = FALSE;
	q->disconnecting = FALSE;
	q->last_stdout_crlf = 0;

	/* Open three pipes for the interface program's stdin, stdout, and stderr. */
	gu_Try {
		if(pipe(q->pipe_stdin) == -1)
			{
			gu_Throw("pipe() failed, errno=%d (%s)", errno, gu_strerror(errno));
			}
		gu_Try {
			if(pipe(q->pipe_stdout) == -1)
				{
				gu_Throw("pipe() failed, errno=%d (%s)", errno, gu_strerror(errno));
				}
			gu_Try {
				if(pipe(q->pipe_stderr) == -1)
					gu_Throw("pipe() failed, errno=%d (%s)", errno, gu_strerror(errno));

				q->maxfd = 0;
				if(q->pipe_stdin[1] > q->maxfd)
					q->maxfd = q->pipe_stdin[1];
				if(q->pipe_stdout[0] > q->maxfd)
					q->maxfd = q->pipe_stdout[1];
				if(q->pipe_stderr[0] > q->maxfd)
					q->maxfd = q->pipe_stderr[0];

				switch(fork())
					{
					case -1:							/* fork failed */
						gu_Throw("fork() failed, errno=%d (%s)", errno, gu_strerror(errno));
						break;
					case 0:								/* child */
						/* paranoid code */
						{
						int new_stdin = dup(q->pipe_stdin[0]);
						int new_stdout = dup(q->pipe_stdout[1]);
						int new_stderr = dup(q->pipe_stderr[1]);

						close(q->pipe_stdin[1]);
						close(q->pipe_stdout[0]);
						close(q->pipe_stderr[0]);

						close(q->pipe_stdin[0]);
						close(q->pipe_stdout[1]);
						close(q->pipe_stderr[1]);

						dup2(new_stdin, 0);
						dup2(new_stdout, 1);
						dup2(new_stderr, 2);

						close(new_stdin);
						close(new_stdout);
						close(new_stderr);
						}

						chdir(HOMEDIR);

						/* launch interface program */
						{
						char fname[MAX_PPR_PATH];
						ppr_fnamef(fname, "%s/%s", INTDIR, q->interface);
						if(probe)
							{
							execl(fname, q->interface, "--probe", "-", q->address, NULL);
							}
						else
							{
							#define STR(a) #a
							execl(fname, q->interface,
								"-",					/* printer name */
								q->address,				/* printer address */
								"",						/* interface options */
								q->control_d ? STR(JOBBREAK_CONTROL_D) : STR(JOBBREAK_NONE),
								"1",					/* feedback */
								NULL
								);
							}
						_exit(255);
						}
						break;
					default:							/* parent */
						if(close(q->pipe_stdin[0]) == -1)
							gu_Throw("close() failed, errno=%d (%s)", errno, gu_strerror(errno));
						if(close(q->pipe_stdout[1]) == -1)
							gu_Throw("close() failed, errno=%d (%s)", errno, gu_strerror(errno));
						if(close(q->pipe_stderr[1]) == -1)
							gu_Throw("close() failed, errno=%d (%s)", errno, gu_strerror(errno));
						break;
					}
				}
			gu_Catch
				{
				close(q->pipe_stdout[0]);
				close(q->pipe_stdout[1]);
				gu_ReThrow();
				}
			}
		gu_Catch
			{
			close(q->pipe_stdin[0]);
			close(q->pipe_stdin[1]);
			gu_ReThrow();
			}
		}
	gu_Catch
		{
		gu_ReThrow();
		}

	q->connected = TRUE;

	/* The interface should be up and running and trying to connect by now.  Wait
	   for it to confirm that it has connected to the printer or report failure.
	   */
	gu_Try
		{
		char *line;
		gu_boolean is_stderr;
		int timeout = 10;
		char temp[80];

		temp[0] = '\0';
		while((line = query_getline(q, &is_stderr, timeout)))
			{
			if(is_stderr)
				{
				fprintf(stderr, "    %s\n", line);
				continue;
				}
			if(strcmp(line, "%%[ PPR connecting ]%%") == 0)		/* so far, so good */
				{
				timeout = 120;									/* our confidence grows, extend the timeout */
				continue;
				}
			if(strcmp(line, "%%[ PPR connected ]%%") == 0)		/* we are home free */
				{
				break;
				}
			if(strncmp(line, "%%[", 3) == 0)					/* something bad happened? */
				{												/* save it in case the interface exits */
				gu_strlcpy(temp, line, sizeof(temp));
				continue;
				}
			fprintf(stderr, "Leading garbage (%d characters): \"%s\"\n", (int)strlen(line), line);
			}
		if(!line)							/* if interface exited, */
			{
			if(strlen(temp))
				gu_Throw("%s", temp);
			else
				gu_Throw("interface program quit");
			}
		}
	gu_Catch
		{
		query_disconnect(q);
		gu_ReThrow();
		}
	} /* end of query_connect() */

/** Send a line to the printer

Actually this function doesn't transmit the line.  It just adds it to the
list of lines which will be sent as you call query_getline().

*/
void query_puts(struct QUERY *q, const char s[])
	{
	int len = strlen(s);
	if(!q->connected)
		gu_Throw("not connected");
	if(q->disconnecting)
		gu_Throw("already disconnecting");
	if((q->buf_stdin_len + len) > sizeof(q->buf_stdin))
		gu_Throw("buffer full");
	memcpy(q->buf_stdin + q->buf_stdin_len, s, len);
	q->buf_stdin_len += len;
	} /* end of query_puts() */

/** Get a line from the printer

Wait for a line from the printer.  Strip the line termination and return a
pointer to it.  The line pointed to will be good until the next call to
this function.  The timeout value is the maximum number of seconds to wait
for data from the printer.

*/
char *query_getline(struct QUERY *q, gu_boolean *is_stderr, int timeout)
	{
	fd_set rfds, wfds;
	char *p;
	struct timeval stop_time, time_now, time_left;

	if(!q->connected)
		gu_Throw("not connected");

	/* Figure out the wall clock time at which the timeout expires. */
	gettimeofday(&stop_time, NULL);
	stop_time.tv_sec += timeout;

	/* This loop continues until either we get a line or a zero byte read()
	   happens on both the pipes from the interface program's stdout and
	   stderr. */
	while(!q->eof_stdout || !q->eof_stderr)
		{
		gettimeofday(&time_now, NULL);
		/*printf("time_now=%d.%06d stop_time=%d.%06d\n", (int)time_now.tv_sec, (int)time_now.tv_usec, (int)stop_time.tv_sec, (int)stop_time.tv_usec);*/
		if(gu_timeval_cmp(&time_now, &stop_time) >= 0)
			gu_Throw("timeout");

		/* If we previously returned a line from the stdout buffer, shift the
		   buffer contents down in order to delete it. */
		if(q->buf_stdout_eaten > 0)
			{
			q->buf_stdout_len -= q->buf_stdout_eaten;
			memmove(q->buf_stdout, q->buf_stdout + q->buf_stdout_eaten, q->buf_stdout_len);
			q->buf_stdout_eaten = 0;
			}

		/* If we previously returned a line from the stderr buffer, shift the
		   buffer contents down in order to delete it. */
		if(q->buf_stderr_eaten > 0)
			{
			q->buf_stderr_len -= q->buf_stderr_eaten;
			memmove(q->buf_stderr, q->buf_stderr + q->buf_stderr_eaten, q->buf_stderr_len);
			q->buf_stderr_eaten = 0;
			}

		/* If the stdout buffer has a complete line, return it.  This code is
		   more complicated than the code below for stderr because we provided
		   for returning control-d as a separate line.  Also, here we have
		   extra code to treat \r\n or \n\r as a single newline mark.
		   */
		if(q->buf_stdout_len > 0)
			{
			if(q->buf_stdout[0] == '\004')	/* control-d is EOJ on many PostScript printers */
				{
				q->buf_stdout_eaten = 1;
				if(is_stderr)
					*is_stderr = FALSE;
				return "\004";				/* control-d */
				}
			if(q->buf_stdout[0] == '\f')	/* form-feed is end-of-response on PJL printers */
				{
				q->buf_stdout_eaten = 1;
				if(is_stderr)
					*is_stderr = FALSE;
				return "\f";				/* formfeed */
				}
			else if((p = memchr(q->buf_stdout, '\n', q->buf_stdout_len)) || (p = memchr(q->buf_stdout, '\r', q->buf_stdout_len)))
				{
				int prev = q->last_stdout_crlf;
				q->last_stdout_crlf = *p;
				q->buf_stdout_eaten = ((p - q->buf_stdout) + 1);
				if(q->buf_stdout_eaten == 1 && *p != prev)
					continue;
				if(is_stderr)
					*is_stderr = FALSE;
				*p = '\0';
				return q->buf_stdout;
				}
			else if(q->buf_stdout_len == sizeof(q->buf_stdout))
				{
				gu_Throw("??? 1 ???");
				}
			}

		/* If the stdout buffer has a complete line, return it.
		   */
		if(q->buf_stderr_len > 0)
			{
			if((p = memchr(q->buf_stderr, '\n', q->buf_stderr_len)) || (p = memchr(q->buf_stderr, '\r', q->buf_stderr_len)))
				{
				q->buf_stderr_eaten = ((p - q->buf_stderr) + 1);
				if(is_stderr)
					*is_stderr = TRUE;
				*p = '\0';
				return q->buf_stderr;
				}
			else if(q->buf_stderr_len == sizeof(q->buf_stderr))
				{
				gu_Throw("??? 2 ???");
				}
			}

		/* If we have no more to send, and we haven't yet closed the pipe to
		   the interface program, do it now.
		   */
		if(q->buf_stdin_len == 0 && q->disconnecting && q->pipe_stdin[1] != -1)
			{
			if(close(q->pipe_stdin[1]) == -1)
				gu_Throw("close() failed on pipe to interface, errno=%d (%s)", errno, gu_strerror(errno));
			q->pipe_stdin[1] = -1;
			}

		/* Create a list of the file descriptors that we are waiting for
		   activity on.
		   */
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		if(q->buf_stdin_len > 0)
			FD_SET(q->pipe_stdin[1], &wfds);
		if(!q->eof_stdout)
			FD_SET(q->pipe_stdout[0], &rfds);
		if(!q->eof_stderr)
			FD_SET(q->pipe_stderr[0], &rfds);

		/* Wait for some action. */
		gu_timeval_cpy(&time_left, &stop_time);
		gu_timeval_sub(&time_left, &time_now);
		while(select(q->maxfd + 1, &rfds, &wfds, NULL, &time_left) == -1)
			{
			if(errno != EINTR)
				gu_Throw("select() failed, errno=%d (%s)", errno, gu_strerror(errno));
			}

		/* If we have data to write and there is now room to write data,
		   go for it
		   */
		if(q->buf_stdin_len > 0 && FD_ISSET(q->pipe_stdin[1], &wfds))
			{
			int len;
			while((len = write(q->pipe_stdin[1], q->buf_stdin, q->buf_stdin_len)) == -1)
				{
				if(errno != EINTR)
					gu_Throw("write() failed, errno=%d (%s)", errno, gu_strerror(errno));
				}
			if(len < 0)
				gu_Throw("%d < 0", len);
			q->buf_stdin_len -= len;
			if(q->buf_stdin_len > 0)
				memmove(q->buf_stdin, q->buf_stdin + len, q->buf_stdin_len);
			}

		/* If there is data from the interface program's stdout, */
		if(FD_ISSET(q->pipe_stdout[0], &rfds))
			{
			int len;
			while((len = read(q->pipe_stdout[0], q->buf_stdout + q->buf_stdout_len, sizeof(q->buf_stdout) - q->buf_stdout_len)) == -1)
				{
				if(errno != EINTR)
					gu_Throw("read() failed, errno=%d (%s)", errno, gu_strerror(errno));
				}
			if(len < 0)
				gu_Throw("%d < 0", errno);

			/* If the read size was zero, that means that the interface closed its end. */
			if(len == 0)
				q->eof_stdout = TRUE;

			q->buf_stdout_len += len;
			}

		/* If there is data from the interface program's stderr, */
		if(FD_ISSET(q->pipe_stderr[0], &rfds))
			{
			int len;
			while((len = read(q->pipe_stderr[0], q->buf_stderr + q->buf_stderr_len, sizeof(q->buf_stderr) - q->buf_stderr_len)) == -1)
				{
				if(errno != EINTR)
					gu_Throw("read() failed, errno=%d (%s)", errno, gu_strerror(errno));
				}
			if(len < 0)
				gu_Throw("%d < 0", len);

			if(len == 0)
				q->eof_stderr = TRUE;

			q->buf_stderr_len += len;
			}
		}

	return NULL;
	} /* end of query_getline() */

/** Send a control-d and wait for the answering handshake

*/
static void query_control_d(struct QUERY *q)
	{
	char *line;
	query_puts(q, "\004");
	while((line = query_getline(q, NULL, 10)))
		{
		/* We expect this test to fire. */
		if(strcmp(line, "\004") == 0)
			break;

		/* If not, whine. */
		fprintf(stderr, "Control-D garbage: \"%s\" (%d characters)\n", line, (int)strlen(line));

		/* And then look harder. */
		if(strchr(line, '\004'))
			break;
		}
	} /* end of query_control_d() */

/** Disconnect from the printer

This query flushes the output buffer and closes the pipe to the interface
program's stdin.

*/
void query_disconnect(struct QUERY *q)
	{
	char *line;
	int wait_status;

	if(!q->connected)
		gu_Throw("not connected");

	/* If we started a PostScript query job, */
	if(q->job_started)
		{
		query_endjob(q);
		}

	/* Set a flag so that query_getline() will close the pipe to the interface
	   as soon as it has emptied the output buffer.
	   */
	q->disconnecting = TRUE;

	/* Wait for the interface program to exit.*/
	while((line = query_getline(q, NULL, 10)))
		{
		fprintf(stderr, "Trailing garbage (%d characters): \"%s\"\n", (int)strlen(line), line);
		}

	/* The pipes from the interface can go now.
	   */
	if(close(q->pipe_stdout[0]) == -1 || close(q->pipe_stderr[0]))
		gu_Throw("close() failed, errno=%d (%s)", errno, gu_strerror(errno));

	/* Reap the child. */
	wait(&wait_status);

	q->connected = FALSE;
	} /* end of query_disconnect() */

/** send a PostScript query

query_name			Name of query as defined in DSC spec, possibly NULL
					Defined values include:
						Feature
						File
						FontList
						Font
						Printer
						ProcSet
					If the parameter is NULL, then the query is a generic query.

generic_query_name	Name of query not defined in DSC spec

default response	Response which spoolers that don't understand the query should

pstext				The PostScript code to be executed on the printer in order to
					produce the query response.

*/
void query_sendquery(struct QUERY *q, const char *query_name, const char generic_query_name[], const char default_response[], const char pstext[])
	{
	if(query_name && generic_query_name)
		gu_Throw("query_name and generic_query_name may not both be specified");

	if(!q->job_started)
		{
		if(q->control_d)
			query_control_d(q);
		query_puts(q, "%!PS-Adobe-3.0 Query\n");
		query_puts(q, "\n");
		q->job_started = TRUE;
		}

	/* Print %%?Begin<query_name>Query or %%BeginQuery: <generic_query_name> */
	query_puts(q, "%%?Begin");
	if(query_name)
		query_puts(q, query_name);
	query_puts(q, "Query");
	if(generic_query_name)
		{
		query_puts(q, ": ");
		query_puts(q, generic_query_name);
		}
	query_puts(q, "\n");

	/* Insert the PostScript code between the two comments. */
	query_puts(q, pstext);

	/* Print either
		%%?End<query_name>Query: <default_response>
			or
	    %%?EndQuery: <default_response>
	*/
	query_puts(q, "%%?End");
	if(query_name)
		query_puts(q, query_name);
	query_puts(q, "Query: ");
	query_puts(q, default_response);
	query_puts(q, "\n");
	} /* end of query_sendquery() */

/**
*/
void query_endjob(struct QUERY *q)
	{
	if(!q->job_started)
		gu_Throw("no started");

	/* Close it in high DSC style. */
	query_puts(q, "%%EOF\n");

	/* If control-d handshaking is appropriate, do it. */
	if(q->control_d)
		query_control_d(q);

	q->job_started = FALSE;
	}

/*
** gcc -I ../include -o query -DTEST query.c ../libppr.a ../libgu.a
*/
#if TEST
int main(int argc, char *argv[])
	{
	struct QUERY *q;
	char *line;
	gu_boolean is_stderr;
	int countdown;

	if(argc != 2)
		{
		fprintf(stderr, "Wrong number of arguments.\n");
		return 10;
		}

	gu_Try {
		q = query_new_byprinter(argv[1]);
		query_connect(q, FALSE);

		countdown = 1;
		query_sendquery(q, NULL, "pagecount", "-1", "statusdict /pagecount get exec == flush\n");
		while(countdown > 0 && (line = query_getline(q, &is_stderr)))
			{
			printf("%s%s\n", is_stderr ? "stderr: " : "", line);
			if(!is_stderr && line[0] != '%')
				countdown--;
			}

		countdown = 3;
		query_sendquery(q, "Printer", NULL, "spooler", "statusdict begin revision == version == product == flush end\n");
		while(countdown > 0 && (line = query_getline(q, &is_stderr)))
			{
			printf("%s%s\n", is_stderr ? "stderr: " : "", line);
			if(!is_stderr && line[0] != '%')
				countdown--;
			}

		query_endjob(q);		/* not strictly necessary */
		query_disconnect(q);	/* not strictly necessary */
		query_delete(q);
		}
	gu_Catch
		{
		fprintf(stderr, "Caught exception %s\n", gu_exception);
		return 1;
		}

	return 0;
	}
#endif

/* end of file */
