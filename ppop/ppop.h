/*
** mouse:~ppr/src/ppop/ppop.h
** Copyright 1995--2002, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 13 March 2002.
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
	char destnode[MAX_NODENAME+1];
	char destname[MAX_DESTNAME+1];
	int id;
	int subid;
	char homenode[MAX_NODENAME+1];
	} ;

struct Destname
	{
	char destnode[MAX_NODENAME+1];
	char destname[MAX_NODENAME+1];
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

FILE *get_ready(const char nodename[]);
FILE *wait_for_pprd(int do_timeout);
int print_reply(void);

int assert_am_operator(void);
int job_permission_check(struct Jobname *job);
int is_my_job(const struct QEntry *qentry, const struct QFileEntry *qfileentry);

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
					const struct QFileEntry*,
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
