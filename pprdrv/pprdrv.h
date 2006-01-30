/*
** mouse:~ppr/src/pprdrv/pprdrv.h
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
** Last modified 14 October 2005.
*/

/*
** Uncomment the defines for the debugging
** code you want to have included.	Most of
** these defines will control the definition
** of a macro at the end of this file.
*/
#if 1
#define DEBUG 1							/* include function debug() */
#define DEBUG_MAIN 1					/* main() */
#define DEBUG_INTERFACE 1				/* show opening, closing, etc. */
/* #define DEBUG_INTERFACE_GRITTY 1 */	/* show what we send to and receive from the interface */
#define DEBUG_FEEDBACK 1				/* show twoway operations */
#define DEBUG_FEEDBACK_LINES 1			/* show the lines read from printer */
/* #define DEBUG_PPD 1 */				/* PPD file parsing */
/* #define DEBUG_PPD_DETAILED 1 */
/* #define DEBUG_FLAGS 1 */				/* banner and trailer pages */
/* #define DEBUG_RESOURCES 1 */			/* fonts, procset, etc. */
/* #define DEBUG_QUERY 1 */				/* patch queries? */
/* #define DEBUG_BINSELECT_INLINE 1 */	/* add comments about binselection to output */
/* #define DEBUG_SIGNITURE_INLINE 1 */	/* add comments about signitures to output */
#define DEBUG_LW_MESSAGES 1				/* LaserWriter message interpretation */
/* #define DEBUG_DIE_DELAY 60 */		/* seconds to delay after receiving sigterm */
#define DEBUG_COMMENTARY 1
/* #define DEBUG_TRUETYPE 1 */			/* truetype fonts, conversion to Postscript */
/* #define DEBUG_CUSTOM_HOOK 1 */
#define DEBUG_PPOP_STATUS 1				/* code to leave info for "ppop status" */
#endif

/*
** Should we keep superseded PostScript feature code as comments?
** If we don't, then the regression tests will fail.
*/
#define KEEP_OLD_CODE 1

/*
** Here is where the debugging output goes as well as notices about abnormal
** conditions and whatever commentators write to stdout and stderr.
*/
#define PPRDRV_LOGFILE LOGDIR"/pprdrv"

/*
** It is difficult to know how high to set these.
**
** Steve Hsieh reports a printer with more than 20 "*PaperDimension:" lines.
** Version 4.1 of the PPD spec. lists about 60 specific sizes but suggests
** that many more are possible.
**
** I have yet to see a printer with more than 45 fonts (QMS-PS 410), so
** the hash table size of 64 is probably adequate, especially since
** it is not a hard limit.
**
** It is conceivable that a large patch could be longer than 8192,
** requiring MAX_PPDTEXT to be increased.  Probably this limit should
** be removed altogether.
*/
#define PPD_TABSIZE 128			/* slots in PPD file string hash table */
#define FONT_TABSIZE 64			/* slots in font hash table */
#define MAX_PPDNAME 50			/* max length of PPD string name */
#define MAX_PPDTEXT 8192		/* max length of PPD string value (including multiline values) */
#define MAX_DRVREQ 40			/* maximum # requirements in a document */
#define MAX_PAPERSIZES 100		/* maximum # "*PaperDimension:" lines in PPD file */

/*
** What should be done if we have trouble converting a MS-Windows
** TrueType font to a PostScript type 42 or type 3 font?  Should
** we view it as a printer error or as a job error?
**
** This will never happen if you don't have corrupt .TTF files.
** If you do, setting EXIT_TTFONT to EXIT_JOBERR could allow
** other jobs to print if they don't use the defective font.
** If they do use it, they will all be arrested.  Setting EXIT_TTFONT
** to EXIT_PRNERR_NORETRY gives the operator a chance to fix
** it before all those jobs get arrested.  Since it is perfectly
** easy to release an arrested job, Steve Hsieh <steveh@eecs.umich.edu>
** is probably right in advocating EXIT_JOBERR.
*/
/* #define EXIT_TTFONT EXIT_PRNERR_NORETRY */
#define EXIT_TTFONT EXIT_JOBERR

/*==================== No tunables below this line. =====================*/

