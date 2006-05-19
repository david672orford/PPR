/*
** mouse:~ppr/src/ppop/ppop.h
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 19 May 2006.
*/

/*
** This is the include file for all the modules in the ppop utility.
*/

/* break these functions since they don't do utf-8 to local charset conversion */
#define printf() dont_printf()
#define puts() dont_puts()
#undef putc
#define putc() dont_putc()
#undef putline
#define putline() dont_putline()

/*#define fputc() dont_fputc()*/
#define fputs() dont_fputs()
/* #define fprintf() dont_fprintf() */

/* ========================== Things in ppop.c ================================= */

const char *parse_destname(const char *destname, gu_boolean resolve_aliases);

const struct Jobname *parse_jobname(const char jobname[]);

extern gu_boolean	opt_verbose;
extern int 			opt_machine_readable;
extern int			opt_arrest_interest_interval;

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

gu_boolean assert_am_operator(void);
gu_boolean job_permission_check(const struct Jobname *job);
int is_my_job(const struct QEntry *qentry, const struct QEntryFile *qentryfile);

/* ============== Functions in ppop_cmds_listq.c =================== */

/* This function is needed by some commands in ppop_cmds_other.c which
   need to use a queue listing internally. */
int custom_list(const char *argv[],
				void(*banner)(void),
				int(*item)(
					int rank,
					const struct QEntry *qentry,
					const struct QEntryFile*,
					const char *onprinter,
					FILE *qstream),
				int suppress,
				int arrested_drop_time);

/* ================ Functions in ppop_cmds_other.c ================ */

int print_aux_status(char *line, int printer_status, const char sep[]);

/* =============== Command routines in ppop_modify.c ========================= */

int ppop_modify(char *argv[]);

/* end of file */
