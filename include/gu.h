/*
** mouse:~ppr/src/include/gu.h
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 19 April 2006.
*/

/*! \file
    \brief Generally Useful Library

This file contains the prototypes and macros for a library of functions which are 
likely to be useful many programs, not just in PPR.

*/

#ifndef _GU_H
#define _GU_H 1

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <stdarg.h>
#include <setjmp.h>
#include <stdlib.h>

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
** Define unix permissions 755 and 644.	 We do this because just
** saying 0755 or 644 is at least theoretically non-portable and
** because these portable expressions are long and unsightly.
*/
#define UNIX_755 (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#define UNIX_644 (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define UNIX_660 (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
#define UNIX_640 (S_IRUSR | S_IWUSR | S_IRGRP)
#define UNIX_600 (S_IRUSR | S_IWUSR)
#define UNIX_022 (S_IWGRP | S_IWOTH)
#define UNIX_002 S_IWOTH
#define UNIX_077 (S_IRWXG | S_IRWXO)
#define UNIX_770 (S_IRWXU | S_IRWXG)

/*
** Macros for looking for keywords at the start of strings.
*/

/** If b matches the first part of a, return true. */
#define lmatch(a, b) (!strncmp(a, b, sizeof(b) - 1))

/** If b matches the first part of a, return a pointer to the first word in a that comes after the match. */
#define lmatchp(a, b) (!strncmp(a, b, sizeof(b) - 1) ? a + sizeof(b) - 1 + strspn(a + sizeof(b) - 1, " \t") : NULL)

/** If b matches the first part of a with a white space following, return pointer to first part 
   after whitespace. */
#define lmatchsp(a, b) (!strncmp(a, b, sizeof(b) - 1) && isspace(a[sizeof(b) - 1]) ? a + sizeof(b) - 1 + strspn(a + sizeof(b) - 1, " \t") : NULL)

/** If b matches the last part of a, return true. */
#define gu_rmatch(a, b) (strlen(a) >= strlen(b) && strcmp(a + strlen(a) - strlen(b), b) == 0)

/** Write constant string s to file descriptor s using write(). */
#define gu_write_string(fd, s) (write(fd, s, sizeof(s) - 1))

/** If b matches the name in name=value pair a, return pointer to value */
#define gu_name_matchp(a, b) (!strncmp(a, b, sizeof(b) - 1) && a[sizeof(b) - 1] == '=' ? a + sizeof(b) : NULL)

/** send a string to stdout */
#define gu_puts(s) fputs(s,stdout)

/** send a line to stdout and add a newline */
#define gu_putline(l) puts(l)

/*===================================================================
** Memory allocation routines
===================================================================*/

/* Actual allocation functions */
void *gu_alloc(size_t number, size_t size);
void *gu_malloc(size_t size);
void *gu_realloc(void *ptr, size_t number, size_t size);
char *gu_strdup(const char *string);
char *gu_strndup(const char *string, size_t len);
char *gu_restrdup(char *ptr, size_t *number, const char *string);
void gu_free(void *ptr);
void gu_free_if(void *ptr);

/* Pool functions */
void *gu_pool_new(void);
void gu_pool_free(void *p);
const void *gu_pool_return(const void * block);
void *gu_pool_push(void *p);
void *gu_pool_pop(void *p);
void gu_pool_suspend(gu_boolean suspend);

/* Debugging functions */
int gu_alloc_checkpoint(void);
void _gu_alloc_assert(const char *file, int line, int assertion);
/** Make an assertion about the number of allocated memory blocks */
#define gu_alloc_assert(assertion) _gu_alloc_assert(__FILE__, __LINE__, assertion)

/*===================================================================
** The gu_ini_ routines read files in a format inspired by Microsoft
** Windows .ini files and Samba's smb.conf file.
===================================================================*/

