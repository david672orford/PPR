/*
** mouse:~ppr/src/papsrv/papsrv.h
** Copyright 1995--2002, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 24 April 2002.
*/

/*
** Master include file for the Printer Access Protocol server.
*/

/* Default names for the configuration, pid, and log files. */
#define DEFAULT_PAPSRV_CONFFILE CONFDIR"/papsrv.conf"
#define DEFAULT_PAPSRV_PIDFILE RUNDIR"/papsrv.pid"
#define DEFAULT_PAPSRV_LOGFILE LOGDIR"/papsrv"
#define PAPSRV_DEFAULT_ZONE_FILE CONFDIR"/papsrv_default_zone.conf"

#define PAPSRV_MAX_NAMES 64						/* maximum number of advertized names */
#define MAX_ARGV 25								/* max arg vector length for invoking ppr */

/* These two timeouts may not be implemented: */
#define READ_TIMEOUT 60*60*1000					/* 1 hour in milliseconds */
#define WRITE_TIMEOUT 60*1000					/* 1 minute in milliseconds */

#define MY_QUANTUM 8							/* the flow quantum at our end */
#define READBUF_SIZE MY_QUANTUM * 512			/* For reading from the client */

#define MAX_REMOTE_QUANTUM 1					/* don't increase this, Mac client can't take it! */
#define WRITEBUF_SIZE MAX_REMOTE_QUANTUM * 512	/* buffer size for writing to the client */

/* #define DEBUG 1 */

#ifdef DEBUG
#define DEBUG_STARTUP 1					/* reading config, adding names and such */
/* #define DEBUG_QUERY 1 */				/* answering queries */
#define DEBUG_LOOP 1					/* debug main loop */
/* #define DEBUG_PRINTJOB 1 */			/* debug printjob() */
/* #define DEBUG_PRINTJOB_DETAILED 1 */ /* debug printjob() */
/* #define DEBUG_PPR_ARGV 1 */			/* print argv[] when execting ppr */
/* #define DEBUG_READBUF 1 */			/* debug input buffering */
/* #define DEBUG_WRITEBUF 1 */			/* debug output buffering */
/* #define DEBUG_REAPCHILD 1 */			/* debug child daemon termination */
#define DEBUG_AUTHORIZE 1				/* debug authorization code */
/* #define DEBUG_PPD 1 */				/* PPD file parsing */
#endif

/*============ end of stuff user might wish to modify ==============*/

/*
** AppleTalk name, type, and zone must not exceed this length.
** (This is part of the AppleTalk protocol definition, so there
** is no reason to change it.)
*/
#define MAX_ATALK_NAME_COMPONENT_LEN 32

/*
** Define macros for debugging.	 Which version of each one
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

/* routines in papsrv.c */
void fatal(int exitvalue, const char string[], ...);
void debug(const char string[], ...);
char *debug_string(char *s);
char *pap_getline(int sesfd);
void postscript_stdin_flushfile(int sesfd);
void printjob_reapchild(int signum);
void sigpipe_handler(int signum);
void child_main_loop(int sesfd, int prnid, int net, int node);
void printjob(int sesfd, int prnid, int net, int node, const char username[], int preauthorized);

/* routines in papsrv_ali.c and papsrv_cap.c */
void appletalk_dependent_daemon_main_loop(void);
int appletalk_dependent_printjob(int sesfd, int pipe);
int cli_getc(int sesfd);
void reset_buffer(int hard);
void reply(int sesfd, char *string);
void reply_eoj(int sesfd);
void close_reply(int sesfd);
void add_name(int prnid);
void appletalk_dependent_cleanup(void);

/* routines in papsrv_query.c */
void answer_query(int sesfd, int prnid, const char **username, int *preauthorized);
void REPLY(int sesfd, char *ptr);

/* routines in papsrv_conf.c */
void read_conf(char *conf_fname);
SHORT_INT get_font_id(const char fontname[]);
const char *get_font_name(SHORT_INT fontid);

