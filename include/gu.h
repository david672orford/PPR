/*
** mouse:~ppr/src/include/gu.h
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
** Last modified 12 March 2003.
*/

#ifndef _GU_H
#define _GU_H 1

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>

/*===================================================================
** Useful macros
===================================================================*/

/* Define a portable boolean type. */
typedef int gu_boolean;

/* define constants for true and false. */
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE !FALSE
#endif

/*
** Define unix permissions 755 and 644.  We do this because just
** saying 0755 or 644 is at least theoretically non-portable and
** because these portable expressions are long and unsightly.
*/
#define UNIX_755 (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#define UNIX_644 (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define UNIX_660 (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
#define UNIX_640 (S_IRUSR | S_IWUSR | S_IRGRP)
#define UNIX_600 (S_IRUSR | S_IWUSR)
#define UNIX_022 (S_IWGRP | S_IWOTH)
#define UNIX_077 (S_IRWXG | S_IRWXO)

/*
** Macros for looking for keywords at the start of strings.
*/

/* Does b match the first part of a? */
#define lmatch(a, b) (!strncmp(a, b, sizeof(b) - 1))

/* If b matches the first part of a, return a pointer to the first word in a that comes after the match. */
#define lmatchp(a, b) (!strncmp(a, b, sizeof(b) - 1) ? a + sizeof(b) - 1 + strspn(a + sizeof(b) - 1, " \t") : NULL)

/* Does b match the last part of a? */
#define rmatch(a, b) (strlen(a) >= strlen(b) && strcmp(a + strlen(a) - strlen(b), b) == 0)

/* If b matches the first part of a with a white space following, return pointer to first part 
   after whitespace. */
#define lmatchsp(a, b) (!strncmp(a, b, sizeof(b) - 1) && isspace(a[sizeof(b) - 1]) ? a + sizeof(b) - 1 + strspn(a + sizeof(b) - 1, " \t") : NULL)

/*
** Macros for writing strings.
*/
#define gu_write_string(fd, s) (write(fd, s, sizeof(s) - 1))

/*===================================================================
** Memory allocation routines
===================================================================*/

/* Actual allocation functions */
void *gu_alloc(size_t number, size_t size);
void *gu_realloc(void *ptr, size_t number, size_t size);
char *gu_strdup(const char *string);
char *gu_strndup(const char *string, size_t len);
char *gu_restrdup(char *ptr, size_t *number, const char *string);
void gu_free(void *ptr);

/* Debugging functions */
void gu_alloc_checkpoint(void);
int gu_alloc_checkpoint_get(void);
void gu_alloc_checkpoint_put(int n);
void _gu_alloc_assert(const char *file, int line, int assertion);
#define gu_alloc_assert(assertion) _gu_alloc_assert(__FILE__, __LINE__, assertion)

/*===================================================================
** The gu_ini_ routines read files in a format inspired by Microsoft
** Windows .ini files and Samba's smb.conf file.
===================================================================*/

struct GU_INI_ENTRY
    {
    char *name;		/* left hand side (not const so we can free) */
    const char *values;	/* list from right hand side (const so we can't free) */
    int nvalues;	/* number of members in right hand side list */
    } ;


enum GU_INI_TYPES {
    GU_INI_TYPE_SKIP,
    GU_INI_TYPE_NONNEG_INT,
    GU_INI_TYPE_STRING,
    GU_INI_TYPE_NONEMPTY_STRING,
    GU_INI_TYPE_POSITIVE_DOUBLE,
    GU_INI_TYPE_NONNEG_DOUBLE,
    GU_INI_TYPE_END
    } ;

struct GU_INI_ENTRY *gu_ini_section_load(FILE *file, const char section_name[]);
const struct GU_INI_ENTRY *gu_ini_section_get_value(const struct GU_INI_ENTRY *section, const char key_name[]);
void gu_ini_section_free(struct GU_INI_ENTRY *section);
const char *gu_ini_value_index(const struct GU_INI_ENTRY *array, int array_index, const char *default_value);
int gu_ini_assign(const struct GU_INI_ENTRY *array, ...);
int gu_ini_vassign(const struct GU_INI_ENTRY *array, va_list args);
const char *gu_ini_scan_list(const char file_name[], const char section_name[], const char key_name[], ...);
char *gu_ini_query(const char file_name[], const char section_name[], const char key_name[], int index, const char default_value[]);

