/*
** mouse:~ppr/src/include/ppr_query.h
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
** Last modified 23 April 2001.
*/

struct QUERY
    {
    const char *interface;
    const char *address;
    gu_boolean control_d;
    char buf_stdin[512];
    char buf_stdout[512];
    char buf_stderr[512];
    int buf_stdin_len;
    int buf_stdout_len;
    int buf_stderr_len;
    int buf_stdout_eaten;
    int buf_stderr_eaten;
    gu_boolean eof_stdout;
    gu_boolean eof_stderr;
    int pipe_stdin[2];
    int pipe_stdout[2];
    int pipe_stderr[2];
    int last_stdout_crlf;
    int maxfd;
    char *line;
    int line_len;
    gu_boolean connected;
    gu_boolean disconnecting;
    gu_boolean started;
    };

struct QUERY *query_new_byaddress(const char interface[], const char address[]);
struct QUERY *query_new_byprinter(const char printer[]);
void query_connect(struct QUERY *q);
void query_puts(struct QUERY *q, const char s[]);
char *query_getline(struct QUERY *q, gu_boolean *is_stderr);
char *query_connect_wait(struct QUERY *q);
void query_control_d(struct QUERY *q);
void query_sendquery(struct QUERY *q, const char *name, const char values[], const char default_response[], const char pstext[]);
void query_disconnect(struct QUERY *q);
void query_delete(struct QUERY *q);

/* end of file */