/* What have PJL messages told us about the online status of the printer? */
enum PJL_ONLINE
		{
		PJL_ONLINE_UNKNOWN,		/** told us nothing as yet */
		PJL_ONLINE_FALSE,		/** told us printer is off-line */
		PJL_ONLINE_TRUE			/** told us printer is on-line */
		};

/*
** The structure which is used to translate between PPR media names and
** the media names in the "%%Media:" lines.
*/
struct	Media_Xlate {
		char *pprname;
		char *dscname;
		} ;

/* pprdrv.c: */
extern volatile gu_boolean sigterm_caught;
extern volatile gu_boolean sigalrm_caught;
extern int test_mode;
extern char line[];
extern int line_len;
extern int line_overflow;
extern struct timeval start_time;
int copies_auto;
int copies_auto_collate;
char *dgetline(FILE *infile);
extern const char *QueueFile;
extern struct QEntryFile job;
extern int group_pass;
extern gu_boolean doing_primary_job;
extern gu_boolean doing_prolog;
extern gu_boolean doing_docsetup;
extern int sheetcount;
extern int print_direction;
extern char *drvreq[MAX_DRVREQ];
extern int drvreq_count;
extern int strip_binselects;	/* for pprdrv_ppd.c */
extern int strip_signature;		/* for pprdrv_ppd.c */
void fault_check(void);
int real_main(int argc, char *argv[]);

/* pprdrv_fault_debug.c: */
void hooked_exit(int rval, const char *explain)
#ifdef __GNUC__
__attribute__ ((noreturn))
#endif
;
void fatal(int exval, const char string[], ...)
#ifdef __GNUC__
__attribute__ (( noreturn, format (printf, 2, 3) ))
#endif
;
void signal_fatal(int exval, const char string[], ...)
#ifdef __GNUC__
__attribute__ (( noreturn, format (printf, 2, 3) ))
#endif
;
void debug(const char format[],...)
#ifdef __GNUC__
__attribute__ (( format (printf, 1, 2) ))
#endif
;
void signal_debug(const char format[],...)
#ifdef __GNUC__
__attribute__ (( format (printf, 1, 2) ))
#endif
;

/* pprdrv_interface.c: */
enum JOBTYPE {JOBTYPE_QUERY, JOBTYPE_FLAG, JOBTYPE_THEJOB, JOBTYPE_THEJOB_TRANSPARENT, JOBTYPE_THEJOB_BARBAR};
void interface_fault_check(void);
void kill_interface(void);
int interface_close(gu_boolean flushit);
extern int intstdin;
extern int intstdout;
gu_boolean interface_sigchld_hook(pid_t pid, int wait_status);
void job_start(enum JOBTYPE jobtype);
void job_end(void);
int job_nomore(void);

/* pprdrv_rip.c: */
void rip_fault_check(void);
int rip_start(int printdata_handle, int stdout_handle);
int rip_stop(int printdata_handle2, gu_boolean flushit);
void rip_cancel(void);
gu_boolean rip_sigchld_hook(pid_t pid, int wait_status);

/* pprdrv_flag.c: */
int print_flag_page(int flagtype, int position, int skiplines);

/* pprdrv_feedback.c: */
void feedback_setup(int fd);
int feedback_reader(void);
int feedback_wait(int timeout, gu_boolean return_on_signal);
void feedback_pjl_wait(void);
void feedback_drain(void);
gu_boolean feedback_posterror(void);
gu_boolean feedback_ghosterror(void);
int feedback_pjl_chargable_pagecount(void);

/* pprdrv_ppd.c: */
void read_PPD_file(const char *name);
const char *find_feature(const char *name, const char *variation);
void begin_stopped(void);
void end_stopped(const char *feature, const char *option);
void insert_features(FILE *qstream, int set);
void include_feature(const char *featuretype, const char *option);
void begin_feature(const char featuretype[], const char option[], FILE *infile);
void include_resource(void);
gu_boolean ppd_font_present(const char fontname[]);
gu_boolean printer_can_collate(void);
void set_collate(gu_boolean collate);

/* pprdrv_res.c: */
void insert_noinclude_fonts(void);
void insert_extra_prolog_resources(void);
void write_resource_comments(void);
void begin_resource(FILE *infile);
struct DRVRES *add_drvres(int needed, int fixinclude, const char type[], const char name[], double version, int revision);
int add_resource(const char type[], const char name[], double version, int revision);

