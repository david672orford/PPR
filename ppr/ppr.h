/*
** mouse:~ppr/src/ppr/ppr.h
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 16 June 2000.
*/

/*
** Debugging options
*/
/* #define DEBUG_RESOURCES 1 */
/* #define DEBUG_RESOURCES_DETAILED 1 */
/* #define DEBUG_REQUIREMENTS 1 */
/* #define DEBUG_MEDIA_MATCHING 1 */
#define DEBUG_SPLIT 1
/* #define DEBUG_ACL 1 */

/*
** This sets the maximum number of PPD file features to insert code for.
** It is ok to change this.  It is the max number of -F switches.
*/
#define MAX_FEATURES 16

/*
** The size in bytes of input buffer to request.  There is no really
** good reasons to change this.  Whatever you do, don't decrease this
** below 8192 bytes.  If you make it smaller, automatic type detection
** may start to fail for certain hard-to-identify formats.
*/
#define IN_BSIZE 8192

/*
** Amount of extra space for ungetting characters.  This is for
** the in_ungetc() function in ppr_infile.c.
*/
#define IN_UNGETC_SIZE 10

/*
** Maximum number of segments to break job into.
*/
#define MAX_SEGMENTS 10

/* ================ Nothing for the user to change below this line ================ */

/*
** Types of warnings
*/
#define WARNING_PEEVE 0                 /* PPr fixed it but was annoyed */
#define WARNING_SEVERE 1                /* could mean something will happen */
#define WARNING_ALMOST_FATAL 2          /* internal table overflow, etc. */
#define WARNING_NONE 10000              /* higher than anything */

/*
** Values for current duplex.
*/
#define DUPLEX_NONE 0			/* not duplex, simplex */
#define DUPLEX_SIMPLEX_TUMBLE 1		/* simplex variant */
#define DUPLEX_DUPLEX_NOTUMBLE 2	/* normal duplex */
#define DUPLEX_DUPLEX_TUMBLE 3		/* duplex(tumble) */

/*
** Types of references to resources.
** For multiple reference types, these are ORed together.
*/
#define REREF_NEEDED 1              /* %%DocumentNeededResources: */
#define REREF_SUPPLIED 2            /* %%DocumentSuppliedResources: */
#define REREF_INCLUDE 4             /* %%IncludeResource: */
#define REREF_REALLY_SUPPLIED 8     /* %%BeginResource: */
#define REREF_PAGE 16               /* %%PageResources: */
#define REREF_UNCLEAR 32            /* %%DocumentFonts:, etc. */
#define REREF_REMOVED 64            /* if resource stript out */
#define REREF_FIXINCLUDE 128        /* missing %%IncludeResource: */

/* structure to describe one resource
   we will have an array of these */
struct Resource
    {
    char *R_Type;                   /* resource type */
    char *R_Name;                   /* resource name */
    double R_Version;               /* incompatiblity version number */
    int R_Revision;                 /* upward compatible version number */
    } ;

/* types of references to media */
#define MREF_DOC 1                  /* need for document */
#define MREF_PAGE 2                 /* if for this page */

/* types of references to requirments */
#define REQ_DOC 1                   /* needed for document */
#define REQ_PAGE 2                  /* needed for page */
#define REQ_DELETED 4               /* if -F switch removes */

/* structure of a requirement */
/* --- no structure, it is just a string --- */

/* structure of a thing, any of Resource, Media, or Requirement */
struct Thing
    {
    int th_type;		/* type of this thing */
    unsigned int R_Flags;	/* types of reference */
    void *th_ptr;		/* pointer to this thing */
    } ;

/* possible Thing types */
#define TH_RESOURCE 0
#define TH_MEDIA 1
#define TH_REQUIREMENT 2

/* ppr respond options (-e switch) */
#define PPR_RESPOND_BY_NONE 0		/* respond() does nothing */
#define PPR_RESPOND_BY_STDERR 1		/* respond() writes to stderr */
#define PPR_RESPOND_BY_RESPONDER 2	/* respond() exec()s responder */
#define PPR_RESPOND_BY_BOTH 3		/* respond() does both */

/* Possible --cache-store settings: */
enum CACHE_STORE { CACHE_STORE_NONE, CACHE_STORE_UNAVAILABLE, CACHE_STORE_UNCACHED };

/*===============================================
** global variables
===============================================*/

/* What getcwd() returned before chdir() */
extern char *starting_directory;

/* output files */
extern FILE *comments;		/* file for header & trailer comments */
extern FILE *page_comments;	/* file for page level comments */
extern FILE *text;		/* file for remainder of text */
extern FILE *cache_file;	/* file to copy resource into */

extern uid_t user_uid;		/* uid of person submitting the job */
extern uid_t setuid_uid;	/* uid of spooler owner */
extern gid_t user_gid;
extern gid_t setgid_gid;

extern int read_copies;			/* TRUE if should auto copies */
extern int read_duplex;			/* TRUE for auto duplex */
extern int read_signature;		/* TRUE for auto signature */
extern int read_For;			/* TRUE to heed "%%For:" */
extern int read_Title;
extern int read_ProofMode;		/* TRUE to heed "%%ProofMode:" */
extern int read_Routing;
extern int current_duplex;		/* what auto duplex code sets */

