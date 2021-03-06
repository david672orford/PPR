/*
** mouse:~ppr/src/include/global_defines.h
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** The PPR project was begun 28 December 1992.
** This file was last modified 28 April 2006.
*/

/*
** There are many things in this file you may want to change.  This file
** should be the first include file.  It is the header file for the whole
** project.
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>

/*=================================================================
** Things that are a matter of personal preference:
=================================================================*/

/*
** Define this if you wish a user's printing privledges to be
** revoked only if he tries to print during business hours when
** he doesn't have any money left in his account.  If this is
** not defined, his privledges are revoked even if he runs out of
** money in the middle of the night.
**
** If you change this you must recompile libpprdb and pprdrv.
*/
/* #define BUSINESS_HOURS 1 */

/*
** These are used by lprsrv and papd.	 They will not accept new
** jobs unless at least this many inodes and blocks are free
** in TEMPDIR and VAR_SPOOL_PPR.
**
** If you change these you must recompile papd and lprsrv.
*/
#define MIN_INODES 100
#define MIN_BLOCKS 2048

/*
** Define if we should include code to make GNU-C happy.
** This generaly means initializing a variable to zero
** in order to supress incorrect warnings about possible
** use of an uninitialized variable.
*/
#define GNUC_HAPPY 1

/*
** Allow Apple's non-standard quote mark quoting.  (Apple LaserWriter
** drivers may enclose a procset name in ASCII double quotes with the
** PostScript () quotes inside.	 Don't change this, you won't like
** the results.
*/
#define APPLE_QUOTE 1

/*
** Define this if you have an AppleTalk printer which gives
** status messages with spaces missing after colons or
** line feed characters appended.  Just leave this defined.
*/
#define DESKJET_STATUS_FIX 1

/*
** This is the prefix that PPR uses for any comments
** which have been created especially for it.  I do
** not recommend changing this.	 The value below stands
** for "Trinity College, Hartford, Connecticut".  See
** RBIIpp 696-698 for furthur information on this topic.
*/
#define PPR_DSC_PREFIX "TCHCT"

/*=====================================================================
** Some Practical Limits
=====================================================================*/

#define MAX_LINE 1024				/* maximum PostScript input line length (now pretty meaningless) */
#define MAX_CONT 32					/* maximum segment represented by "%%+" */
#define MAX_TOKENIZED 512			/* longest line we may pass to tokenize() */
#define MAX_TOKENS 20				/* limit on words per comment line */
#define MAX_PPR_PATH 128			/* space to reserve for building a file name */

#define MAX_BINNAME 16				/* max chars in name of input bin */
#define MAX_MEDIANAME 16			/* max chars in media name */
#define MAX_COLOURNAME 16			/* max chars in colour name */
#define MAX_TYPENAME 16				/* max chars media type name */

#define MAX_DOCMEDIA 4				/* max media types per job */

#define MAX_PRINTERS 250			/* no more than 250 printers */
#define MAX_BINS 10					/* max bins per printer */
#define MAX_GROUPS 150				/* no more than this may groups */
#define MAX_GROUPSIZE 8				/* no more than 8 printers per group */

#define STATE_UPDATE_MAXLINES 1000
#define STATE_UPDATE_PPRDRV_MAXBYTES 30000

/*=======================================================================
** System Dependent Stuff
** These are adjustments that are made on the basis of things defined
** in config.h or in the system header files.
=======================================================================*/

/* A signed number of at least 16 bits: */
typedef short int INT16_T;
typedef unsigned short int UINT16_T;

/* Some of our code assumes that signal()
   sets a BSD style signal handler. */
#undef signal
#define signal(a,b) signal_interupting(a,b)

/* BSD symbolic links aren't part of POSIX.  Some systems implement
 * them and define S_IFLNK but mysteriously fail to define the test
 * macro S_ISLNK().
 */
#ifndef S_ISLNK
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#endif

/* uClibc and dietlibc don't support NIS.  We have to put this here in 
 * global_defines.h since no uClibc headers have been read at the
 * time config.h is included, thus these tests are not possible. */
#ifdef __UCLIBC__
#undef HAVE_INNETGRP
#endif
#ifdef __dietlibc__
#undef HAVE_INNETGRP
#endif

#ifndef WCOREDUMP
#define WCOREDUMP(stat) ((stat)&0200)
#endif

#ifndef HAVE_SETSID
#define setsid() setpgrp(0, getpid())
#endif

#ifndef HAVE_DIFFTIME
#define difftime(t1,t0) ((double)((t1)-(t0)))
#endif

#ifndef HAVE_MEMMOVE
#define memmove(ARG1,ARG2,LENGTH) bcopy(ARG1,ARG2,LENGTH)
#endif

