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
** Last modified 31 July 2003.
*/

#include "before_system.h"
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include "gu.h"
#include "global_defines.h"
#include "interface.h"
#include "ppr_query.h"

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

	fclose(f);

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

/** connect to the printer

*/
void query_connect(struct QUERY *q)
	{
	if(q->connected)
		gu_Throw("already connected");

	q->buf_stdin_len = q->buf_stdout_len = q->buf_stderr_len = 0;
	q->buf_stdout_eaten = q->buf_stderr_eaten = 0;
	q->eof_stdout = q->eof_stderr = FALSE;
	q->started = FALSE;
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

						/* launch interface program */
						{
						char fname[MAX_PPR_PATH];
						ppr_fnamef(fname, "%s/%s", INTDIR, q->interface);
						execl(fname, q->interface, "-", q->address, NULL);
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
	}

/** Send a line to the printer

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
	}

/** Receive a line from the printer

*/
char *query_getline(struct QUERY *q, gu_boolean *is_stderr)
	{
	fd_set rfds, wfds;
	char *p;

	/* This loop continues until either we get a line a zero byte read()
	   happens on both the pipes from the interface programs stdout
	   and stderr. */
	while(!q->eof_stdout || !q->eof_stderr)
		{
		/* If we previously returned a line from the stdout buffer, delete it. */
		if(q->buf_stdout_eaten > 0)
			{
			q->buf_stdout_len -= q->buf_stdout_eaten;
			memmove(q->buf_stdout, q->buf_stdout + q->buf_stdout_eaten, q->buf_stdout_len);
			q->buf_stdout_eaten = 0;
			}

		/* If we previously returned a line from the stderr buffer, delete it. */
		if(q->buf_stderr_eaten > 0)
			{
			q->buf_stderr_len -= q->buf_stderr_eaten;
			memmove(q->buf_stderr, q->buf_stderr + q->buf_stderr_eaten, q->buf_stderr_len);
			q->buf_stderr_eaten = 0;
			}

		/* If one of the stdout buffer has a complete line, return it. */
		if(q->buf_stdout_len > 0)
			{
			if(q->buf_stdout[0] == '\004')
				{
				q->buf_stdout_eaten = 1;
				if(is_stderr)
					*is_stderr = FALSE;
				return "\004";
				}
			else if((p = memchr(q->buf_stdout, '\n', q->buf_stdout_len)) || (p = memchr(q->buf_stdout, '\r', q->buf_stdout_len)))
				{
				int prev = q->last_stdout_crlf;
				q->last_stdout_crlf = *p;
				q->buf_stdout_eaten = ((p - q->buf_stdout) + 1);
				if(q->buf_stdout_eaten == 1 && *p != prev)
					{
					continue;
					}
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

		/* If one of the stdout buffer has a complete line, return it. */
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

		/* If we have no more to send, close the pipe to the interface program. */
		if(q->buf_stdin_len == 0 && q->disconnecting)
			{
			if(close(q->pipe_stdin[1]) == -1)
				gu_Throw("close() failed");
			}

		/* Create a list of the file descriptors that we are waiting for. */
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		if(q->buf_stdin_len > 0)
			FD_SET(q->pipe_stdin[1], &wfds);
		if(!q->eof_stdout)
			FD_SET(q->pipe_stdout[0], &rfds);
		if(!q->eof_stderr)
			FD_SET(q->pipe_stderr[0], &rfds);

		/* Wait for some action. */
		while(select(q->maxfd + 1, &rfds, &wfds, NULL, NULL) == -1)
			{
			if(errno != EINTR)
				gu_Throw("select() failed, errno=%d (%s)", errno, gu_strerror(errno));
			}

		/* If there is room to write data, */
		if(FD_ISSET(q->pipe_stdin[1], &wfds))
			{
			int len;
			while((len = write(q->pipe_stdin[1], q->buf_stdin, q->buf_stdin_len)) == -1)
				{
				if(errno != EINTR)
					gu_Throw("write() failed, errno=%d (%s)");
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
	}

/** wait for the interface program to connect to the printer

*/
char *query_connect_wait(struct QUERY *q)
	{
	char *line;
	while((line = query_getline(q, NULL)))
		{
		if(strcmp(line, "%%[ PPR connecting ]%%") == 0)
			continue;
		if(strncmp(line, "%%[", 3) == 0)
			return line;
		fprintf(stderr, "Leading garbage: \"%s\" (%d characters)\n", line, (int)strlen(line));
		}
	return NULL;
	}

/** Send a control-d and wait for the answering handshake

*/
void query_control_d(struct QUERY *q)
	{
	char *line;
	if(q->control_d)
		{
		query_puts(q, "\004");
		while((line = query_getline(q, NULL)))
			{
			if(strcmp(line, "\004") == 0)
				break;
			fprintf(stderr, "Control-D garbage: \"%s\" (%d characters)\n", line, (int)strlen(line));
			if(strchr(line, '\004'))
				break;
			}
		}
	}

/** send a PostScript query

*/
void query_sendquery(struct QUERY *q, const char *name, const char values[], const char default_response[], const char pstext[])
	{
	if(!q->started)
		{
		query_control_d(q);
		query_puts(q, "%!PS-Adobe-3.0 Query\n");
		query_puts(q, "\n");
		q->started = TRUE;
		}

	query_puts(q, "%%?Begin");
	if(name)
		query_puts(q, name);
	query_puts(q, "Query");
	if(values)
		{
		query_puts(q, ": ");
		query_puts(q, values);
		}
	query_puts(q, "\n");

	query_puts(q, pstext);

	query_puts(q, "%%EndQuery: ");
	query_puts(q, default_response);
	query_puts(q, "\n");
	}

/** Disconnect from the printer

*/
void query_disconnect(struct QUERY *q)
	{
	char *line;

	if(!q->connected)
		gu_Throw("not connected");

	if(q->started)
		{
		query_puts(q, "%%EOF\n");
		}

	query_control_d(q);

	q->disconnecting = TRUE;

	while((line = query_getline(q, NULL)))
		{
		fprintf(stderr, "Trailing garbage: \"%s\" (%d characters)\n", line, (int)strlen(line));
		}

	q->connected = FALSE;
	}

/** destroy a query object

*/
void query_delete(struct QUERY *q)
	{
	if(q->line) gu_free(q->line);
	gu_free(q);
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
		query_connect(q);

		printf("Connect result: %s\n", query_connect_wait(q));

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

		query_disconnect(q);
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