extern struct QFileEntry qentry;	/* where we build our queue entry */
extern int pagenumber;			/* count of %%Page: comments */

/* Command line option settings. */
extern int ppr_respond_by;		/* should ppr use responder or stderr or both? */
extern int option_nofilter_hexdump;	/* if TRUE, always use hexdump when no filter */
extern char *option_filter_options;	/* Options from -o switch */
extern unsigned int option_gab_mask;
extern int option_editps_level;
enum MARKUP {MARKUP_FORMAT, MARKUP_LP, MARKUP_PR, MARKUP_FALLBACK_LP, MARKUP_FALLBACK_PR};
extern enum MARKUP option_markup;	/* handling of markup languages */

/* Things */
extern struct Thing *things;
extern int thing_count;

/* lines */
extern char line[MAX_LINE+2];	/* current input line */
extern int line_len;		/* length of line */
extern gu_boolean line_overflow;	/* true or false */
extern char *tokens[];		/* array of tokens broken out of line[] */
extern int cont;                /* is this comment a continuation? */

extern char *AuthCode;
extern struct Media guess_media;

/*===============================================================
** external functions
===============================================================*/
/* ppr_main.c */
extern const char *myname;
void warning(int level, const char *message, ...)
#ifdef __GNUC__
__attribute__ (( format (printf, 2, 3) ))
#endif
;
void fatal(int exitval, const char *message, ...)
#ifdef __GNUC__
__attribute__ (( noreturn, format (printf, 2, 3) ))
#endif
;
void file_cleanup(void);
int write_queue_file(struct QFileEntry *qentry);
void submit_job(struct QFileEntry *qe, int segment);
void become_user(void);
void unbecome_user(void);
int parse_feature_option(const char name[]);
int parse_hack_option(const char *name);

/* ppr_selpgs.c */
void select_pages(struct QFileEntry *qentry, const char *page_list);

/* ppr_split.c */
void prepare_thing_bitmap(void);
void set_thing_bit(int bitoffset);
void Y_switch(const char *optarg);
int split_job(struct QFileEntry *qentry);
int is_thing_in_current_fragment(int thing_number, int fragment);
extern char default_pagemedia[MAX_MEDIANAME+1];

/* ppr_outfile.c */
void get_next_id(struct QFileEntry *q);
void open_output(void);

/* ppr_dscdoc.c */
void read_header_comments(void);
gu_boolean read_prolog(void);
void read_pages(void);
void end_of_page_processing(void);
void read_trailer(void);
void do_dsc_Routing(const char *routingstr);
void do_dsc_For(const char *forstr);

/* ppr_res.c */
int resource(int reftype, const char *restype, int first);
void resource_clear(int reftype, const char *restype);
void dump_page_resources(void);
void rationalize_resources(void);
void write_resource_lines(FILE *out, int fragment);

/* ppr_media.c */
void dump_document_media(FILE *ofile, int fragment);
void dump_page_media(void);
void media(int reftype, int first);
void write_media_lines(FILE *out, int fragment);

/* ppr_req.c */
void dump_document_requirements(void);
void dump_page_requirements(void);
void requirement(int reftype, const char req_name[]);
void write_requirement_lines(FILE *out, int fragment);
void delete_requirement(const char req_name[]);

/* ppr_old2new.c */
void old_DSC_to_new(void);

/* ppr_simplify.c */
void copy_comment(FILE *out);
void getline_simplify_cache(void);
void getline_simplify_cache_hide_nest(void);
void getline_simplify_cache_hide_nest_hide_ps(void);
extern int dsc_comment_number;
extern int eof_comment_present;

/* ppr_rcache.c */
int get_cache_strip_count(void);
void begin_resource(void);
void end_resource(void);
void abort_resource(void);

/* ppr_mactt.c */
gu_boolean truetype_more_needed(int current_features);
int truetype_set_fontmode(const char filename[]);
void truetype_merge_fonts(char *fontname, char *oldfont, char *newfont);

/* ppr_respond.c */
int respond(int respcode, const char *extra);

/* ppr_nest.c */
void nest_push(int leveltype, const char *name);
void nest_pop(int leveltype);
int nest_level(void);
int nest_inermost_type(void);
#define NEST_NONE 0
#define NEST_RES 1
#define NEST_DOC 2
#define NEST_BADEPS 3
#define NEST_BADRES 4
const char *str_outermost_types(int sectioncode);
void outermost_start(int sectioncode);
void outermost_end(int sectioncode);
int outermost_current(void);
#define OUTERMOST_UNDEFINED 0
#define OUTERMOST_HEADER_COMMENTS 1
#define OUTERMOST_PROLOG 2
#define OUTERMOST_DOCDEFAULTS 3
#define OUTERMOST_DOCSETUP 4
#define OUTERMOST_SCRIPT 5
#define OUTERMOST_TRAILER 6

/* ppr_things.c */
void things_space_check(void);

/* ppr_editps.c */
const char **editps_identify(const unsigned char *in_ptr, int in_left);

/* end of file */
