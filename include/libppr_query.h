/*
** mouse:~ppr/src/include/libppr_query.h
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
** Last modified 28 February 2005.
*/

/*
** This is the definition of the query object.  Call one of the query_new_*() functions
** to create one of these things.
*/
struct QUERY
	{
	const char *interface;
	const char *address;
	const char *options;
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
	int maxfd;						/* first argument for select() */
	char *line;						/* last line read from printer */
	int line_len;					/* number of bytes currently allocated to line */
	gu_boolean connected;
	gu_boolean disconnecting;		/* are we going to close() once the output buffer is empty? */
	gu_boolean job_started;			/* has query_puts() been called yet? */
	};

struct QUERY *query_new_byaddress(const char interface[], const char address[], const char options[]);
struct QUERY *query_new_byprinter(const char printer[]);
void query_free(struct QUERY *q);
void query_connect(struct QUERY *q, gu_boolean probe);
void query_puts(struct QUERY *q, const char s[]);
char *query_getline(struct QUERY *q, gu_boolean *is_stderr, int timeout);
void query_sendquery(struct QUERY *q, const char *name, const char values[], const char default_response[], const char pstext[]);
void query_endjob(struct QUERY *q);
void query_disconnect(struct QUERY *q);

/* end of file */
