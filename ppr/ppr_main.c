/*
** mouse:~ppr/src/ppr/ppr_main.c
** Copyright 1995--2010, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 2 August 2010.
*/

/*
** Main module of the program used to submit jobs to the
** PPR print spooler.
*/

#include "config.h"
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
#include "ppr_conffile.h"		/* for switchsets, deffiltopts, and passthru */
#include "userdb.h"				/* for user charge accounts */
#include "ppr_exits.h"
#include "respond.h"
#include "version.h"			/* for "--version" switch */
#include "ppr_gab.h"

/*
** global variables
*/

/* Name of this program (for error messages). */
const char *myname = "ppr";

/* input line */
char line[MAX_LINE+2];					/* input line plus one, plus NULL */
int line_len;							/* length of input line in bytes */
gu_boolean line_overflow;				/* is lines truncated? true or false */

/* output files */
FILE *comments = (FILE*)NULL;			/* file for header & trailer comments */
FILE *page_comments = (FILE*)NULL;		/* file for page level comments */
FILE *text = (FILE*)NULL;				/* file for remainder of text */
static int spool_files_created = FALSE; /* TRUE once we have created some files, used for cleanup on abort */

/* Information used for determining privileges and such. */
uid_t user_uid;							/* Unix user id of person submitting the job */
gid_t user_gid;
uid_t ppr_uid;							/* uid of spooler owner (ppr) */
gid_t ppr_gid;
char *user_pw_name, *user_pw_gecos;		/* result of looking up user_uid */

/* Command line option settings, static */
static int warning_level = WARNING_SEVERE;		/* these and more serious allowed */
static int warning_log = FALSE;					/* set true if warnings should go to log */
static int option_unlink_jobfile = FALSE;		/* Was the -U switch used? */
static int ignore_truncated = FALSE;			/* TRUE if should discard without %%EOF */
static gu_boolean option_show_jobid = FALSE;
static int option_print_id_to_fd = -1;			/* -1 for don't, file descriptor number otherwise */
static const char *option_page_list = NULL;
static int option_skeleton_jobid = 0;

/* Command line option settings */
const char *features[MAX_FEATURES];				/* -F switch features to add */
int features_count = 0;							/* number asked for (number of -F switches?) */
gu_boolean option_strip_fontindex = FALSE;		/* TRUE if should strip those in fontindex.db */
int ppr_respond_by = PPR_RESPOND_BY_STDERR;
int option_nofilter_hexdump = FALSE;			/* don't encourage use of hexdump when no filter */
char *option_filter_options = NULL;				/* contents of -o switch */
int option_TrueTypeQuery = TT_UNKNOWN;			/* for ppr_mactt.c */
unsigned int option_gab_mask = 0;				/* Mask to tell what to gab about. */
int option_editps_level = 1;					/* 1 thru 10 */
enum MARKUP option_markup = MARKUP_FALLBACK_LP; /* how to treat markup languages */

/* -R switch command line option settings. */
int read_copies = TRUE;					/* TRUE if should read copies from file */
int read_duplex = TRUE;					/* TRUE if we should guess duplex */
int read_signature = TRUE;				/* TRUE if we should implement signature features */
int read_nup = TRUE;
int read_For = FALSE;					/* Pay attention to "%%For:" lines? */
int read_ProofMode = TRUE;
int read_Title = TRUE;
int read_Routing = TRUE;

/* Duplex handling is complex. */
int current_duplex = DUPLEX_NONE;
gu_boolean current_duplex_enforce = FALSE;

/* odds and ends */
static FILE *FIFO = (FILE*)NULL;		/* streams library thing for pipe to pprd */
struct QEntryFile qentry;				/* structure in which we build our queue entry */
int pagenumber = 0;						/* count of %%Page: comments */

/* default media */
const char *default_medium = NULL;		/* medium to start with if no "%%DocumentMedia:" comment */
struct Media guess_media;				/* used to gather information to guess proper medium */

/* Page factors */
static int duplex_pagefactor;			/* pages per sheet multiple due to duplex */

/* Array of things (resources, requirements, etc.). */
struct Thing *things = (struct Thing *)NULL;	/* fonts, media and such */
int thing_count = 0;							/* number of entries used */

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
		case PPREXIT_SYNTAX:							/* from fatal() */
			responder_code = RESP_FATAL_SYNTAX;
			template = "%s";
			break;
		default:										/* from fatal() */
			responder_code = RESP_FATAL;
			template = "%s";
			break;
		}

	if(ppr_respond_by & PPR_RESPOND_BY_STDERR)
		{
		fflush(stdout);							/* <-- prevent confusion */
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

	va_start(va, message);								/* Format the error message */
	vsnprintf(errbuf, sizeof(errbuf), message, va);		/* as a string. */
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
	va_start(va,message);				/* Format the error message */
	vfprintf(stderr,message,va);		/* as a string. */
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

	if(!qentry.jobname.destname)
		fatal(PPREXIT_OTHERERR, "%s(): assertion failed", function);

	if(level < warning_level)	/* if warning level too low */
		return;					/* do not print it */

	if(warning_log)				/* if warnings are to go to log file, */
		{						/* open this job's log file */
		ppr_fnamef(wfname, "%s/%s-%d.0-log", DATADIR, qentry.jobname.destname, qentry.jobname.id);
		if(!(wfile = fopen(wfname, "a")))
			{
			fprintf(stderr, _("Failed to open log file, using stderr.\n"));
			wfile = stderr;		/* if fail, use stderr anyway */
			}
		}
	else						/* if not logging warnings, */
		{						/* send them to stderr */
		wfile = stderr;
		}

	va_start(va, message);
	fprintf(wfile, _("WARNING: "));		/* now, print the warning message */
	vfprintf(wfile, message, va);
	fprintf(wfile, "\n");
	va_end(va);

	if(wfile != stderr)					/* if we didn't use stderr, */
		fclose(wfile);					/* close what we did use */
	else								/* if was stderr, */
		fflush(stderr);					/* empty output buffer */
										/* (important for papd) */
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
** value						-- the value to be checked
** null_ok						-- is a NULL pointer acceptable?
** empty_ok						-- is a zero length string acceptable?
** entirely_whitespace_ok		-- is a value consisting entirely of whitespace ok?
** name							-- what should we call the value in error messages?
** is_argument					-- should we describe it as an argument to name?
*/
static void assert_ok_value(const char value[], gu_boolean null_ok, gu_boolean empty_ok, gu_boolean entirely_whitespace_ok, const char name[], gu_boolean is_argument)
	{
	if(!value)
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
				{
				if(is_argument)
					fatal(PPREXIT_OTHERERR, _("%s argument is empty"), name);
				else
					fatal(PPREXIT_OTHERERR, _("%s is empty"), name);
				}
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

				if(is_argument)
					fatal(PPREXIT_OTHERERR, _("%s argument is entirely whitespace"), name);
				else
					fatal(PPREXIT_OTHERERR, _("%s is entirely whitespace"), name);
				}
			}
		}
	} /* end of assert_ok_value() */