/* routines in papsrv_authorize.c */
void preauthorize(int sesfd, int prnid, int net, int node, const char **user, int *preauthorized);
void login_request(int sesfd, int destid, const char **username, int *preauthorized);

/* Structure used to describe an *Option entry. */
struct OPTION
	{
	char *name;
	char *value;
	struct OPTION *next;
	} ;

/* Structure which describes each advertised name. */
struct ADV
	{
	const char *PAPname;		/* name to advertise */
	int fd;						/* file descriptor */
	const char *PPRname;		/* PPR destination to submit to */
	const char *PPDfile;		/* name of Adobe PPD file */
	SHORT_INT *fontlist;		/* id numbers of fonts on this printer */
	int fontcount;				/* number of fonts on this printer */
	char **argv;				/* array of arguments to pass to ppr */
	int LanguageLevel;			/* 1 or 2, 1 is default */
	char *PSVersion;			/* a rather complicated string */
	char *Resolution;			/* "300dpi", "600x300dpi" */
	gu_boolean BinaryOK;				/* TRUE or FALSE */
	int FreeVM;					/* free printer memory from "*FreeVM:" line */
	char *InstalledMemory;		/* Selected "*InstalledMemory" option */
	int VMOptionFreeVM;			/* Value from selected "*VMOption" */
	char *Product;				/* *Product string */
	gu_boolean ColorDevice;				/* TRUE or FALSE */
	int RamSize;				/* an integer (LaserWriter 8) */
	char *FaxSupport;			/* a string such as "Base" */
	char *TTRasterizer;			/* "None", "Type42", "Accept68K" */
	gu_boolean isprotected;				/* TRUE or FALSE */
	gu_boolean ForceAUFSSecurity;		/* Should we use AUFS security even if not protected? */
	int AUFSSecurityName;		/* AUFSSECURIYNAME_DSC, AUFSSECURITYNAME_USERNAME, AUFSSECURITYNAME_REALNAME */
	struct OPTION *options;		/* PPD file option settings */
	gu_boolean query_font_cache;
	} ;

#define AUFSSECURITYNAME_DSC 0
#define AUFSSECURITYNAME_USERNAME 1
#define AUFSSECURITYNAME_REALNAME 2

extern struct ADV adv[PAPSRV_MAX_NAMES];
extern char line[];				/* input line */
extern int name_count;			/* total advertised names */
extern int preauthorized;		/* flag read by printjob() */
extern int onebuffer;			/* buffer control flag */
extern int children;			/* count of children */
extern char *aufs_security_dir; /* argument to -S switch */
extern char *default_zone;		/* default zone for advertised names, initialy set to "*" */

/*
** Simulated PostScript error messages to send to a client when errors occur.
*/
#define MSG_NOCHARGEACCT "%%[ Error: you don't have a charge account ]%%\n"
#define MSG_BADAUTH "%%[ Error: password incorrect ]%%\n"
#define MSG_NOFOR "%%[ Error: No \"%%For:\" line in PostScript ]%%\n"
#define MSG_NOVOL "%%[ Error: you don't have a volume mounted ]%%\n"
#define MSG_OVERDRAWN "%%[ Error: account overdrawn ]%%\n"
#define MSG_DBERR "%%[ Error: user charge account database error ]%%\n"
#define MSG_NONCONFORMING "%%[ Error: insufficient DSC conformance ]%%\n"
#define MSG_SYNTAX "%%[ Error: bad ppr invokation syntax ]%%\n"
#define MSG_NOSPOOLER "%%[ Error: spooler is not running ]%%\n"
#define MSG_FATALPPR "%%[ Error: fatal error, see papsrv log ]%%\n"
#define MSG_NOPROC "%%[ Error: spooler is out of processes ]%%\n"
#define MSG_NODISK "%%[ Error: spooler is out of disk space ]%%\n"
#define MSG_TRUNCATED "%%[ Error: input file is truncated ]%%\n"
#define MSG_ACL "%%[ Error: ACL forbids you access to selected print destination ]%%\n"

/* end of file */
