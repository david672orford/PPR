/*
** mouse:~ppr/src/ppad/ppad.h
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
** Last modified 23 October 2003.
*/

/*
** This is the header file for the PPR administrator's utility ppad.
*/

#include "libppr_query.h"

/* A useful macro considering how often we write to stdout. */
#define PUTS(string) fputs(string, stdout)

/* functions and global variables in ppad.c */
extern const char myname[];

void fatal(int exitval, const char *string, ...)
#ifdef __GNUC__
__attribute__ (( noreturn, format (printf, 2, 3) ))
#endif
;

gu_boolean am_administrator(void);
extern FILE *errors;
extern int machine_readable;
extern int debug_level;

/* functions in ppad_media.c */
int media_show(const char *argv[]);
int media_put(const char *argv[]);
int media_delete(const char *argv[]);
int media_export(void);
int media_import(const char *argv[]);

/* functions in ppad_printer.c */
int printer_show(const char *argv[]);
int printer_comment(const char *argv[]);
int printer_location(const char *argv[]);
int printer_department(const char *argv[]);
int printer_contact(const char *argv[]);
int printer_interface(const char *argv[]);
int printer_options(const char *argv[]);
int printer_jobbreak(const char *argv[]);
int printer_feedback(const char *argv[]);
int printer_codes(const char *argv[]);
int printer_rip(const char *argv[]);
int printer_ppd(const char *argv[]);
int printer_ppdq(const char *argv[]);
int printer_alerts(const char *argv[]);
int printer_frequency(const char *argv[]);
int printer_flags(const char *argv[]);
int printer_outputorder(const char *argv[]);
int printer_charge(const char *argv[]);
int printer_bins_ppd(const char *argv[]);
int printer_bins_set_or_add(gu_boolean add, const char *argv[]);
int printer_bins_delete(const char *argv[]);
int printer_delete(const char *argv[]);
int printer_new_alerts(const char *argv[]);
int printer_touch(const char *argv[]);
int printer_switchset(const char *argv[]);
int printer_deffiltopts(const char *argv[]);
int printer_passthru(const char *argv[]);
int printer_ppdopts(const char *argv[]);
int printer_limitpages(const char *argv[]);
int printer_limitkilobytes(const char *argv[]);
int printer_grayok(const char *argv[]);
int printer_addon(const char *argv[]);
int printer_acls(const char *argv[]);
int printer_pagetimelimit(const char *argv[]);
int printer_userparams(const char *argv[]);

/* functions in ppad_group.c */
int group_show(const char *argv[]);
int group_comment(const char *argv[]);
int group_rotate(const char *argv[]);
int group_members_add(const char *argv[], gu_boolean do_add);
int group_remove(const char *argv[]);
int _group_remove(const char *group, const char *member);
int group_delete(const char *argv[]);
int group_touch(const char *argv[]);
int group_switchset(const char *argv[]);
int _group_deffiltopts(const char *group);
int group_deffiltopts(const char *argv[]);
int group_passthru(const char *argv[]);
int group_addon(const char *argv[]);
int group_acls(const char *argv[]);

/* functions in ppad_alias.c */
int alias_show(const char *argv[]);
int alias_forwhat(const char *argv[]);
int alias_delete(const char *argv[]);
int alias_comment(const char *argv[]);
int alias_switchset(const char *argv[]);
int alias_passthru(const char *argv[]);
int alias_addon(const char *argv[]);

/* functions in ppad_conf.c */
extern char *confline;
enum QUEUE_TYPE { QUEUE_TYPE_PRINTER, QUEUE_TYPE_GROUP, QUEUE_TYPE_ALIAS };
int prnopen(const char prnname[], gu_boolean modify);
int grpopen(const char grpname[], gu_boolean modify);
int confopen(enum QUEUE_TYPE queue_type, const char destname[], gu_boolean modify);
int confread(void);
int conf_printf(const char string[], ...);
int conf_vprintf(const char string[], va_list va);
int confabort(void);
int confclose(void);
int conf_set_name(enum QUEUE_TYPE queue_type, const char queue_name[], const char name[], const char value[], ...);

/* functions in ppad_util.c */
int ppop2(const char *parm1, const char *parm2);
void write_fifo(const char string[], ...);
int make_switchset_line(char *line, const char *argv[]);
void print_switchset(char *switchset);
int print_wrapped(const char *deffiltopts, int starting_column);
char *list_to_string(const char *argv[]);

/* functions in ppad_filt.c */
void deffiltopts_open(void);
int deffiltopts_add_ppd(const char printer_name[], const char ppd_name[], const char *InstalledMemory);
int deffiltopts_add_printer(const char printer_name[]);
char *deffiltopts_line(void);
void deffiltopts_close(void);

/* functions in ppad_ppd.c */
int ppd_query_core(const char printer[], struct QUERY *q);
int ppd_query(const char *argv[]);

/* end of file */