/*
** Create and fill the queue file.
** Return -1 if we run out of disk space.
*/
int write_queue_file(struct QEntryFile *qentry, gu_boolean overwrite)
	{
	const char function[] = "write_queue_file";
	char magic_cookie[16];
	char qfname[MAX_PPR_PATH];
	int fd;
	FILE *Qfile;

	/* This code looks for things that could make a mess of the queue file. */
	assert_ok_value(qentry->For, FALSE, FALSE, FALSE, "qentry->For", FALSE);
	assert_ok_value(qentry->charge_to, TRUE, FALSE, FALSE, "qentry->charge_to", FALSE);
	assert_ok_value(qentry->Title, TRUE, TRUE, FALSE, "qentry->Title", FALSE);
	assert_ok_value(qentry->draft_notice, TRUE, FALSE, FALSE, "qentry->draft_notice", FALSE);
	assert_ok_value(qentry->Creator, TRUE, FALSE, FALSE, "qentry->Creator", FALSE);
	assert_ok_value(qentry->Routing, TRUE, FALSE, FALSE, "qentry->Routing", FALSE);
	assert_ok_value(qentry->lpqFileName, TRUE, FALSE, FALSE, "qentry->lpqFileName", FALSE);
	assert_ok_value(qentry->responder.name, FALSE, FALSE, FALSE, "qentry->responder", FALSE);
	assert_ok_value(qentry->responder.address, FALSE, FALSE, FALSE, "qentry->responder.address", FALSE);
	assert_ok_value(qentry->responder.options, TRUE, TRUE, TRUE, "qentry->responder.options", FALSE);
	assert_ok_value(qentry->ripopts, TRUE, TRUE, TRUE, "qentry->ripopts", FALSE);
	assert_ok_value(qentry->lc_messages, TRUE, FALSE, FALSE, "LC_MESSAGES", FALSE);

	/* Create a new magic cookie. */
	snprintf(magic_cookie, sizeof(magic_cookie), "%08X", rand());
	qentry->magic_cookie = magic_cookie;

	/* Construct the queue file name. */
	ppr_fnamef(qfname, "%s/%s-%d.%d", QUEUEDIR, qentry->jobname.destname, qentry->jobname.id, qentry->jobname.subid);

	/* Very carefully open the queue file. */
	if((fd = open(qfname, O_WRONLY | O_CREAT | (overwrite ? 0 : O_EXCL), (S_IRUSR | S_IWUSR))) < 0)
		fatal(PPREXIT_OTHERERR, _("can't open queue file \"%s\", errno=%d (%s)"), qfname, errno, gu_strerror(errno) );
	if((Qfile = fdopen(fd, "w")) == (FILE*)NULL)
		fatal(PPREXIT_OTHERERR, _("%s(): %s() failed, errno=%d (%s)"), function, "fdopen", errno, gu_strerror(errno));

	/* Use library code to write the body. */
	qentryfile_save(qentry, Qfile);

	/* Write an empty Addon section.  Job ticket information may be added later by ppop. */
	fprintf(Qfile, "EndAddon\n");

	/* Add "Media:" lines which indicate what kind of media the job requires. */
	write_media_lines(Qfile, qentry->jobname.subid);
	fprintf(Qfile, "EndMedia\n");

	/* Add "Res:" lines which indicate which fonts, procedure sets, etc. the job requires. */
	write_resource_lines(Qfile, qentry->jobname.subid);
	fprintf(Qfile, "EndRes\n");

	/* Add "Req:" lines which indicate what printer features are required. */
	write_requirement_lines(Qfile, qentry->jobname.subid);
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
** Open the FIFO to pprd.  This FIFO will be used to tell
** the pprd that the job has been placed in the queue.
*/
static FILE *open_fifo(const char name[])
	{
	int fifo;			/* file handle of pipe to pprd */
	int newfifo;

	/*
	** Try to open the FIFO.  If we fail it is probably because
	** pprd is not running.  In that case, errno will be set to
	** ENOENT or ENXIO (depending on whether the spooler has
	** ever been run).
	*/
	if((fifo = open(name, O_WRONLY | O_NONBLOCK)) == -1)
		{
		if(errno != ENXIO && errno != ENOENT)
			fatal(PPREXIT_OTHERERR, _("Can't open \"%s\", errno=%d (%s)"), name, errno, gu_strerror(errno));
		return (FILE*)NULL;
		}

	/*
	** This loop takes care of things if this program was invoked without
	** file descriptors 0 thru 2 already open.  It is popularly supposed that
	** the operating system connects these to stdin, stdout, and stderr, but
	** actually the shell does this.  Some daemons may invoke ppr without
	** opening something on these file descriptors.  If this happens, we must
	** move the fifo up and open /dev/null on descriptors 0 thru 2.  Otherwise
	** our messages to stdout and stderr will confound pprd.
	*/
	while(fifo <= 2)
		{
		newfifo = dup(fifo);			/* move fifo up to next handle */
		close(fifo);					/* close old one */
		if(open("/dev/null", (fifo==0 ? O_RDONLY : O_WRONLY) ) != fifo)
			exit(PPREXIT_OTHERERR);		/* Mustn't call fatal! */
		fifo = newfifo;					/* adopt the new handle */
		}

	/* Create an I/O stream which represents the FIFO. */
	return fdopen(fifo, "w");
	} /* end of open_fifo() */

/*
** Function to send a command to the FIFO to submit a job.
*/
void submit_job(struct QEntryFile *qe, int subid)
	{
	if(option_skeleton_jobid)
		fprintf(FIFO, "J %s-%d.%d\n", qe->jobname.destname, qe->jobname.id, subid);
	else
		fprintf(FIFO, "j %s-%d.%d\n", qe->jobname.destname, qe->jobname.id, subid);

	/*
	** If --show-jobid has been used, display the id of the 
	** job being submitted.
	**
	** Notice that --show-jobid produces a message in almost
	** the same format as System V lp.
	*/
	if(option_show_jobid)
		{
		if(subid == 0)
			printf(_("request id is %s-%d (1 file)\n"), qentry.jobname.destname, qentry.jobname.id);
		else
			printf(_("Request id: %s-%d.%d\n"), qentry.jobname.destname, qentry.jobname.id, subid);
		}
	} /* end of submit_job() */

/*
** Code to check if there is a charge (money) for printing to the selected
** destination which would mean that the user needs to be specially authorized
** to submit jobs to this destination.  If so, check if he is authorized.  If
** authorization is required but has not been obtained, it is a fatal error
** and this routine never returns.
**
** When this routine is called, qentry.For should already be set.
*/
static void authorization_charge(void)
	{
	if(destination_protected(qentry.jobname.destname))
		{
		struct userdb user;
		int ret;

		ret = db_auth(&user, qentry.charge_to);		/* user lookup */

		if(ret == USER_ISNT)			/* If user not found, then, turn away. */
			ppr_abort(PPREXIT_NOCHARGEACCT, qentry.charge_to);
		else if(ret == USER_OVERDRAWN)	/* If account overdrawn, then, turn away. */
			ppr_abort(PPREXIT_OVERDRAWN, qentry.charge_to);
		else if(ret == USER_ERROR)		/* We check for database error. */
			fatal(PPREXIT_OTHERERR, _("can't open printing charge database"));

		/* Do not allow jobs w/out page counts on cost per page printers. */
		if(qentry.attr.pages < 0)
			ppr_abort(PPREXIT_NONCONFORMING, (char*)NULL);

		if(qentry.opts.copies == -1)	/* If `secure' printer, force */
			qentry.opts.copies = 1;		/* unspecified copies to 1. */
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
		gu_boolean result = FALSE;

		#ifdef DEBUG_ACL
		printf("ACL list: %s\n", acl_list);
		#endif

		ptr = acl_list_copy = gu_strdup(acl_list);

		while((substr = strtok(ptr, " ")))
			{
			#ifdef DEBUG_ACL
			printf("Trying ACL %s...\n", substr);
			#endif

			if(user_acl_allows(qentry.user, substr))
				result = TRUE;
			}

		gu_free(acl_list_copy);

		if(!result)
			ppr_abort(PPREXIT_ACL, qentry.user);
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

	/* Remove any files created by ppr_infile.c */
	infile_file_cleanup();

	/* Remove the partially completed job files. */
	if(spool_files_created)
		{
		char fname[MAX_PPR_PATH];

		if(!option_skeleton_jobid)
			{
			ppr_fnamef(fname, "%s/%s-%d.0", QUEUEDIR, qentry.jobname.destname, qentry.jobname.id);
			unlink(fname);

			ppr_fnamef(fname, "%s/%s-%d.0-cmdline", DATADIR, qentry.jobname.destname, qentry.jobname.id);
			unlink(fname);

			ppr_fnamef(fname, "%s/%s-%d.0-log", DATADIR, qentry.jobname.destname, qentry.jobname.id);
			unlink(fname);
			}

		ppr_fnamef(fname, "%s/%s-%d.0-comments", DATADIR, qentry.jobname.destname, qentry.jobname.id);
		unlink(fname);

		ppr_fnamef(fname, "%s/%s-%d.0-text", DATADIR, qentry.jobname.destname, qentry.jobname.id);
		unlink(fname);

		ppr_fnamef(fname, "%s/%s-%d.0-pages", DATADIR, qentry.jobname.destname, qentry.jobname.id);
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
	pid_t pid;
	int wstat;			/* storage for child's exit status */
	const char *filter_name;

	/* Get the child's exit code. */
	if((pid = wait(&wstat)) == -1)
		fatal(PPREXIT_OTHERERR, "%s(): wait() failed, errno=%d (%s)", function, errno, gu_strerror(errno));

	filter_name = infile_filter_name_by_pid(pid);

	if(WIFEXITED(wstat))
		{
		switch(WEXITSTATUS(wstat))
			{
			case 0:						/* Normal exit, */
				return;					/* this signal handler need do nothing. */
			case 242:
				fatal(PPREXIT_OTHERERR, _("%s failed, exit code = 242 (exec() failed?)"), filter_name);
				break;
			default:
				fatal(PPREXIT_OTHERERR, _("%s failed, exit code = %d"), filter_name, WEXITSTATUS(wstat));
				return;
			}
		}
	else if(WIFSIGNALED(wstat))
		{
		if(WCOREDUMP(wstat))
			fatal(PPREXIT_OTHERERR, _("%s died on receipt of signal %d, core dumped"), filter_name, WTERMSIG(wstat));
		else
			fatal(PPREXIT_OTHERERR, _("%s died on receipt of signal %d"), filter_name, WTERMSIG(wstat));
		}
	else
		{
		fatal(PPREXIT_OTHERERR, "%s(): bizzar child termination", function);
		}
	} /* end of reapchild() */

/*
** Function to return true if the current user is a privileged
** user.  A privileged user is defined as a a member of the
** ACL called "pprprox".  The users USER_PPR and "root" are
** non-removable members of that ACL.
**
** The result of this routine determines whether the user is allowed
** to override his user name on headers with a new name in the
** "-f" switch or "%%For:" header line.  It also determines whether
** a user can use the "-A" switch.
**
** The first time this routine is called, it will cache the answer.
**
** Don't change the 0's and 1's in this function to TRUE
** and FALSE because answer starts out with the value -1
** which might be confounded with TRUE.
*/
static int privileged(void)
	{
	static int answer = -1;		/* -1 means undetermined */

	if(answer == -1)			/* if undetermined */
		{
		if(user_acl_allows(user_pw_name, "pprprox"))
			answer = 1;
		else
			answer = 0;
		}

	return answer;
	} /* end of privileged() */

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
	if(setegid(ppr_gid))
		fatal(PPREXIT_OTHERERR, "unbecome_user(): can't setegid(%ld)", (long)ppr_gid);

	if(seteuid(ppr_uid))
		fatal(PPREXIT_OTHERERR, "unbecome_user(): can't seteuid(%ld)", (long)ppr_uid);
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
static const char *option_description_string = "d:e:f:i:m:r:b:t:w:D:F:T:q:B:N:n:C:H:R:Z:O:K:s:P:Io:UY:u:G:Q:";

/*
** This table maps long option names to short options or to large integers.
*/
static const struct gu_getopt_opt option_words[] =
		{
		/* These aliases are preliminary assignments and are not yet documented. */
		{"queue", 'd', TRUE},
		{"for", 'f', TRUE},
		{"user", 'u', TRUE},
		{"banner", 'b', TRUE},
		{"trailer", 't', TRUE},
		{"errors", 'e', TRUE},
		{"warnings", 'w', TRUE},
		{"proofmode", 'P', TRUE},
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
		{"truetype", 'Q', TRUE},
		{"keep-bad-features", 'K', TRUE},
		{"filter-options", 'o', TRUE},

		/* These are final and documented in the --help output and the ppr(1) manpage.
		   (Note: that they are documented should be verified.)
		   */
		{"title",				'C', TRUE},
		{"feature",				'F', TRUE},
		{"gab",					'G', TRUE},
		{"hack",				'H', TRUE},
		{"routing",				'i', TRUE},
		{"responder",			'm', TRUE},
		{"copies",				'n', TRUE},
		{"responder-address",	'r', TRUE},
		{"file-type",			'T', TRUE},
		{"features",			1000, FALSE},
		{"ipp-priority",		1002, TRUE},
		{"lpq-filename",		1003, TRUE},
		{"hold",				1004, FALSE},
		{"responder-options",	1005, TRUE},
		{"show-jobid",			1006, FALSE},
		{"charge-to",			1008, TRUE},
		{"editps-level",		1009, TRUE},
		{"markup",				1010, TRUE},
		{"page-list",			1012, TRUE},
		{"strip-fontindex",		1015, TRUE},
		{"strip-printer",		1016, TRUE},
		{"save",				1017, FALSE},		/* not documented because not implemented */
		{"question",			1018, TRUE},		/* not documented */
		{"ripopts",				1019, TRUE},

		{"commentary", 1100, TRUE},

		/* These are for the IPP server. */
		{"print-id-to-fd",		2000, TRUE},		/* documented */
		{"skeleton-create",		2001, FALSE},		/* not documented */
		{"skeleton-jobid",		2002, TRUE},		/* not documented */

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

HELP(_(
"\t-u <username>              username of job submitter\n"
"\t-f <realname>              real name of cooresponding to username\n"));

HELP(_(
"\t--charge-to <string>       PPR charge account to bill for job\n"));

HELP(_(
"\t--title <string>           set default document title\n"));

HELP(_(
"\t-O <string>                overlay `Draft' notice\n"));

HELP(_(
"\t--routing <string>         provide routing (delivery) instructions\n"));

HELP(_(
"\t-m <method>                response method\n"
"\t-m none                    no response\n"
"\t--responder                same as -m\n"
"\t-r <address>               response address\n"
"\t--responder-address        same as -r\n"
"\t--responder-options <list> list of name=value responder options\n"));

HELP(_(
"\t--commentary {status,stall,exit}\n"
"\t                           requested commentary of listed types\n"));

HELP(_(
"\t-b {yes,no,dontcare}       express banner page preference\n"
"\t-t {yes,no,dontcare}       express trailer page preference\n"));

HELP(_(
"\t-w log                     routes warnings to log file\n"
"\t-w stderr                  routes warnings to stderr (default)\n"
"\t-w both                    routes warnings to stderr and log file\n"
"\t-w {severe,peeve,none}     sets warning level\n"));

HELP(_(
"\t--strip-fontindex true     strip out fonts listed in font index\n"
"\t--strip-fontindex false    don't strip these fonts (default)\n"));

HELP(_(
"\t--strip-printer true       strip out resources already in printer\n"
"\t--strip-printer false      don't strip these resources (default)\n"));

HELP(_(
"\t-F '<feature name>'        inserts setup code for a printer feature\n"
"\t--feature '<feature name>' same as above\n"
"\t--features                 list available printer features\n"));

HELP(_(
"\t--ripopts '<options>'      options to pass to external RIP\n"));

HELP(_(
"\t-K true                    keep feature code though not in PPD file\n"
"\t-K false                   don't keep bad feature code (default)\n"));

HELP(_(
"\t-D <mediumname>            sets default medium\n"));

HELP(_(
"\t-B false                   disable automatic bin selection\n"
"\t-B true                    enable automatic bin selection (default)\n"));

minus_tee_help(outfile);		/* drag in -T stuff */

HELP(_(
"\t--file-type <option>       same as -T\n"));

HELP(_(
"\t-o <string>                specify filter options\n"
"\t--markup [format, lp, pr, fallback-lp, fallback-pr]\n"
"\t                           set handling of LaTeX, HTML, and such\n"));

HELP(_(
"\t-N <positive integer>      print pages N-Up\n"
"\t-N noborders               turn off N-Up borders\n"));

HELP(_(
"\t-n <positive integer>      print n copies\n"
"\t-n collate                 print collated copies\n"
"\t--copies <option>          same as -n\n"));

HELP(_(
"\t-R for                     read \"%%For:\" line\n"
"\t-R ignore-for              don't read \"%%For:\" (default)\n"));

HELP(_(
"\t-R title                   use title from \"%%Title:\" (default)\n"
"\t-R ignore-title            ignore \"%%Title:\" comment\n"));

HELP(_(
"\t-R copies                  read copy count from document (default)\n"
"\t-R ignore-copies           don't read copy count\n"));

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
"\t-e none                    discard error messages\n"
"\t-e stderr                  report errors on stderr (default)\n"
"\t-e responder               report errors by responder\n"
"\t-e both                    report errors with both\n"
"\t-e hexdump                 always use hexdump in absence of filter\n"
"\t-e no-hexdump              discourage hexdump in absence of filter\n"));

HELP(_(
"\t-s <positive integer>      signature sheet count\n"
"\t-s booklet                 automatic signature sheet count\n"
"\t-s {both,fronts,backs}     indicate part of each signature to print\n"));

HELP(_(
"\t-Y <options>               experimental job splitting feature\n"));

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
"\t-q <integer>               job priority on scale 39--0\n"));
HELP(_(
"\t--ipp-priority <integer>   job priority on scale 1--100\n"));

