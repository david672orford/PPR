/*
** mouse:~ppr/src/ppr/ppr_main.c
** Copyright 1995--2002, Trinity College Computing Center.
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
** Last revised 15 November 2002.
*/

/*
** Main module of the program used to submit jobs to the
** PPR print spooler.
*/

#include "before_system.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include <ctype.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "ppr.h"
#include "ppr_infile.h"
#include "ppr_conffile.h"	/* for switchsets, deffiltopts, and passthru */
#include "userdb.h"		/* for user charge accounts */
#include "ppr_exits.h"
#include "respond.h"
#include "version.h"		/* for "--version" switch */
#include "ppr_gab.h"

/*
** global variables
*/

/* Name of this program (for error messages). */
const char *myname = "ppr";

/* input line */
char line[MAX_LINE+2];	    	    /* input line plus one, plus NULL */
int line_len;			    /* length of input line in bytes */
gu_boolean line_overflow;		    /* is lines truncated? true or false */

/* output files */
FILE *comments = (FILE*)NULL;		/* file for header & trailer comments */
FILE *page_comments = (FILE*)NULL;	/* file for page level comments */
FILE *text = (FILE*)NULL;		/* file for remainder of text */
FILE *cache_file;			/* file to copy resource into */
static int spool_files_created = FALSE;	/* TRUE once we have created some files, used for cleanup on abort */

/* Information used for determining privledges and such. */
uid_t user_uid;				/* Unix user id of person submitting the job */
uid_t setuid_uid;			/* uid of spooler owner (ppr) */
gid_t user_gid;
gid_t setgid_gid;

/* Command line option settings, static */
static int warning_level = WARNING_SEVERE;	/* these and more serious allowed */
static int warning_log = FALSE;			/* set true if warnings should go to log */
static int option_unlink_jobfile = FALSE;	/* Was the -U switch used? */
static int use_username = FALSE;		/* User username instead of comment as default For: */
static int ignore_truncated = FALSE;		/* TRUE if should discard without %%EOF */
static int option_show_jobid = 0;		/* 0 for don't , 1 for short, 2 for long */
static int use_authcode = FALSE;		/* true if using authcode line to id */
static int preauthorized = FALSE;		/* set true by -A switch */
static const char *option_page_list = (char*)NULL;

/* Command line option settings */
const char *features[MAX_FEATURES];		/* -F switch features to add */
int features_count = 0;				/* number asked for (number of -F switches?) */
gu_boolean option_strip_cache = FALSE;		/* TRUE if should strip those in cache */
gu_boolean option_strip_fontindex = FALSE;		/* TRUE if should strip those in fontindex.db */
enum CACHE_STORE option_cache_store = CACHE_STORE_NONE;
int ppr_respond_by = PPR_RESPOND_BY_STDERR;
int option_nofilter_hexdump = FALSE;		/* don't encourage use of hexdump when no filter */
char *option_filter_options = (char*)NULL;	/* contents of -o switch */
int option_TrueTypeQuery = TT_UNKNOWN;		/* for ppr_mactt.c */
unsigned int option_gab_mask = 0;		/* Mask to tell what to gab about. */
int option_editps_level = 1;			/* 1 thru 10 */
enum MARKUP option_markup = MARKUP_FALLBACK_LP;	/* how to treat markup languages */

/* -R switch command line option settings. */
int read_copies = TRUE;			/* TRUE if should read copies from file */
int read_duplex = TRUE;			/* TRUE if we should guess duplex */
int read_signature = TRUE;		/* TRUE if we should implement signature features */
int read_nup = TRUE;
int read_For = FALSE;			/* Pay attention to "%%For:" lines? */
int read_ProofMode = TRUE;
int read_Title = TRUE;
int read_Routing = TRUE;

/* Duplex handling is complex. */
int current_duplex = DUPLEX_NONE;
gu_boolean current_duplex_enforce = FALSE;

/* odds and ends */
static FILE *FIFO = (FILE*)NULL;	/* streams library thing for pipe to pprd */
struct QFileEntry qentry;		/* structure in which we build our queue entry */
int pagenumber = 0;			/* count of %%Page: comments */
char *AuthCode = (char*)NULL;		/* clear text of authcode */
static int auth_needed;			/* TRUE if use_authcode or dest protect */
char *starting_directory = (char*)NULL;	/* Not MAX_PPR_PATH!  (MAX_PPR_PATH might be just large enough for PPR file names) */
static const char *charge_to_switch_value = (char*)NULL;
static const char *default_For = (char*)NULL;

/* default media */
const char *default_medium = NULL;	/* medium to start with if no "%%DocumentMedia:" comment */
struct Media guess_media;		/* used to gather information to guess proper medium */

/* Page factors */
static int duplex_pagefactor;		/* pages per sheet multiple due to duplex */

/* Array of things (resources, requirements, etc.). */
struct Thing *things = (struct Thing *)NULL;	/* fonts, media and such */
int thing_count = 0;				/* number of entries used */

/* Table of hacks which may be enabled or disabled with the -H switch. */
static struct {const char *name; unsigned int bit;} hack_table[]=
{
{"keepinfile", HACK_KEEPINFILE},
{"transparent", HACK_TRANSPARENT},
{"badeps", HACK_BADEPS},
{"editps", HACK_EDITPS},
{(char*)NULL, 0}
};

/* Table of things to gab about. */
static struct {const char *name; unsigned int bit;} gab_table[]=
{
{"infile:autotype", GAB_INFILE_AUTOTYPE},
{"infile:filter", GAB_INFILE_FILTER},
{"infile:editps", GAB_INFILE_EDITPS},
{"structure:nesting", GAB_STRUCTURE_NESTING},
{"structure:sections", GAB_STRUCTURE_SECTIONS},
{"media:matching", GAB_MEDIA_MATCHING},
{(const char*)NULL, 0}
};

/*=========================================================================
** Error Routines
=========================================================================*/

/*
** This should get called for _all_ abnormal terminations.  We figure
** out the cause from the exit code and print an error message
** on stderr or run a responder or both.
*/
void ppr_abort(int exitval, const char *extra)
    {
    int responder_code;
    const char *template;

    switch(exitval)
	{
	case PPREXIT_NOFILTER:
	    responder_code = RESP_NOFILTER;
	    template = "no filter for %s";
	    break;
	case PPREXIT_NOMATCH:
	    responder_code = RESP_BADMEDIA;
	    template = "can't match medium \"%s\"";
	case PPREXIT_NOCHARGEACCT:
	    responder_code = RESP_CANCELED_NOCHARGEACCT;
	    template = "charge account \"%s\" doesn't exist";
	    break;
	case PPREXIT_OVERDRAWN:
	    responder_code = RESP_CANCELED_OVERDRAWN;
	    template = "the account \"%s\" is overdrawn";
	    break;
	case PPREXIT_BADAUTH:
	    responder_code = RESP_CANCELED_BADAUTH;
	    template = "bad authcode";
	    break;
	case PPREXIT_NONCONFORMING:
	    responder_code = RESP_CANCELED_NONCONFORMING;
	    template = "input does not conform to DSC";
	    break;
	case PPREXIT_ACL:
	    responder_code = RESP_CANCELED_ACL;
	    template = "user \"%s\" denied access by queue ACL";
	    break;
	case PPREXIT_NOSPOOLER:
	    responder_code = RESP_NOSPOOLER;
	    template = "pprd is not running";
	    break;
	case PPREXIT_NOTPOSSIBLE:
	    responder_code = RESP_CANCELED_NOPAGES;
	    template = "can't selected pages because they are not delimited";
	    break;
	case PPREXIT_SYNTAX:				/* from fatal() */
	    responder_code = RESP_FATAL_SYNTAX;
	    template = "%s";
	    break;
	default:					/* from fatal() */
	    responder_code = RESP_FATAL;
	    template = "%s";
	    break;
	}

    if(ppr_respond_by & PPR_RESPOND_BY_STDERR)
	{
	fflush(stdout);				/* <-- prevent confusion */
	fprintf(stderr, "%s: ", myname);
	fprintf(stderr, template, extra);
	fputs("\n", stderr);
	}

    if(ppr_respond_by & PPR_RESPOND_BY_RESPONDER)
	{
	respond(responder_code, extra ? extra : "(null)");
	}

    file_cleanup();

    exit(exitval);
    } /* end of ppr_abort() */

/*
** Handle fatal errors by formatting a message and then calling
** ppr_abort().
*/
void fatal(int exitval, const char *message, ... )
    {
    va_list va;
    char errbuf[256];

    va_start(va, message);				/* Format the error message */
    vsnprintf(errbuf, sizeof(errbuf), message, va);	/* as a string. */
    va_end(va);

    ppr_abort(exitval, errbuf);
    } /* end of fatal() */

/*
** A few routines in libppr use this to print warnings to stderr.  They can't
** print to stderr directly because they are also called from pprdrv, in
** which case the messages should go to the log file.
**
** We also use this function ourselves to print non-fatal error messages.
** Of course, we use warning() when the non fatal error is related to
** the contents of the job file.
*/
void error(const char *message, ... )
    {
    va_list va;
    fprintf(stderr, "%s: ", myname);
    va_start(va,message);		/* Format the error message */
    vfprintf(stderr,message,va);	/* as a string. */
    va_end(va);
    fputc('\n', stderr);
    } /* end of error() */

/*
** Call this function to issue a warning related to the contents of the job.
** It is printed on stderr or sent to the log file, depending on whether or
** not the ``-w log'' switch was used.
*/
void warning(int level, const char *message, ... )
    {
    const char function[] = "warning";
    va_list va;
    char wfname[MAX_PPR_PATH];
    FILE *wfile;

    if(!qentry.destnode || !qentry.destname || !qentry.homenode)
	fatal(PPREXIT_OTHERERR, "%s(): assertion failed", function);

    if(level < warning_level)	/* if warning level too low */
	return;			/* do not print it */

    if(warning_log)		/* if warnings are to go to log file, */
	{			/* open this job's log file */
	ppr_fnamef(wfname, "%s/%s:%s-%d.0(%s)-log",
		DATADIR,
		qentry.destnode,
		qentry.destname, qentry.id, qentry.homenode);
	if((wfile = fopen(wfname, "a")) == (FILE*)NULL)
	    {
	    fprintf(stderr, _("Failed to open log file, using stderr.\n"));
	    wfile = stderr;	/* if fail, use stderr anyway */
	    }
	}
    else			/* if not logging warnings, */
	{			/* send them to stderr */
	wfile = stderr;
	}

    va_start(va, message);
    fprintf(wfile, _("WARNING: "));	/* now, print the warning message */
    vfprintf(wfile, message, va);
    fprintf(wfile, "\n");
    va_end(va);

    if(wfile != stderr)			/* if we didn't use stderr, */
	fclose(wfile);			/* close what we did use */
    else				/* if was stderr, */
	fflush(stderr);			/* empty output buffer */
					/* (important for papsrv) */
    } /* end of warning() */

/*=========================================================================
** Miscelanious support routines for main().
=========================================================================*/
/*
** Write the Feature: lines to the queue file.
*/
static void write_feature_lines(FILE *qfile)
    {
    int x;

    for(x=0; x < features_count; x++)
	fprintf(qfile, "Feature: %s\n", features[x]);

    } /* end of write_feature_lines() */

/*
** Assert that a variable contains a valid value.  We must check for invalid
** values because if they get into the queue file, the queue file reading
** routines will choke on them.
**
** value 			-- the value to be checked
** null_ok			-- is a NULL pointer acceptable?
** empty_ok			-- is a zero length string acceptable?
** entirely_whitespace_ok	-- is a value consisting entirely of whitespace ok?
** name				-- what should we call the value in error messages?
** is_argument			-- should we describe it as an argument to name?
*/
static void assert_ok_value(const char value[], gu_boolean null_ok, gu_boolean empty_ok, gu_boolean entirely_whitespace_ok, const char name[], gu_boolean is_argument)
    {
    if(value == (const char *)NULL)
    	{
	if(!null_ok)
	    fatal(PPREXIT_OTHERERR, _("NULL value for %s%s"), name, is_argument ? _(" argument") : "");
    	}

    else
    	{
	int len;

    	if(strchr(value, '\n'))
	    fatal(PPREXIT_OTHERERR, _("line feed in %s%s"), name, is_argument ? _(" argument") : "");

	len = strlen(value);

	if(len == 0)
	    {
	    if(! empty_ok)
	    	fatal(PPREXIT_OTHERERR, _("%s%s is empty"), name, is_argument ? _(" argument") : "");
	    }
	else
	    {
	    if(! entirely_whitespace_ok )
		{
		int x;

		for(x=0; x < len; x++)
		    {
		    if(! isspace(value[x]))
			return;
		    }

		fatal(PPREXIT_OTHERERR, _("%s%s is entirely whitespace"), name, is_argument ? _(" argument") : "");
		}
	    }
	}
    } /* end of assert_ok_value() */

