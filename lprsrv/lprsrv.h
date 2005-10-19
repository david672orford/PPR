/*
** mouse:~ppr/src/include/lprsrv.h
** Copyright 1995--2005, Trinity College Computing Center.
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
** Last modified 19 October 2005.
*/

/*
** Header file for lprsrv, the Berkeley LPR/LPD
** compatible print server.
*/

/* Which operations do we want debugging information generated for? */
#if 0
/* #define DEBUG_MAIN 1 */				/* main loop */
/* #define DEBUG_STANDALONE 1 */		/* standalone daemon operation */
#define DEBUG_SECURITY 1				/* gethostbyname() and gethostbyaddr() */
#define DEBUG_PRINT 1					/* take job command */
#define DEBUG_CONTROL_FILE 1			/* read control file */
/* #define DEBUG_GRITTY 1 */			/* details of many things */
/* #define DEBUG_DISKSPACE 1 */			/* disk space checks */
#define DEBUG_LPQ 1						/* queue listing */
#define DEBUG_LPRM 1					/* job removal */
/* #define DEBUG_CONF 1 */				/* lprsrv.conf parsing */
#endif

/* Configuration file: */
#define LPRSRV_CONF CONFDIR"/lprsrv.conf"

/* Max length of configuration file line (not including NULL) : */
#define LPRSRV_CONF_MAXLINE 1023

/* Where do we want debugging and error messages? */
#define LPRSRV_LOGFILE LOGDIR"/lprsrv"

/* Number of data structure slots for files received: */
#define MAX_FILES_PER_JOB 100

/* Parameter separators as defined in RFC 1179: */
#define RFC1179_WHITESPACE " \t\v\f"

/* Max hostname length for non RFC 1179 purposes (not including NULL): */
#define MAX_HOSTNAME 127

/* Should we include code for standalone mode? */
#define STANDALONE 1

/*
** Structure to store information from lprsrv.conf.
*/
#define MAX_USERNAME 16
#define MAX_USER_DOMAIN 64
struct ACCESS_INFO
	{
	gu_boolean allow;
	gu_boolean insecure_ports;
	char user_domain[MAX_USER_DOMAIN+1];
	gu_boolean force_mail;
	} ;

/* Some internal str_*[] length limits for struct UPRINT. */
#define MAX_MAILADDR 127
#define MAX_PRINCIPAL 127
#define MAX_FOR 127

/* A structure to describe a print job: */
#define UPRINT_SIGNITURE 0x8391
struct UPRINT
	{
	int signiture;

	const char *fakername;		/* which fake program was used? */
	const char **argv;			/* argv[] from origional command */
	int argc;

	const char *dest;			/* the printer */
	const char **files;			/* list of files to print */

	uid_t uid;					/* user id number */
	gid_t gid;
	const char *user;			/* user name */
	const char *from_format;	/* queue display from field format */
	const char *lpr_mailto;		/* lpr style address to send email to */
	const char *lpr_mailto_host;	/* @ host */
	const char *fromhost;		/* lpr style host name */
	const char *user_domain;	/* string for after "@" sign in -X argument */
	const char *lpr_class;		/* lpr -C switch */
	const char *jobname;		/* lpr -J switch, lp -t switch */
	const char *pr_title;		/* title for pr (lpr -T switch) */

	const char *content_type_lp;	/* argument for lp -T or "raw" for -r */
	char content_type_lpr;		/* lpr switch such as -f, -c, or -d */
	int copies;					/* number of copies */
	gu_boolean banner;			/* should we ask for a banner page? */
	gu_boolean nobanner;		/* should we ask for suppression? */
	gu_boolean filebreak;
	int priority;				/* queue priority */
	gu_boolean immediate_copy;	/* should we copy file before exiting? */

	const char *form;			/* form name */
	const char *charset;		/* job character set */
	const char *width;
	const char *length;
	const char *indent;
	const char *cpi;
	const char *lpi;
	const char *troff_1;
	const char *troff_2;
	const char *troff_3;
	const char *troff_4;

	/* System V lp style options: */
	char *lp_interface_options;	/* lp -o switch */
	char *lp_filter_modes;		/* lp -y switch */
	const char *lp_pagelist;	/* lp -P switch */
	const char *lp_handling;	/* lp -H switch */

	/* DEC OSF/1 style options: */
	const char *osf_LT_inputtray;
	const char *osf_GT_outputtray;
	const char *osf_O_orientation;	/* portrait or landscape */
	const char *osf_K_duplex;
	int nup;	 				/* N-Up setting */

	gu_boolean unlink;			/* should job files be unlinked? */
	gu_boolean show_jobid;		/* should be announce the jobid? */
	gu_boolean notify_email;	/* send mail when job complete? */
	gu_boolean notify_write;	/* use write(1) when job complete? */

	/* Scratch space: */
	char str_numcopies[5];
	char str_typeswitch[3];
	char str_mailaddr[MAX_MAILADDR + 1];
	char str_user[MAX_PRINCIPAL + 1];				/* argument for ppr --user switch */
	char str_for[MAX_FOR + 1];						/* argument for ppr -f switch */
	char str_pr_title[11 + (LPR_MAX_T * 2) + 1];	/* space for "pr-title=\"my title\"" */
	char str_width[12];
	char str_length[12];
	char str_lpi[12];
	char str_cpi[12];
	char str_priority[3];
	char str_inputtray[sizeof("*InputSlot ") + LPR_MAX_DEC + 1];
	char str_outputtray[sizeof("*OutputBin ") + LPR_MAX_DEC + 1];
	char str_orientation[sizeof("orientation=") + LPR_MAX_DEC + 1];
	char str_nup[4];
	} ;

