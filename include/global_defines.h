/*
** mouse:~ppr/src/include/global_defines.h
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
** The PPR project was begun 28 December 1992.
** This file was last modified 30 August 2003.
*/

/*
** There are many things in this file you may want to change.  This file
** should be the first include file.  It is the header file for the whole
** project.
*/

#ifndef _INC_BEFORE_SYSTEM
#warning Failed to include "before_system.h"
#include "before_system.h"
#endif

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
** These are used by lprsrv and papsrv.	 They will not accept new
** jobs unless at least this many inodes and blocks are free
** in TEMPDIR and VAR_SPOOL_PPR.
**
** If you change these you must recompile papsrv and lprsrv.
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

/*
** Include obsolete stuff?
*/
/* #define CRUFT_AUTH 1 */

/*======================================================================
** These are the default values for the PPR user and group names.  If
** you want to change these, don't do it here, it won't work.  You must
** change them in include/global.mk (which is created when you run
** Configure).
=======================================================================*/
#ifndef USER_PPR
#define USER_PPR "ppr"
#endif
#ifndef GROUP_PPR
#define GROUP_PPR "ppr"
#endif
#ifndef USER_PPRWWW
#define USER_PPRWWW "pprwww"
#endif

/*======================================================================
** This section defines various directory and file names.
** You shouldn't want to change anything in this section.
** If you do you may have to change some makefiles too.
======================================================================*/

/*
** These are the basic PPR directories in which its files and
** other directories are placed.  Notice that these are defined
** only if they have not been defined already.	Often, Configure
** will have generated an include/global.mk which defines these
** values first.
*/
#ifndef CONFDIR
#define CONFDIR "/etc/ppr"				/* configuration files */
#endif
#ifndef HOMEDIR
#define HOMEDIR "/usr/lib/ppr"			/* architechture dependent files */
#endif
#ifndef SHAREDIR
#define SHAREDIR "/usr/share/ppr"		/* architechture independent files */
#endif
#ifndef VAR_SPOOL_PPR
#define VAR_SPOOL_PPR "/var/spool/ppr"	/* work files */
#endif
#ifndef TEMPDIR
#define TEMPDIR "/tmp"					/* short life temporary files */
#endif
#ifndef SYSBINDIR
#define SYSBINDIR "/usr/bin"			/* for standard system programs */
#endif
#ifndef XWINBINDIR
#define XWINBINDIR "/usr/bin/X11"		/* for standard X-Windows programs */
#endif

/* Subdirectories */
#define RUNDIR VAR_SPOOL_PPR"/run"
#define VAR_PRINTERS VAR_SPOOL_PPR"/printers"

/* The main PPR configuration file. */
#define PPR_CONF CONFDIR"/ppr.conf"

/* Various configuration files: */
#define DBNAME CONFDIR"/charge_users.db"						/* users database file name */
#define MEDIAFILE CONFDIR"/media.db"							/* media definitions */
#define NEWPRN_CONFIG CONFDIR"/newprn.conf"						/* new printer configuration lines */
#define UPRINTCONF CONFDIR"/uprint.conf"						/* for libuprint.a */
#define UPRINTREMOTECONF CONFDIR"/uprint-remote.conf"			/* list of remote printers */

/* Configuration files that aren't meant to be changed: */
#define MFMODES SHAREDIR"/lib/mfmodes.conf"						/* MetaFont modes for various printers */
#define FONTSUB SHAREDIR"/lib/fontsub.conf"						/* font substitution database */
#define LW_MESSAGES_CONF SHAREDIR"/lib/lw-messages.conf"		/* LaserWriter errors file */
#define PJL_MESSAGES_CONF SHAREDIR"/lib/pjl-messages.conf"		/* PJL USTATUS DEVICE errors file */
#define EDITPSCONF HOMEDIR"/editps/editps.conf"					/* for ppr -H editps */
#define CHARSETSCONF SHAREDIR"/lib/charsets.conf"				/* map characters set to PostScript encodings */
#define FONTSCONF SHAREDIR"/lib/fonts.conf"						/* list of fonts and the PostScript encodings they support */
#define PAGESIZES_CONF SHAREDIR"/lib/pagesizes.conf"			/* additional PostScript *PageSize names */
#define PSERRORS_CONF SHAREDIR"/lib/pserrors.conf"				/* additional PostScript error explainations */

/* Various configuration directories: */
#define PRCONF CONFDIR"/printers"				/* printer configuration files */
#define GRCONF CONFDIR"/groups"					/* group configuration files */
#define ALIASCONF CONFDIR"/aliases"				/* queue alias configuration files */
#define MOUNTEDDIR CONFDIR"/mounted"			/* directory for media mounted files */
#define ACLDIR CONFDIR"/acl"					/* Access Control Lists */