/* pprdrv_capable.c: */
int check_if_capable(FILE *qfile, int group_pass);

/* pprdrv_media.c: */
int load_mountedlist(void);
int select_medium(const char name[]);
int select_medium_by_dsc_name(const char name[]);
void read_media_lines(FILE *q);
extern struct Media_Xlate media_xlate[];
extern int media_count;

/* pprdrv_buf.c: */
extern int control_d_count;
void printer_bufinit(void);
int printer_flush(void);
extern void (*ptr_printer_putc)(int c);
extern void (*ptr_printer_puts)(const char *str);
extern void (*ptr_printer_write)(const char *buf, size_t size);
#define printer_putc (*ptr_printer_putc)
#define printer_puts (*ptr_printer_puts)
#define printer_write (*ptr_printer_write)
void printer_TBCP_on(void);
void printer_TBCP_off(void);
void printer_putline(const char *str);
void printer_printf(const char *str, ...);
void printer_puts_QuotedValue(const char *str);
void printer_putc_escaped(int c);
void printer_puts_escaped(const char *str);
void printer_universal_exit_language(void);
void printer_display_printf(const char message[], ...);

/* pprdrv_writemon.c: */
void writemon_init(void);
void writemon_start(const char operation[]);
int writemon_sleep_time(struct timeval *sleep_time, int timeout);
void writemon_unstalled(const char operation[]);
void writemon_pagedrop(void);
void writemon_online(gu_boolean online);

/* pprdrv_nup.c: */
void prestart_N_Up_hook(void);
void invoke_N_Up(void);
void close_N_Up(void);

/* pprdrv_req.c: */
void write_requirement_comments(void);

/* pprdrv_signature.c: */
int signature(int sheetnumber,int thissheet);

/* pprdrv_reason.c: */
void give_reason(const char *reason);
void describe_postscript_error(const char creator[], const char error[], const char command[]);

/* pprdrv_patch.c: */
void patchfile(void);
void jobpatchfile(void);
gu_boolean patchfile_query_callback(const char message[]);

/* pprdrv_commentary.c: */
void commentary(int category, const char cooked[], const char raw1[], const char raw2[], int duration, int severity);
void commentary_exit_hook(int rval, const char explain[]);
void commentator_wait(void);

/* pprdrv_tt.c: */
void want_ttrasterizer(void);
void send_font_tt(const char filename[]);

/* pprdrv_pfb.c: */
void send_font_pfb(const char filename[], FILE *ifile);

/* pprdrv_mactt.c: */
void send_font_mactt(const char filename[]);

/* pprdrv_progress.c: */
void state_update_pprdrv_puts(const char line[]);
void progress_page_start_comment_sent(void);
void progress_pages_truly_printed(int n);
void progress_bytes_sent(int n);
void progress_bytes_sent_correction(int n);
void progress_new_status(const char text[]);
long int progress_bytes_sent_get(void);

/* pprdrv_snmp.c: */
int snmp_DeviceStatus(const char name[]);
int snmp_PrinterStatus(const char name[]);
int snmp_PrinterDetectedErrorState(const char name[]);

/* pprdrv_lw_messages.c: */
int translate_lw_message(const char raw_message[], int *value1, int *value2, int *value3, const char **details);

/* pprdrv_pjl_ustatus.c: */
int translate_pjl_message(int code, const char display[], int *value1, int *value2, int *value3, const char **details);

/* pprdrv_notppd.c: */
void set_jobname(const char jobname[]);
void set_numcopies(int copies);
void begin_nonppd_feature(const char feature_name[], const char feature_value[], FILE *infile);

/* pprdrv_persistent.c: */
int persistent_query_callback(char *message);
void persistent_download_now(void);
void persistent_finish(void);

/* pprdrv_pagecount.c: */
gu_boolean pagecount_query_callback(const char message[]);
int pagecount(void);

/* pprdrv_custom_hook.c: */
#define CUSTOM_HOOK_BANNER 1
#define CUSTOM_HOOK_TRAILER 2
#define CUSTOM_HOOK_COMMENTS 4
#define CUSTOM_HOOK_PROLOG 8
#define CUSTOM_HOOK_DOCSETUP 16
#define CUSTOM_HOOK_PAGEZERO 32
gu_boolean custom_hook(int code, int parameter);

