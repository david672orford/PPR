/*
** mouse:~ppr/src/ppr-papd.h
** Copyright 1995--2004, Trinity College Computing Center.
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
** Last modified 9 November 2004.
*/

#include "queueinfo.h"

#define PIDFILE RUNDIR"/papd.pid"
#define LOGFILE LOGDIR"/papd"

#define MY_QUANTUM 8							/* the flow quantum at our end */
#define READBUF_SIZE MY_QUANTUM * 512			/* For reading from the client */

#define MAX_REMOTE_QUANTUM 1					/* don't increase this, Mac client can't take it! */
#define WRITEBUF_SIZE MAX_REMOTE_QUANTUM * 512	/* buffer size for writing to the client */

#define DEBUG 0

#if DEBUG
#define FUNCTION4DEBUG(a) const char function[] = a ;
#else
#define FUNCTION4DEBUG(a)
#endif

#if DEBUG
#define DEBUG_STARTUP 1					/* debug reading config, adding names and such */
#define DEBUG_QUERY 1					/* debug query handling */
#define DEBUG_LOOP 1					/* debug main loop */
#define DEBUG_PRINTJOB					/* debug printjob() */
/* #define DEBUG_PRINTJOB_DETAILED 1 */ /* debug printjob() in more detail */
#define DEBUG_PPR_ARGV 1				/* print argv[] when execting ppr */
/* #define DEBUG_READBUF 1 */			/* debug input buffering */
/* #define DEBUG_WRITEBUF 1 */			/* debug output buffering */
#define DEBUG_REAPCHILD 1				/* debug child daemon termination */
/* #define DEBUG_PPD 1 */				/* PPD file parsing */
#define DEBUG_AUTHORIZE 1	
#endif

/*============ end of stuff user might wish to modify ==============*/

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

#ifdef DEBUG_PPD
#define DODEBUG_PPD(a) debug a
#else
#define DODEBUG_PPD(a)
#endif

#ifdef DEBUG_AUTHORIZE
#define DODEBUG_AUTHORIZE(a) debug a
#else
#define DODEBUG_AUTHORIZE(a)
#endif

enum ADV_TYPE { ADV_LAST, ADV_ACTIVE, ADV_RELOADING, ADV_DELETED };

/* Structure which describes each advertised name. */
struct ADV
	{
	enum ADV_TYPE adv_type;				/* active, delete, last, etc. */
	enum QUEUEINFO_TYPE queue_type;		/* alias, group, or printer */
	const char *PPRname;				/* PPR destination to submit to */
	const char *PAPname;				/* name to advertise */
	int fd;								/* file descriptor of listening socket */
	} ;

extern char line[];				/* input line */

/* Structure which describes a user account. */
struct USER
	{
	char *username;
	char *fullname;
	};

/* routines in papd.c */
void debug(const char string[], ...);
char *debug_string(char *s);
char *pap_getline(int sesfd);
void postscript_stdin_flushfile(int sesfd);
void connexion_callback(int sesfd, struct ADV *this_adv, int net, int node);

/* routines in papd_ali.c and papd_cap.c */
void at_service(struct ADV *adv);
int	 at_printjob_copy(int sesfd, int pipe);
int	 at_getc(int sesfd);
void at_reset_buffer(void);
void at_reply(int sesfd, const char *string);
void at_reply_eoj(int sesfd);
void at_close_reply(int sesfd);
int	 at_add_name(const char papname[]);
void at_remove_name(const char papname[], int fd);

/* routines in papd_printjob.c */
void printjob(int sesfd, struct ADV *adv, void *qc, int net, int node, const struct USER *user, const char log_file_name[]);
void printjob_abort(void);

/* routines in papd_query.c */
extern int query_trace;
void answer_query(int sesfd, void *qc);
void sigusr1_handler(int sig);
void REPLY(int sesfd, const char *string);

/* routines in papd_conf.c */
struct ADV *conf_load(struct ADV *old_config);

/* routines in papd_login_aufs.c */
void login_aufs(int net, int node, struct USER *user);

/* routines in papd_login_rbi.c */
int rbi_query(int sesfd, void *qc);
void login_rbi(struct USER *user);
gu_boolean login_rbi_active(void);

/* end of file */