struct GU_INI_ENTRY
	{
	char *name;			/* left hand side (not const so we can free) */
	const char *values; /* list from right hand side (const so we can't free) */
	int nvalues;		/* number of members in right hand side list */
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
const struct GU_INI_ENTRY *gu_ini_section_get_value_by_index(const struct GU_INI_ENTRY *section, int key_index);
void gu_ini_section_free(struct GU_INI_ENTRY *section);
const char *gu_ini_value_index(const struct GU_INI_ENTRY *array, int array_index, const char *default_value);
int gu_ini_assign(const struct GU_INI_ENTRY *array, ...);
int gu_ini_vassign(const struct GU_INI_ENTRY *array, va_list args);
const char *gu_ini_scan_list(const char file_name[], const char section_name[], const char key_name[], ...);
char *gu_ini_query(const char file_name[], const char section_name[], const char key_name[], int index, const char default_value[]);
int gu_ini_section_from_sample(const char filename[], const char section_name[]);

/*===================================================================
** Other stuff
===================================================================*/

void (*signal_interupting(int signum, void (*handler)(int sig)))(int);
void (*signal_restarting(int signum, void (*handler)(int sig)))(int);
char *gu_getline(char *line, int *space_available, FILE *fstream);
char *gu_strerror(int n);
int gu_wildmat(const char *text, const char *p);
int compile_string_escapes(char *s);
char *gu_strsep(char **stringp, const char *delim);
char *gu_strsep_quoted(char **stringp, const char *delim, const char *discard);
void gu_set_cloexec(int fd);
void gu_nonblock(int fd, gu_boolean on);
void gu_trim_whitespace_right(char *s);
int gu_strcasecmp(const char *s1, const char *s2);
int gu_strncasecmp(const char *s1, const char *s2, int n);
int gu_lock_exclusive(int filenum, int waitmode);
int gu_torf_setBOOL(gu_boolean *b, const char *s);
double gu_getdouble(const char *);
const char *gu_dtostr(double);
void ASCIIZ_to_padded(char *padded, const char *asciiz, int len);
void padded_to_ASCIIZ(char *asciiz, const char *padded, int len);
gu_boolean padded_cmp(const char *padded1, const char *padded2, int len);
gu_boolean padded_icmp(const char *padded1, const char *padded2, int len);
int gu_ascii_isdigit(int c);
int gu_ascii_digit_value(int c);
int gu_ascii_isxdigit(int c);
int gu_ascii_xdigit_value(int c);
int gu_ascii_isspace(int c);
int gu_sscanf(const char *input, const char *pattern, ...);
int gu_fgetint(FILE *input);
void gu_daemon_close_fds(void);
void gu_daemon(const char progname[], gu_boolean standalone, mode_t daemon_umask, const char lockfile[]);
int disk_space(const char *path, unsigned int *free_blocks, unsigned int *free_files);
int gu_wordwrap(char *string, int width);
int gu_wrap_printf(const char format[], ...)
	#ifdef __GNUC__
	__attribute__ (( format (printf, 1, 2) ))
	#endif
	;
int gu_wrap_eprintf(const char format[], ...)
	#ifdef __GNUC__
	__attribute__ (( format (printf, 1, 2) ))
	#endif
	;
int gu_vsnprintf (char *str, size_t count, const char *fmt, va_list args)
	#ifdef __GNUC__
	__attribute__ (( format (printf, 3, 0) ))
	#endif
	;
int gu_snprintf(char *str, size_t count, const char *fmt, ...)
	#ifdef __GNUC__
	__attribute__ (( format (printf, 3, 4) ))
	#endif
	;
int gu_vasprintf(char **ptr, const char *format, va_list ap)
	#ifdef __GNUC__
	__attribute__ (( format (printf, 2, 0) ))
	#endif
	;
int gu_asprintf(char **ptr, const char *format, ...)
	#ifdef __GNUC__
	__attribute__ (( format (printf, 2, 3) ))
	#endif
	;
size_t gu_strlcpy(char *dst, const char *src, size_t siz);
size_t gu_strlcat(char *dst, const char *src, size_t siz);
int gu_mkstemp(char *template);
char *gu_strsignal(int signum);
int gu_snprintfcat(char *buffer, size_t max, const char *format, ...);
int gu_timeval_cmp(const struct timeval *t1, const struct timeval *t2);
void gu_timeval_sub(struct timeval *t1, const struct timeval *t2);
void gu_timeval_add(struct timeval *t1, const struct timeval *t2);
void gu_timeval_cpy(struct timeval *t1, const struct timeval *t2);
void gu_timeval_zero(struct timeval *t);
int gu_runl(const char *myname, FILE *errors, const char *progname, ...);
void gu_psprintf(const char *format, ...)
	#ifdef __GNUC__
	__attribute__ (( format (printf, 1, 2) ))
	#endif
	;
char *gu_ascii_strlower(char *string);
char *gu_strtrim(char *string);
char *gu_stresc_convert(char *string);
char *gu_name_int_value(const char name[], int value);
char *gu_name_str_value(const char name[], const char value[]);
char *gu_name_long_value(const char name[], long int value);

/*===================================================================
** Unicode and internationalization stuff
===================================================================*/

/* gu_locale.c */
void gu_locale_init(int argc, char *argv[], const char *domainname, const char *localedir);
size_t gu_utf8_strftime(char *s, size_t max, const char *format, const struct tm *tm);
const char *gu_utf8_getenv(const char name[]);

/* gu_utf8_decode.c */
wchar_t gu_utf8_fgetwc(FILE *f);
wchar_t gu_utf8_sgetwc(const char **pp);

/* gu_utf8_printf.c */
int gu_utf8_vfprintf(FILE *f, const char *format, va_list args)
	#ifdef __GNUC__
	__attribute__ (( format (printf, 2, 0) ))
	#endif
	;
int gu_utf8_fprintf(FILE *f, const char *format, ...)
	#ifdef __GNUC__
	__attribute__ (( format (printf, 2, 3) ))
	#endif
	;
int gu_utf8_printf(const char *format, ...)
	#ifdef __GNUC__
	__attribute__ (( format (printf, 1, 2) ))
	#endif
	;

/* gu_utf8_put.c */
wchar_t gu_fputwc(wchar_t wc, FILE *f);
int gu_utf8_fputs(const char *string, FILE *f);
wchar_t gu_putwc(wchar_t wc);
int gu_utf8_puts(const char *string);
int gu_utf8_putline(const char *string);

/*===================================================================
** Command line option parsing
===================================================================*/

/** an array of these hold the list of valid long options */
struct gu_getopt_opt
		{
		const char *name;
		int code;
		gu_boolean needsarg;
		} ;

/** holds state of command line option parser */
struct gu_getopt_state
		{
		int argc;										/* private */
		char **argv;									/* private */
		const char *opt_chars;							/* private */
		const struct gu_getopt_opt *opt_words;			/* private */
		int optind;										/* public */
		char *optarg;									/* public */
		const char *name;								/* public */
		int x;											/* very private */
		int len;										/* very private */
		char scratch[3];								/* very private */
		char *putback;
		} ;

void gu_getopt_init(struct gu_getopt_state *state, int argc, char **argv, const char *opt_chars, const struct gu_getopt_opt *opt_words);
int ppr_getopt(struct gu_getopt_state *state);
void gu_getopt_default(const char myname[], int optchar, const struct gu_getopt_state *getopt_state, FILE *errors);

/*===================================================================
** Parse space separated name=value pairs
===================================================================*/

struct OPTIONS_STATE {
	int magic;							/* should be 689 */

	const char *options;				/* the string we are working on */

	int index;							/* index of where we are working now */
	int index_of_name;					/* index of start of this name=value pair */
	int index_of_prev_name;				/* index of start of prev pair */

	const char *error;					/* an error message */

	int next_time_skip;					/* distance from index to start of next name */
	};

void options_start(const char *options_str, struct OPTIONS_STATE *o);
int options_get_one(struct OPTIONS_STATE *o, char *name, int maxnamelen, char *value, int maxvaluelen);

/*===================================================================
** Exception handling code
===================================================================*/

extern char gu_exception[];					/* text of exception message */
extern int  gu_exception_code;				/* machine readable message */
extern int  gu_exception_try_depth;			/* how deap are we? */
extern int  gu_exception_temp;
extern int  gu_exception_debug;

/** Start an exception handling Try block.
 *
 * This macro creates a setjmp() context can calls
 * gu_Try_funct() which saves it in an array.  The array enables gu_Throw()
 * to find the context even if it is called from inside a function called
 * by the function which called gu_Try().
 */
#define gu_Try { \
	jmp_buf gu_exception_jmp_buf; \
	int gu_exception_setjmp_retcode; \
	int gu_exception_pop = 1; \
	gu_Try_funct(&gu_exception_jmp_buf); \
	if((gu_exception_setjmp_retcode = setjmp(gu_exception_jmp_buf)) == 0)
	
void gu_Try_funct(jmp_buf *p_jmp_buf);
void gu_Throw(const char message[], ...)
#ifdef __GNUC__
__attribute__ (( noreturn, format (printf, 1, 2) ))
#endif
;
void gu_CodeThrow(int code, const char message[], ...)
#ifdef __GNUC__
__attribute__ (( noreturn, format (printf, 2, 3) ))
#endif
;
void gu_ReThrow(void)
#ifdef __GNUC__
__attribute__ (( noreturn ))
#endif
;

/** Run cleanup code during exception handing.

A gu_Final block is executed after the gu_Try block regardless of whether an
exception was caught.  If an exception was caught, then the gu_Catch block
is executed after the gu_Final block.

Note that the semantics of gu_Final are difference from the semantics of
Java finally blocks.  Java finally blocks are executed after any matching
catch clauses.
   
Note that exceptions which occur in the gu_Final block are not caught by the
associated gu_Catch block.
   
*/
#define gu_Final \
gu_exception_try_depth--; \
gu_exception_pop = 0; \
if(1)

/** Handle exceptions encountered in gu_Try

The block introduced by this macro is executed if an exception was
caught in the gu_Try block.

*/
#define gu_Catch \
	gu_exception_try_depth -= gu_exception_pop; \
	gu_exception_temp = gu_exception_setjmp_retcode; \
	} \