/*
** Create and fill the queue file.
** Return -1 if we run out of disk space.
*/
int write_queue_file(struct QFileEntry *qentry)
    {
    const char function[] = "write_queue_file";
    char magic_cookie[16];
    char qfname[MAX_PPR_PATH];
    int fd;
    FILE *Qfile;

    /* This code looks for things that could make a mess of the queue file. */
    assert_ok_value(qentry->For, FALSE, FALSE, FALSE, "qentry->For", FALSE);
    assert_ok_value(qentry->charge_to, TRUE, FALSE, FALSE, "qentry->charge_to", FALSE);
    assert_ok_value(qentry->Title, TRUE, TRUE, TRUE, "qentry->Title", FALSE);
    assert_ok_value(qentry->draft_notice, TRUE, FALSE, FALSE, "qentry->draft_notice", FALSE);
    assert_ok_value(qentry->Creator, TRUE, FALSE, FALSE, "qentry->Creator", FALSE);
    assert_ok_value(qentry->Routing, TRUE, FALSE, FALSE, "qentry->Routing", FALSE);
    assert_ok_value(qentry->lpqFileName, TRUE, FALSE, FALSE, "qentry->lpqFileName", FALSE);
    assert_ok_value(qentry->responder, FALSE, FALSE, FALSE, "qentry->responder", FALSE);
    assert_ok_value(qentry->responder_address, FALSE, FALSE, FALSE, "qentry->responder_address", FALSE);
    assert_ok_value(qentry->responder_options, TRUE, TRUE, TRUE, "qentry->responder_options", FALSE);

    if(qentry->CachePriority == CACHE_PRIORITY_AUTO)
    	fatal(PPREXIT_OTHERERR, "%s(): CachePriority assertion failed", function);

    /* Create a new magic cookie. */
    snprintf(magic_cookie, sizeof(magic_cookie), "%08X", rand());
    qentry->magic_cookie = magic_cookie;

    /* Construct the queue file name. */
    ppr_fnamef(qfname, "%s/%s:%s-%d.%d(%s)", QUEUEDIR,
    	qentry->destnode, qentry->destname, qentry->id, qentry->subid, qentry->homenode);

    /* Very carefully open the queue file. */
    if((fd = open(qfname, O_WRONLY | O_CREAT | O_EXCL, (S_IRUSR | S_IWUSR))) < 0)
	fatal(PPREXIT_OTHERERR, _("can't open queue file \"%s\", errno=%d (%s)"), qfname, errno, gu_strerror(errno) );
    if((Qfile = fdopen(fd, "w")) == (FILE*)NULL)
    	fatal(PPREXIT_OTHERERR, "%s(): fdopen() failed", function);

    /* Use library code to write the body. */
    write_struct_QFileEntry(Qfile, qentry);
    fprintf(Qfile, "EndMisc\n");

    /* Write an empty Addon section.  Job ticket information may be added later by ppop. */
    fprintf(Qfile, "EndAddon\n");

    /* Add "Media:" lines which indicate what kind of media the job requires. */
    write_media_lines(Qfile, qentry->subid);
    fprintf(Qfile, "EndMedia\n");

    /* Add "Res:" lines which indicate which fonts, procedure sets, etc. the job requires. */
    write_resource_lines(Qfile, qentry->subid);
    fprintf(Qfile, "EndRes\n");

    /* Add "Req:" lines which indicate what printer features are required. */
    write_requirement_lines(Qfile, qentry->subid);
    fprintf(Qfile, "EndReq\n");

    /* Write "@PJL" lines which preserve certain PJL commands from the input file. */
    if(qentry->PJL)
    	{
	int len = strlen(qentry->PJL);
	fprintf(Qfile, "PJLHint: %d\n", len);
	fwrite(qentry->PJL, sizeof(char), len, Qfile);
	}
    fputs("EndPJL\n", Qfile);

    /* Add "Feature:" lines.  This must come last. */
    write_feature_lines(Qfile);
    fputs("EndSetups\n", Qfile);

    /* Close the file while watching for signs of disk full. */
    if(fclose(Qfile) == EOF)
	{
	unlink(qfname);
    	return -1;
    	}

    return 0;
    } /* end of write_queue_file() */

/*
** Open the FIFO to pprd or rpprd.  This FIFO will be used to tell
** the daemon that the job has been placed in the queue.
*/
static FILE *open_fifo(const char name[])
    {
    int fifo;		/* file handle of pipe to pprd */
    int newfifo;

    /*
    ** Try to open the FIFO.  If we fail it is probably because
    ** pprd is not running.  In that case, errno will be set to
    ** ENOENT or ENXIO (depending on whether the spooler has
    ** ever been run).
    */
    #ifdef HAVE_MKFIFO
    if((fifo = open(name, O_WRONLY | O_NONBLOCK)) < 0)
    #else
    if((fifo = open(name, O_WRONLY | O_APPEND)) < 0)
    #endif
	{
	if(errno != ENXIO && errno != ENOENT)
	    fatal(PPREXIT_OTHERERR, _("Can't open \"%s\", errno=%d (%s)"), name, errno, gu_strerror(errno));
	return (FILE*)NULL;
	}

    /*
    ** This loop takes care of things if this program was
    ** invoked without file descriptors 0 thru 2 already open.
    ** It is popularly supposed that the operating system
    ** connects these to stdin, stdout, and stderr, but actually
    ** the shell does this.  Some daemons may invoke ppr without
    ** opening something on these file descriptors.  If this
    ** happens, we must move the fifo up and open /dev/null
    ** on descriptors 0 thru 2.
    */
    while(fifo <= 2)
    	{
	newfifo = dup(fifo);		/* move fifo up to next handle */
	close(fifo);			/* close old one */
	if(open("/dev/null", (fifo==0 ? O_RDONLY : O_WRONLY) ) != fifo)
	    exit(PPREXIT_OTHERERR);	/* Mustn't call fatal! */
	fifo = newfifo;			/* adopt the new handle */
    	}

    /* Create an I/O stream which represents the FIFO. */
    return fdopen(fifo, "w");
    } /* end of open_fifo() */

/*
** Function to send a command to the FIFO to submit a job.
*/
void submit_job(struct QFileEntry *qe, int subid)
    {
    fprintf(FIFO, "j %s:%s-%d.%d(%s)\n",
	qe->destnode,
    	qe->destname,
    	qe->id, subid,
	qe->homenode);

    /*
    ** If --show-jobid or --show-long-jobid has been used,
    ** display the id of the job being submitted.
    **
    ** Notice that --show-jobid produces a message in almost
    ** the same format as System V lp.
    */
    if(option_show_jobid)
	{
	if(option_show_jobid == 1)	/* --show-jobid */
	    printf(_("request id is %s (1 file)\n"), remote_jobid(qentry.destnode, qentry.destname, qentry.id, subid, qentry.homenode));
	else				/* --show-long-jobid */
	    printf(_("Request id: %s:%s-%d.%d(%s)\n"), qentry.destnode, qentry.destname, qentry.id, subid, qentry.homenode);
	}
    } /* end of submit_job() */

/*
** Code to check if there is a charge (money) for printing to the selected
** destination and thus the user needs to be specially authorized to submit
** jobs to this destination.  If so, check if he is authorized.  If
** authorization is required but has not been obtained, it is a fatal error
** and this routine never returns.
**
** When this routine is called, qentry.For should already be set.
*/
static void authorization_charge(void)
    {
    if((auth_needed = (use_authcode || destination_protected(qentry.destnode, qentry.destname))))
	{
	struct userdb user;
	int ret;

    	/*
    	** Figure out which account we must charge this to.
    	** If the --charge-to switch was used that is the
    	** answer.  If not and the For was set by the -f switch
    	** or a "%%For:" line with -R for then use that.  If that
    	** doesn't work, use the Unix username.
    	*/
	if(charge_to_switch_value)
	    qentry.charge_to = charge_to_switch_value;
	else if(qentry.For != default_For)
	    qentry.charge_to = qentry.For;
	else
	    qentry.charge_to = qentry.username;

	if(!preauthorized)			/* do user lookup only */
	    {					/* if not preauthorized */
	    ret = db_auth(&user, qentry.charge_to);	/* user lookup */

	    if(ret == USER_ISNT)		/* If user not found, */
		{				/* then, turn away. */
		ppr_abort(PPREXIT_NOCHARGEACCT, qentry.charge_to);
		}

	    else if(ret == USER_OVERDRAWN)	/* If account overdrawn, */
		{				/* then, turn away. */
		ppr_abort(PPREXIT_OVERDRAWN, qentry.charge_to);
		}

	    /* We check for database error. */
	    else if(ret == USER_ERROR)
	    	{
	    	fatal(PPREXIT_OTHERERR, _("can't open printing charge database"));
	    	}

	    /*
	    ** If in authcode mode and "-u yes" has not been set
	    ** then use the comment field from the user database
	    ** as the For.
	    */
	    if(use_authcode && !use_username)
		qentry.For = gu_strdup(user.fullname);

	    /*
	    ** If -a switch and an authcode is required for this user and
	    ** no or wrong authcode, send a message and cancel the job.
	    */
	    if(use_authcode && user.authcode[0] != '\0' &&
			(AuthCode == (char*)NULL || strcmp(AuthCode, user.authcode)) )
		{
		ppr_abort(PPREXIT_BADAUTH, qentry.charge_to);
		}
	    } /* end of if not preauthorized */
	} /* end of if use_authcode or protected printer */

    /* Do not allow jobs w/out page counts on cost per page printers. */
    if(auth_needed)
	{
	if(qentry.attr.pages < 0)
	    {
	    ppr_abort(PPREXIT_NONCONFORMING, (char*)NULL);
	    }

	if(qentry.opts.copies == -1)	/* If `secure' printer, force */
	    qentry.opts.copies = 1;	/* unspecified copies to 1. */
	}

    } /* end of authorization_charge() */

/*
** If there is an access control list on this printer, see if the user has
** access.
*/
static void authorization_acl(void)
    {
    const char *acl_list;
    if((acl_list = extract_acls()))
	{
	char *acl_list_copy, *ptr, *substr;
	gu_boolean real_passes, proxy_for_passes;

	#ifdef DEBUG_ACL
	printf("ACL list: %s\n", acl_list);
	#endif

	real_passes = FALSE;
	proxy_for_passes = qentry.proxy_for ? FALSE : TRUE;

	ptr = acl_list_copy = gu_strdup(acl_list);

	while((!real_passes || !proxy_for_passes) && (substr = strtok(ptr, " ")))
	    {
	    ptr = NULL;
	    #ifdef DEBUG_ACL
	    printf("Trying ACL %s...\n", substr);
	    #endif

	    if(!real_passes && user_acl_allows(qentry.username, substr))
	    	real_passes = TRUE;

	    if(!proxy_for_passes && user_acl_allows(qentry.proxy_for, substr))
	    	proxy_for_passes = TRUE;

	    #ifdef DEBUG_ACL
	    printf("real_passes=%s, proxy_for_passes=%s\n", real_passes ? "TRUE" : "FALSE", proxy_for_passes ? "TRUE" : "FALSE");
	    #endif
	    }

	gu_free(acl_list_copy);

	if(!real_passes)
	    {
	    ppr_abort(PPREXIT_ACL, qentry.username);
	    }

	if(!proxy_for_passes)
	    {
	    ppr_abort(PPREXIT_ACL, qentry.proxy_for);
	    }
	}
    } /* end of authorization_acl() */

/*------------------------------------------------------------------
** `Mop routines' for accident cleanup
------------------------------------------------------------------*/