#ifdef HAVE_SETRESUID
#ifndef HAVE_SETEUID
#define seteuid(x)     setresuid(-1,(x),-1)
#endif
#ifndef HAVE_SETREUID
#define setreuid(x,y)  setresuid((x),(y),-1)
#endif
#endif

#ifdef HAVE_SETRESGID
#ifndef HAVE_SETEGID
#define setegid(x)     setresgid(-1,(x),-1)
#endif
#ifndef HAVE_SETREGID
#define setregid(x,y)  setresgid((x),(y),-1)
#endif
#endif

/* C99 provides va_copy().  In draft proposals it was called __va_copy(). */
#ifndef va_copy
#ifdef __va_copy
#define va_copy(dest,src) __va_copy(dest,src)
#else
#define va_copy(dest,src) dest=src
#endif
#endif

/*=======================================================================
** Internationalization Macros
**
** _()	returns the internationalized version of a string.
** N_() marks a string for internationalization but returns the
**		uninternationalized version.
** X_() marks a string which could be internationalized but won't be
**		because it is not worth it.
=======================================================================*/

#define LOCALEDIR SHAREDIR"/locale"
#define PACKAGE "PPR"
#define PACKAGE_INTERFACES "PPR"
#define PACKAGE_PPRWWW "PPRWWW"
#define PACKAGE_PPRD "PPRD"
#define PACKAGE_PPRDRV "PPRDRV"
#define PACKAGE_PAPD "PAPD"

#ifdef INTERNATIONAL
#define _(String) gettext(String)
#else
#define _(String) (String)
#define gettext(String) (String)
#define ngettext(sing,plur,n) (n == 1 ? sing : plur)
#endif

/** Mark for translation but don't pass thru gettext() */
#define N_(String) (String)
#define gettext_noop(String) (String)

/** Not worth translating */
#define X_(String) (String)

/*==================================================================
** Sundry constants which the end user should not change because
** they are not options or because changing their values will only
** lead to undesireable results.
==================================================================*/

/*
** Define if we should include code to make GNU-C happy.
** This generaly means initializing a variable to zero
** in order to supress incorrect warnings about possibly
** uninitialized variables.
*/
#ifdef __GNUC__
#define GNUC_HAPPY 1
#endif

/*
** Characters which are not allowed in printer
** and group names:
**
** The tilde is not allowed because at the head of the name it is a
** shell user name interpolation character and at the end it is an
** Emacs-style backup file.	 A period is not allowed as the initial
** character because it would make for a hidden configuration file.
*/
#define DEST_DISALLOWED "/\n\r\t \b~"
#define DEST_DISALLOWED_LEADING ".-"

/*
** Value which umask should be set to.
** The daemons and control programs set
** this soon after they begin to execute.
*/
#define PPR_UMASK UNIX_022

/*
** Pprd needs a slightly more relaxed umask so that it can communicate
** with ipp.
*/
#define PPR_PPRD_UMASK UNIX_002

/*
** This umask is used by the job submission program "ppr".
*/
#define PPR_JOBS_UMASK S_IRWXO

/*
** The umasks used when running interfaces and filters.	 These are
** very restrictive so as to protect the privacy of temporary
** files.
*/
#define PPR_INTERFACE_UMASK UNIX_077
#define PPR_FILTER_UMASK UNIX_077

/* Types of PostScript language extensions.	 This is used by ppr and pprdrv. */
#define EXTENSION_DPS 1
#define EXTENSION_CMYK 2
#define EXTENSION_Composite 4
#define EXTENSION_FileSystem 8

/*
** Valid banner and trailer options.
*/

/* ppr submits the jobs with one of these */
#define BANNER_DONTCARE 0
#define BANNER_YESPLEASE 1
#define BANNER_NOTHANKYOU 2

/* the printer configuration specifies one of these */
#define BANNER_FORBIDDEN 0
#define BANNER_DISCOURAGED 1
#define BANNER_ENCOURAGED 2
#define BANNER_REQUIRED 3
#define BANNER_INVALID 4			/* used internally in ppad */

/*
** These flags describe the formats of fonts available for transmission to
** printers.  Both pprdrv and ppr use these values.
*/
#define FONT_TYPE_1 1			/* Font has Type 1 components present */
#define FONT_TYPE_3 2
#define FONT_TYPE_42 4			/* Font has Type 42 components present */
#define FONT_MACTRUETYPE 8		/* Is a Macintosh TrueType font in PostScript form */
#define FONT_TYPE_TTF 16		/* Font is MS-Windows .ttf format file (file mode isn't really set) */

/* File modes used to represent some of those above in the file system. */
#define FONT_MODE_MACTRUETYPE S_IXUSR
#define FONT_MODE_TYPE_1 S_IXGRP
#define FONT_MODE_TYPE_42 S_IXOTH