if(gu_exception_temp != 0)

/** make the indicated object memory pool current in an exception-safe manner */
#define GU_OBJECT_POOL_PUSH(pool) gu_pool_push(pool); gu_Try {

/** pop the indicated object memory pool in an exception-safe manner */
#define GU_OBJECT_POOL_POP(pool) } gu_Final { gu_pool_pop(pool); } gu_Catch { gu_ReThrow(); }

/*===================================================================
** SNMP functions
===================================================================*/

/** An object which can make SNMP queries */
struct gu_snmp
	{
	int socket;
	const char *community;
	unsigned int request_id;
	char result[1024];
	int result_len;
	};

/*
** An array of these structures is used to hold a list of the expected return
** values, their types, and pointers to variables into which they should
** be stored.
*/
struct gu_snmp_items {
	const char *oid;
	int type;
	void *ptr;
	};

struct gu_snmp *gu_snmp_open(unsigned long int ip_address, const char community[]);
void gu_snmp_close(struct gu_snmp *p);
void gu_snmp_get(struct gu_snmp *p, ...);
int gu_snmp_fd(struct gu_snmp *p);
char *gu_snmp_recv_buf(struct gu_snmp *p, int *len);
void gu_snmp_set_result_len(struct gu_snmp *p, int len);
int gu_snmp_create_packet(struct gu_snmp *p, char *buffer, int *request_id, struct gu_snmp_items *items, int items_count);
int gu_snmp_parse_response(struct gu_snmp *p, int request_id, struct gu_snmp_items *items, int items_count);