/* Special files used by the spooler: */
#define NEXTIDFILE RUNDIR"/lastid_ppr"			/* file with previous queue id number */
#define FIFO_NAME VAR_SPOOL_PPR"/PIPE"			/* name of pipe between ppr & pprd */
#define PPRD_LOCKFILE RUNDIR"/pprd.pid"			/* created and locked by pprd */

/* Directories where the spooler and friends find components: */
#define FILTDIR HOMEDIR"/filters"				/* directory for input filter programs */
#define INTDIR HOMEDIR"/interfaces"				/* directory for interface programs */
#define COMDIR HOMEDIR"/commentators"			/* directory for commentator programs */
#define RESPONDERDIR HOMEDIR"/responders"		/* responder programs */
#define PPDDIR SHAREDIR"/PPDFiles"				/* our PPD file library (must be absolute) */
#define STATIC_CACHEDIR SHAREDIR"/cache"		/* pre-loaded cache files */

/* Directories where the spooler writes stuff: */
#define QUEUEDIR VAR_SPOOL_PPR"/queue"			/* queue directory */
#define DATADIR VAR_SPOOL_PPR"/jobs"			/* job data directory */
#define ALERTDIR VAR_PRINTERS"/alerts"			/* directory for printer alert files */
#define STATUSDIR VAR_PRINTERS"/status"			/* directory for printer status files */
#define LOGDIR VAR_SPOOL_PPR"/logs"				/* directory for log files */
#define CACHEDIR VAR_SPOOL_PPR"/cache"			/* directory for automatically cached files */
#define ADDRESS_CACHE VAR_PRINTERS"/addr_cache" /* directory for cache of printer addresses */
#define FONT_INDEX VAR_SPOOL_PPR"/fontindex.db"

/* The spooler state file for GUI interface: */
#define STATE_UPDATE_FILE RUNDIR"/state_update"
#define STATE_UPDATE_MAXLINES 1000
#define STATE_UPDATE_PPRDRV_FILE RUNDIR"/state_update_pprdrv"
#define STATE_UPDATE_PPRDRV_MAXBYTES 30000

/* If this file exists, it will be filled with a log of all jobs printed: */
#define PRINTLOG_PATH LOGDIR"/printlog"

/* Paths to invoke various PPR components: */
#define PPRDRV_PATH HOMEDIR"/lib/pprdrv"
#define PPAD_PATH HOMEDIR"/bin/ppad"
#define PPOP_PATH HOMEDIR"/bin/ppop"
#define PPR_PATH HOMEDIR"/bin/ppr"
#define TBCP2BIN_PATH HOMEDIR"/lib/tbcp2bin"
#define PPR2SAMBA_PATH HOMEDIR"/bin/ppr2samba"
#define TAIL_STATUS_PATH HOMEDIR"/lib/tail_status"
#define PPJOB_PATH HOMEDIR"/bin/ppjob"

/*=====================================================================
** Some Practical Limits
=====================================================================*/

#define MAX_LINE 1024					/* maximum PostScript input line length (now pretty meaningless) */
#define MAX_CONT 32						/* maximum segment represented by "%%+" */
#define MAX_TOKENIZED 512				/* longest line we may pass to tokenize() */
#define MAX_TOKENS 20					/* limit on words per comment line */
#define MAX_PPR_PATH 128				/* space to reserve for building a file name */

#define MAX_BINNAME 16					/* max chars in name of input bin */
#define MAX_MEDIANAME 16				/* max chars in media name */
#define MAX_COLOURNAME 16				/* max chars in colour name */
#define MAX_TYPENAME 16					/* max chars media type name */

#define MAX_DOCMEDIA 4					/* max media types per job */

#define MAX_DESTNAME 16					/* max length of destination name */
#define MAX_NODENAME 16					/* max length of node name */
#define MAX_NODES 25					/* max number of nodes pprd can keep track of */
#define MAX_PRINTERS 250				/* no more than 250 printers */
#define MAX_BINS 10						/* max bins per printer */
#define MAX_GROUPS 150					/* no more than this may groups */
#define MAX_GROUPSIZE 8					/* no more than 8 printers per group */
#define MAX_ALIASES 150					/* no more than 150 queue aliases */

#define MAX_STATUS_MESSAGE 80			/* maximum length of last message from printer */