/*
** Call this function before aborting if there is any chance
** of files existing.  It closes the open files and removes them.
**
** Since ppr_abort() calls this function, this function must not call
** ppr_abort() or fatal() since doing so would almost certainly create
** infinite recursion.
*/
void file_cleanup(void)
    {
    /* Close any job spool files that are open. */
    if(comments)
    	fclose(comments);
    if(page_comments)
    	fclose(page_comments);
    if(text)
    	fclose(text);

    /* Remove any temporary cache file. */
    abort_resource();

    /* Remove any files created by ppr_infile.c */
    infile_file_cleanup();

    /* Remove the partially completed job files. */
    if(spool_files_created)
    	{
	char fname[MAX_PPR_PATH];

	ppr_fnamef(fname, "%s/%s:%s-%d.0(%s)",
		QUEUEDIR,
		qentry.destnode, qentry.destname, qentry.id, qentry.homenode);
	unlink(fname);

	ppr_fnamef(fname, "%s/%s:%s-%d.0(%s)-comments",
		DATADIR,
		qentry.destnode, qentry.destname, qentry.id, qentry.homenode);
	unlink(fname);

	ppr_fnamef(fname, "%s/%s:%s-%d.0(%s)-text",
		DATADIR,
		qentry.destnode, qentry.destname, qentry.id, qentry.homenode);
	unlink(fname);

	ppr_fnamef(fname, "%s/%s:%s-%d.0(%s)-pages",
		DATADIR,
		qentry.destnode, qentry.destname, qentry.id, qentry.homenode);
	unlink(fname);

	ppr_fnamef(fname, "%s/%s:%s-%d.0(%s)-log",
		DATADIR,
		qentry.destnode, qentry.destname, qentry.id, qentry.homenode);
	unlink(fname);
    	}

    /* Let's not do this twice: */
    comments = page_comments = text = (FILE*)NULL;
    spool_files_created = FALSE;
    } /* end of file_cleanup() */

/*
** Signal handler which deletes the partially
** formed queue entry if we are killed.
*/
static void gallows_speach(int signum)
    {
    fatal(PPREXIT_KILLED, _("mortally wounded by signal %d (%s)."), signum, gu_strsignal(signum));
    } /* end of gallows_speach() */

/*
** Child Exit Handler
**
** The purpose of this routine is to detect core dumps and
** other failures in filters.
*/
static void reapchild(int signum)
    {
    const char function[] = "reapchild";
    int wstat;		/* storage for child's exit status */

    /* Get the child's exit code. */
    if(wait(&wstat) == -1)
    	fatal(PPREXIT_OTHERERR, "%s(): wait() failed, errno=%d (%s)", function, errno, gu_strerror(errno));

    if(WIFEXITED(wstat))
    	{
    	switch(WEXITSTATUS(wstat))
    	    {
    	    case 0:			/* Normal exit, */
    	        return;			/* this signal handler need do nothing. */
    	    case 242:
    	        fatal(PPREXIT_OTHERERR, _("filter failed, exit code = 242 (exec() failed?)"));
    	        break;
    	    default:
    	        fatal(PPREXIT_OTHERERR, _("filter failed, exit code = %d"), WEXITSTATUS(wstat));
    	        return;
    	    }
	}
    else if(WIFSIGNALED(wstat))
	{
	if(WCOREDUMP(wstat))
	    fatal(PPREXIT_OTHERERR, _("filter died on receipt of signal %d, core dumped"), WTERMSIG(wstat));
	else
	    fatal(PPREXIT_OTHERERR, _("filter died on receipt of signal %d"), WTERMSIG(wstat));
	}
    else
    	{
    	fatal(PPREXIT_OTHERERR, "%s(): bizzar child termination", function);
    	}
    } /* end of reapchild() */

/*
** Function to return true if the current user is a privledged
** user.  A privledged user is defined as "root", "ppr", or a
** member of the group called "pprprox".  The first
** time this routine is called, it will cache the answer.
**
** The result of this routine determines whether the user is allowed
** to override his user name on headers with a new name in the
** "-f" switch or "%%For:" header line.  It also determines whether
** a user can use the "-A" switch.
**
** Don't change the 0's and 1's in this function to TRUE
** and FALSE because answer starts out with the value -1
** which might be confounded with TRUE.
*/
static int privledged(void)
    {
    static int answer = -1;	/* -1 means undetermined */

    if(answer == -1)		/* if undetermined */
    	{
	answer = 0;		/* Start with 0 for "false". */

	/*
	** Of course, "ppr" is privledged, as is "root" (uid 0).
	*/
	if(user_uid == 0 || user_uid == setuid_uid)
	    answer = 1;		/* Change to "true". */

	/*
	** As are all users who are members of the ACL list "pprprox".
	*/
	else if(user_acl_allows(qentry.username, "pprprox"))
	    answer = 1;
    	} /* end of if answer not determined yet */

    return answer;
    } /* end of privledged() */

/*
** Aquire the user's credentials so that
** we will be able to open only files the user may open.
*/
void become_user(void)
    {
    if(seteuid(user_uid))
	fatal(PPREXIT_OTHERERR, "become_user(): can't seteuid(%ld)", (long)user_uid);

    if(setegid(user_gid))
	fatal(PPREXIT_OTHERERR, "become_user(): can't setegid(%ld)", (long)user_gid);
    }

/*
** Become the PPR owner again.
**
** We do the EGID before the EUID in the hope of bypassing
** bugs in ULTRIX.  This is not well understood.
*/
void unbecome_user(void)
    {
    if(setegid(setgid_gid))
	fatal(PPREXIT_OTHERERR, "unbecome_user(): can't setegid(%ld)", (long)setgid_gid);

    if(seteuid(setuid_uid))
	fatal(PPREXIT_OTHERERR, "unbecome_user(): can't seteuid(%ld)", (long)setuid_uid);
    }

/*--------------------------------------------------------------------
** Routines for parsing the parameters.
--------------------------------------------------------------------*/

/*
** This string lists the single character switches.  If the letter is followed
** by a ":" then the switch requires an argument.  So far the following
** letters have been used:
** aAbBCdDefFGHIKmNnoOPqQrRsStTUvwYXZ
*/
static const char *option_description_string = "ad:e:f:i:m:r:b:t:w:D:F:T:S:q:B:N:n:AC:H:R:Z:O:K:s:P:Io:UY:X:u:G:Q:";

/*
** This table maps long option names to short options or to large integers.
*/
static const struct gu_getopt_opt option_words[] =
	{
	/* These are preliminary assignments and are not yet documented. */
	{"queue", 'd', TRUE},
	{"for", 'f', TRUE},
	{"use-username", 'u', TRUE},
	{"banner", 'b', TRUE},
	{"trailer", 't', TRUE},
	{"errors", 'e', TRUE},
	{"warnings", 'w', TRUE},
	{"proofmode", 'P', TRUE},
	{"strip-cache", 'S', TRUE},
	{"unlink", 'U', FALSE},
	{"signature", 's', TRUE},
	{"n-up", 'N', TRUE},
	{"draft-notice", 'O', TRUE},
	{"split", 'Y', TRUE},
	{"default-medium", 'D', TRUE},
	{"priority", 'q', TRUE},
	{"auto-bin-select", 'B', TRUE},
	{"read", 'R', TRUE},
	{"require-eof", 'Z', TRUE},
	{"authcode-mode", 'a', FALSE},
	{"preauthorized", 'A', FALSE},
	{"truetype", 'Q', TRUE},
	{"keep-bad-features", 'K', TRUE},
	{"filter-options", 'o', TRUE},

	/* These are final. */
	{"feature", 'F', TRUE},
	{"gab", 'G', TRUE},
	{"hack", 'H', TRUE},
	{"file-type", 'T', TRUE},
	{"copies", 'n', TRUE},
	{"routing", 'i', TRUE},
	{"title", 'C', TRUE},
	{"proxy-for", 'X', TRUE},

	/* These are final. */
	{"features", 1000, FALSE},
	{"lpq-filename", 1003, TRUE},
	{"hold", 1004, FALSE},
	{"responder", 'm', TRUE},
	{"responder-address", 'r', TRUE},
	{"responder-options", 1005, TRUE},
    	{"show-jobid", 1006, FALSE},
	{"show-long-jobid", 1007, FALSE},
	{"charge-to", 1008, TRUE},
	{"editps-level", 1009, TRUE},
	{"markup", 1010, TRUE},
	{"page-list", 1012, TRUE},
	{"cache-store", 1013, TRUE},
	{"cache-priority", 1014, TRUE},
	{"strip-cache", 'S', TRUE},
	{"strip-fontindex", 1015, TRUE},
	{"strip-printer", 1016, TRUE},
	{"save", 1017, FALSE},
	{"question", 1018, TRUE},
	{"commentary", 1100, TRUE},

	/* These are also documented and final. */
	{"help", 9000, FALSE},
	{"version", 9001, FALSE},
	{(char*)NULL, 0, FALSE}
	} ;