enum GU_SNMP_TYPES {
	GU_SNMP_INT=1,		/**< tells gu_snmp_get() to fetch an integer value */
	GU_SNMP_STR=2,		/**< tells gu_snmp_get() to fetch a string value */
	GU_SNMP_BIT=3		/**< tells gu_snmp_get() to fetch a bitstring value */
	} ;

/*===================================================================
** Perl Compatibility Functions
===================================================================*/

/* Perl Compatible String */
void *gu_pcs_new(void);
void *gu_pcs_new_pcs(void **pcs);
void *gu_pcs_new_cstr(const char cstr[]);
void gu_pcs_free(void **pcs);
char *gu_pcs_free_keep_cstr(void **pcs);
void gu_pcs_debug(void **pcs, const char name[]);
void *gu_pcs_snapshot(void **pcs);
void gu_pcs_grow(void **pcs, int size);
void gu_pcs_set_cstr(void **pcs, const char cstr[]);
void gu_pcs_set_pcs(void **pcs, void **pcs2);
const char *gu_pcs_get_cstr(void **pcs);
char *gu_pcs_get_editable_cstr(void **pcs);
int gu_pcs_length(void **pcs);
int gu_pcs_truncate(void **pcs, size_t newlen);
void gu_pcs_append_char(void **pcs, int c);
void gu_pcs_append_cstr(void **pcs, const char cstr[]);
void gu_pcs_append_pcs(void **pcs, void **pcs2);
void gu_pcs_append_sprintf(void **pcs, const char format[], ...);
int gu_pcs_cmp(void **pcs1, void **pcs2);