#define MAX_PPD_NEST 10					/* maximum PPD file include levels */
#define MAX_PPD_LINE 255				/* maximum line length for PPD files */
#define MAX_VMOPTIONS 50				/* maximun number of *VMOption lines in PPD file */

/*=========================================================================
** End of values which an end user might want to change.
=========================================================================*/

/*=======================================================================
** System Dependent Stuff
** The system differences are handled in a separate file which
** is included here.
=======================================================================*/

#define PASS2
#include "sysdep.h"
#undef PASS2

/* A signed number of at least 16 bits: */
#ifndef SHORT_INT
typedef short int SHORT_INT;
#endif

/* Some of our code assumes that signal()
   sets a BSD style signal handler. */
#undef signal
#define signal(a,b) signal_interupting(a,b)


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
#define PACKAGE_PPRD "PPRD"
#define PACKAGE_PPRDRV "PPRDRV"
#define PACKAGE_PAPSRV "PAPSRV"
#define PACKAGE_PAPD "PAPD"
#define PACKAGE_PPRWWW "PPRWWW"

#ifdef INTERNATIONAL
#define _(String) gettext(String)
#else
#define gettext(String) (String)
#define _(String) (String)
#endif

#define gettext_noop(String) (String)
#define N_(String) (String)
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
** Allow Apple's non-standard quote mark quoting.  (Apple LaserWriter
** drivers may enclose a procset name in ASCII double quotes with the
** PostScript () quotes inside.
*/
#define APPLE_QUOTE 1

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
** Flags stored in the unix file permissions of a font in
** the resource cache.	Both pprdrv and ppr use these
** values.
*/
#define FONT_TYPE_1 1			/* Font has Type 1 components present */
#define FONT_TYPE_3 2
#define FONT_TYPE_42 4			/* Font has Type 42 components present */
#define FONT_MACTRUETYPE 8		/* Is a Macintosh TrueType font in PostScript form */
#define FONT_TYPE_TTF 16				/* Font is MS-Windows .ttf format file (file mode isn't really set) */

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

/* Possible Codes values: */
enum CODES { CODES_DEFAULT = -1, CODES_UNKNOWN = 0, CODES_Clean7Bit = 1, CODES_Clean8Bit = 2, CODES_Binary = 3, CODES_TBCP = 4 };

/*
** This is for find_cached_resource().
*/
enum RES_SEARCH
	{
	RES_SEARCH_CACHE,
	RES_SEARCH_FONTINDEX,
	RES_SEARCH_END
	};

/* Function prototypes */
char *datestamp(void);
void tokenize(void);
extern char *tokens[];
extern int tokens_count;
const char *quote(const char *);
int destination_protected(const char *destnode, const char *destname);
char *money(int amount_times_ten);
char *local_jobid(const char *destname, int id, int subid, const char *homenode);
char *remote_jobid(const char *destnode, const char *destname, int id, int subid, const char *homenode);
const char *network_destspec(const char *destnode, const char *destname);
int pagesize(const char keyword[], char **corrected_keyword, double *width, double *length, gu_boolean *envelope);
const char *noalloc_find_cached_resource(const char res_type[], const char res_name[], double version, int revision, const enum RES_SEARCH search_list[], int *new_revision, int *features, enum RES_SEARCH *where_found);
char *find_cached_resource(const char res_type[], const char res_name[], double version, int revision, const enum RES_SEARCH search_list[], int *new_revision, int *features, enum RES_SEARCH *where_found);
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
const char *ppr_get_nodename(void)
#ifdef __GNUC__
__attribute__ ((const))
#endif
;
int ppd_open(const char *name, FILE *errors);
char *ppd_readline(void);
void set_ppr_env(void);
void prune_env(void);
gu_boolean is_unsafe_ps_name(const char name[]);
gu_boolean is_pap_PrinterError(const unsigned char *status);
gu_boolean user_acl_allows(const char user[], const char acl[]);
void ppr_fnamef(char target[], const char pattern[], ...);
gu_boolean interface_default_feedback(const char interface[], const struct PPD_PROTOCOLS *prot);
int interface_default_jobbreak(const char interface[], const struct PPD_PROTOCOLS *prot);
enum CODES interface_default_codes(const char interface[], const struct PPD_PROTOCOLS *prot);
void valert(const char printername[], int dateflag, const char string[], va_list args);
void alert(const char printername[], int dateflag, const char string[], ...);
void tail_status(gu_boolean tail_pprd, gu_boolean tail_pprdrv, gu_boolean (*callback)(char *p, void *extra), int timeout, void *extra);
const char *dest_ppdfile(const char destnode[], const char destname[]);

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