/*===================================================================
** Other stuff
===================================================================*/

char *gu_getline(char *line, int *space_available, FILE *fstream);
char *gu_strerror(int n);
int ppr_wildmat(const char *text, const char *p);
int compile_string_escapes(char *s);
char *gu_strsep(char **stringp, const char *delim);
char *gu_strsep_quoted(char **stringp, const char *delim, const char *discard);
void gu_set_cloexec(int fd);
void gu_nonblock(int fd, gu_boolean on);
void gu_trim_whitespace_right(char *s);
int gu_strcasecmp(const char *s1, const char *s2);
int gu_strncasecmp(const char *s1, const char *s2, int n);
int gu_lock_exclusive(int filenum, int waitmode);
int gu_torf(const char *s);
int gu_torf_setBOOL(gu_boolean *b, const char *s);
double gu_getdouble(const char *);
const char *gu_dtostr(double);
void ASCIIZ_to_padded(char *padded, const char *asciiz, int len);
void padded_to_ASCIIZ(char *asciiz, const char *padded, int len);
gu_boolean padded_cmp(const char *padded1, const char *padded2, int len);
gu_boolean padded_icmp(const char *padded1, const char *padded2, int len);
int gu_sscanf(const char *input, const char *pattern, ...);
void gu_sscanf_checkpoint(void);
void gu_sscanf_rollback(void);
int gu_fscanf(FILE *input, const char *format, ...);
void gu_daemon(mode_t daemon_umask);
int disk_space(const char *path, unsigned int *free_blocks, unsigned int *free_files);
void gu_wordwrap(char *string, int width);
void (*signal_interupting(int signum, void (*handler)(int sig)))(int);
void (*signal_restarting(int signum, void (*handler)(int sig)))(int);
int gu_vsnprintf (char *str, size_t count, const char *fmt, va_list args);
int gu_snprintf(char *str, size_t count, const char *fmt, ...);
int gu_vasprintf(char **ptr, const char *format, va_list ap);
int gu_asprintf(char **ptr, const char *format, ...);
int gu_mkstemp(char *template);
char *gu_strsignal(int signum);
char *gu_StrCopyMax(char *target, size_t max, const char *source);
char *gu_StrAppendMax(char *target, size_t max, const char *source);
int gu_snprintfcat(char *buffer, size_t max, const char *format, ...);
int gu_timeval_cmp(const struct timeval *t1, const struct timeval *t2);
void gu_timeval_sub(struct timeval *t1, const struct timeval *t2);
void gu_timeval_add(struct timeval *t1, const struct timeval *t2);
void gu_timeval_cpy(struct timeval *t1, const struct timeval *t2);
void gu_timeval_zero(struct timeval *t);
int gu_runl(const char *myname, FILE *errors, const char *progname, ...);

/*
** Values for gu_torf(), a function which examines a string
** and tries to determine whether it represents a true or
** a false value.
*/
#define ANSWER int
#define ANSWER_UNKNOWN -1
#define ANSWER_FALSE 0
#define ANSWER_TRUE 1
/* enum ANSWER { ANSWER_UNKNOWN = -1, ANSWER_FALSE = 0, ANSWER_TRUE = 1 }; */

/*===================================================================
** Command line option parsing
===================================================================*/

struct gu_getopt_opt
	{
	const char *name;
	int code;
	gu_boolean needsarg;
	} ;

struct gu_getopt_state
	{
	int argc;					/* private */
	char **argv;					/* private */
	const char *opt_chars;				/* private */
	const struct gu_getopt_opt *opt_words;		/* private */
	int optind;					/* public */
	char *optarg;					/* public */
	const char *name;				/* public */
	int x;						/* very private */
	int len;					/* very private */
	char scratch[3];				/* very private */
	char *putback;
	} ;

