/*
** mouse:~ppr/src/include/global_structs.h
** Copyright 1995--2013, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified: 10 October 2013
*/

/* =================== for pprd queue entries =====================*/

/* A queue entry as stored by pprd and passed back to ppop.
   Notice that this is shorter than struct QEntryFile. */
struct QEntry
	{
	/* encoded in queue file name */
	INT16_T destid;					/* destination key number */
	INT16_T id;						/* queue id */
	INT16_T subid;					/* fractional queue id */

	/* encoding in "PPRD:" mini header */
	INT16_T priority;					/* priority number (1=lowest, 100=highest) */
	unsigned int sequence_number;
	INT16_T status;						/* printer id if printing, < 0 for other status */
	unsigned short int flags;			/* --keep, responding, etc. */
	
	time_t resend_message_at;			/* time at which to retry responder to questioner */

	INT16_T media[MAX_DOCMEDIA];		/* list of id numbers of media types req. */
	INT16_T pass;						/* number of current pass thru printers in group */
	unsigned char never;				/* bitmap of group member which can't print */
	unsigned char notnow;				/* bitmap of group members without required media mounted */
	} ;

/*
** Printer status values
*/
#define PRNSTATUS_IDLE 0				/* idle but ready to print */
#define PRNSTATUS_PRINTING 1			/* printing right now */
#define PRNSTATUS_CANCELING 2			/* canceling a job */
#define PRNSTATUS_SEIZING 3				/* stopping printing current job and holding it */
#define PRNSTATUS_FAULT 4				/* waiting for auto retry */
#define PRNSTATUS_ENGAGED 5				/* printer is printing for another computer */
#define PRNSTATUS_STARVED 6				/* starved for system resources */
#define PRNSTATUS_STOPT 7				/* stopt by user */
#define PRNSTATUS_STOPPING 8			/* will go to PRNSTATUS_STOPT at job end */
#define PRNSTATUS_HALTING 9				/* pprdrv being killed */
#define PRNSTATUS_DELETED 10			/* printer has been deleted */
#define PRNSTATUS_DELIBERATELY_DOWN 7	/* 1st non-printing value (stopt) */

/* =================== for disk queue entries =====================*/

struct Jobname
	{
	const char *destname;
	INT16_T id;
	INT16_T subid;
	} ;

struct JOB_SPOOL_STATE
	{
	int priority;						/* priority number (1--100) */
	unsigned int sequence_number;
	INT16_T status;						/* job status */
	short unsigned int flags;			/* job flags */
	} ;
	
struct RESPONDER {
	const char *name;		/* program for sending messages to user */
	const char *address;	/* address for errors, possibly NULL */
	const char *options;	/* name=value list of responder options */
	};

