/*
** mouse:~ppr/src/ppr-papd.h
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
** Last modified 10 January 2003.
*/

#include "queueinfo.h"

#define PIDFILE RUNDIR"/ppr-papd.pid"
#define LOGFILE LOGDIR"/ppr-papd"

#define PPR_PAPD_MAX_NAMES 64			/* maximum number of advertized names */
#define MAX_ARGV 25				/* max arg vector length for invoking ppr */

/* These two timeouts may not be implemented: */
#define READ_TIMEOUT 60*60*1000			/* 1 hour in milliseconds */
#define WRITE_TIMEOUT 60*1000			/* 1 minute in milliseconds */

#define MY_QUANTUM 8				/* the flow quantum at our end */
#define READBUF_SIZE MY_QUANTUM * 512		/* For reading from the client */

#define MAX_REMOTE_QUANTUM 1 			/* don't increase this, Mac client can't take it! */
#define WRITEBUF_SIZE MAX_REMOTE_QUANTUM * 512	/* buffer size for writing to the client */

/* #define DEBUG 1 */

#ifdef DEBUG
#define DEBUG_STARTUP 1			/* reading config, adding names and such */
/* #define DEBUG_QUERY 1 */		/* answering queries */
#define DEBUG_LOOP 1 			/* debug main loop */
/* #define DEBUG_PRINTJOB 1 */		/* debug printjob() */
/* #define DEBUG_PRINTJOB_DETAILED 1 */	/* debug printjob() */
/* #define DEBUG_PPR_ARGV 1 */		/* print argv[] when execting ppr */
/* #define DEBUG_READBUF 1 */		/* debug input buffering */
/* #define DEBUG_WRITEBUF 1 */		/* debug output buffering */
/* #define DEBUG_REAPCHILD 1 */		/* debug child daemon termination */
/* #define DEBUG_PPD 1 */		/* PPD file parsing */
#endif

/*============ end of stuff user might wish to modify ==============*/

/*
** AppleTalk name, type, and zone must not exceed this length.
** (This is part of the AppleTalk protocol definition, so there
** is no reason to change it.)
*/
#define MAX_ATALK_NAME_COMPONENT_LEN 32

/*
** Define macros for debugging.  Which version of each one
** is defined depends on the debugging options selected above.
*/
#ifdef DEBUG_STARTUP
#define DODEBUG_STARTUP(a) debug a
#else
#define DODEBUG_STARTUP(a)
#endif

#ifdef DEBUG_QUERY
#define DODEBUG_QUERY(a) debug a
#else
#define DODEBUG_QUERY(a)
#endif

#ifdef DEBUG_LOOP
#define DODEBUG_LOOP(a) debug a
#else
#define DODEBUG_LOOP(a)
#endif

#ifdef DEBUG_PRINTJOB
#define DODEBUG_PRINTJOB(a) debug a
#else
#define DODEBUG_PRINTJOB(a)
#endif

#ifdef DEBUG_PRINTJOB_DETAILED
#define DODEBUG_PRINTJOB_DETAILED(a) debug a
#else
#define DODEBUG_PRINTJOB_DETAILED(a)
#endif

#ifdef DEBUG_READBUF
#define DODEBUG_READBUF(a) debug a
#else
#define DODEBUG_READBUF(a)
#endif

#ifdef DEBUG_WRITEBUF
#define DODEBUG_WRITEBUF(a) debug a
#else
#define DODEBUG_WRITEBUF(a)
#endif

#ifdef DEBUG_REAPCHILD
#define DODEBUG_REAPCHILD(a) debug a
#else
#define DODEBUG_REAPCHILD(a)
#endif

#ifdef DEBUG_AUTHORIZE
#define DODEBUG_AUTHORIZE(a) debug a
#else
#define DODEBUG_AUTHORIZE(a)
#endif

#ifdef DEBUG_PPD
#define DODEBUG_PPD(a) debug a
#else
#define DODEBUG_PPD(a)
#endif

enum ADV_TYPE { ADV_LAST, ADV_ACTIVE, ADV_RELOADING, ADV_DELETED };

/* Structure which describes each advertised name. */
struct ADV
    {
    enum ADV_TYPE adv_type;		/* active, delete, last, etc. */
    enum QUEUEINFO_TYPE queue_type;	/* alias, group, or printer */
    const char *PPRname;		/* PPR destination to submit to */
    const char *PAPname;		/* name to advertise */
    int fd;				/* file descriptor of listening socket */
    } ;