/* Description of a table which translates lpr to lp types: */
struct LP_LPR_TYPE_XLATE
	{
	const char *lpname;
	char lprcode;
	} ;

extern struct LP_LPR_TYPE_XLATE lp_lpr_type_xlate[];

/* Values for uprint_errno: */
#define UPE_NONE 0		/* not an error */
#define UPE_MALLOC 1	/* malloc(), calloc(), etc. failed */
#define UPE_BADARG 2	/* bad argument */
#define UPE_FORK 3		/* fork() failed */
#define UPE_CORE 4		/* child dumped core */
#define UPE_KILLED 5	/* child killed */
#define UPE_WAIT 6		/* wait() failed */
#define UPE_EXEC 7		/* execl() failed */
#define UPE_CHILD 8		/* child did non-zero exit */
#define UPE_NODEST 9	/* no print destination specified */
#define UPE_UNDEST 10	/* unknown print destination specified */
#define UPE_BADSYS 11	/* unknown remote system */
#define UPE_TEMPFAIL 12 /* temporary failure */
#define UPE_INTERNAL 13 /* internal library error */
#define UPE_DENIED 14	/* permission denied */

/* lprsrv.c: */
const char *this_node(void);
void fatal(int exitcode, const char message[], ... );
void debug(const char message[], ...);
void warning(const char string[], ...);
extern char *tcpbind_pidfile;

/* lprsrv_client_info.c: */
void get_client_info(char *client_dns_name, char *client_ip, int *client_port);

/* lprsrv_conf.c: */
void get_access_settings(struct ACCESS_INFO *access_info, const char hostname[]);
void get_user_domain(const char **user_domain, const char fromhost[], const char requested_user[], gu_boolean is_ppr_queue, const struct ACCESS_INFO *access_info);

/* lprsrv_print.c: */
void do_request_take_job(const char printer[], const char fromhost[], const struct ACCESS_INFO *access_info);

/* lprsrv_list.c: */
void do_request_lpq(char *command);

/* lprsrv_cancel.c: */
void do_request_lprm(char *command, const char fromhost[], const struct ACCESS_INFO *access_info);

/* lprsrv_standalone.c: */
void standalone_accept(char *tcpbind_sockets);

/* uprint_claim_*.c: */
gu_boolean uprint_claim_ppr(const char dest[]);