/* pprdrv_ppop_status.c: */
void ppop_status_init(void);
void ppop_status_writemon(const char operation[], int minutes);
void ppop_status_pagemon(const char string[]);
void ppop_status_connecting(const char connecting[]);
void ppop_status_exit_hook(int retval);
void ppop_status_shutdown(void);
void handle_snmp_status(int device_status, int printer_status, unsigned int errorstate);
void handle_lw_status(const char status[], const char job[]);
void handle_ustatus_device(enum PJL_ONLINE online, int code, const char display[], int code2, const char display2[]);

/* pprdrv_userparams.c: */
void insert_userparams(void);
void insert_userparams_jobtimeout(void);

/* pprdrv_log.c: */
FILE *log_reader(void);
void log_putc(int c);
void log_puts(const char data[]);
void log_vprintf(const char format[], va_list va);
void log_printf(const char format[], ...);
void log_flush(void);
void log_close(void);

/*
** This structure holds the information we compile about the printer and
** the current job.
*/
struct PPRDRV {
		char *Name;							/* Name of printer */

		char *Interface;					/* Interface program */
		char *Address;						/* Address for interface program */
		char *Options;						/* Options for interface program */
		gu_boolean Feedback;				/* true or false */
		int Jobbreak;						/* enum of jobbreak methods */
		int Codes;							/* Passable Codes */

		struct								/* Raster Image Processor (such as Ghostscript) */
			{
			char *name;
			char *output_language;
			char *options_storage;
			const char **options;
			int options_count;
			} RIP;

		gu_boolean do_banner;
		gu_boolean do_trailer;
		int OutputOrder;					/* 1 or -1 or 0 if unknown */
		char *PPDFile;						/* name of description file */
		gu_boolean type42_ok;				/* Can we use type42 fonts? */
		gu_boolean GrayOK;					/* permission to print non-colour jobs? */
		struct PPD_PROTOCOLS prot;			/* List of protocols such as TBCP and PJL PPD files says are supported */
		int PageCountQuery;					/* Which method?  0 means don't. */
		int PageTimeLimit;					/* max seconds to allow per page */

		struct								/* Limit allowed page sizes for jobs. */
			{
			int lower;
			int upper;
			} limit_pages;

		struct								/* Limit allowed sizes in kilobytes. */
			{
			int lower;
			int upper;
			} limit_kilobytes;

		struct
			{
			int per_duplex;					/* duplexed sheet charge in cents */
			int per_simplex;				/* simplex sheet sharge in cents */
			} charge;

		struct
			{
			int flags;						/* call for banner, trailer, header, etc. */
			const char *path;				/* program to call */
			} custom_hook;

		struct
			{
			int JobTimeout;
			int WaitTimeout;
			int ManualfeedTimeout;
			int DoPrintErrors;
			} userparams;

		} ;

/* This structure is in pprdrv.c */
extern struct PPRDRV printer;

/*
** A PPD file string entry.
*/
struct PPDSTR {
		char *name;
		char *value;
		struct PPDSTR *next;
		} ;

/*
** A PPD file font entry.
*/
struct PPDFONT {
		char *name;
		struct PPDFONT *next;
		} ;

/*
** The list of device features
*/
struct FEATURES {
	int ColorDevice;				/* TRUE or FALSE, real colour printing */
	int Extensions;					/* 0 or EXTENSION_* */
	int FaxSupport;					/* FAXSUPPORT_* */
	int FileSystem;					/* TRUE or FALSE */
	int LanguageLevel;				/* 1, 2, etc. */
	int TTRasterizer;				/* TT_NONE, TT_ACCEPT68K, TT_TYPE42 */
	} ;

/* This structure is in pprdrv_ppd.c */
extern struct FEATURES Features;

/* Types of fax support: */
#define FAXSUPPORT_NONE 0
#define FAXSUPPORT_Base 1

/*
** Structure to describe a mounted media list entry.
** The mounted media list files are generated by pprd,
** but pprd uses two fwrite calles for each record and does
** not use this structure.
*/
struct MOUNTED
	{
	char bin[MAX_BINNAME];
	char media[MAX_MEDIANAME];
	} ;

/* this structure is in pprdrv_media.c */
extern struct MOUNTED mounted[MAX_BINS];