/** Holds the contents of a queue file.
 * This structure is loaded using read_struct_QEntryFile(), saved using
 * write_struct_QEntryFile(), and destroyed using destroy_struct_QEntryFile().
 * The members are minipulated directly by the caller as there are no
 * member functions for that purpose.
*/
struct QEntryFile
	{
	float PPRVersion;					/* version number of PPR that created queue entry */
	const char *lc_messages;			/* language setting for messages */

	struct Jobname jobname;				/* destname-id.subid */

	struct JOB_SPOOL_STATE spool_state;

	long time;							/* time job was submitted (don't use time_t) */
	const char *user;					/* username or username@host */
	const char *magic_cookie;			/* secret about this job */
	const char *For;					/* %%For: line for PostScript header */
	const char *charge_to;				/* charge account to debit */
	const char *Routing;				/* %%Routing: line for PostScript header */
	const char *Title;					/* %%Title: for Postscript header */
	const char *Creator;				/* application which created it */
	const char *lpqFileName;			/* name of input file */
	struct RESPONDER responder;
	int commentary;						/* bitmask of commentary to send thru the responder */
	int nmedia;							/* number of media types */
	int media[MAX_DOCMEDIA];			/* list of required media types */
	int do_banner;						/* should we print a banner page? */
	int do_trailer;						/* should we print a trailer page? */
	struct {							/* various attributes */
		int langlevel;					/* PostScript language level */
		int extensions;					/* bit fields of extension to level 1 */
		float DSClevel;					/* DSC comments version */
		char *DSC_job_type;				/* NULL for normal jobs, EPSF-* for EPS jobs */
		int pages;						/* number of pages, -1 means unknown */
		int pageorder;					/* -1 (reverse), 0 (special), or 1 (normal) */
		int pagefactor;					/* virtual pages per physical sheet */
		gu_boolean prolog;				/* true if valid prolog section present */
		gu_boolean docsetup;			/* true if valid document setup section present */
		gu_boolean script;				/* delineated pages present */
		int proofmode;					/* desired proofmode value */
		int orientation;				/* one of ORIENTATION_* */
		long input_bytes;				/* Size of input file in bytes */
		long postscript_bytes;			/* Size of input PostScript code in bytes */
		int parts;						/* number of sub jobs job was divided into */
		int docdata;					/* Clean7Bit, Clean8Bit, Binary */
		} attr;
	struct {
		gu_boolean binselect;			/* do automatic bin selection */
		int copies;						/* number of copies to print, -1=unspecified */
		gu_boolean collate;				/* TRUE if we should collate copies */
		gu_boolean keep_badfeatures;	/* keep Feature code we can not replace from PPD file */
		unsigned int hacks;				/* enables code to deal with problems */
		gu_boolean resume;				/* TRUE if should try to resume jobs in the middle */
		} opts;
	struct {							/* N Up parameters */
		int N;							/* virtual pages per physical side */
		gu_boolean borders;				/* TRUE or false, should we print borders */
		int sigsheets;					/* Number of sheets to user per signiture */
		int sigpart;					/* fronts, backs, both */
		gu_boolean job_does_n_up;		/* if true, we won't insert our code */
		} N_Up;
	const char *draft_notice;			/* `Draft' string */
	const char *PassThruPDL;			/* "pcl", "hpgl2", etc., NULL for PostScript */
	const char *Filters;				/* filter chain: "pcl", "gzip pcl", etc. */
	const char *PJL;					/* HP PJL lines, newline separated */
	gu_boolean StripPrinter;			/* Strip resources that printer has? */
	struct {
		char *mask;						/* which pages should be printed? */
		int count;						/* how many 1's in the mask? */
		} page_list;
	const char *question;				/* partial URL for question */
	const char *ripopts;				/* name=value pairs for Ghostscript and such */
	} ;

/* Possible values for orientation member of struct QEntryFile. */
#define ORIENTATION_UNKNOWN 0
#define ORIENTATION_PORTRAIT 1
#define ORIENTATION_LANDSCAPE 2

/* Bitfield values for opts.hacks. */
#define HACK_KEEPINFILE 1
#define HACK_TRANSPARENT 2
#define HACK_BADEPS 4
#define HACK_EDITPS 8
#define HACK_DEFAULT_HACKS 8	/* documented in ppr(1) */

/* "%%ProofMode:" values. */
#define PROOFMODE_NOTIFYME -1
#define PROOFMODE_SUBSTITUTE 0			/* default mode */
#define PROOFMODE_TRUSTME 1

/* Signiture part values. */
#define SIG_FRONTS 1
#define SIG_BACKS 2
#define SIG_BOTH (SIG_FRONTS | SIG_BACKS)

/* "%%PageOrder:" settings. */
#define PAGEORDER_ASCEND 1
#define PAGEORDER_DESCEND -1
#define PAGEORDER_SPECIAL 0

