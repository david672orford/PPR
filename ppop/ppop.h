/*
** mouse:~ppr/src/ppop/ppop.h
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
** Last modified 14 January 2005.
*/

/*
** This is the include file for all the modules in the ppop utility.
*/

/* A useful macros considering how often we write to stdout. */
#define PUTS(string) fputs(string, stdout)
#define PUTC(c) fputc(c, stdout)

/* ========================== Things in ppop.c ================================= */

struct Jobname
	{
	char destname[MAX_DESTNAME+1];
	int id;
	int subid;
	} ;

struct Destname
	{
	char destname[MAX_DESTNAME+1];
	} ;

int parse_job_name(struct Jobname *job, const char *jobname);
int parse_dest_name(struct Destname *dest, const char *destname);

extern pid_t pid;
extern int machine_readable;
extern FILE *errors;
extern int arrest_interest_interval;
extern gu_boolean verbose;

void fatal(int exitval, const char *string, ...)
#ifdef __GNUC__
__attribute__ (( noreturn, format (printf, 2, 3) ))
#endif
;

void error(const char *string, ...)
#ifdef __GNUC__
__attribute__ ((format (printf, 1, 2)))
#endif
;

void puts_detabbed(const char *string);

FILE *get_ready(void);
FILE *wait_for_pprd(int do_timeout);
int print_reply(void);

int assert_am_operator(void);
int job_permission_check(struct Jobname *job);
int is_my_job(const struct QEntry *qentry, const struct QFile *qfileentry);

/* ============== Functions in ppop_cmds_listq.c =================== */

/* Subcommand routines */
int ppop_short(char *argv[]);
int ppop_details(char *argv[]);
int ppop_list(char *argv[],int suppress);
int ppop_lpq(char *argv[]);
int ppop_qquery(char *argv[]);
int ppop_progress(char *argv[]);

/* This function is needed by some commands in ppop_cmds_other.c which
   need to use a queue listing internally. */
int custom_list(char *argv[],
				void(*help)(void),
				void(*banner)(void),
				int(*item)(const struct QEntry *qentry,
					const struct QFile*,
					const char *onprinter,
					FILE *qstream),
				int suppress,
				int arrested_drop_time);

/* ================ Functions in ppop_cmds_other.c ================ */

/* Subcommand routines */
int ppop_status(char *argv[]);
int ppop_message(char *argv[]);
int ppop_media(char *argv[]);
int ppop_mount(char *argv[]);
int ppop_start_stop_wstop_halt(char *argv[], int variation);
int ppop_cancel(char *argv[], int inform);
int ppop_purge(char *argv[], int inform);
int ppop_clean(char *argv[]);
int ppop_cancel_active(char *argv[], int my, int inform);
int ppop_move(char *argv[]);
int ppop_rush(char *argv[],int direction);
int ppop_hold_release(char *argv[], int release);
int ppop_accept_reject(char *argv[], int reject);
int ppop_destination(char *argv[], int comment_too);
int ppop_alerts(char *argv[]);
int ppop_log(char *argv[]);
int print_aux_status(char *line, int printer_status, const char sep[]);

/* =============== Command routines in ppop_modify.c ========================= */

int ppop_modify(char *argv[]);

/* end of file */
