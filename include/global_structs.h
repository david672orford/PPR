/*
** mouse:~ppr/src/include/global_structs.h
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
** Last modified 10 September 2004.
*/

/* =================== destined for libppr_queueentry.h =====================*/

#define CACHE_PRIORITY_AUTO -1
#define CACHE_PRIORITY_LOW 0
#define CACHE_PRIORITY_HIGH 1

/*
** A record which describes a printer commentator which
** should be fed information about what is going on with
** the printer.
**
** This structure is included in struct QFileEntry.	 It is also
** used to store "Commentator:" lines from the printer configuration
** file.  The "next" member is only used in the latter instance.
*/
struct COMMENTATOR
		{
		int interests;					/* bitmask telling which events to invoke for */
		const char *progname;			/* "file" or program to invoke */
		const char *address;			/* first parameter to feed to it */
		const char *options;			/* second parameter to feed to it */
		struct COMMENTATOR *next;
		} ;

/*
** This structure can hold the contents of a queue file.
*/
struct QFileEntry
	{
	float PPRVersion;					/* version number of PPR that created queue entry */

	const char *destnode;				/* node this job will be sent to */
	const char *destname;				/* destination (group or printer) */
	short int id;						/* queue id number */
	short int subid;					/* fractional part of id number */
	const char *homenode;				/* the node this job origionated on */

	SHORT_INT status;					/* job status */
	short unsigned int flags;			/* job flags */
	const char *magic_cookie;			/* secret about this job */

	int priority;						/* priority number (0-39) */
	long time;							/* time job was submitted (don't use time_t) */
	long user;							/* id of user who submitted it (don't use uid_t) */
	const char *username;				/* text version of "user" */
	const char *proxy_for;				/* -X switch string */
	const char *lc_messages;			/* language setting for messages */
	const char *For;					/* %%For: line for PostScript header */
	const char *charge_to;				/* charge account to debit */
	const char *Routing;				/* %%Routing: line for PostScript header */
	const char *Title;					/* %%Title: for Postscript header */
	const char *Creator;				/* application which created it */
	const char *lpqFileName;			/* name of input file */
	const char *responder;				/* program for sending messages to user */
	const char *responder_address;		/* address for errors, possibly NULL */
	const char *responder_options;		/* name=value list of responder options */
	int commentary;						/* bitmask of commentary to send thru the responder */
	int nmedia;							/* number of media types */
	int media[MAX_DOCMEDIA];			/* list of required media types */
	int do_banner;						/* should we print a banner page? */
	int do_trailer;						/* should we print a trailer page? */
	struct {							/* various attributes */
		int langlevel;					/* PostScript language level */
		int extensions;					/* bit fields of extension to level 1 */
		float DSClevel;					/* DSC comments version */
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
		} N_Up;
	const char *draft_notice;			/* `Draft' string */
	const char *PassThruPDL;			/* "pcl", "hpgl2", etc., NULL for PostScript */
	const char *Filters;				/* filter chain: "pcl", "gzip pcl", etc. */
	const char *PJL;					/* HP PJL lines, newline separated */
	int CachePriority;
	gu_boolean StripPrinter;			/* Strip resources that printer has? */
	struct {
		char *mask;						/* which pages should be printed? */
		int count;						/* how many 1's in the mask? */
		} page_list;
	const char *question;				/* partial URL for question */
	const char *ripopts;				/* name=value pairs for Ghostscript and such */
	} ;

/* Possible values for orientation member of struct QFileEntry. */
#define ORIENTATION_UNKNOWN 0
#define ORIENTATION_PORTRAIT 1
#define ORIENTATION_LANDSCAPE 2

/* Bitfield values for opts.hacks. */
#define HACK_KEEPINFILE 1
#define HACK_TRANSPARENT 2
#define HACK_BADEPS 4
#define HACK_EDITPS 8
#define HACK_DEFAULT_HACKS 0	/* documented in ppr(1) */

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

/* First end of file marker in a transmitted queue file. */
#define QF_ENDTAG1 "..\n"
#define QF_ENDTAG2 ".\n"
#define CHOPT_QF_ENDTAG1 ".."
#define CHOPT_QF_ENDTAG2 "."

/* Values used in ppop -> pprd communication: */
#define NODEID_WILDCARD -1
#define NODEID_NOTFOUND -2
#define NODEID_LOCALHOST 0
#define QUEUEID_WILDCARD -1
#define WILDCARD_JOBID -1
#define WILDCARD_SUBID -1

int read_struct_QFileEntry(FILE *qfile, struct QFileEntry *job);
int write_struct_QFileEntry(FILE *Qfile, const struct QFileEntry *qentry);
void destroy_struct_QFileEntry(struct QFileEntry *job);
int parse_qfname(char *buffer, const char **destnode, const char **destname, short int *id, short int *subid, const char **homenode);
int pagemask_encode(struct QFileEntry *job, const char pages[]);
void  pagemask_print(const struct QFileEntry *job);
int pagemask_get_bit(const struct QFileEntry *job, int page);
int pagemask_count(const struct QFileEntry *job);

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