/*
** Valid TrueType rasterizer settings.	These values
** are not communicated between ppr and pprdrv, but
** both use them.
*/
#define TT_UNKNOWN 0
#define TT_NONE 1
#define TT_ACCEPT68K 2
#define TT_TYPE42 3

/*
** Job Flags Bitmap Values
*/
#define JOB_FLAG_SAVE 1					/* job should be kept after printed */
#define JOB_FLAG_DO_NOTIFY 2			/* please schedual notification */
#define JOB_FLAG_NOTIFYING 4			/* notify in progress */
#define JOB_FLAG_QUESTION_UNANSWERED 8	/* there is a "Question:" not yet answered */
#define JOB_FLAG_QUESTION_ASKING_NOW 16 /* are we asking it now? */

/*=========================================================================
** Stuff in libppr.a
=========================================================================*/

/* Description of the protocol abilities of a printer. */
struct PPD_PROTOCOLS
	{
	gu_boolean TBCP;
	gu_boolean PJL;
	} ;

/* Possible Codes values.  This is in this is in global_defines.h rather
 * than interface.h because it is used to describe both the capabilities
 * of the interface and the characteristics of the document.
 */ 
#define CODES_DEFAULT -1
#define CODES_UNKNOWN 0
#define CODES_Clean7Bit 1
#define CODES_Clean8Bit 2
#define CODES_Binary 3
#define CODES_TBCP 4

/* Function prototypes */
char *datestamp(void);
void tokenize(void);
extern char *tokens[];
extern int tokens_count;
const char *quote(const char *);
gu_boolean destination_protected(const char destname[]);
char *money(int amount_times_ten);
const char *jobid(const char *destname, int id, int subid);
int pagesize(const char keyword[], char **corrected_keyword, double *width, double *length, gu_boolean *envelope);
char *find_resource(const char res_type[], const char res_name[], double version, int revision, int *features);
int get_responder_width(const char *name);
double convert_dimension(const char *string);
void filter_options_error(int exlevel, struct OPTIONS_STATE *o, const char *format, ...)
#ifdef __GNUC__
__attribute__ ((noreturn))
#endif
;
const char *pap_strerror(int err);
const char *nbp_strerror(int err);
const char *pap_look_string(int n);
char *ppr_get_command(const char *prompt, int machine_input);

char *ppd_find_file(const char ppdname[]);
typedef struct PPDOBJ *PPDOBJ;
#if 0
int ppd_open(const char name[], FILE *errors);
char *ppd_readline(void);
#endif
PPDOBJ ppdobj_new(const char ppdname[]);
void ppdobj_free(PPDOBJ self);
char *ppdobj_readline(PPDOBJ self);
void *ppd_finish_quoted_string(PPDOBJ self, char *initial_segment);
char *ppd_finish_QuotedValue(PPDOBJ self, char *initial_segment);
int ppd_decode_QuotedValue(char *p);

int renounce_root_privs(const char progname[], const char username[], const char groupname[]);
void set_ppr_env(void);
void prune_env(void);
gu_boolean is_unsafe_ps_name(const char name[]);
gu_boolean is_pap_PrinterError(const unsigned char *status);
gu_boolean username_match(const char username[], const char pattern[]);
gu_boolean user_acl_allows(const char user[], const char acl[]);
void ppr_fnamef(char target[], const char pattern[], ...);
gu_boolean interface_default_feedback(const char interface[], const struct PPD_PROTOCOLS *prot);
int interface_default_jobbreak(const char interface[], const struct PPD_PROTOCOLS *prot);
int interface_default_codes(const char interface[], const struct PPD_PROTOCOLS *prot);
void valert(const char printername[], int dateflag, const char string[], va_list args);
void alert(const char printername[], int dateflag, const char string[], ...);
void tail_status(gu_boolean tail_pprd, gu_boolean tail_pprdrv, gu_boolean (*callback)(char *p, void *extra), int timeout, void *extra);
const char *dest_ppdfile(const char destname[]);
int translate_snmp_error(int bit, const char **set_description, const char **set_raw1, int *set_severity);
int translate_snmp_status(int device_status, int printer_status, const char **set_description, const char **set_raw1, int *set_severity);
char *ppr_get_default(void);

/* RPC from ippd to pprd */
struct PPRD_CALL_RETVAL {
	int status_code;
	int extra_code;
	};
struct PPRD_CALL_RETVAL pprd_call(const char command[], ...)
	#ifdef __GNUC__
	__attribute__ ((format (printf, 1, 2)))
	#endif
	;
int pprd_status_code(struct PPRD_CALL_RETVAL retval);

/*
** The callers of certain libppr routines must provide an error()
** callback function so that certain non-fatal conditions may be
** noted in their log files.
*/
void error(const char string[], ...)
	#ifdef __GNUC__
	__attribute__ ((format (printf, 1, 2)))
	#endif
	;

/* end of file */

