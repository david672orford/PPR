/*
** mouse:~ppr/src/libppr/query.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 2 February 2001.
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
#include "cexcept.h"
#include "interface.h"
#include "ppr_query.h"

define_exception_type(int);
static struct exception_context the_exception_context[1];

struct QUERY *query_new_byaddress(const char interface[], const char address[])
    {
    struct QUERY *q;

    if(!interface)
	Throw(-1);
    if(!address)
	Throw(-2);

    q = gu_alloc(1, sizeof(struct QUERY));

    q->interface = interface;
    q->address = address;
    q->line = NULL;
    q->line_len = 128;
    q->connected = FALSE;

    q->control_d = (strcmp(interface, "atalk") == 0) ? FALSE : TRUE;

    return q;
    }

struct QUERY *query_new_byprinter(const char printer[])
    {
    char fname[MAX_PPR_PATH];
    FILE *f;
    char *line = NULL;
    int line_len = 128;
    char *interface = NULL;
    char *address = NULL;
    char *tptr;
    int e;

    if(!printer)
	Throw(-3);

    ppr_fnamef(fname, "%s/%s", PRCONF, printer);
    if(!(f = fopen(fname, "r")))
	Throw(-104);

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

    Try	{
	if(!interface)
	    Throw(-4);
	if(!address)
	    Throw(-5);
    	}
    Catch(e)
    	{
	if(interface)
	    gu_free(interface);
	if(address)
	    gu_free(address);
	Throw(e);
    	}

    return query_new_byaddress(interface, address);
    }

void query_connect(struct QUERY *q)
    {
    int e;

    if(q->connected)
	Throw(-100);

    q->buf_stdin_len = q->buf_stdout_len = q->buf_stderr_len = 0;
    q->buf_stdout_eaten = q->buf_stderr_eaten = 0;
    q->eof_stdout = q->eof_stderr = FALSE;
    q->started = FALSE;
    q->disconnecting = FALSE;
    q->last_stdout_crlf = 0;

    /* Open three pipes for the interface program's stdin, stdout, and stderr. */
    Try	{
	if(pipe(q->pipe_stdin) == -1)
	    {
	    Throw(-101);
	    }
	Try {
	    if(pipe(q->pipe_stdout) == -1)
	    	{
	    	Throw(-102);
	    	}
	    Try	{
	    	if(pipe(q->pipe_stderr) == -1)
	    	    Throw(-103);

		q->maxfd = 0;
		if(q->pipe_stdin[1] > q->maxfd)
		    q->maxfd = q->pipe_stdin[1];
		if(q->pipe_stdout[0] > q->maxfd)
		    q->maxfd = q->pipe_stdout[1];
		if(q->pipe_stderr[0] > q->maxfd)
		    q->maxfd = q->pipe_stderr[0];

		switch(fork())
		    {
		    case -1:				/* fork failed */
		    	Throw(-104);
		    	break;
		    case 0:				/* child */
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
		    default:				/* parent */
			if(close(q->pipe_stdin[0]) == -1) Throw(-105);
			if(close(q->pipe_stdout[1]) == -1) Throw(-106);
			if(close(q->pipe_stderr[1]) == -1) Throw(-107);
			break;
		    }
	    	}
	    Catch(e)
	    	{
		close(q->pipe_stdout[0]);
		close(q->pipe_stdout[1]);
		Throw(e);
		}
	    }
	Catch(e)
	    {
	    close(q->pipe_stdin[0]);
	    close(q->pipe_stdin[1]);
	    Throw(e);
	    }
    	}
    Catch(e)
    	{
	Throw(e);
    	}

    q->connected = TRUE;
    }

void query_puts(struct QUERY *q, const char s[])
    {
    int len = strlen(s);
    if(!q->connected)
	Throw(-201);
    if(q->disconnecting)
    	Throw(-202);
    if((q->buf_stdin_len + len) > sizeof(q->buf_stdin))
    	Throw(-203);
    memcpy(q->buf_stdin + q->buf_stdin_len, s, len);
    q->buf_stdin_len += len;
    }

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
                Throw(-120);
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
                Throw(-121);
                }
            }

	/* If we have no more to send, close the pipe to the interface program. */
	if(q->buf_stdin_len == 0 && q->disconnecting)
	    {
	    if(close(q->pipe_stdin[1]) == -1)
		Throw(-213);
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
                Throw(-210);
            }

	/* If there is room to write data, */
        if(FD_ISSET(q->pipe_stdin[1], &wfds))
            {
	    int len;
	    while((len = write(q->pipe_stdin[1], q->buf_stdin, q->buf_stdin_len)) == -1)
	    	{
		if(errno != EINTR)
		    Throw(-211);
	    	}
	    if(len < 0)
	    	Throw(-212);
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
		    Throw(-214);
	    	}
	    if(len < 0)
	    	Throw(-215);

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
		    Throw(-216);
	    	}
	    if(len < 0)
	    	Throw(-217);

	    if(len == 0)
	    	q->eof_stderr = TRUE;

	    q->buf_stderr_len += len;
	    }
	}

    return NULL;
    }

char *query_connect_wait(struct QUERY *q)
    {
    char *line;
    while((line = query_getline(q, NULL)))
    	{
	if(strcmp(line, "%%[ PPR connecting ]%%") == 0)
	    continue;
	if(strncmp(line, "%%[", 3) == 0)
	    return line;
	fprintf(stderr, "Leading garbage: \"%s\" (%d characters)\n", line, strlen(line));
    	}
    return NULL;
    }

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
	    fprintf(stderr, "Control-D garbage: \"%s\" (%d characters)\n", line, strlen(line));
	    if(strchr(line, '\004'))
	    	break;
	    }
    	}
    }

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

void query_disconnect(struct QUERY *q)
    {
    char *line;

    if(!q->connected)
	Throw(-150);

    if(q->started)
	{
	query_puts(q, "%%EOF\n");
	}

    query_control_d(q);

    q->disconnecting = TRUE;

    while((line = query_getline(q, NULL)))
    	{
	fprintf(stderr, "Trailing garbage: \"%s\" (%d characters)\n", line, strlen(line));
    	}

    q->connected = FALSE;
    }

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
    int e;
    char *line;
    gu_boolean is_stderr;
    int countdown;

    if(argc != 2)
    	{
	fprintf(stderr, "Wrong number of arguments.\n");
	return 10;
    	}

    Try	{
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
    Catch(e)
	{
	fprintf(stderr, "Caught exception %d, errno=%d (%s)\n", e, errno, strerror(errno));
	return 1;
	}

    return 0;
    }
#endif

/* end of file */
