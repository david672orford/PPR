/*
** mouse:~ppr/src/include/lprsrv.h
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 9 August 1999.
*/

/*
** Header file for lprsrv, the Berkeley LPR/LPD
** compatible print server.
*/

/* Which operations do we want debugging information generated for? */
#if 0
/* #define DEBUG_MAIN 1 */		/* main loop */
/* #define DEBUG_STANDALONE 1 */	/* standalone daemon operation */
#define DEBUG_SECURITY 1		/* gethostbyname() and gethostbyaddr() */
#define DEBUG_PRINT 1			/* take job command */
#define DEBUG_CONTROL_FILE 1		/* read control file */
/* #define DEBUG_GRITTY 1 */		/* details of many things */
/* #define DEBUG_DISKSPACE 1 */		/* disk space checks */
#define DEBUG_LPQ 1			/* queue listing */
#define DEBUG_LPRM 1			/* job removal */
/* #define DEBUG_CONF 1	*/		/* lprsrv.conf parsing */
#endif

/* Configuration file: */
#define LPRSRV_CONF CONFDIR"/lprsrv.conf"

/* Max length of configuration file line (not including NULL) : */
#define LPRSRV_CONF_MAXLINE 1023

/* Where do we want debugging and error messages? */
#define LPRSRV_LOGFILE LOGDIR"/lprsrv"

/* This locked file is created when running in standalone mode: */
#define LPRSRV_LOCKFILE RUNDIR"/lprsrv.pid"

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
#define MAX_PROXY_CLASS 64
#define MAX_FROM_FORMAT 16
struct ACCESS_INFO
    {
    gu_boolean allow;
    gu_boolean insecure_ports;
    gu_boolean ppr_become_user;
    gu_boolean other_become_user;
    char ppr_root_as[MAX_USERNAME+1];
    char other_root_as[MAX_USERNAME+1];
    char ppr_proxy_user[MAX_USERNAME+1];
    char other_proxy_user[MAX_USERNAME+1];
    char ppr_proxy_class[MAX_PROXY_CLASS+1];
    char ppr_from_format[MAX_FROM_FORMAT+1];
    } ;

/* lprsrv.c: */
const char *this_node(void);
void fatal(int exitcode, const char message[], ... );
void debug(const char message[], ...);
void warning(const char string[], ...);

/* lprsrv_client_info.c: */
void get_client_info(char *client_dns_name, char *client_ip, int *client_port);

/* lprsrv_conf.c: */
void clipcopy(char *dest, const char *source, int maxlen);
void get_access_settings(struct ACCESS_INFO *access_info, const char hostname[]);
void get_proxy_identity(uid_t *uid_to_use, const char **proxy_class, const char fromhost[], const char requested_user[], gu_boolean is_ppr_queue, const struct ACCESS_INFO *access_info);

/* lprsrv_print.c: */
void do_request_take_job(const char printer[], const char fromhost[], const struct ACCESS_INFO *access_info);

/* lprsrv_list.c: */
void do_request_lpq(char *command);

/* lprsrv_cancel.c: */
void do_request_lprm(char *command, const char fromhost[], const struct ACCESS_INFO *access_info);

/* lprsrv_standalone.c: */
int port_name_lookup(const char *name);
void run_standalone(int server_port, uid_t root_uid, uid_t safe_uid);
extern int am_standalone_parent;

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

/* end of file */