/* uprint_obj.c */
extern int uprint_errno;
extern const char *uprint_arrest_interest_interval;
void *uprint_new(const char *fakername, int argc, const char *argv[]);
int uprint_delete(void *p);
const char *uprint_set_dest(void *p, const char *dest);
const char *uprint_get_dest(void *p);
int uprint_set_files(void *p, const char *files[]);
const char *uprint_set_user(void *p, uid_t uid, gid_t gid, const char *user);
const char *uprint_get_user(void *p);
const char *uprint_set_from_format(void *p, const char *from_format);
const char *uprint_set_lpr_mailto(void *p, const char *lpr_mailto);
const char *uprint_set_lpr_mailto_host(void *p, const char *lpr_mailto_host);
const char *uprint_set_fromhost(void *p, const char *fromhost);
const char *uprint_set_user_domain(void *p, const char *user_domain);
const char *uprint_set_pr_title(void *p, const char *title);
const char *uprint_set_lpr_class(void *p, const char *lpr_class);
const char *uprint_set_jobname(void *p, const char *lpr_jobname);
const char *uprint_set_content_type_lp(void *p, const char *content_type);
int uprint_set_content_type_lpr(void *p, char content_type);
int uprint_set_copies(void *p, int copies);
gu_boolean uprint_set_banner(void *p, gu_boolean banner);
gu_boolean uprint_set_nobanner(void *p, gu_boolean nobanner);
gu_boolean uprint_set_filebreak(void *p, gu_boolean filebreak);
int uprint_set_priority(void *p, int priority);
gu_boolean uprint_set_immediate_copy(void *p, gu_boolean val);
const char *uprint_set_form(void *p, const char *formname);
const char *uprint_set_charset(void *p, const char *charset);
int uprint_set_length(void *p, const char *length);
int uprint_set_width(void *p, const char *width);
int uprint_set_indent(void *p, const char *indent);
int uprint_set_lpi(void *p, const char *lpi);
int uprint_set_cpi(void *p, const char *cpi);
const char *uprint_set_troff_1(void *p, const char *font);
const char *uprint_set_troff_2(void *p, const char *font);
const char *uprint_set_troff_3(void *p, const char *font);
const char *uprint_set_troff_4(void *p, const char *font);
const char *uprint_set_lp_interface_options(void *p, const char *lp_interface_options);
const char *uprint_set_lp_filter_modes(void *p, const char *lp_filter_options);
const char *uprint_set_lp_pagelist(void *p, const char *lp_pagelist);
const char *uprint_set_lp_handling(void *p, const char *lp_handling);
const char *uprint_set_osf_LT_inputtray(void *p, const char *inputtray);
const char *uprint_set_osf_GT_outputtray(void *p, const char *outputtray);
const char *uprint_set_osf_O_orientation(void *p, const char *orientation);
const char *uprint_set_osf_K_duplex(void *p, const char *duplex);
int uprint_set_nup(void *p, int nup);
gu_boolean uprint_set_unlink(void *p, gu_boolean do_unlink);
gu_boolean uprint_set_notify_email(void *p, gu_boolean notify_email);
gu_boolean uprint_set_notify_write(void *p, gu_boolean notify_write);
gu_boolean uprint_set_show_jobid(void *p, gu_boolean say_jobid);

/* uprint_run.c: */
int uprint_run(const char *exepath, const char *const argv[]);

/* uprint_strerror.c: */
const char *uprint_strerror(int errnum);

/* uprint_print.c, uprint_print_*.c: */
int uprint_print(void *p, gu_boolean remote_too);
int uprint_print_argv_ppr(void *p, const char **argv, int argv_len);

/* uprint_sys5_to_bsd.c: */
int uprint_parse_lp_interface_options(void *p);
int uprint_parse_lp_filter_modes(void *p);

/* uprint_conf.c */
const char *uprint_lpr_printcap(void);
const char *uprint_lp_printers(void);
const char *uprint_lp_classes(void);
gu_boolean uprint_lp_printers_conf(void);

/* uprint_print_bsd.c */
char uprint_get_content_type_lpr(void *p);

/* Provided by the caller: */
#ifdef __GNUC__
void uprint_error_callback(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
#endif
void uprint_error_callback(const char *format, ...);

/* Debugging macros: */
#ifdef DEBUG_MAIN
#define DODEBUG_MAIN(a) debug a
#else
#define DODEBUG_MAIN(a)
#endif

#ifdef DEBUG_STANDALONE
#define DODEBUG_STANDALONE(a) debug a
#else
#define DODEBUG_STANDALONE(a)
#endif

#ifdef DEBUG_SECURITY
#define DODEBUG_SECURITY(a) debug a
#else
#define DODEBUG_SECURITY(a)
#endif

#ifdef DEBUG_PRINT
#define DODEBUG_PRINT(a) debug a
#else
#define DODEBUG_PRINT(a)
#endif

#ifdef DEBUG_CONTROL_FILE
#define DODEBUG_CONTROL_FILE(a) debug a
#else
#define DODEBUG_CONTROL_FILE(a)
#endif

#ifdef DEBUG_GRITTY
#define DODEBUG_GRITTY(a) debug a
#else
#define DODEBUG_GRITTY(a)
#endif

#ifdef DEBUG_DISKSPACE
#define DODEBUG_DISKSPACE(a) debug a
#else
#define DODEBUG_DISKSPACE(a)
#endif

#ifdef DEBUG_LPQ
#define DODEBUG_LPQ(a) debug a
#else
#define DODEBUG_LPQ(a)
#endif

#ifdef DEBUG_LPRM
#define DODEBUG_LPRM(a) debug a
#else
#define DODEBUG_LPRM(a)
#endif

#ifdef DEBUG_CONF
#define DODEBUG_CONF(a) debug a
#else
#define DODEBUG_CONF(a)
#endif

#ifdef DEBUG_UPRINT
#define DODEBUG_UPRINT(a) debug a
#else
#define DODEBUG_UPRINT(a)
#endif

/* end of file */