/*
** Structure used by pprdrv to describe a resource:
*/
struct DRVRES
	{
	const char *type;			/* procset, font, etc. */
	const char *name;			/* name of this resource */
	double version;				/* procset only, 0.0 for others */
	int revision;				/* procset only, 0 for others */
	int needed;					/* TRUE or FALSE */
	const char *former_name;	/* name of substituted resource */
	const char *subst_code;		/* PostScript code to aid font substitution */
	const char *filename;		/* NULL or file to load cached resource from */
	int dot_ttf;				/* resource is a TrueType font in Microsoft Windows format */
	int mactt;					/* resource is a TrueType font converted by a Mac to PostScript */
	int force_into_docsetup;	/* relocate inclusion to docsetup? */
	int force_into_prolog;		/* relocate inclusion to prolog? */
	} ;

extern struct DRVRES *drvres;	/* in pprdrv.c */
extern int drvres_count;		/* in pprdrv.c */
extern int drvres_space;		/* in pprdrv.c */

/*
** Structure in pprdrv_ppd.c for paper sizes.
**
** An array of these structures contains a list of
** the paper sizes enumerated in this printer's
** PPD file.
*/
struct PAPERSIZE
	{
	char *name;					/* name of paper size */
	double width;				/* width in 1/72ths */
	double height;
	double lm;					/* left margin */
	double tm;					/* top margin */
	double rm;					/* right margin */
	double bm;					/* bottom margin */
	} ;

extern struct PAPERSIZE papersize[];
extern int num_papersizes;

/*
** Define the debugging macros based on what was chosen above.
*/
#ifdef DEBUG
#define FUNCTION4DEBUG(a) const char function[] = a ;
#else
#define FUNCTION4DEBUG(a)
#endif

#ifdef DEBUG_TRUETYPE
#define DODEBUG_TRUETYPE(a) debug a
#else
#define DODEBUG_TRUETYPE(a)
#endif

#ifdef DEBUG_MAIN
#define DODEBUG_MAIN(a) debug a
#else
#define DODEBUG_MAIN(a)
#endif

#ifdef DEBUG_PPD
#define DODEBUG_PPD(a) debug a
#else
#define DODEBUG_PPD(a)
#endif

#ifdef DEBUG_PPD_DETAILED
#define DODEBUG_PPD_DETAILED(a) debug a
#else
#define DODEBUG_PPD_DETAILED(a)
#endif

#ifdef DEBUG_INTERFACE
#define DODEBUG_INTERFACE(a) debug a
#else
#define DODEBUG_INTERFACE(a)
#endif

#ifdef DEBUG_INTERFACE_GRITTY
#define DODEBUG_INTERFACE_GRITTY(a) debug a
#else
#define DODEBUG_INTERFACE_GRITTY(a)
#endif

#ifdef DEBUG_FLAGS
#define DODEBUG_FLAGS(a) debug a
#else
#define DODEBUG_FLAGS(a)
#endif

#ifdef DEBUG_RESOURCES
#define DODEBUG_RESOURCES(a) debug a
#else
#define DODEBUG_RESOURCES(a)
#endif

#ifdef DEBUG_QUERY
#define DODEBUG_QUERY(a) debug a
#else
#define DODEBUG_QUERY(a)
#endif

#ifdef DEBUG_FEEDBACK
#define DODEBUG_FEEDBACK(a) debug a
#else
#define DODEBUG_FEEDBACK(a)
#endif

#ifdef DEBUG_COMMENTARY
#define DODEBUG_COMMENTARY(a) debug a
#else
#define DODEBUG_COMMENTARY(a)
#endif

#ifdef DEBUG_LW_MESSAGES
#define DODEBUG_LW_MESSAGES(a) debug a
#else
#define DODEBUG_LW_MESSAGES(a)
#endif

#ifdef DEBUG_PPOP_STATUS
#define DODEBUG_PPOP_STATUS(a) debug a
#else
#define DODEBUG_PPOP_STATUS(a)
#endif

/* Values used to descibe the second argument to "*OrderDependency:": */
#define ORD_EXITSERVER 1
#define ORD_PROLOG 2
#define ORD_DOCUMENTSETUP 3
#define ORD_PAGESETUP 4
#define ORD_JCLSETUP 5
#define ORD_ANYSETUP 6

/* end of file */