/*
** Print the help screen to the indicated file (stdout or stderr).
*/
static void help(FILE *outfile)
    {
#define HELP(a) fputs(a, outfile);
HELP(_(
"usage: ppr [switches] [filename]\n\n"
"\t-d <destname>              selects desired printer or group\n"
"\t-I                         insert destination's switchset macro here\n"));

HELP((
"\t-u yes                     use username to identify jobs in queue\n"
"\t-u no                      use /etc/passwd comment instead (default)\n"
"\t-f <string>                override user identication from -u\n"));

HELP(_(
"\t-X <string>                used by network servers to tell PPR the\n"
"\t                           identity of alien users for whom they act\n"));

HELP(_(
"\t--charge-to <string>       PPR charge account to bill for job\n"));

HELP(_(
"\t--title <string>           set default document title\n"));

HELP(_(
"\t-O <string>                overlay `Draft' notice\n"));

HELP(_(
"\t--routing <string>         set DSC routing instructions\n"));

HELP(_(
"\t-m <method>                response method\n"
"\t-m none                    no response\n"
"\t-r <address>               response address\n"
"\t--responder-options <list> list of name=value responder options\n"));

HELP(_(
"\t--commentary <n>           send commentary messages of types <n>\n"));

HELP(_(
"\t-b {yes,no,dontcare}       express banner page preference\n"
"\t-t {yes,no,dontcare}       express trailer page preference\n"));

HELP(_(
"\t-w log                     routes warnings to log file\n"
"\t-w stderr                  routes warnings to stderr (default)\n"
"\t-w both                    routes warnings to stderr and log file\n"
"\t-w {severe,peeve,none}     sets warning level\n"));

HELP(_(
"\t-a                         turns on authcode mode\n"));

HELP(_(
"\t-A                         root or ppr has preauthorized\n"));

HELP(_(
"\t--strip-cache true         strip cached resources from job file\n"
"\t--strip-cache false        don't strip cached resources (default)\n"));

HELP(_(
"\t--strip-fontindex true     strip fonts in font index from job file\n"
"\t--strip-fontindex false    don't strip fonts in font index (defualt)\n"));

HELP(_(
"\t--strip-printer true       strip out resources already in printer\n"
"\t--strip-printer false      don't strip printer resources (default)\n"));

HELP(_(
"\t--cache-store none         don't store new resources (default)\n"
"\t--cache-store unavailable  store if not in cache or fontindex\n"
"\t--cache-store uncached     store if not already in cache\n"));

HELP(_(
"\t--cache-priority {auto,low,high}\n"
"\t                           prefer to insert resources from cache?\n"));

HELP(_(
"\t-F '<feature name>'        inserts setup code for a printer feature\n"
"\t--feature '<feature name>' same as above\n"
"\t--features                 list available printer features\n"));

HELP(_(
"\t-K true                    keep feature code though not in PPD file\n"
"\t-K false                   don't keep bad feature code (default)\n"));

HELP(_(
"\t-D <mediumname>            sets default medium\n"));

HELP(_(
"\t-B false                   disable automatic bin selection\n"
"\t-B true                    enable automatic bin selection (default)\n"));

minus_tee_help(outfile);	/* drag in -T stuff */

HELP(_(
"\t-o <string>                specify filter options\n"
"\t--markup [format, lp, pr, fallback-lp, fallback-pr]\n"
"\t                           set handling of LaTeX, HTML, and such\n"));

HELP(_(
"\t-N <positive integer>      print pages N-Up\n"
"\t-N noborders               turn off N-Up borders\n"));

HELP(_(
"\t-n <positive integer>      print n copies\n"
"\t-n collate                 print collated copies\n"));

HELP(_(
"\t-R for                     read \"%%For:\" line\n"
"\t-R ignore-for              don't read \"%%For:\" (default)\n"));

HELP(_(
"\t-R title                   use title from \"%%Title:\" (default)\n"
"\t-R ignore-title            ignore \"%%Title:\" comment\n"));

HELP(_(
"\t-R copies                  read copy count from document\n"
"\t-R ignore-copies           don't read copy count (default)\n"));

HELP(_(
"\t-R duplex:softsimplex      read duplex mode, assume simplex (default)\n"
"\t-R duplex:simplex          read duplex mode, assume simplex\n"
"\t-R duplex:duplex           read duplex mode, assume normal duplex\n"
"\t-R duplex:duplextumble     read duplex mode, assume tumble duplex\n"
"\t-R ignore-duplex           don't read duplex mode\n"));

HELP(_(
"\t-R routing                 use routing from \"%%Routing:\" (default)\n"
"\t-R ignore-routing          ignore \"%%Routing:\" comment\n"));

HELP(_(
"\t-Z true                    ignore jobs without %%EOF\n"
"\t-Z false                   don't ignore (default)\n"));

HELP(_(
"\t-P notifyme                set ProofMode to NotifyMe\n"
"\t-P substitute              set ProofMode to Substitute\n"
"\t-P trustme                 set ProofMode to TrustMe\n"));

HELP(_(
"\t-e none                    don't user stderr or responder for errors\n"
"\t-e stderr                  report errors on stderr (default)\n"
"\t-e responder               report errors by responder\n"
"\t-e both                    report errors with both\n"
"\t-e hexdump                 always use hexdump for no filter\n"
"\t-e no-hexdump              discourage hexdump for no filter\n"));

HELP(_(
"\t-s <positive integer>      signature sheet count\n"
"\t-s booklet                 automatic signature sheet count\n"
"\t-s {both,fronts,backs}     indicate part of each signature to print\n"));

HELP(_(
"\t-Y                         experimental job splitting feature\n"));

HELP(_(
"\t--page-list <page-list>    print only the pages indicated\n"));

HELP(_(
"\t-G <string>                gab about subject indicated by <string>\n"
"\t--gab <string>             same as -G\n"));

HELP(_(
"\t--hold                     job should be held immediately\n"));

HELP(_(
"\t--question <path>          path of HTML question page\n"));

HELP(_(
"\t--save                     job should be saved after printing\n"));

HELP(_(
"\t-q <integer>               sets priority of print job\n"));

HELP(_(
"\t-U                         unlink job file after queuing it\n"));

HELP(_(
"\t-Q <string>                TrueType query answer given (for papsrv)\n"));

HELP(_(
"\t--lpq-filename <string>    filename for ppop lpq listings\n"));

HELP(_(
"\t-H <hack name>             turn on a hack\n"
"\t-H no-<hack name>          turn off a hack\n"
"\t--hack [no-]<hack name>    same as -H\n"));

HELP(_(
"\t--editps-level <pos int>   set level of editing for -H editps\n"));

HELP(_(
"\t--show-jobid               print the queue id of submitted jobs\n"
"\t--show-long-jobid          print queue id in long format\n"));

HELP(_(
"\t--version                  print PPR version information\n"
"\t--help                     print this help\n"));
    } /* end of help() */

/*
** Convert a flag page option string to a code number.
*/
static int flag_option(const char *optstr)
    {
    if(strcmp(optstr, "yes") == 0)
	return BANNER_YESPLEASE;
    else if(strcmp(optstr, "no") == 0)
	return BANNER_NOTHANKYOU;
    else if(strcmp(optstr, "dontcare") == 0)
	return BANNER_DONTCARE;
    else                                        /* no match */
	return -1;                              /* is an error */
    } /* end of flag_option() */

/*
** Add a PPD file feature to the list of those that should be added
** to the document setup section of the job.
**
** This is called from parse_feature_option() which is called during command
** line parsing for each -F switch.  It is also called towards the end of
** job processing as part of the handling of the -R duplex:* option.
*/
static void mark_feature_for_insertion(const char name[])
    {
    if(features_count >= MAX_FEATURES)
	fatal(PPREXIT_SYNTAX, _("Too many -F switches"));

    /* Add it to the list. */
    features[features_count++] = name;
    } /* end of mark_feature_for_insertion() */

/*
** This function is called for the -F (--feature) option and for ".feature:"
** lines in ".ppr" headers.
**
** There are two acceptable formats for such an option:
**	-F "*Duplex DuplexNoTumble"
**	-F Duplex=DuplexNoTumble
**
** This function returns 0 if the option is parsable, -1 if it isn't.
*/
int parse_feature_option(const char option_text[])
    {
    const char *name = option_text;
    char *name_storage = NULL;

    /* Original format:
       -F '*Feature'
       -F '*Feature Option'
       */
    if(name[0] == '*')
	{
	/* syntax error if more than 1 space */
	const char *p;
	if((p = strchr(name, ' ')) && strchr(p+1, ' '))
	    return -1;
	}

    /* Code from Steve Hsieh to accept Adobe pslpr format:
       -F Feature=Option
       The code converts this format to the origional -F format. */
    if(name[0] != '*')
	{
	char *p;

	/* syntax error if no equals or any spaces */
	if(!strchr(name, '=') || strchr(name, ' '))
	    return -1;

	p = name_storage = (char*)gu_alloc(strlen(name) + 2, sizeof(char));
	*p++ = '*';
	strcpy(p, name);
	*strchr(p, '=') = ' ';
	name = name_storage;
	}

    /* Duplex gets special handling. */
    if(lmatch(name, "*Duplex "))
	{
	const char *option = &name[8];

	/* Any %%BeginFeature: *Duplex or %%IncludeFeature: *Duplex won't
	   override what has been selected.
	   */
	read_duplex = FALSE;

	/* After reading the whole document, the value in current_duplex
	   will be used to select the proper argument for a call
	   to mark_feature_for_insertion().
	   */
	current_duplex_enforce = TRUE;

	/* Simplex */
	if(strcmp(option, "None") == 0)
	    current_duplex = DUPLEX_NONE;
	else if(strcmp(option, "SimplexTumble") == 0)
	    current_duplex = DUPLEX_SIMPLEX_TUMBLE;
	else if(strcmp(option, "DuplexTumble") == 0)
	    current_duplex = DUPLEX_DUPLEX_TUMBLE;
	else
	    current_duplex = DUPLEX_DUPLEX_NOTUMBLE;

	if(name_storage)
	    gu_free(name_storage);
	}

    /* Others get entered into the table right away.  Remember that
       this function keeps a pointer to the array we pass to it,
       so don't free it! */
    else
	{
	mark_feature_for_insertion(name);
	}

    return 0;
    } /* end of parse_feature_option */

/*
** This routine is called from main() if it finds a switchset
** in the printer or group configuration file.
** This routine calls doopt() once for each switch in the line.
*/
static void doopt_pass2(int optchar, const char *optarg, const char *option);
static void parse_switchset(const char *switchset)
    {
    int x;			/* line index */
    int optchar;		/* switch character we are processing */
    char *argument;		/* character's argument */
    char *ptr;			/* pointer into option_description_string[] */

    char *line = gu_strdup(switchset);	/* we copy becuase we insert NULLs */
    int stop = strlen(line);

    for(x=0; x < stop; x++)	/* move thru the line */
    	{
	optchar = line[x++];

	argument = &line[x];				/* The stuff after the switch */
	argument[strcspn(argument, "|")] = '\0';	/* NULL terminate */

	if(optchar == '-')				/* if it is a long option */
	    {
	    int y;
	    size_t len;
	    char *value = strchr(argument, '=');

	    if(value)
	    	{
	    	len = value - argument;
	    	value++;
	    	}
	    else
	    	{
	    	len = strlen(argument);
	    	}

	    for(y = 0; option_words[y].name != (char*)NULL; y++)
	    	{
		if(strlen(option_words[y].name) == len && strncmp(option_words[y].name, argument, len) == 0)
		    {
		    if(value == (char*)NULL && option_words[y].needsarg)
			fatal(PPREXIT_SYNTAX, "Missing argument to --%s option in -I switch insertion", argument);

		    if(value != (char*)NULL && ! option_words[y].needsarg)
			fatal(PPREXIT_SYNTAX, "Unneeded argument to --%.*s option in -I switch insertion", (int)len, argument);

		    doopt_pass2(option_words[y].code, value, option_words[y].name);
		    break;
		    }
	    	}

	    if(option_words[y].name == (char*)NULL)
	    	fatal(PPREXIT_SYNTAX, _("Unknown option --%.*s inserted by -I switch"), (int)len, argument);
	    }
	else
	    {
	    if((ptr=strchr(option_description_string, optchar)) == (char*)NULL)
		fatal(PPREXIT_SYNTAX, _("Unknown option -%c inserted by -I switch"), optchar);

	    if(argument[0] == '\0' && ptr[1] == ':')
		fatal(PPREXIT_SYNTAX, _("Option -%c inserted by -I switch is missing an argument"), optchar);

	    if(argument[0] != '\0' && ptr[1] != ':')
		fatal(PPREXIT_SYNTAX, _("Option -%c inserted by -I switch has an unneeded argument"), optchar);

	    doopt_pass2(optchar, argument, "");	/* actually process it */
	    }

    	x += strlen(argument);		/* move past this one */
    	}				/* (x++ in for() will move past the NULL) */

    /* Don't gu_free(line)! */
    } /* parse_switchset */

/*
** Parse a destination queue specification.  This gets called if the
** -d switch is used.  It is also called in the absence of a -d switch
** to process the queue name taken from an environment variable
** or the default queue name.  So, it is always called at least once.
*/
static void parse_d_option(const char arg[])
    {
    int len;

    if(qentry.destnode) gu_free((char*)qentry.destnode);

    len = strcspn(arg, ":");		/* length before first ':' */

    if(arg[len] == ':')			/* if there is a node name specified, */
	{
	qentry.destname = &arg[len+1];
	qentry.destnode = gu_strndup(arg,len);
	}
    else				/* If no node specified, */
	{				/* the whole argument is */
	qentry.destname = arg;		/* the destination and there */
	qentry.destnode = (char*)NULL;	/* is not destination node. */
	}
    } /* end of parse_d_option() */

/*
** Parse an -H option or .ppr header ".hack:" option.
*/
int parse_hack_option(const char name[])
    {
    const char *ptr = name;
    int no = FALSE;
    int x;

    if(gu_strncasecmp(ptr, "no-", 3) == 0)
	{
	no = TRUE;
	ptr += 3;
	}

    for(x=0; hack_table[x].name; x++)
	{
	if(gu_strcasecmp(ptr, hack_table[x].name) == 0)
	    {
	    if( ! no )
		qentry.opts.hacks |= hack_table[x].bit;
	    else
		qentry.opts.hacks &= ~hack_table[x].bit;
	    return 0;
	    }
	}

    return -1;
    } /* end of parse_hack_option */

/*
** These are the action routines which are called to process the
** options which ppr_getopt() isolates.  There are 2 passes made
** over the option list.  The purpose of the first pass is to find
** the -d switch so that the correct switchset may be processed
** before pass 2 begins.
**
** We feed these routines the option character, such as 'd', and the
** option string such as "chipmunk".
**
** For long options, these routines are fed the short option character
** which cooresponds to the long option.  If a long option does not
** have a short form, then optchar is a code number.  These code
** numbers should all be greater than 255.  We have chosen to make
** them 1000 and greater.
*/
static void doopt_pass1(int optchar, const char *optarg, const char *true_option)
    {
    switch(optchar)
	{
	case 'd':				/* destination queue */
	    parse_d_option(optarg);
	    break;
	}
    } /* end of doopt_pass1() */