HELP(_(
"\t-U                         unlink job file after queuing it\n"));

HELP(_(
"\t-Q <string>                TrueType query answer given (for papd)\n"));

HELP(_(
"\t--lpq-filename <string>    filename for ppop lpq listings\n"));

HELP(_(
"\t-H <hack name>             turn on a hack\n"
"\t-H no-<hack name>          turn off a hack\n"
"\t--hack [no-]<hack name>    same as -H\n"));

HELP(_(
"\t--editps-level <pos int>   set level of editing for -H editps\n"));

HELP(_(
"\t--show-jobid               print the queue id of submitted jobs\n"));

HELP(_(
"\t--print-id-to-fd           print the numberic queue id file descriptor\n"));

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
	else										/* no match */
		return -1;								/* is an error */
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
**		-F "*Duplex DuplexNoTumble"
**		-F Duplex=DuplexNoTumble
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
	   This code converts this format to the origional -F format.
	   Later converted to use snprintf() instead of strcpy() and
	   moving pointers.
	 */
	if(name[0] != '*')
		{
		char *equals_sign;
		int name_storage_len;

		/* syntax error if no equals or any spaces */
		if(!(equals_sign=strchr(name, '=')) || strchr(name, ' '))
			return -1;

		name_storage_len = strlen(name) + 2;	/* two is for '*' and '\0' terminator */
		name_storage = (char*)gu_alloc(name_storage_len, sizeof(char));
		snprintf(name_storage, name_storage_len, "*%.*s %s", (int)(equals_sign-name), name, equals_sign+1);
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

		if(strcmp(option, "None") == 0)			/* i.e., simplex */
			current_duplex = DUPLEX_NONE;
		else if(strcmp(option, "SimplexTumble") == 0)
			current_duplex = DUPLEX_SIMPLEX_TUMBLE;
		else if(strcmp(option, "DuplexTumble") == 0)
			current_duplex = DUPLEX_DUPLEX_TUMBLE;
		else if(strcmp(option, "DuplexNoTumble") == 0)
			current_duplex = DUPLEX_DUPLEX_NOTUMBLE;
		else									/* unrecognized!!! */
			{
			warning(WARNING_SEVERE, "Unrecognized duplex type %s, substituting DuplexNoTumble", option);
			current_duplex = DUPLEX_DUPLEX_NOTUMBLE;
			}

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
	int x;						/* line index */
	int optchar;				/* switch character we are processing */
	char *argument;				/* character's argument */
	char *ptr;					/* pointer into option_description_string[] */

	char *line = gu_strdup(switchset);	/* we copy becuase we insert NULLs */
	int stop = strlen(line);

	for(x=0; x < stop; x++)		/* move thru the line */
		{
		optchar = line[x++];

		argument = &line[x];							/* The stuff after the switch */
		argument[strcspn(argument, "|")] = '\0';		/* NULL terminate */

		if(optchar == '-')								/* if it is a long option */
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

			doopt_pass2(optchar, argument, ""); /* actually process it */
			}

		x += strlen(argument);			/* move past this one */
		}								/* (x++ in for() will move past the NULL) */

	/* Don't gu_free(line)! */
	} /* parse_switchset */

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
 * Parse the --commentary option or PPR_COMMENTARY value.
 */