/* Structure used to describe an *Option entry. */
struct OPTION
    {
    char *name;
    char *value;
    struct OPTION *next;
    } ;

struct QUEUE_CONFIG
    {
    const char *PPDfile;	/* name of Adobe PPD file */
    const char **fontlist;	/* list of fonts in this printer */
    int fontcount;
    int LanguageLevel;          /* 1 or 2, 1 is default */
    char *PSVersion;            /* a rather complicated string */
    char *Resolution;           /* "300dpi", "600x300dpi" */
    gu_boolean BinaryOK;	/* TRUE or FALSE */
    int FreeVM;			/* free printer memory from "*FreeVM:" line */
    char *InstalledMemory;	/* Selected "*InstalledMemory" option */
    int VMOptionFreeVM;		/* Value from selected "*VMOption" */
    const char *Product;	/* *Product string from PPD file */
    gu_boolean ColorDevice;	/* TRUE or FALSE */
    int RamSize;		/* an integer (LaserWriter 8) */
    char *FaxSupport;		/* a string such as "Base" */
    char *TTRasterizer;		/* "None", "Type42", "Accept68K" */
    struct OPTION *options;	/* PPD file option settings */
    gu_boolean query_font_cache;
    } ;

extern struct ADV *adv;
extern int name_count;          /* total advertised names */
extern char line[];             /* input line */
extern int onebuffer;           /* buffer control flag */
extern int children;		/* count of children */
extern char *default_zone;	/* default zone for advertised names, initialy set to "*" */
extern int i_am_master;

/* routines in ppr-papd.c */
void fatal(int exitvalue, const char string[], ...);
void debug(const char string[], ...);
char *debug_string(char *s);
char *pap_getline(int sesfd);
void postscript_stdin_flushfile(int sesfd);
void sigpipe_handler(int signum);
void child_main_loop(int sesfd, int prnid, int net, int node);
void child_reapchild(int signum);

/* routines in ppr-papd_ali.c and ppr-papd_cap.c */
void appletalk_dependent_daemon_main_loop(struct ADV *adv);
int appletalk_dependent_printjob(int sesfd, int pipe);
int cli_getc(int sesfd);
void reset_buffer(int hard);
void reply(int sesfd, char *string);
void reply_eoj(int sesfd);
void close_reply(int sesfd);
int add_name(const char papname[]);
void remove_name(const char papname[], int fd);

/* routines in ppr-papd_printjob.c */
void printjob(int sesfd, struct ADV *adv, struct QUEUE_CONFIG *qc, int net, int node, const char log_file_name[]);
void printjob_abort(void);
void printjob_reapchild(void);

/* routines in ppr-papd_query.c */
void answer_query(int sesfd, struct QUEUE_CONFIG *qc);

/* routines in ppr-papd_conf.c */
struct ADV *conf_load(struct ADV *old_config);
void conf_load_queue_config(struct ADV *adv, struct QUEUE_CONFIG *queue_config);

/*
** Simulated PostScript error messages to send to a client when errors occur.
*/
#define MSG_NOCHARGEACCT "%%[ Error: you don't have a charge account ]%%\n"
#define MSG_BADAUTH "%%[ Error: password incorrect ]%%\n"
#define MSG_NOFOR "%%[ Error: No \"%%For:\" line in PostScript ]%%\n"
#define MSG_OVERDRAWN "%%[ Error: account overdrawn ]%%\n"
#define MSG_NONCONFORMING "%%[ Error: insufficient DSC conformance ]%%\n"
#define MSG_SYNTAX "%%[ Error: bad ppr invokation syntax ]%%\n"
#define MSG_NOSPOOLER "%%[ Error: spooler is not running ]%%\n"
#define MSG_FATALPPR "%%[ Error: fatal error, see ppr-papd log ]%%\n"
#define MSG_NOPROC "%%[ Error: spooler is out of processes ]%%\n"
#define MSG_NODISK "%%[ Error: spooler is out of disk space ]%%\n"
#define MSG_TRUNCATED "%%[ Error: input file is truncated ]%%\n"
#define MSG_ACL "%%[ Error: ACL forbids you access to selected print destination ]%%\n"

/* end of file */