static void doopt_pass2(int optchar, const char *optarg, const char *true_option)
    {
    switch(optchar)
	{
	case 'd':				/* destination queue */
	    /* parse_d_option(optarg); */	/* do nothing since handled on first pass */
	    break;

	case 'f':				/* for whom */
	    assert_ok_value(optarg, FALSE, FALSE, FALSE, true_option, TRUE);
	    qentry.For = optarg;
	    break;

	case 'u':				/* Use username in stead of comment */
	    if((use_username=gu_torf(optarg)) == ANSWER_UNKNOWN)
	    	fatal(PPREXIT_SYNTAX, _("The %s option must be followed by \"yes\" or \"no\""), true_option);
	    break;

	case 'm':				/* responder */
	    qentry.responder = optarg;
	    break;

	case 'r':				/* responder address */
	    qentry.responder_address = optarg;
	    break;

	case 'b':				/* banner */
	    if( (qentry.do_banner=flag_option(optarg)) == -1 )
		fatal(PPREXIT_SYNTAX, _("Invalid %s option"), true_option);
	    break;

	case 't':				/* trailer */
	    if( (qentry.do_trailer=flag_option(optarg)) == -1 )
		fatal(PPREXIT_SYNTAX, _("Invalid %s option"), true_option);
	    break;

	case 'w':				/* set warning option */
	    if( strcmp(optarg, "log") == 0)
		warning_log = TRUE;
	    else if( gu_strcasecmp(optarg, "stderr") == 0)
	    	warning_log = FALSE;
	    else if(strcmp(optarg, "severe") == 0)
		warning_level = WARNING_SEVERE;
	    else if(strcmp(optarg, "peeve") == 0)
		warning_level = WARNING_PEEVE;
	    else if(strcmp(optarg, "none") == 0)
		warning_level = WARNING_NONE;
	    else
		fatal(PPREXIT_SYNTAX, _("invalid %s option"), true_option);
	    break;

	case 'a':				/* authcode mode */
	    use_authcode = TRUE;
	    read_For = TRUE;
	    break;

	case 'D':				/* set default media */
	    default_medium = optarg;
	    break;

	case 'F':				/* add a feature */
	    if(parse_feature_option(optarg) == -1)
	    	fatal(PPREXIT_SYNTAX, _("Invalid %s argument format"), true_option);
	    break;

	case 'T':				/* force input type */
	    if(infile_force_type(optarg))
		fatal(PPREXIT_SYNTAX, _("%s option specifies unrecognized file type \"%s\""), true_option, optarg);
	    break;

	case 'S':				/* -S, --strip-cache */
	    if(gu_torf_setBOOL(&option_strip_cache, optarg) == -1)
	    	fatal(PPREXIT_SYNTAX, _("%s must be followed by \"true\" or \"false\""), true_option);
	    break;

	case 'q':				/* queue priority */
	    if((qentry.priority = atoi(optarg)) < 0 || qentry.priority > 39)
		fatal(PPREXIT_SYNTAX, _("%s option must be between 0 and 39"), true_option);
	    break;

	case 'B':				/* disable or enable automatic bin selects */
	    if(gu_torf_setBOOL(&qentry.opts.binselect, optarg) == -1)
	    	fatal(PPREXIT_SYNTAX, _("%s must be followed by \"true\" or \"false\""), true_option);
	    break;

	case 'N':				/* N-Up */
	    if(strcmp(optarg, "noborders") == 0)	/* noborders option, */
	    	{				/* turn */
	    	qentry.N_Up.borders = FALSE;	/* borders off */
	    	}
	    else
		{
	    	qentry.N_Up.N = atoi(optarg);
	    	if( (qentry.N_Up.N < 1) /* || (qentry.N_Up.N > 16) */ )
		    fatal(PPREXIT_SYNTAX, _("%s option (N-Up) must be between 1 and 16"), true_option);
		}
	    break;

	case 'n':				/* number of copies */
	    if(strcmp(optarg, "collate") == 0)	/* (or ``collate'') */
	    	{
		qentry.opts.collate = TRUE;
		}
	    else
	    	{
		if((qentry.opts.copies = atoi(optarg)) < 1)
		    fatal(PPREXIT_SYNTAX, _("Number of copies must be positive"));
		read_copies = FALSE;
		}
	    break;

	case 'A':				/* a preauthorized job */
	    if( ! privledged() )
		fatal(PPREXIT_SYNTAX, _("Only privledged users may use the %s switch"), true_option);
	    else
		preauthorized = TRUE;
	    break;

	case 'C':				/* default title */
	    assert_ok_value(optarg, FALSE, TRUE, FALSE, true_option, TRUE);
	    qentry.Title = optarg;
	    break;

	case 'i':				/* routing instructions */
	    assert_ok_value(optarg, FALSE, FALSE, FALSE, true_option, TRUE);
	    qentry.Routing = optarg;
	    break;

	case 'R':
	    if(strcmp(optarg, "for") == 0)
	    	{
		if(!privledged())
		    fatal(PPREXIT_SYNTAX, _("Only privledged users may use \"%s for\""), true_option);
	    	read_For = TRUE;
	    	}
	    else if(strcmp(optarg, "ignore-for") == 0)
	    	{
	    	read_For = FALSE;
	    	}
	    else if(strcmp(optarg, "title") == 0)
	    	{
		read_Title = TRUE;
	    	}
	    else if(strcmp(optarg, "ignore-title") == 0)
	    	{
	    	read_Title = FALSE;
	    	}
	    else if(strcmp(optarg, "copies") == 0)
		{
		read_copies = TRUE;
		}
	    else if(strcmp(optarg, "ignore-copies") == 0)
	    	{
	    	read_copies = FALSE;
	    	}
	    else if(strcmp(optarg, "duplex:duplex") == 0)
		{
		read_duplex=TRUE;
		current_duplex_enforce = TRUE;
		current_duplex = DUPLEX_DUPLEX_NOTUMBLE;
		}
	    else if(strcmp(optarg, "duplex:simplex") == 0)
	    	{
	    	read_duplex = TRUE;
	    	current_duplex_enforce = TRUE;
	    	current_duplex = DUPLEX_NONE;
	    	}
	    else if(strcmp(optarg, "duplex:duplextumble") == 0)
	        {
	        read_duplex = TRUE;
	        current_duplex_enforce = TRUE;
	        current_duplex = DUPLEX_DUPLEX_TUMBLE;
	        }
	    else if(strcmp(optarg, "duplex:softsimplex") == 0)
	    	{
	    	read_duplex = TRUE;
	    	current_duplex_enforce = FALSE;
	    	current_duplex = DUPLEX_NONE;
	    	}
	    else if(strcmp(optarg, "ignore-duplex") == 0)
	    	{
		read_duplex = FALSE;
		current_duplex_enforce = FALSE;
		/* An old note says we shouldn't do this below, but I am not sure
		   if that is still valid now that the handling of -F *Duplex has
		   been changed to stuff its value into current_duplex.
		   */
		/* current_duplex = DUPLEX_NONE; */
		}
	    else if(strcmp(optarg, "routing") == 0)
	    	{
	    	read_Routing = TRUE;
	    	}
	    else if(strcmp(optarg, "ignore-routing") == 0)
	    	{
	    	read_Routing = FALSE;
	    	}
	    else
		{
		fatal(PPREXIT_SYNTAX, _("Unrecognized %s option: %s"), true_option, optarg);
		}
	    break;

	case 'Z':
	    if((ignore_truncated = gu_torf(optarg)) == ANSWER_UNKNOWN)
	    	fatal(PPREXIT_SYNTAX, _("%s must be followed by \"true\" or \"false\""), true_option);
	    break;

	case 'O':			/* overlay `Draft' notice */
	    assert_ok_value(optarg, FALSE, FALSE, FALSE, true_option, TRUE);
	    qentry.draft_notice = optarg;
	    break;

	case 'P':
	    if(gu_strcasecmp(optarg, "NotifyMe") == 0)
	    	qentry.attr.proofmode = PROOFMODE_NOTIFYME;
	    else if(gu_strcasecmp(optarg, "Substitute") == 0)
	    	qentry.attr.proofmode = PROOFMODE_SUBSTITUTE;
	    else if(gu_strcasecmp(optarg, "TrustMe") == 0)
	    	qentry.attr.proofmode = PROOFMODE_TRUSTME;
	    else
	    	fatal(PPREXIT_SYNTAX, _("%s must be followed by \"notifyme\", \"substitute\", or \"trustme\""), true_option);
	    read_ProofMode = FALSE;
	    break;

	case 'K':
	    if(gu_torf_setBOOL(&qentry.opts.keep_badfeatures, optarg) == -1)
	    	fatal(PPREXIT_SYNTAX, _("%s must be set to \"true\" or \"false\""), true_option);
	    break;

	case 'e':
	    if(strcmp(optarg, "none") == 0)
	    	ppr_respond_by = PPR_RESPOND_BY_NONE;
	    else if(strcmp(optarg, "stderr") == 0)
	    	ppr_respond_by = PPR_RESPOND_BY_STDERR;
	    else if(strcmp(optarg, "responder") == 0)
	    	ppr_respond_by = PPR_RESPOND_BY_RESPONDER;
	    else if(strcmp(optarg,"both") == 0)
	    	ppr_respond_by = PPR_RESPOND_BY_BOTH;
	    else if(strcmp(optarg,"hexdump") == 0)
	    	option_nofilter_hexdump = TRUE;
	    else if(strcmp(optarg, "dishexdump") == 0 || strcmp(optarg, "no-hexdump") == 0)
	    	option_nofilter_hexdump = FALSE;
	    else
		fatal(PPREXIT_SYNTAX, _("%s must be followed by \"none\", \"stderr\", \"responder\", or \"both\""), true_option);
	    break;

	case 's':					/* signature option */
	    if(strcmp(optarg, "fronts") == 0)
	    	qentry.N_Up.sigpart = SIG_FRONTS;
	    else if(strcmp(optarg, "backs") == 0)
	    	qentry.N_Up.sigpart = SIG_BACKS;
	    else if(strcmp(optarg, "both") == 0)
	    	qentry.N_Up.sigpart = SIG_BOTH;
	    else if(strcmp(optarg, "booklet") == 0)
		qentry.N_Up.sigsheets = (-1);
	    else if((qentry.N_Up.sigsheets = atoi(optarg)) >= 0)
		{ /* no action */ }
	    else
	    	fatal(PPREXIT_SYNTAX, _("%s option must be \"fronts\", \"backs\", \"both\", \"booklet\", or a positive integer"), true_option);
	    break;

	case 'I':					/* insert switch set */
	    /* I_switch(); */
	    fprintf(stderr, "%s: -I switch is obsolete\n", myname);
	    break;

	case 'o':					/* Filter options */
	    {
	    const char *ptr;				/* eat leading space */
	    ptr = &optarg[strspn(optarg, " \t")];	/* (It may cause trouble for exec_filter()) */

	    if(!option_filter_options)			/* If this is the first -o switch, */
	    	{					/* Just make a copy of it. */
	    	option_filter_options = gu_strdup(ptr);	/* (True, it is not necessary to make a copy, */
	    	}					/* but it makes the else clause simpler. */
	    else
	    	{
		int len = strlen(option_filter_options) + 1 + strlen(ptr) + 1;
		char *ptr2 = (char*)gu_alloc(len, sizeof(char));
		snprintf(ptr2, len, "%s %s", option_filter_options, ptr);	/* concatenate */
		gu_free(option_filter_options);				/* free old one */
		option_filter_options = ptr2;				/* make new one current */
	    	}
	    }
	    break;

	case 'U':				/* unlink the input file */
	    option_unlink_jobfile = TRUE;
	    break;

	case 'Y':				/* Split the job */
	    Y_switch(optarg);
	    break;

	case 'X':				/* Identify principal */
	    qentry.proxy_for = optarg;
	    break;

	case 'G':				/* What to gab about */
	    {
	    const char *ptr = optarg;
	    int no = FALSE;
	    int x;

	    if(gu_strncasecmp(ptr, "no-", 3) == 0)
	    	{
	    	no = TRUE;
	    	ptr += 3;
	    	}

	    for(x=0; gab_table[x].name; x++)
	    	{
		if(gu_strcasecmp(ptr, gab_table[x].name) == 0)
		    {
		    if(! no)
			option_gab_mask |= gab_table[x].bit;
		    else
		    	option_gab_mask &= ~gab_table[x].bit;
		    break;
		    }
	    	}

	    if(gab_table[x].name == (char*)NULL)
		fatal(PPREXIT_SYNTAX, _("Unrecognized %s option: %s"), true_option, optarg);
	    }
	    break;

	case 'Q':				/* TrueType query answer (for papsrv) */
	    if(strcmp(optarg, "None") == 0)
	    	option_TrueTypeQuery = TT_NONE;
	    else if(strcmp(optarg, "Accept68K") == 0)
	    	option_TrueTypeQuery = TT_ACCEPT68K;
	    else if(strcmp(optarg, "Type42") == 0)
	    	option_TrueTypeQuery = TT_TYPE42;
	    else
	    	fatal(PPREXIT_SYNTAX, _("Unrecognized %s option: \"%s\""), true_option, optarg);
	    break;

	case 'H':				/* turn on a hack */
	    if(parse_hack_option(optarg) == -1)
		fatal(PPREXIT_SYNTAX, _("Unrecognized %s option: %s"), true_option, optarg);
	    break;

	case 1000:				/* --features */
	    exit(option_features(qentry.destnode, qentry.destname));

	case 1003:				/* --lpq-filename */
	    qentry.lpqFileName = optarg;
	    break;

	case 1004:				/* --hold */
	    qentry.status = STATUS_HELD;
	    break;

	case 1005:				/* --responder-options */
	    {
	    if(!qentry.responder_options)
		{
		qentry.responder_options = gu_strdup(optarg);
		}		/* duplicate because we might free */
	    else
	    	{
		int len = strlen(qentry.responder_options) + 1 + strlen(optarg) + 1;
		char *ptr = (char*)gu_alloc(len, sizeof(char));
		snprintf(ptr, len, "%s %s", qentry.responder_options, optarg);
		gu_free((char*)qentry.responder_options);	/* !!! */
		qentry.responder_options = ptr;
	    	}
	    }
	    break;

    	case 1006:  	    	    	    	/* --show-jobid */
    	    option_show_jobid = 1;
    	    break;

	case 1007:				/* --show-long-jobid */
	    option_show_jobid = 2;
	    break;

	case 1008:				/* --charge-to */
	    if(! privledged() && ! use_authcode)
	    	fatal(PPREXIT_SYNTAX, _("Non-privledged users may not use %s w/out -a"), true_option);
	    /* This is not copied into the queue structure unless it is needed. */
	    charge_to_switch_value = optarg;
	    break;

	case 1009:				/* --editps-level */
	    if((option_editps_level = atoi(optarg)) < 1 || option_editps_level > 10)
	    	fatal(PPREXIT_SYNTAX, _("Argument for %s must be between 1 and 10"), true_option);
	    break;

	case 1010:				/* --markup */
	    if(strcmp(optarg, "format") == 0)
	    	option_markup = MARKUP_FORMAT;
	    else if(strcmp(optarg, "lp") == 0)
	    	option_markup = MARKUP_LP;
	    else if(strcmp(optarg, "pr") == 0)
	    	option_markup = MARKUP_PR;
	    else if(strcmp(optarg, "fallback-lp") == 0)
	    	option_markup = MARKUP_FALLBACK_LP;
	    else if(strcmp(optarg, "fallback-pr") == 0)
	    	option_markup = MARKUP_FALLBACK_PR;
	    else
	    	fatal(PPREXIT_SYNTAX, _("Invalid setting for %s"), true_option);
	    break;

	case 1012:				/* --page-list */
	    option_page_list = optarg;
	    break;

	case 1013:				/* --cache-store */
	    if(strcmp(optarg, "none") == 0)
		option_cache_store = CACHE_STORE_NONE;
	    else if(strcmp(optarg, "unavailable") == 0)
		option_cache_store = CACHE_STORE_UNAVAILABLE;
	    else if(strcmp(optarg, "uncached") == 0)
		option_cache_store = CACHE_STORE_UNCACHED;
	    else
		fatal(PPREXIT_SYNTAX, _("Unrecognized value for %s option: %s"), true_option, optarg);
	    break;

	case 1014:				/* --cache-priority */
	    if(strcmp(optarg, "auto") == 0)
	 	qentry.CachePriority = CACHE_PRIORITY_AUTO;
	    else if(strcmp(optarg, "low") == 0)
	 	qentry.CachePriority = CACHE_PRIORITY_LOW;
	    else if(strcmp(optarg, "high") == 0)
	 	qentry.CachePriority = CACHE_PRIORITY_HIGH;
	    else
	    	fatal(PPREXIT_SYNTAX, _("Unrecognized value for %s option: %s"), true_option, optarg);
	    break;

	case 1015:				/* --strip-fontindex */
	    if(gu_torf_setBOOL(&option_strip_fontindex, optarg) == -1)
	    	fatal(PPREXIT_SYNTAX, _("%s must be followed by \"true\" or \"false\""), true_option);
	    break;

	case 1016:				/* --strip-printer */
	    if(gu_torf_setBOOL(&qentry.StripPrinter, optarg) == -1)
	    	fatal(PPREXIT_SYNTAX, _("%s must be followed by \"true\" or \"false\""), true_option);
	    break;

	case 1017:				/* --save */
	    qentry.flags |= JOB_FLAG_SAVE;
	    break;

	case 1018:				/* --question */
	    qentry.question = optarg;
	    qentry.flags |= JOB_FLAG_QUESTION_UNANSWERED;
	    break;

	case 1100:				/* --commentary */
	    qentry.commentary = atoi(optarg);
	    break;

	case 9000:				/* --help */
	    help(stdout);
	    exit(PPREXIT_OK);	/* not ppr_abort() */

	case 9001:				/* --version */
	    puts(VERSION);
	    puts(COPYRIGHT);
	    puts(AUTHOR);
	    exit(PPREXIT_OK);	/* not ppr_abort() */

	case '?':				/* Unrecognized switch */
	    fatal(PPREXIT_SYNTAX, _("Unrecognized switch %s.  Try --help"), true_option);

	case ':':				/* argument required */
	    fatal(PPREXIT_SYNTAX, _("The %s option requires an argument"), true_option);

	case '!':				/* bad aggreation */
	    fatal(PPREXIT_SYNTAX, _("The %s switch takes an argument and so must stand alone"), true_option);

	case '-':				/* spurious argument */
	    fatal(PPREXIT_SYNTAX, _("The %s switch does not take an argument"), true_option);

	default:				/* Missing case */
	    fatal(PPREXIT_OTHERERR, "Missing case %d in switch dispatch switch()", optchar);
	} /* end of switch */
    } /* end of doopt_pass2() */