static int parse_commentary_option(const char source[], const char list[])
	{
	int mask = 0;
	char *temp, *place, *next;
	for(place=temp=gu_strdup(list); (next = gu_strsep(&place, ", ")); )
		{
		if(strcmp(next, "status") == 0)
			{
			mask |= COM_PRINTER_STATUS;
			mask |= COM_PRINTER_ERROR;
			}
		else if(strcmp(next, "stall") == 0)
			mask |= COM_STALL;
		else if(strcmp(next, "exit") == 0)
			mask |= COM_EXIT;
		else
			fatal(PPREXIT_SYNTAX, _("Invalid category in %s: %s"), source, next);
		}
	gu_free(temp);
	return mask;
	} /* parse_commentary_option() */

/*
** These are the action routines which are called to process the
** options which ppr_getopt() isolates.  There are 2 passes made
** over the option list.  The purpose of the first pass is to find
** the -d switch so that the correct switchset may be processed
** before pass 2 begins.
**
** These functions do not iterate over the options lists themselves. Instead,
** we feed them the option character, such as 'd', and the option string such
** as "chipmunk".
**
** For long options, these routines are fed the short option character
** which cooresponds to the long option.  If a long option does not
** have a short form, then optchar is a code number.  These code
** numbers should all beyond the ASCII range.  We have chosen to make
** them 1000 and greater.
*/
static void doopt_pass1(int optchar, const char *optarg, const char *true_option)
	{
	switch(optchar)
		{
		case 'd':								/* destination queue */
			qentry.jobname.destname = optarg;
			break;
		}
	} /* end of doopt_pass1() */