void gu_getopt_init(struct gu_getopt_state *state, int argc, char **argv, const char *opt_chars, const struct gu_getopt_opt *opt_words);
int ppr_getopt(struct gu_getopt_state *state);
void gu_getopt_default(const char myname[], int optchar, const struct gu_getopt_state *getopt_state, FILE *errors);

/*===================================================================
** Parse space separated name=value pairs
===================================================================*/

struct OPTIONS_STATE {
    int magic;				/* should be 689 */

    const char *options;		/* the string we are working on */

    int index;				/* index of where we are working now */
    int index_of_name;			/* index of start of this name=value pair */
    int index_of_prev_name;		/* index of start of prev pair */

    const char *error;			/* an error message */

    int next_time_skip;			/* distance from index to start of next name */
    };

void options_start(const char *options_str, struct OPTIONS_STATE *o);
int options_get_one(struct OPTIONS_STATE *o, char *name, int maxnamelen, char *value, int maxvaluelen);

/*===================================================================
** Exception handling code
===================================================================*/

#define EXCEPTION_OTHER 0
#define EXCEPTION_STARVED 1
#define EXCEPTION_BADUSAGE 2
#define EXCEPTION_MISSING 3
#define EXCEPTION_OVERFLOW 4

/* This function should be replaced if real exceptions are desired. */
void libppr_throw(int exception_type, const char function[], const char format[], ...)
#ifdef __GNUC__
__attribute__ (( noreturn, format (printf, 3, 4) ))
#endif
;

/*===================================================================
** SNMP functions
===================================================================*/

struct gu_snmp
    {
    int socket;
    const char *community;
    unsigned int request_id;
    char result[1024];
    };

struct gu_snmp *gu_snmp_open(unsigned long int ip_address, const char community[], int *error_code);
void gu_snmp_close(struct gu_snmp *p);
int gu_snmp_get(struct gu_snmp *p, int *error_code, ...);

#define GU_SNMP_INT 1
#define GU_SNMP_STR 2
#define GU_SNMP_BIT 3

/*===================================================================
** Perl Compatibility Functions
===================================================================*/

/* Perl Compatible String */
void *gu_pcs_new(void);
void gu_pcs_free(void **pcs);
void gu_pcs_debug(void **pcs, const char name[]);
void *gu_pcs_snapshot(void **pcs);
void gu_pcs_grow(void **pcs, int size);
void *gu_pcs_new_pcs(void **pcs);
void *gu_pcs_new_cstr(const char cstr[]);
void gu_pcs_set_cstr(void **pcs, const char cstr[]);
void gu_pcs_set_pcs(void **pcs, void **pcs2);
const char *gu_pcs_get_cstr(void **pcs);
int gu_pcs_bytes(void **pcs);
void gu_pcs_append_byte(void **pcs, int c);
void gu_pcs_append_cstr(void **pcs, const char cstr[]);
void gu_pcs_append_pcs(void **pcs, void **pcs2);
int gu_pcs_cmp(void **pcs1, void **pcs2);
int gu_pcs_hash(void **pcs_key);

/* Perl Compatible Hash */
void *gu_pch_new(int buckets_count);
void gu_pch_free(void **pch);
void gu_pch_debug(void **pch, const char name[]);
void gu_pch_set(void **pch, void **pcs_key, void **pcs_value);
void *gu_pch_get(void **pch, void **pcs_key);
void gu_pch_delete(void **pch, void **pcs_key);
void gu_pch_rewind(void **pch);
void *gu_pch_nextkey(void **pch);


/*===================================================================
** Replacements for missing functions
** Leave this last in the file.
===================================================================*/

#ifndef HAVE_STRSIGNAL
#define strsignal(signum) gu_strsignal(signum)
#endif
#ifndef HAVE_SNPRINTF
#define snprintf gu_snprintf
#endif
#ifndef HAVE_VSNPRINTF
#define vsnprintf(s, n, format, ap) gu_vsnprintf(s, n, format, ap)
#endif
#ifndef HAVE_MKSTEMP
#define mkstemp(template) gu_mkstemp(template)
#endif

#define strerror(err) gu_strerror(err)

#endif	/* _LIBGU */

/* end of file */
