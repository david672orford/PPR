/*
** mouse:~ppr/src/ppad/ppad.h
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 25 April 2006.
*/

#include "libppr_query.h"

/* break these functions since they don't do utf-8 to local charset conversion */
#define printf() dont_printf()
#define puts(a) dont_puts(a)
#undef putc
#define putc(a) dont_putc(a)
#undef putline
#define putline(a) dont_putline(a)

#define fputc(a,b) dont_fputc(a,b)
#define fputs(a,b) dont_fputs(a,b)
#define fprintf() dont_fprintf()

/* global functions and global variables in ppad.c */
extern const char myname[];
void fatal(int exitval, const char *string, ...)
#ifdef __GNUC__
__attribute__ (( noreturn, format (printf, 2, 3) ))
#endif
;
extern int machine_readable;
extern int debug_level;

/* === functions in ppad_util.c === */
int ppop2(const char *parm1, const char *parm2);
void write_fifo(const char string[], ...);
int make_switchset_line(char *line, const char *argv[]);
void print_switchset(char *switchset);
int print_wrapped(const char *deffiltopts, int starting_column);
char *list_to_string(const char *argv[]);
int exception_to_exitcode(int exception_code);
	
/* === functions in ppad_conf.c === */
enum QUEUE_TYPE { QUEUE_TYPE_PRINTER, QUEUE_TYPE_GROUP, QUEUE_TYPE_ALIAS };
struct CONF_OBJ
	{
	enum QUEUE_TYPE queue_type;
	const char *name;
	int flags;
	char in_name[MAX_PPR_PATH];
	FILE *in;
	char out_name[MAX_PPR_PATH];
	FILE *out;
	char *line;
	int line_space;
	};
#define CONF_MODIFY 1			/** open for modify */
#define CONF_CREATE 2			/** create if doesn't exist */
#define CONF_ENOENT_PRINT 4		/** print a message if doesn't exist and no modify */
#define CONF_RELOAD 8			/** instruct pprd to reload after modify */
struct CONF_OBJ *conf_open(enum QUEUE_TYPE queue_type, const char destname[], int flags);
char *conf_getline(struct CONF_OBJ *obj);
int conf_printf(struct CONF_OBJ *obj, const char string[], ...);
int conf_vprintf(struct CONF_OBJ *obj, const char string[], va_list va);
int conf_abort(struct CONF_OBJ *obj);
int conf_close(struct CONF_OBJ *obj);
int conf_set_name(enum QUEUE_TYPE queue_type, const char queue_name[], int extra_flags, const char name[], const char value[], ...);
int conf_copy(enum QUEUE_TYPE, const char from[], const char to[]);

/* functions in ppad_group.c */
int group_remove_internal(const char *group, const char *member);
int group_deffiltopts_internal(const char *group);

/* functions in ppad_ppd.c */
int ppd_query_core(const char printer[], struct QUERY *q);

/* end of file */