/*
** Main procedure.
**
** Among other things, we set default values in the queue file structure,
** parse the command line parameters, order the input file opened, order
** verious routines to read it until they get to the ends of various
** sections, order our authority to print to be verified, order a slew
** of checks for various inconsistencies, and order the file submitted
** to the spooler.
*/
int main(int argc, char *argv[])
    {
    const char *function = "main";
    char *real_filename = (char*)NULL;		/* the file we read from if not stdin */
    struct gu_getopt_state getopt_state;	/* defined here because used to find filename */
    struct passwd *pw;				/* to get information on invoking user */
    int x;					/* various very short term uses */
    char *ptr;					/* general use */

    /* Initialize internation messages library. */
    #ifdef INTERNATIONAL
    setlocale(LC_MESSAGES, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);
    #endif

    /* Set umask to prevent user cripling us. */
    umask(PPR_JOBS_UMASK);

    /* Set environment values such as PATH, IFS, and HOME
       to useful and correct values. */
    set_ppr_env();

    /*
    ** Deduce the non-setuid user id and user name.
    ** In other words, we want to know who is running it.
    */
    user_uid = getuid();
    user_gid = getgid();
    if((pw = getpwuid(user_uid)) == (struct passwd *)NULL)
    	fatal(PPREXIT_OTHERERR, _("getpwuid() fails to find your account"));
    if(pw->pw_name == (char*)NULL)
    	fatal(PPREXIT_OTHERERR, "strange getpwuid() error, pw_name is NULL");

    /*
    ** Save the uid that is effective now.
    ** (This will be ``ppr''.)
    */
    setuid_uid = geteuid();
    setgid_gid = getegid();

    /*
    ** If the real and effective UIDs are the same and they do
    ** not correspond to the user "ppr", then we are not running
    ** setuid.  We also make sure we are not being run by root
    ** while setuid root or not setuid at all.
    **
    ** In other words, we only check the user name of our
    ** effective uid if we have reason to be suspicious.
    */
#ifndef BROKEN_SETUID_BIT
    if((setuid_uid == user_uid && strcmp(pw->pw_name, USER_PPR)) || setuid_uid == 0)
	{
	fprintf(stderr, "uid=%u(%s) euid=%u(%s)\n", (unsigned)user_uid, pw->pw_name, (unsigned)setuid_uid, pw->pw_name);
	fatal(PPREXIT_OTHERERR, _("This program must be setuid and owned by \"%s\""), USER_PPR);
	}
#endif

    /*
    ** Remember what directory we started in.  Some of the input filters
    ** need to know this.  This operation is complicated because of
    ** the strange design of the POSIX getcwd() function.  It can't even tell
    ** us how large a buffer it needs!
    */
    become_user();
    {
    int len=64;
    starting_directory = (char*)gu_alloc(len, sizeof(char));
    while(!getcwd(starting_directory, len))
	{
	if(errno != ERANGE)
	    fatal(PPREXIT_OTHERERR, "getcwd() failed, errno=%d (%s)", errno, gu_strerror(errno));
	len *= 2;
	starting_directory = (char*)gu_realloc(starting_directory, len, sizeof(char));
	}
    starting_directory = (char*)gu_realloc(starting_directory, strlen(starting_directory) + 1, sizeof(char));
    }
    unbecome_user();

    /*
    ** Clear parts of the queue entry, fill in default values
    ** elsewhere.  It is important that we do this right away
    ** since one of the things we do is get our queue id.
    */
    qentry.destnode = (char*)NULL;			/* name of node to send job to */
    qentry.destname = (char*)NULL;			/* name of printer or group */
    qentry.id = 0;					/* not assigned yet */
    qentry.subid = 0;					/* job fragment number (unused) */
    qentry.homenode = ppr_get_nodename();		/* this are the node this job came from */
    qentry.status = STATUS_WAITING;
    qentry.flags = 0;
    qentry.time = time((time_t*)NULL);			/* job submission time */
    qentry.priority = 20;				/* default priority */
    qentry.user = user_uid;				/* record real user id of submitter */
    qentry.username = pw->pw_name;			/* fill in name associated with id */
    qentry.For = (char*)NULL;				/* start with no %%For: line */
    qentry.charge_to = (char*)NULL;			/* ppuser account to charge to */
    qentry.Title = (char*)NULL;				/* start with no %%Title: line */
    qentry.Creator = (char*)NULL;			/* "%%Creator:" */
    qentry.Routing = (char*)NULL;			/* "%%Routing:" */
    qentry.lpqFileName = (char*)NULL;			/* filename for lpq */
    qentry.nmedia = 0;					/* no forms */
    qentry.do_banner = BANNER_DONTCARE;			/* don't care */
    qentry.do_trailer = BANNER_DONTCARE;		/* don't care */
    qentry.attr.langlevel = 1;				/* default to PostScript level 1 */
    qentry.attr.pages = -1;				/* number of pages undetermined */
    qentry.attr.pageorder = PAGEORDER_ASCEND;		/* assume ascending order */
    qentry.attr.extensions = 0;				/* no Level 2 extensions */
    qentry.attr.orientation = ORIENTATION_UNKNOWN;	/* probably not landscape */
    qentry.attr.proofmode = PROOFMODE_SUBSTITUTE; 	/* don't change this default (RBII p. 664-665) */
    qentry.attr.input_bytes = 0;			/* size before filtering if filtered */
    qentry.attr.postscript_bytes = 0;			/* size of PostScript */
    qentry.attr.parts = 1;				/* for now, assume one part */
    qentry.attr.docdata = CODES_UNKNOWN;		/* clear "%%DocumentData:" */
    qentry.opts.binselect = TRUE;			/* do auto bin select */
    qentry.opts.copies = -1;				/* unspecified number of copies */
    qentry.opts.collate = FALSE;			/* by default, don't collate */
    qentry.opts.keep_badfeatures = TRUE;		/* delete feature code not in PPD file */
    qentry.opts.hacks = HACK_DEFAULT_HACKS;		/* essential hacks? */
    qentry.N_Up.N = 1;					/* start with 1 Up */
    qentry.N_Up.borders = TRUE;				/* print borders when doing N-Up */
    qentry.N_Up.sigsheets = 0;				/* don't print signatures */
    qentry.N_Up.sigpart = SIG_BOTH;			/* print both sides of signature */
    qentry.draft_notice = (char*)NULL;			/* message to print diagonally */
    qentry.PassThruPDL = (const char *)NULL;		/* default (means PostScript) */
    qentry.Filters = (const char *)NULL;		/* default (means none) */
    qentry.PJL = (const char *)NULL;
    qentry.CachePriority = CACHE_PRIORITY_AUTO;
    qentry.StripPrinter = FALSE;
    qentry.page_list.mask = NULL;

    /* Figure out what language the user is getting messages in and
       attach its name to the job. */
    if(!(qentry.lc_messages = getenv("LC_MESSAGES")))
    	qentry.lc_messages = getenv("LANG");

    /*
    ** We would like to find a default response method in the variable
    ** PPR_RESPONDER.  If we don't we will use "write".
    */
    if(!(qentry.responder = getenv("PPR_RESPONDER")))
    	qentry.responder = "write";

    /*
    ** Since a responder address may not be specified with the -r switch,
    ** we will look for a default value in the environment variable
    ** PPR_RESPONDER_ADDRESS.  If no such variable exists,
    ** we must use the user name which the user logged in with.
    ** The login user name is the proper address for the default
    ** responder "write".  Notice that if the user used su to become
    ** a different user after loging in, the current user id will differ
    ** from the current user id.  In order for the responder to use the
    ** "write" command sucessfully, we must determine the login name.
    ** That is why we try the environment variables "USER" and "LOGNAME"
    ** before resorting to the current user id.
    */
    if(!(qentry.responder_address = getenv("PPR_RESPONDER_ADDRESS")))
	{
	if(!(qentry.responder_address = getenv("LOGNAME")))
	    {
	    if(!(qentry.responder_address = getenv("USER")))
		{
		qentry.responder_address = qentry.username;
		}
	    }
	}

    /*
    ** The default responder options come from the environment.
    ** If the variable is undefined, getenv() will return NULL
    ** which is just what we want.
    */
    qentry.responder_options = getenv("PPR_RESPONDER_OPTIONS");

    /*
    ** Look for default commentatary option in the environment.
    ** Remember that getenv() returns NULL for variables that
    ** are not found.
    */
    qentry.commentary = 0;
    if((ptr = getenv("PPR_COMMENTARY")))
    	qentry.commentary = atoi(ptr);

    /*
    ** Remove unnecessary variables from the environment.
    ** Notice that there is code above which defined safe
    ** values for PATH and IFS and defined HOME and PPR_VERSION.
    ** We must wait until here to remove variables we don't
    ** want passed to responders and filters because prune_env()
    ** delets USER and LOGNAME which we read just above.
    */
    prune_env();

    /*
    ** Clear the structure which is used to build
    ** up a description of the meduim which
    ** the printer specific code is probably
    ** trying to select:
    */
    guess_media.medianame[0] = '\0';
    guess_media.width = 0.0;
    guess_media.height = 0.0;
    guess_media.weight = 0.0;
    guess_media.colour[0] = '\0';
    guess_media.type[0] = '\0';

    /* Establish the fatal signal handler. */
    signal_interupting(SIGTERM, gallows_speach);	/* software termination */
    signal_interupting(SIGHUP, gallows_speach);		/* parent terminates */
    signal_interupting(SIGINT, gallows_speach);		/* control-C pressed */

    /*
    ** Arrange for child termination to be noted.  We use
    ** signal_restarting() because we want to be sure that
    ** is causes as little disruption as possible.
    */
    signal_restarting(SIGCHLD, reapchild);

    /*==============================================================
    ** Start of option parsing code
    ==============================================================*/

    /*
    ** Do a first pass over the command line to try to
    ** determine the desired destination queue.
    */
    {
    int optchar;
    gu_getopt_init(&getopt_state, argc, argv, option_description_string, option_words);
    while((optchar = ppr_getopt(&getopt_state)) != -1)
	doopt_pass1(optchar, getopt_state.optarg, getopt_state.name);
    }

    /*
    ** If no -d switch set qentry.destname, try to set it
    ** from the environment variable "PPRDEST", if that
    ** doesn't work, use a destination name of "default".
    **
    ** (We want to do this as soon as possible so that
    ** messages which refer to the job id will look right.)
    **
    ** After that we see if the destination node has been
    ** set.  If it hasn't been, then it is this node.
    */
    if(!qentry.destname)
	{
	const char *ptr;
	if(!(ptr = getenv("PPRDEST")))
	    if(!(ptr = getenv("PRINTER")))
		ptr = "default";
	parse_d_option(ptr);
	}

    /*
    ** Here is where we handle queue aliases.
    */
    {
    const char *ptr;
    if((ptr = extract_forwhat()))
    	parse_d_option(ptr);
    }

    /*
    ** If after all that the destination node is not known,
    ** it must be this node.
    */
    if(!qentry.destnode)
    	qentry.destnode = qentry.homenode;

    /*
    ** Now that we know the destination, we can pull in
    ** the switchset.  Notice that we do this before the
    ** main command line parsing pass.  This is so that
    ** settings on the command line can override settings
    ** from the switchset.
    */
    {
    const char *ptr;
    if((ptr = extract_switchset()))
	parse_switchset(ptr);
    }

    /*
    ** Do the second option parsing pass.
    */
    {
    int optchar;
    gu_getopt_init(&getopt_state, argc, argv, option_description_string, option_words);
    while((optchar = ppr_getopt(&getopt_state)) != -1)
	doopt_pass2(optchar, getopt_state.optarg, getopt_state.name);
    }

    /*
    ** A empty or syntactically invalid destination name
    ** can cause real problems later, therefor, try to
    ** detect it now.
    */
    if(qentry.destname[0] == '\0')
    	fatal(PPREXIT_SYNTAX, _("Destination (printer or group) name is empty"));
    if(strlen(qentry.destname) > MAX_DESTNAME)
	fatal(PPREXIT_SYNTAX, _("Destination (printer or group) name is too long"));
    if(strpbrk(qentry.destname, DEST_DISALLOWED))
	fatal(PPREXIT_SYNTAX, _("Destination (printer or group) name contains a disallowed character"));
    if(strchr(DEST_DISALLOWED_LEADING, (int)qentry.destname[0]))
	fatal(PPREXIT_SYNTAX, _("Destination (printer or group) name starts with a disallowed character"));

    /*
    ** Check for illegal combinations of options.
    */
    if(use_authcode && preauthorized)
	fatal(PPREXIT_SYNTAX, _("-a and -A are mutually exclusive"));

    /*
    ** Do a crude test of the syntax of the -o option.
    ** (The -o option passes options to an input file filter.)
    */
    if(option_filter_options)
	{
	ptr = option_filter_options;

	/* eat leading space */
	ptr += strspn(ptr," \t");

	while(*ptr)
	    {
	    /* eat up keyword */
	    ptr += strcspn(ptr, " \t=");

	    /* Look for space before "=". */
	    if(isspace(*ptr))
		fatal(PPREXIT_SYNTAX, _("spaces may not precede \"=\" in filter options"));

	    if(*ptr != '=')
		fatal(PPREXIT_SYNTAX, _("filter options must take form keyword=value"));

	    ptr++;                  /* move beyond equal sign */

	    if(*ptr == '\0')
		fatal(PPREXIT_SYNTAX, _("last -o keyword has no value"));

	    if(isspace(*ptr))
		fatal(PPREXIT_SYNTAX, _("spaces may not follow \"=\" in filter options"));

	    if(*ptr != '"')
	    	ptr += strcspn(ptr, " \t");
	    else
	    	{
		int c, lastc = '\0';
		ptr++;
		while((c = *ptr++) != '"' || lastc == '\\')
		    {
		    if(!c)
		    	fatal(PPREXIT_SYNTAX, _("unclosed quote in filter option value"));
		    lastc = c;
		    }
	    	}

	    /* each separating space */
	    ptr += strspn(ptr, " \t");
	    }
	} /* end of if there were -o switches */

    /*
    ** Any `non option' arguments should be processed here.
    ** (All that is allowed is one and only one file name.)
    */
    if(argc > getopt_state.optind)
	real_filename = argv[getopt_state.optind];

    if((argc - getopt_state.optind) > 1)
	fatal(PPREXIT_SYNTAX, _("only one file name allowed"));

    /*
    ** If no --title switch was used but a file name was used, make the file
    ** name the default title.  The default title may be overridden by a
    ** "%%Title:" line.  (Note that if the input is stdin, real_filename will
    ** be NULL.  Also, qentry.lpqFileName will be non-NULL only if the
    ** --lpq-filename switch has been used.)
    */
    if(!qentry.Title)
    	if(!(qentry.Title = qentry.lpqFileName))
    	    qentry.Title = real_filename;

    /*
    ** If we don't have at least a provisional For, then use either the Unix
    ** user name or the comment field from /etc/passwd.
    **
    ** The variable default_For is used by authorization()
    ** to determine if the current value of qentry.For
    ** was set here.
    **
    ** The setting controled by the the -u switch determines
    ** if we try to use the password comment (gecos) field.
    ** If the gecos field is empty we use the username anyway.
    */
    if(qentry.For == (char*)NULL || (! privledged() && ! use_authcode))
	{
	if(use_username || !pw->pw_gecos[0])
	    default_For = qentry.For = qentry.username;
	else
	    default_For = qentry.For = pw->pw_gecos;
	}

    /*===========================================================
    ** End of option parsing code
    ===========================================================*/

    /*
    ** Open the input file before we change to our home directory.
    ** When this function returns, the PPR home directory will be
    ** the current directory.
    **
    ** If no file was specified, real_filename will still be NULL.
    */
    if(infile_open(real_filename))	/* If input file is of zero length, */
	goto zero_length_file;		/* we have nothing to do. */

    /*
    ** Open the FIFO now so we will find out right away if the daemon is running.
    ** In former versions, we did this sooner so that the responder
    ** in pprd could be reached, but now that we have our own
    ** responder launcher we can wait until the destination name is set so
    ** that respond() will use the correct job name.
    **
    ** Note that at this point qentry.destnode will be defined already.
    */
    if((FIFO = open_fifo(FIFO_NAME)) == (FILE*)NULL)
        {
        ppr_abort(PPREXIT_NOSPOOLER, (char*)NULL);
        }

    /*
    ** We are about to start creating queue files.  We must
    ** assign a queue id now.
    **
    ** It is possible that get_input_file() will have been
    ** oblidged to call get_next_id() because it created
    ** a "-infile" file.  That is why we test to see
    ** if it has been assigned yet.
    */
    if(qentry.id == 0)
    	get_next_id(&qentry);

    /* ================== Input PostScript Processing Starts ===================== */

    /*
    ** Set a flag so that if we abort suddenly or are killed and
    ** are able to exit gracefully, we will know to remove the
    ** output files we are about to create.
    */
    spool_files_created = TRUE;

    /*
    ** Open the queue files in which we will store the output.
    */
    open_output();

    /*
    ** Chew thru the whole input file.
    */
    read_header_comments();	/* read header comments */

    if( read_prolog() )		/* If prolog (prolog, docdefaults, and docsetup) doesn't */
	{			/* doesn't stretch to EOF, %%EOF, or %%Trailer, */
	read_pages();		/* read the pages. */
	}

    /* We will always have a "%%Trailer" comment */
    fputs("%%Trailer\n", page_comments);
    fprintf(page_comments, "Offset: %ld\n", ftell(text));
    fputs("%%Trailer\n", text);

    /* If we hit "%%Trailer", read the trailer, otherwise
       the line had better be blank or say "%%EOF". */
    if(strcmp(line, "%%Trailer") == 0)
	read_trailer();
    else if(line[0] && strcmp(line, "%%EOF"))
    	fatal(PPREXIT_OTHERERR, "%s(): assertion failed at %s line %d: line[]=\"%s\"", function, __FILE__, __LINE__, line);

    /*
    ** Eat up any characters that are left.  There might be characters
    ** left because we stopt at an "%%EOF".  Or, in_getline() (defined
    ** in ppr_infile.c) may have detected an end of file marker such as
    ** an HP UEL which might be followed by junk characters.
    **
    ** If there are fewer than 50 characters it is probably something
    ** like PJL code, so we only express anoyance.  If there are
    ** more than that, it looks serious so it merits a severe warning.
    */
    for(x = 0; in_getc() != EOF; x++);

    if(x > 0)
	{
	if(x==1)
	    warning(WARNING_PEEVE, _("1 character follows EOF mark"));
	else if(x < 50)
	    warning(WARNING_PEEVE, _("%d characters follow EOF mark"), x);
	else if(x)
	    warning(WARNING_SEVERE, _("%d characters follow EOF mark"), x);
	}

    /*
    ** We are now done with the input file.
    */
    infile_close();

    /*
    ** Check if this is a truncated job that we should ignore.
    ** (That is, a job without "%%EOF" when "-S true".)
    ** (We do this check first since other checks are pretty
    ** meaninless if the input file is truncated.)
    **
    ** (It may be important to remember that if no "%%EOF" comment
    ** is present, getline() may insert an "%%EOF" comment to
    ** replace a protocol specific end of file indication.)
    */
    if(ignore_truncated && !eof_comment_present)
    	fatal(PPREXIT_TRUNCATED, _("input file was truncated (no %%%%EOF)"));

    /*
    ** Check for unclosed resources.  (This is an example of a check
    ** for which an abnormal result would not be suprising if the
    ** input file where truncated.)
    **
    ** The call to abort_resource() does not cause an exit, it
    ** mearly removes any temporary resource cache file.
    */
    if( nest_level() )
	{
	warning(WARNING_SEVERE, _("Unclosed resource or resources"));
	abort_resource();
	}

    /*
    ** Check if the file has ended in the middle of a section that must
    ** be closed.
    */
    if(outermost_current() != OUTERMOST_UNDEFINED)
	{
	if(outermost_current() == OUTERMOST_SCRIPT)
	    warning(WARNING_PEEVE, _("No \"%%%%Trailer:\" comment"));
	else
	    warning(WARNING_SEVERE, _("Unexpected EOF in unclosed %s"), str_outermost_types(outermost_current()));
	}

    /*
    ** Turn on duplexing if we are printing booklets or signatures
    ** and we are printing both sides at once,
    */
    if(qentry.N_Up.sigsheets)		/* if not zero (not disabled) */
    	{
	current_duplex_enforce = TRUE;	/* try to turn duplex on or off */
	qentry.N_Up.borders = FALSE;	/* no borders around virtual pages */

	if(qentry.N_Up.N == 1)		/* if N-Up has not been set yet, */
	    qentry.N_Up.N = 2;		/* set it now. */

	if(qentry.N_Up.sigpart == SIG_BOTH)			/* if printing both sides */
	    {							/* use duplex */
	    if(qentry.N_Up.N == 2)	    			/* if 2-Up, */
		current_duplex = DUPLEX_DUPLEX_TUMBLE;		/* use tumble duplex */
	    else						/* for 4-Up, */
	    	current_duplex = DUPLEX_DUPLEX_NOTUMBLE;	/* use no-tumble duplex */
	    }
	else					/* if printing only one side, */
	    {					/* use simplex */
	    current_duplex = DUPLEX_NONE;
	    }
    	}

    /*
    ** We may add duplex settings to the list of PPD options to be applied to
    ** this job.  Thus we implement the orders of an -F *Duplex option,
    ** -R duplex:* option, or -s optin.
    **
    ** Notice that we add the duplex mode to the list of job requirements
    ** and delete conflicting requirements.
    */
    if(current_duplex_enforce)		/* if we are to enforce our duplex finding, */
    	{				/* then */
	switch(current_duplex)		/* insert appropriate code */
	    {
	    case DUPLEX_NONE:
	    	mark_feature_for_insertion("*Duplex None");
		delete_requirement("duplex");
	    	break;
	    case DUPLEX_DUPLEX_NOTUMBLE:
	    	mark_feature_for_insertion("*Duplex DuplexNoTumble");
		delete_requirement("duplex");
		requirement(REQ_DOC, "duplex(tumble)");
	    	break;
	    case DUPLEX_DUPLEX_TUMBLE:
	    	mark_feature_for_insertion("*Duplex DuplexTumble");
		delete_requirement("duplex");
		requirement(REQ_DOC, "duplex");
	    	break;
	    case DUPLEX_SIMPLEX_TUMBLE:
	    	mark_feature_for_insertion("*Duplex SimplexTumble");
		/* Adobe yet hasn't defined a requirement name for this. */
		#if 0
		delete_requirement("duplex");
		requirement_REQ_DOC, "duplex(simplextumble)");
		#endif
	    	break;
	    default:
		fatal(PPREXIT_OTHERERR, "%s(): assertion failed at %s line %d", function, __FILE__, __LINE__);
		break;
	    }
    	}

    /*
    ** Set the duplex_pagefactor variable according to the
    ** duplex setting.
    */
    switch(current_duplex)
	{
	case DUPLEX_NONE:
	case DUPLEX_SIMPLEX_TUMBLE:
	    duplex_pagefactor = 1;
	    break;
	case DUPLEX_DUPLEX_NOTUMBLE:
	case DUPLEX_DUPLEX_TUMBLE:
	    duplex_pagefactor = 2;
	    break;
	default:
	    fatal(PPREXIT_OTHERERR, "%s(): assertion failed at %s line %d", function, __FILE__, __LINE__);
	    break;
	}

    /*
    ** Dump the gathered information into the -comments file.
    ** If this seems sparse, it is because most of the comments
    ** will be held in the queue file until the job is reconstructed.
    */
    dump_document_media(comments, 0);

    /*
    ** Close those queue files which we are done with.
    ** Only the one in the "queue" directory remains open.
    **
    ** We must be sure to set the pointers to NULL, otherwise
    ** file_cleanup() could try to close them again and thereby
    ** cause a core dump.
    */
    fclose(comments);
    fclose(page_comments);
    fclose(text);
    comments = page_comments = text = (FILE*)NULL;

    /* =================== Input PostScript Processing Ends ===================== */

    if(qentry.CachePriority == CACHE_PRIORITY_AUTO)
    	{
	if(get_cache_strip_count() > 0)
	    qentry.CachePriority = CACHE_PRIORITY_HIGH;
	else
	    qentry.CachePriority = CACHE_PRIORITY_LOW;
    	}

    /*
    ** Compare "qentry.attr.pages" to "pagenumber".
    ** If we have "%%Page: " comments but no "%%Pages: "
    ** comment, use the number of "%%Page: " comments.
    */
    if(pagenumber > 0 && qentry.attr.pages == -1)
	{
	warning(WARNING_SEVERE, _("No valid \"%%%%Pages:\" comment"));
	qentry.attr.pages = pagenumber;
	}

    /*
    ** The variable "pagenumber" now contains the number of "%%Page:"
    ** comments encountered.
    **
    ** If we have "%%Page: " comments and there are more "%%Page: "
    ** comments than the number of pages stated in the "%%Pages: "
    ** comment, then use the count of "%%Page: " comments
    ** since it would seem harder to get that wrong.
    **
    ** What to do when the "%%Pages:" comment indicates more pages than
    ** there are "%%Page:" comments is more troublesome.  The program
    ** pslpr will make this mistake when extracting selected pages from
    ** a document.  The current solution is to use the count of "%%Page:"
    ** comments.
    */
    if(pagenumber > 0)
	{
	if(pagenumber > qentry.attr.pages)
	    {
	    warning(WARNING_SEVERE, _("\"%%%%Pages:\" comment is wrong, changing it from %d to %d"), qentry.attr.pages, pagenumber);
	    qentry.attr.pages = pagenumber;
	    }
	else if(pagenumber < qentry.attr.pages)
	    {
	    warning(WARNING_SEVERE, _("Missing \"%%%%Page:\" comments or incorrect \"%%%%Pages:\" comment"));
	    qentry.attr.pages = pagenumber;
	    }
	}

    /*
    ** If we found no "%%Page:" comments, then the pageorder is special.
    ** (Re-ordering pages which we can not locate is not possible.)
    */
    if(pagenumber == 0)
	qentry.attr.pageorder = PAGEORDER_SPECIAL;

    /*
    ** If we found "%%Page:" comments, then qentry.attr.script
    ** should be set to TRUE.  This will indicate that this file
    ** has what the DSC specification calls a "script" section.
    */
    qentry.attr.script = (pagenumber != 0) ? TRUE : FALSE;

    /*
    ** Compute the final pagefactor.
    ** The pagefactor is the number of virtual pages on each sheet
    ** of paper.  It is the N-Up N times two if we are printing
    ** in duplex.
    */
    qentry.attr.pagefactor = duplex_pagefactor * qentry.N_Up.N;

    /*
    ** If in booklet mode (automatically sized single signature),
    ** then compute qentry.N_Up.sigsheets.
    **
    ** We will know we are in booklet mode if qentry.N_Up.sigsheets
    ** is -1.  If we are not doing signitures, it will be 0.
    */
    if(qentry.N_Up.sigsheets == -1 && qentry.attr.pages > 0)
    	{
    	int sigfactor = (qentry.N_Up.N * 2);
    	qentry.N_Up.sigsheets = (qentry.attr.pages+sigfactor-1)/sigfactor;
    	}

    /*
    ** Attempt to resolve resource descrepancies before we
    ** write the queue file.
    */
    rationalize_resources();

    /*
    ** If there is a charge (money) for using this printer or any printer
    ** in this group, make sure the user has a charge account with
    ** a sufficient balance.  If not, this function will not return.
    */
    authorization_charge();

    /*
    ** Make sure the access control lists permit the user to send jobs to this
    ** printer.  If not, then this function too will not return.
    */
    authorization_acl();

    /*
    ** Has the user asked for only a subset of the pages?
    */
    if(option_page_list)
	{
	/* We can't do it if pages aren't marked off. */
	if(! qentry.attr.script)
	    {
	    ppr_abort(PPREXIT_NOTPOSSIBLE, (char*)NULL);
 	    }

	/* Try to parse the list of pages to be printed. */
	if(pagemask_encode(&qentry, option_page_list) == -1)
 	    {
 	    fatal(PPREXIT_SYNTAX, "Can't parse page list");
 	    }
    	}

    /*
    ** Give the job splitting machinery a chance to work.
    ** If it does, we will skip the normal job submission
    ** code.
    */
    if(split_job(&qentry))	/* If it does do a split, */
	{			/* remove what would have been */
	file_cleanup();		/* the files for a monolithic job. */
	}

    /*
    ** If this else clause is executed, the job was not submitted
    ** as a bunch of little jobs by split_job(), so we must submit it
    ** ourselves as one large job.
    */
    else
    	{
    	/*
    	** Create and fill the queue file.  It is sad that this
    	** is the only place we check for disk full.
    	**
    	** Note that fatal() file_cleanup(), so the queue file should be
    	** removed.
    	*/
    	if(write_queue_file(&qentry) == -1)
    	    fatal(PPREXIT_DISKFULL, _("Disk full"));

	/*
	** FIFO was opened above, write a command to it now.  This command
	** tells pprd that the file is ready to print.
	*/
	submit_job(&qentry, 0);
	}

    /*
    ** We are done with the communications channel to
    ** the spooler, we can close it now.
    */
    fclose(FIFO);

    /*
    ** This goto target is used when the length of the input file is zero.
    */
    zero_length_file:

    /*
    ** If -U switch was used, unlink the input file.  This feature
    ** is really handy if Samba is submitting the jobs!
    */
    if(option_unlink_jobfile && real_filename)
	{
	become_user();

	if(chdir(starting_directory) == -1)
	    fprintf(stderr, "%s: -U: can't chdir(\"%s\")\n", myname, starting_directory);

    	if(unlink(real_filename) == -1)
    	    fprintf(stderr, _("%s: -U: can't unlink(\"%s\"), errno=%d (%s)\n"), myname, real_filename, errno, gu_strerror(errno));

	unbecome_user();
    	}

    /*
    ** We think we may safely state that we
    ** have fulfilled our job correctly.
    */
    return PPREXIT_OK;
    } /* end of main */

/* end of file */