/* Perl Compatible Hash */
void *gu_pch_new(int bucket_count);
void gu_pch_free(void *pch);
void gu_pch_debug(void *pch, const char name[]);
void gu_pch_set(void *pch, char key[], void *value);
void *gu_pch_get(void *pch, const char key[]);
void *gu_pch_delete(void *pch, char key[]);
void gu_pch_rewind(void *pch);
char *gu_pch_nextkey(void *pch, void **value);
int gu_pch_size(void *pch);
int gu_hash(const char string[], int modus);

/* Perl Compatible Array */
void *gu_pca_new(int initial_size, int increment);
void  gu_pca_free(void *pca);
int gu_pca_size(void *pca);
void *gu_pca_index(void *pca, int index);
void *gu_pca_pop(void *pca); 
void  gu_pca_push(void *pca, void *item); 
void *gu_pca_shift(void *pca); 
void  gu_pca_unshift(void *pca, void *item); 
char **gu_pca_ptr(void *pca);
char *gu_pca_join(const char separator[], void *array);

/* Perl Compatible Regular Expressions */
void *gu_pcre_match(const char pattern[], const char string[]);
void *gu_pcre_split(const char pattern[], const char string[]);

/*===================================================================
** HTTP functions
===================================================================*/

struct URI {
	char *method;
	char *node;
	int port;
	char *path;
	char *dirname;
	char *basename;
	char *query;
	};

struct URI *gu_uri_new(const char uri_string[]);
void gu_uri_free(struct URI *uri);

/*===================================================================
** Replacements for frequently missing functions
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
#ifndef HAVE_ASPRINTF
#define asprintf gu_asprintf
#endif
#ifndef HAVE_MKSTEMP
#define mkstemp(template) gu_mkstemp(template)
#endif
#ifndef HAVE_STRLCPY
#define strlcpy(a,b,c) gu_strlcpy(a,b,c)
#endif
#ifndef HAVE_STRLCAT
#define strlcat(a,b,c) gu_strlcat(a,b,c)
#endif
#if 1
#define strerror(err) gu_strerror(err)
#endif

#endif	/* _LIBGU */

/* end of file */