static void doopt_pass2(int optchar, const char *optarg, const char *true_option)
	{
	switch(optchar)
		{
		case 'd':								/* destination queue */
			/* qentry.destname = optarg; */		/* do nothing since handled on first pass */
			break;

		case 'f':								/* for whom */
			if(! privileged())
				fatal(PPREXIT_SYNTAX, _("Non-privileged users may not use %s"), true_option);
			assert_ok_value(optarg, FALSE, FALSE, FALSE, true_option, TRUE);
			qentry.For = optarg;
			break;

		case 'u':								/* specify user */
			if(! privileged())
				fatal(PPREXIT_SYNTAX, _("Non-privileged users may not use %s"), true_option);
			qentry.user = optarg;
			break;

		case 'm':								/* responder */
			qentry.responder.name = optarg;
			break;

		case 'r':								/* responder address */
			qentry.responder.address = optarg;
			break;

		case 'b':								/* banner */
			if((qentry.do_banner=flag_option(optarg)) == -1)
				fatal(PPREXIT_SYNTAX, _("Invalid %s option"), true_option);
			break;

		case 't':								/* trailer */
			if((qentry.do_trailer=flag_option(optarg)) == -1)
				fatal(PPREXIT_SYNTAX, _("Invalid %s option"), true_option);
			break;

		case 'w':								/* set warning option */
			if(strcmp(optarg, "log") == 0)
				warning_log = TRUE;
			else if(gu_strcasecmp(optarg, "stderr") == 0)
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

		case 'D':								/* set default media */
			default_medium = optarg;
			break;

		case 'F':								/* add a feature */
			if(parse_feature_option(optarg) == -1)
				fatal(PPREXIT_SYNTAX, _("Invalid %s argument format"), true_option);
			break;

		case 'T':								/* force input type */
			if(infile_force_type(optarg))
				fatal(PPREXIT_SYNTAX, _("%s option specifies unrecognized file type \"%s\""), true_option, optarg);
			break;

		case 'q':								/* queue priority */
			{
			int sysv_priority;
			if((sysv_priority = atoi(optarg)) < 0 || sysv_priority > 39)
				fatal(PPREXIT_SYNTAX, _("%s option must be between 0 and 39"), true_option);
			/* Scale to IPP priority range of 1--100 inclusive. */
			qentry.spool_state.priority = (100 - (int)(sysv_priority * 2.55));
			}
			break;

		case 'B':								/* disable or enable automatic bin selects */
			if(gu_torf_setBOOL(&qentry.opts.binselect, optarg) == -1)
				fatal(PPREXIT_SYNTAX, _("%s must be followed by \"true\" or \"false\""), true_option);
			break;

		case 'N':								/* N-Up */
			if(strcmp(optarg, "noborders") == 0)		/* noborders option, */
				{								/* turn */
				qentry.N_Up.borders = FALSE;	/* borders off */
				}
			else
				{
				qentry.N_Up.N = atoi(optarg);
				if( (qentry.N_Up.N < 1) /* || (qentry.N_Up.N > 16) */ )
					fatal(PPREXIT_SYNTAX, _("%s option (N-Up) must be between 1 and 16"), true_option);
				}
			break;

		case 'n':								/* number of copies */
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

		case 'C':								/* --title, default title */
			/* Note that --title="" is allowed. */
			assert_ok_value(optarg, FALSE, TRUE, FALSE, true_option, TRUE);
			qentry.Title = optarg;
			break;

		case 'i':								/* routing instructions */
			assert_ok_value(optarg, FALSE, FALSE, FALSE, true_option, TRUE);
			qentry.Routing = optarg;
			break;

		case 'R':
			if(strcmp(optarg, "for") == 0)
				{
				if(!privileged())
					fatal(PPREXIT_SYNTAX, _("Only privileged users may use \"%s for\""), true_option);
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
			else if(strcmp(optarg, "duplex:simplex") == 0)
				{
				read_duplex = TRUE;
				current_duplex_enforce = TRUE;
				current_duplex = DUPLEX_NONE;
				}
			else if(strcmp(optarg, "duplex:duplex") == 0)
				{
				read_duplex=TRUE;
				current_duplex_enforce = TRUE;
				current_duplex = DUPLEX_DUPLEX_NOTUMBLE;
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
			if(gu_torf_setBOOL(&ignore_truncated,optarg) == -1)
				fatal(PPREXIT_SYNTAX, _("%s must be followed by \"true\" or \"false\""), true_option);
			break;

		case 'O':						/* overlay `Draft' notice */
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

		case 's':										/* signature option */
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

		case 'I':										/* insert switch set */
			fprintf(stderr, "%s: -I switch is obsolete\n", myname);
			break;

		case 'o':										/* Filter options */
			{
			const char *ptr;							/* eat leading space */
			ptr = &optarg[strspn(optarg, " \t")];		/* (It may cause trouble for exec_filter()) */

			if(!option_filter_options)					/* If this is the first -o switch, */
				{										/* Just make a copy of it. */
				option_filter_options = gu_strdup(ptr); /* (True, it is not necessary to make a copy, */
				}										/* but it makes the else clause simpler. */
			else
				{
				int len = strlen(option_filter_options) + 1 + strlen(ptr) + 1;
				char *ptr2 = (char*)gu_alloc(len, sizeof(char));
				snprintf(ptr2, len, "%s %s", option_filter_options, ptr);		/* concatenate */
				gu_free(option_filter_options);							/* free old one */
				option_filter_options = ptr2;							/* make new one current */
				}
			}
			break;

		case 'U':								/* unlink the input file */
			option_unlink_jobfile = TRUE;
			break;

		case 'Y':								/* Split the job */
			Y_switch(optarg);
			break;

		case 'G':								/* What to gab about */
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

		case 'Q':								/* TrueType query answer (for papd) */
			if(strcmp(optarg, "None") == 0)
				option_TrueTypeQuery = TT_NONE;
			else if(strcmp(optarg, "Accept68K") == 0)
				option_TrueTypeQuery = TT_ACCEPT68K;
			else if(strcmp(optarg, "Type42") == 0)
				option_TrueTypeQuery = TT_TYPE42;
			else
				fatal(PPREXIT_SYNTAX, _("Unrecognized %s option: \"%s\""), true_option, optarg);
			break;

		case 'H':								/* turn on a hack */
			if(parse_hack_option(optarg) == -1)
				fatal(PPREXIT_SYNTAX, _("Unrecognized %s option: %s"), true_option, optarg);
			break;

		case 1000:								/* --features */
			exit(option_features(qentry.jobname.destname));

		case 1002:								/* --ipp-priority */
			if((qentry.spool_state.priority = atoi(optarg)) < 1 || qentry.spool_state.priority > 100)
				fatal(PPREXIT_SYNTAX, _("%s option must be between 1 and 100"), true_option);
			break;

		case 1003:								/* --lpq-filename */
			qentry.lpqFileName = optarg;
			break;

		case 1004:								/* --hold */
			qentry.spool_state.status = STATUS_HELD;
			break;

		case 1005:								/* --responder-options */
			{
			if(!qentry.responder.options)
				{
				qentry.responder.options = gu_strdup(optarg);
				}				/* duplicate because we might free */
			else
				{
				int len = strlen(qentry.responder.options) + 1 + strlen(optarg) + 1;
				char *ptr = (char*)gu_alloc(len, sizeof(char));
				snprintf(ptr, len, "%s %s", qentry.responder.options, optarg);
				gu_free((char*)qentry.responder.options);		/* !!! */
				qentry.responder.options = ptr;
				}
			}
			break;

		case 1006:								/* --show-jobid */
			option_show_jobid = TRUE;
			break;

		case 1008:								/* --charge-to */
			if(! privileged())
				fatal(PPREXIT_SYNTAX, _("Non-privileged users may not use %s"), true_option);
			qentry.charge_to = optarg;
			break;

		case 1009:								/* --editps-level */
			if((option_editps_level = atoi(optarg)) < 1 || option_editps_level > 10)
				fatal(PPREXIT_SYNTAX, _("Argument for %s must be between 1 and 10"), true_option);
			break;

		case 1010:								/* --markup */
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

		case 1012:								/* --page-list */
			option_page_list = optarg;
			break;

		case 1015:								/* --strip-fontindex */
			if(gu_torf_setBOOL(&option_strip_fontindex, optarg) == -1)
				fatal(PPREXIT_SYNTAX, _("%s must be followed by \"true\" or \"false\""), true_option);
			break;

		case 1016:								/* --strip-printer */
			if(gu_torf_setBOOL(&qentry.StripPrinter, optarg) == -1)
				fatal(PPREXIT_SYNTAX, _("%s must be followed by \"true\" or \"false\""), true_option);
			break;

		case 1017:								/* --save */
			qentry.spool_state.flags |= JOB_FLAG_SAVE;
			break;

		case 1018:								/* --question */
			qentry.question = optarg;
			qentry.spool_state.flags |= JOB_FLAG_QUESTION_UNANSWERED;
			break;

		case 1019:								/* --ripopts */
			qentry.ripopts = optarg;
			break;

		case 1100:								/* --commentary */
			qentry.commentary = parse_commentary_option("--commentary", optarg);
			break;

		case 2000:								/* --print-id-to-fd (for IPP server) */
			option_print_id_to_fd = atoi(optarg);
			break;

		case 2001:								/* --skeleton-create */
			qentry.spool_state.status = STATUS_RECEIVING;
			break;

		case 2002:								/* --skeleton-jobid */
			qentry.jobname.id = option_skeleton_jobid = atoi(optarg);
			break;

		case 9000:								/* --help */
			help(stdout);
			exit(PPREXIT_OK);	/* not ppr_abort() */

		case 9001:								/* --version */
			puts(VERSION);
			puts(COPYRIGHT);
			puts(AUTHOR);
			exit(PPREXIT_OK);	/* not ppr_abort() */

		case '?':								/* Unrecognized switch */
			fatal(PPREXIT_SYNTAX, _("Unrecognized switch %s.  Try --help"), true_option);

		case ':':								/* argument required */
			fatal(PPREXIT_SYNTAX, _("The %s option requires an argument"), true_option);

		case '!':								/* bad aggreation */
			fatal(PPREXIT_SYNTAX, _("The %s switch takes an argument and so must stand alone"), true_option);

		case '-':								/* spurious argument */
			fatal(PPREXIT_SYNTAX, _("The %s switch does not take an argument"), true_option);

		default:								/* Missing case */
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
	char *real_filename = (char*)NULL;			/* the file we read from if not from stdin */
	struct gu_getopt_state getopt_state;		/* defined here because used to find filename */
	int x;										/* various very short term uses */
	char *ptr;									/* general use */

	/* Initialize internation messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	/* Set umask to prevent user cripling us. */
	umask(PPR_JOBS_UMASK);

	/* Set environment values such as PATH, IFS, and HOME
	   to useful and correct values. */
	set_ppr_env();

	/* Since this is a setuid and setgid program, the effective
	   IDs will be those of the ppr user and group.  The real ones
	   will be those of the user.  We memorize all of these so that
	   we can switch to the PPR's IDs to open files in PPR's
	   directories and to the user's IDs to open the file to be
	   printed.
	   */
	ppr_uid = geteuid();
	ppr_gid = getegid();
	user_uid = getuid();
	user_gid = getgid();

	/*
	** Make sure we are running setuid to the right user.
	** This check can be disabled for lame OSs such as Win95.
	*/
	#ifndef BROKEN_SETUID_BIT
	{
	struct passwd *pw;
	if(!(pw = getpwnam(USER_PPR)))
		fatal(PPREXIT_OTHERERR, _("getpwnam(\"%s\") failed"), USER_PPR);
	if(ppr_uid != pw->pw_uid)
		fatal(PPREXIT_OTHERERR, _("This program must be setuid and owned by \"%s\""), USER_PPR);
	if(ppr_gid != pw->pw_gid)
		fprintf(stderr, _("%s: warning: either account \"%s\" or this program has the wrong group ID.\n"), myname, USER_PPR);
	}
	#endif

	/*
	** Clear parts of the queue entry, fill in default values
	** elsewhere.  It is important that we do this right away
	** since one of the things we do is get our queue id.
	*/
	qentry.time = time((time_t*)NULL);					/* job submission time */
	qentry.jobname.destname = NULL;						/* name of printer or group */
	qentry.jobname.id = 0;								/* not assigned yet */
	qentry.jobname.subid = 0;							/* job fragment number (unused) */
	qentry.spool_state.priority = 50;					/* default priority */
	qentry.spool_state.sequence_number = qentry.time;	/* <--temporary hack */
	qentry.spool_state.status = STATUS_WAITING;			/* ready to print */
	qentry.spool_state.flags = 0;
	qentry.user = NULL;
	qentry.For = NULL;
	qentry.charge_to = NULL;
	qentry.Title = NULL;								/* start with no %%Title: line */
	qentry.Creator = NULL;								/* "%%Creator:" */
	qentry.Routing = NULL;								/* "%%Routing:" */
	qentry.lpqFileName = NULL;							/* filename for lpq */
	qentry.nmedia = 0;									/* no forms */
	qentry.do_banner = BANNER_DONTCARE;					/* don't care */
	qentry.do_trailer = BANNER_DONTCARE;				/* don't care */
	qentry.attr.DSClevel = 0.0;
	qentry.attr.DSC_job_type = NULL;
	qentry.attr.langlevel = 1;							/* default to PostScript level 1 */
	qentry.attr.pages = -1;								/* number of pages undetermined */
	qentry.attr.pagefactor = 1;
	qentry.attr.pageorder = PAGEORDER_ASCEND;			/* assume ascending order */
	qentry.attr.extensions = 0;							/* no Level 2 extensions */
	qentry.attr.orientation = ORIENTATION_UNKNOWN;		/* probably not landscape */
	qentry.attr.proofmode = PROOFMODE_SUBSTITUTE;		/* don't change this default (RBII p. 664-665) */
	qentry.attr.input_bytes = 0;						/* size before filtering if filtered */
	qentry.attr.postscript_bytes = 0;					/* size of PostScript */
	qentry.attr.parts = 1;								/* for now, assume one part */
	qentry.attr.docdata = CODES_UNKNOWN;				/* clear "%%DocumentData:" */
	qentry.opts.binselect = TRUE;						/* do auto bin select */
	qentry.opts.copies = -1;							/* unspecified number of copies */
	qentry.opts.collate = FALSE;						/* by default, don't collate */
	qentry.opts.keep_badfeatures = TRUE;				/* delete feature code not in PPD file */
	qentry.opts.hacks = HACK_DEFAULT_HACKS;				/* essential hacks? */
	qentry.N_Up.N = 1;									/* start with 1 Up */
	qentry.N_Up.borders = TRUE;							/* print borders when doing N-Up */
	qentry.N_Up.sigsheets = 0;							/* don't print signatures */
	qentry.N_Up.sigpart = SIG_BOTH;						/* print both sides of signature */
	qentry.N_Up.job_does_n_up = FALSE;
	qentry.draft_notice = NULL;							/* message to print diagonally */
	qentry.PassThruPDL = NULL;							/* default (means PostScript) */
	qentry.Filters = NULL;								/* default (means none) */
	qentry.PJL = NULL;
	qentry.StripPrinter = FALSE;
	qentry.page_list.mask = NULL;
	qentry.ripopts = NULL;

	/* Figure out what language the user is getting messages in and
	   attach its name to the job. */
	if(!(qentry.lc_messages = getenv("LC_MESSAGES")))
		qentry.lc_messages = getenv("LANG");

	/*
	** Use the real UID to look up the username and real name of the
	** person who is running this command.  These values will be the
	** defaults for certain settings.
	*/
	{
	struct passwd *pw;
	if(!(pw = getpwuid(user_uid)))
		fatal(PPREXIT_OTHERERR, _("getpwuid() fails to find your account"));
	if(!pw->pw_name || !pw->pw_gecos)
		fatal(PPREXIT_OTHERERR, "strange getpwuid() error, pw_name is NULL");
	user_pw_name = gu_strdup(pw->pw_name);
	user_pw_gecos = gu_strdup(pw->pw_gecos);
	}

	/*
	** Determine the default responder options.  The default responder is
	** "followme" which is a meta-responder which by default delegates 
	** to the responder called "write".
	**
	** Since a responder address may not have been specified with the -r switch,
	** we must supply a default value.  This is the name that the user logged in
	** as.  The login user name is the proper address for the default 
	** responder "followme" .  Notice that if the user used su to become a 
	** different user after loging in, the current user id will differ from 
	** the current user id.  In order for the responder to use the "write" 
	** command sucessfully, we must determine the login name.  That is why we
	** try the environment variables "LOGNAME" and "USER" before resorting to
   	** the name which cooresponds to the current user id.
	*/
	qentry.responder.name = "followme";
	{
	char *p;
	if((p = getenv("LOGNAME")) || (p = getenv("USER")))
		qentry.responder.address = gu_strdup(p);	/* copy because prune_env() deletes */
	else
		qentry.responder.address = qentry.user;
	}
	qentry.responder.options = NULL;

	/*
	** Look for default commentatary option in the environment.
	** Remember that getenv() returns NULL for variables that
	** are not found.
	*/
	qentry.commentary = 0;
	if((ptr = getenv("PPR_COMMENTARY")))
		qentry.commentary = parse_commentary_option("PPR_COMMENTARY", ptr);

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
	signal_interupting(SIGTERM, gallows_speach);		/* software termination */
	signal_interupting(SIGHUP, gallows_speach);			/* parent terminates */
	signal_interupting(SIGINT, gallows_speach);			/* control-C pressed */

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
	** doesn't work, use the system default destination.
	**
	** (We want to do this as soon as possible so that
	** messages which refer to the job id will look right.)
	*/
	if(!qentry.jobname.destname)
		{
		const char *ptr;
		if(!(ptr = getenv("PPRDEST")))
			if(!(ptr = getenv("PRINTER")))
				if(!(ptr = ppr_get_default()))
					ptr = "default";
		qentry.jobname.destname = ptr;
		}

	/*
	** Here is where we handle queue aliases.
	*/
	{
	const char *ptr;
	if((ptr = extract_forwhat()))
		qentry.jobname.destname = ptr;
	}

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
	if(qentry.jobname.destname[0] == '\0')
		fatal(PPREXIT_SYNTAX, _("Destination (printer or group) name is empty"));
	if(strpbrk(qentry.jobname.destname, DEST_DISALLOWED))
		fatal(PPREXIT_SYNTAX, _("Destination (printer or group) name contains a disallowed character"));
	if(strchr(DEST_DISALLOWED_LEADING, (int)qentry.jobname.destname[0]))
		fatal(PPREXIT_SYNTAX, _("Destination (printer or group) name starts with a disallowed character"));

	/*
	** --receiving-jobid
	** This option is used to submit the text of a job the skeleton of which was
	** previously created with the --receiving option.
	*/
	if(qentry.jobname.id)
		{
		char temp[MAX_PPR_PATH];
		FILE *f;
		int len = 128;
		char *line = NULL;
		int optchar;
		char *optname, *optarg;

		ppr_fnamef(temp, "%s/%s-%d.%d-cmdline", DATADIR, qentry.jobname.destname, qentry.jobname.id, qentry.jobname.subid);
		if(!(f = fopen(temp, "r")))
			fatal(PPREXIT_OTHERERR, _("can't open \"%s\", errno=%d (%s)"), temp, errno, gu_strerror(errno));

		while((line = gu_getline(line, &len, f)))
			{
			/* printf("line=\"%s\"\n", line); */
			optarg = NULL;
			gu_sscanf(line, "%d %S %T", &optchar, &optname, &optarg);
			doopt_pass2(optchar, optarg, optname);
			gu_free_if(optname);
			/* gu_free_if(optarg); */	/* doopt_pass2() will have saved pointers to this */
			}

		fclose(f);
		}

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

			ptr++;					/* move beyond equal sign */

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
	** If no --title switch was used, but a file name was used, make the file
	** name the default title.  The default title may be overridden by a
	** "%%Title:" line.	 (Note that if the input is stdin, real_filename will
	** be NULL.  Also, qentry.lpqFileName will be non-NULL only if the
	** --lpq-filename switch has been used.)
	*/
	if(!qentry.Title)
		if(!(qentry.Title = qentry.lpqFileName))
			qentry.Title = real_filename;

	/* Defaults for -u, -f, and --charge-to */
	if(!qentry.user)
		qentry.user = user_pw_name;
	if(!qentry.For)
		{
		if(qentry.user == user_pw_name)
			qentry.For = user_pw_gecos;
		else
			qentry.For = qentry.user;
		}
	if(!qentry.charge_to)
		qentry.charge_to = qentry.For;
	
	/*===========================================================
	** End of option parsing code
	===========================================================*/

	/*
	** Open the FIFO now so we will find out right away if the daemon is
	** running. In former versions, we did this sooner so that the responder
	** in pprd could be reached, but now that we have our own responder
	** launcher we can wait until the destination name is set (during 
	** command-line parsing) so that respond() will use the correct job name.
	*/
	if((FIFO = open_fifo(FIFO_NAME)) == (FILE*)NULL)
		{
		ppr_abort(PPREXIT_NOSPOOLER, (char*)NULL);
		}

	/*
	** We will soon create the job. Assign a queue ID.
	** Yes, if we fail to open the input file, a queue ID will be skipt.
	** By accepting this slightly odd behavior, we greatly simplify
	** the code.
	*/
	if(qentry.jobname.id == 0)
		get_next_id(&qentry.jobname);

	/*
	** If --skeleton-create was used: 
	**  1) Write the queue file
	**  2) Dump the command-line arguments to a file 
	**  3) Submit the job (such as it is)
	*/
	if(qentry.spool_state.status == STATUS_RECEIVING)
		{
		char temp[MAX_PPR_PATH];
		FILE *f;
		int optchar;

		if(write_queue_file(&qentry, FALSE) == -1)
			fatal(PPREXIT_DISKFULL, _("Disk full"));

		ppr_fnamef(temp, "%s/%s-%d.%d-cmdline", DATADIR, qentry.jobname.destname, qentry.jobname.id, qentry.jobname.subid);
		if(!(f = fopen(temp, "wb")))
			fatal(PPREXIT_OTHERERR, _("can't open \"%s\", errno=%d (%s)"), temp, errno, gu_strerror(errno));

		gu_getopt_init(&getopt_state, argc, argv, option_description_string, option_words);
		qentry.spool_state.status = STATUS_WAITING;

		while((optchar = ppr_getopt(&getopt_state)) != -1)
			{
			if(strcmp(getopt_state.name, "--skeleton-create") == 0)
				continue;
			if(strcmp(getopt_state.name, "--print-id-to-fd") == 0)
				continue;

			if(getopt_state.optarg)
				fprintf(f, "%d %s %s\n", optchar, getopt_state.name, getopt_state.optarg);
			else
				fprintf(f, "%d %s\n", optchar, getopt_state.name);
			}

		fclose(f);

		submit_job(&qentry, 0);

		goto skeleton_skip_point;
		}

	/*
	** Open the input file. Note that if no filename was specified on the
	** command line, real_filename will be NULL.
	*/
	if(infile_open(real_filename))		/* If input file is of zero length, */
		{								/* we have nothing to do. */
		warning(WARNING_SEVERE, _("Input file is empty, nothing to print"));
		goto zero_length_file;
		}

	/* ================== Input PostScript Processing Starts ===================== */

	/*
	** Set a flag so that if we abort suddenly or are killed and are able to
	** exit gracefully, we will know to remove the output files we are about
	** to create.
	*/
	spool_files_created = TRUE;

	/*
	** Open the queue files in which we will store the output.
	*/
	open_output();

	/*
	** Chew thru the whole input file.
	*/
	read_header_comments();		/* read header comments */

	if( read_prolog() )			/* If prolog (prolog, docdefaults, and docsetup) doesn't */
		{						/* doesn't stretch to EOF, %%EOF, or %%Trailer, */
		read_pages();			/* read the pages. */
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
	for(x = 0; in_getc() != EOF; x++)
		;

	if(x > 0)
		warning(x >= 50 ? WARNING_SEVERE : WARNING_PEEVE, ngettext("%d character follows EOF mark", "%d characters follow EOF mark", x), x);

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
	*/
	if( nest_level() )
		{
		warning(WARNING_SEVERE, _("Unclosed resource or resources"));
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
	if(qentry.N_Up.sigsheets)			/* if not zero (not disabled) */
		{
		current_duplex_enforce = TRUE;	/* try to turn duplex on or off */
		qentry.N_Up.borders = FALSE;	/* no borders around virtual pages */

		if(qentry.N_Up.N == 1)			/* if N-Up has not been set yet, */
			qentry.N_Up.N = 2;			/* set it now. */

		if(qentry.N_Up.sigpart == SIG_BOTH)						/* if printing both sides */
			{													/* use duplex */
			if(qentry.N_Up.N == 2)								/* if 2-Up, */
				current_duplex = DUPLEX_DUPLEX_TUMBLE;			/* use tumble duplex */
			else												/* for 4-Up, */
				current_duplex = DUPLEX_DUPLEX_NOTUMBLE;		/* use no-tumble duplex */
			}
		else									/* if printing only one side, */
			{									/* use simplex */
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
	if(current_duplex_enforce)			/* if we are to enforce our duplex finding, */
		{								/* then */
		switch(current_duplex)			/* insert appropriate code */
			{
			case DUPLEX_NONE:
				mark_feature_for_insertion("*Duplex None");
				delete_requirement("duplex");
				break;
			case DUPLEX_DUPLEX_NOTUMBLE:
				mark_feature_for_insertion("*Duplex DuplexNoTumble");
				delete_requirement("duplex");
				requirement(REQ_DOC, "duplex");
				break;
			case DUPLEX_DUPLEX_TUMBLE:
				mark_feature_for_insertion("*Duplex DuplexTumble");
				delete_requirement("duplex");
				requirement(REQ_DOC, "duplex(tumble)");
				break;
			case DUPLEX_SIMPLEX_TUMBLE:
				mark_feature_for_insertion("*Duplex SimplexTumble");
				delete_requirement("duplex");
				/* Adobe yet hasn't defined a requirement name for this. */
				/*requirement(REQ_DOC, "duplex(simplextumble)"); */
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

	/*
	** The variable "pagenumber" contains the number of "%%Page:"
	** comments encountered.  The variable "qentry.attr.pages" contains
	** the number of pages stated in the "%%Pages:" comment.  If there
	** was no "%%Pages:" comment, then it will still be set to -1.
	*/
	if(pagenumber > 0)
		{
		/* There is a "script" section. */
		qentry.attr.script = TRUE;

		if(qentry.attr.pages == -1)				/* If no "%%Pages:", */
			{									/* infer from actual page count. */
			warning(WARNING_SEVERE, _("No valid \"%%%%Pages:\" comment"));
			qentry.attr.pages = pagenumber;
			}
		else if(pagenumber > qentry.attr.pages)
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
	else		/* No "%%Page:" comments */
		{
		qentry.attr.pageorder = PAGEORDER_SPECIAL;		/* re-ordering not possible */
		qentry.attr.script = FALSE;						/* no "script" section */
		}

	/* If the number of pages is still unknown but this is an EPS file,
	 * then assume it is one since EPS files are by definition one
	 * page long.  (See RBII p. 712)
	 */
	if(qentry.attr.pages == -1 && qentry.attr.DSC_job_type && lmatch(qentry.attr.DSC_job_type, "EPSF-"))
		qentry.attr.pages = 1;

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
	if(split_job(&qentry))		/* If it does do a split, */
		{						/* remove what would have been */
		file_cleanup();			/* the files for a monolithic job. */
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
		** Note that fatal() calls file_cleanup(), so the queue file should be
		** removed.
		*/
		if(write_queue_file(&qentry, option_skeleton_jobid ? TRUE : FALSE) == -1)
			fatal(PPREXIT_DISKFULL, _("Disk full"));

		/*
		** Tell PPRD that there is a new job to be entered into the queue.
		*/
		submit_job(&qentry, 0);
		}

	skeleton_skip_point:

	/*
	** We are done with the communications channel to
	** the spooler, we can close it now.
	*/
	fclose(FIFO);

	/*
	** The IPP server will have invoked this program with the option --print-id-to-fd=3.
	*/
	if(option_print_id_to_fd != -1)
		{
		char temp[10];
		snprintf(temp, sizeof(temp), "%d\n", qentry.jobname.id);
		if(write(option_print_id_to_fd, temp, strlen(temp)) < 0)
			error("ppr: failed to write job ID to fd %d", option_print_id_to_fd);
		}

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

