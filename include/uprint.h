/*
** mouse:~ppr/src/include/uprint.h
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
** Last modified 18 February 2003.
*/

/* Maximum lengths of control file fields. */
#define LPR_MAX_QUEUE 31	/* my guess */
#define LPR_MAX_P 31		/* RFC 1179 */
#define LPR_MAX_C 31		/* RFC 1179 */
#define LPR_MAX_H 31		/* RFC 1179 */
#define LPR_MAX_J 99		/* RFC 1179 */
#define LPR_MAX_L 31		/* my guess */
#define LPR_MAX_M 31		/* my guess */
#define LPR_MAX_N 131		/* RFC 1179 */
#define LPR_MAX_P 31		/* RFC 1179 */
#define LPR_MAX_T 79		/* RFC 1179 */
#define LPR_MAX_I 8		/* my guess */
#define LPR_MAX_W 8		/* my guess */
#define LPR_MAX_1 131		/* my guess */
#define LPR_MAX_2 131		/* my guess */
#define LPR_MAX_3 131		/* my guess */
#define LPR_MAX_4 131		/* my guess */

/* DEC OSF extensions, my guess */
#define LPR_MAX_DEC 31

/* Solaris 2.6 extension, arbitrary limits */
#define LPR_MAX_5f 31
#define LPR_MAX_5H 31
#define LPR_MAX_O 131
#define LPR_MAX_5P 131
#define LPR_MAX_5S 31
#define LPR_MAX_5T 31
#define LPR_MAX_5y 131

/* Structure to describe a remote queue selected
   by claim_printdest_remote(). */
struct REMOTEDEST
    {
    char *node;				/* DNS name of remote node */
    char printer[LPR_MAX_QUEUE + 1];	/* name of remote printer */
    gu_boolean osf_extensions;		/* send OSF/1 control file extensions */
    gu_boolean solaris_extensions;		/* send Solaris control file extensions */
    gu_boolean ppr_extensions;		/* send PPR control file extensions */
    } ;

/* Provided by the caller: */
#ifdef __GNUC__
void uprint_error_callback(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
#endif
void uprint_error_callback(const char *format, ...);

/* lpr_connect.c: */
int uprint_lpr_make_connection(const char address[]);
int uprint_lpr_make_connection_with_failover(const char address[]);
int uprint_lpr_send_cmd(int fd, const char text[], int length);
int uprint_lpr_response(int fd, int timeout);

/* lpr_sendfile.c: */
const char *uprint_lpr_nodename(void);
int uprint_lpr_nextid(void);
int uprint_file_stdin(int *length);
int uprint_lpr_send_data_file(int sourcefd, int sockfd);

/* claim_*.c: */
gu_boolean printdest_claim_ppr(const char dest[]);
gu_boolean printdest_claim_sysv(const char dest[]);
gu_boolean printdest_claim_bsd(const char dest[]);
gu_boolean printdest_claim_remote(const char dest[], struct REMOTEDEST *scratchpad);

/* uprint.c (object atributes): */
extern int uprint_errno;
extern const char *uprint_arrest_interest_interval;
void *uprint_new(const char *fakername, int argc, const char *argv[]);
int uprint_delete(void *p);

const char *uprint_set_dest(void *p, const char *dest);
const char *uprint_get_dest(void *p);
int uprint_set_files(void *p, const char *files[]);

const char *uprint_set_user(void *p, uid_t uid, const char *user);
const char *uprint_get_user(void *p);
const char *uprint_set_from_format(void *p, const char *from_format);
const char *uprint_set_lpr_mailto(void *p, const char *lpr_mailto);
const char *uprint_set_lpr_mailto_host(void *p, const char *lpr_mailto_host);
const char *uprint_set_fromhost(void *p, const char *fromhost);
const char *uprint_set_proxy_class(void *p, const char *proxy_class);
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

const char *uprint_set_ppr_responder(void *p, const char *ppr_responder);
const char *uprint_set_ppr_responder_address(void *p, const char *ppr_responder_address);
const char *uprint_set_ppr_responder_options(void *p, const char *ppr_responder_options);

/* uprint_loop.c: */
int uprint_loop_check(const char *myname);

/* uprint_uid(): */
int uprint_re_uid_setup(uid_t *uid, uid_t *safe_uid);

/* uprint_run.c: */
int uprint_run(uid_t uid, const char *exepath, const char *const argv[]);

/* uprint_strerror.c: */
const char *uprint_strerror(int errnum);

/* uprint_print.c, uprint_print_*.c: */
int uprint_print(void *p, gu_boolean remote_too);
int uprint_print_argv_sysv(void *p, const char **argv, int argv_len);
int uprint_print_argv_bsd(void *p, const char **argv, int argv_len);
int uprint_print_argv_ppr(void *p, const char **argv, int argv_len);
int uprint_print_rfc1179(void *p, struct REMOTEDEST *scratchpad);

/* uprint_lpq.c: */
int uprint_lpq(uid_t uid, const char agent[], const char queue[], int format, const char *arglist[], gu_boolean remote_too);

/* uprint_lpq_rfc1179.c: */
int uprint_lpq_rfc1179(const char *queue, int format, const char **arglist, struct REMOTEDEST *scratchpad);

/* uprint_lprm.c: */
int uprint_lprm(uid_t uid, const char agent[], const char proxy_class[], const char queue[], const char **arglist, gu_boolean remote_too);

/* uprint_lprm_rfc1179.c: */
int uprint_lprm_rfc1179(const char *user, const char *athost, const char *queue, const char **arglist, struct REMOTEDEST *scratchpad);

/* uprint_conf.c: */
const char *uprint_default_destinations_lpr(void);
const char *uprint_default_destinations_lp(void);
const char *uprint_path_lpr(void);
const char *uprint_path_lpq(void);
const char *uprint_path_lprm(void);
const char *uprint_path_lp(void);
const char *uprint_path_lpstat(void);
const char *uprint_path_cancel(void);
gu_boolean uprint_lpr_installed(void);
gu_boolean uprint_lp_installed(void);

/* Values for uprint_errno: */
#define UPE_NONE 0	/* not an error */
#define UPE_MALLOC 1	/* malloc(), calloc(), etc. failed */
#define UPE_BADARG 2	/* bad argument */
#define UPE_FORK 3	/* fork() failed */
#define UPE_CORE 4	/* child dumped core */
#define UPE_KILLED 5	/* child killed */
#define UPE_WAIT 6	/* wait() failed */
#define UPE_EXEC 7	/* execl() failed */
#define UPE_CHILD 8	/* child did non-zero exit */
#define UPE_NODEST 9	/* no print destination specified */
#define UPE_UNDEST 10	/* unknown print destination specified */
#define UPE_TOOMANY 11	/* too many files to print */
#define UPE_BADSYS 12	/* unknown remote system */
#define UPE_TEMPFAIL 13	/* temporary failure */
#define UPE_NOSPACE 14	/* not enough spool room for file */
#define UPE_INTERNAL 15	/* internal library error */
#define UPE_DENIED 16	/* permission denied */
#define UPE_NOFILE 17	/* file not found */
#define UPE_BADORDER 18	/* functions called in wrong order */
#define UPE_BADCONFIG 19 /* error in configuration file */
#define UPE_SETUID 20	/* setuid() failed */

/* end of file */