/*
** Job status values.
** A positive value is the ID of a printer which
** is currently printing the job.
*/
#define STATUS_WAITING -1				/* waiting for printer */
#define STATUS_HELD -2					/* put on hold by user */
#define STATUS_WAITING4MEDIA -3			/* proper media not mounted */
#define STATUS_ARRESTED -4				/* automaticaly put on hold because of a job error */
#define STATUS_CANCEL -5				/* being canceled */
#define STATUS_SEIZING -6				/* going from printing to held (get rid of this) */
#define STATUS_STRANDED -7				/* no printer can print it */
#define STATUS_FINISHED -8				/* job has been printed */
#define STATUS_FUNDS -9					/* insufficient funds to print it */
#define STATUS_RECEIVING -10			/* waiting for job text to arrive */

/* First end of file marker in a transmitted queue file. */
#define QF_ENDTAG1 "..\n"
#define QF_ENDTAG2 ".\n"
#define CHOPT_QF_ENDTAG1 ".."
#define CHOPT_QF_ENDTAG2 "."

/* Values used in ppop -> pprd communication: */
#define QUEUEID_WILDCARD -1
#define WILDCARD_JOBID -1
#define WILDCARD_SUBID -1

void qentryfile_clear(struct QEntryFile *job);
int qentryfile_load(struct QEntryFile *job, FILE *qfile);
int qentryfile_save(const struct QEntryFile *qentry, FILE *Qfile);
void qentryfile_free(struct QEntryFile *job);

int parse_qfname(char *buffer, const char **destname, short int *id, short int *subid);
int pagemask_encode(struct QEntryFile *job, const char pages[]);
void  pagemask_print(const struct QEntryFile *job);
int pagemask_get_bit(const struct QEntryFile *job, int page);
int pagemask_count(const struct QEntryFile *job);

/* ======================== Destinations ================================ */

struct PRINTER_SPOOL_STATE {
	gu_boolean accepting;				/* TRUE if is accepting as destination */
	int previous_status;				/* saved previous status */
	int status;							/* idle, disabled, etc */
	int next_error_retry;				/* number of next retry */
	int next_engaged_retry;				/* number of times otherwise engaged or off-line */
	int countdown;						/* seconds till next retry */
	int printer_state_change_time;		/* see RFC 3995 6.1 */
	gu_boolean protected;				/* TRUE if "Charge:" line in conf file */
	int job_count;						/* how many jobs in printer's own queue? */
	} ;

struct GROUP_SPOOL_STATE {
	gu_boolean accepting;				/* TRUE if accepting new jobs */
	gu_boolean held;					/* is the queue held? */
	int printer_state_change_time;		/* see RFC 3995 6.1 */
	gu_boolean protected;				/* TRUE if we should restrict use */
	int job_count;						/* how many jobs queued for this group? */
	} ;

struct ALERT {
	int interval;
	char *method;
	char *address;
	} ;

int printer_spool_state_load(struct PRINTER_SPOOL_STATE *pstate, const char prnname[]);
int group_spool_state_load(struct GROUP_SPOOL_STATE *gstate, const char grpname[]);

/* ======================== Media file format =========================== */
struct Media
	{
	char medianame[MAX_MEDIANAME];	/* from PostScript comment */
	double width;					/* width in 72nds of an inch */
	double height;					/* height in 72nds of an inch */
	double weight;					/* weight grams per square metre */
	char colour[MAX_COLOURNAME];	/* colour name */
	char type[MAX_TYPENAME];		/* type string, preprinted forms */
	int flag_suitability;			/* rank on scale of 1 to 10 */
	} ;

/* ============= money charged for printing ================= */

struct COMPUTED_CHARGE
	{
	int duplex_sheets;
	int per_duplex;
	int simplex_sheets;
	int per_simplex;
	int total;
	} ;

void compute_charge(struct COMPUTED_CHARGE *charge, int per_duplex_sheet, int per_simplex_sheet, int vpages,
		int n_up_n, int vpages_per_sheet, int sigsheets, int sigpart, int copies);

/* end of file */

