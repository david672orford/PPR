/*
** mouse:~ppr/src/ppop/ppop.c
** Copyright 1995--2010, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 1 June 2010.
*/

/*
** Operators utility for PostScript page printers.  It allows the queue
** to be viewed, jobs to be canceled, jobs to be held, printers to
** be started and stopped, and much more.
*/

#include "config.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>
#include <pwd.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "ppop.h"
#include "util_exits.h"
#include "dispatch.h"
#include "dispatch_table.h"
#include "version.h"

const char myname[] = "ppop";

/* Command line options */
gu_boolean			opt_verbose = FALSE;
int 				opt_machine_readable = FALSE;
static char			*opt_user = NULL;	
static const char	*opt_magic_cookie = NULL;
int					opt_arrest_interest_interval = -1;

/*
** Handle fatal errors.
** Print a message and exit.
*/
void fatal(int exitval, const char message[], ... )
	{
	va_list va;

	if(opt_machine_readable)
		gu_utf8_fputs("*FATAL\t", stderr);
	else
		gu_utf8_fputs(_("Fatal: "), stderr);

	va_start(va, message);
	gu_utf8_vfprintf(stderr, message, va);
	va_end(va);

	gu_utf8_fputs("\n", stderr);

	exit(exitval);
	} /* end of fatal() */

/*
** Non-fatal errors.
** (This function is called by the library
** function read_queue_file().)
*/
void error(const char message[], ... )
	{
	va_list va;
	va_start(va, message);
	gu_utf8_fputs(_("Error: "), stderr);
	gu_utf8_vfprintf(stderr, message,va);
	gu_utf8_fputs("\n", stderr);
	va_end(va);
	} /* end of error() */

/*
** This is used in machine-readable output (which is often
** tab-delimited).  This function replaces each tab with
** a single space.
*/
void puts_detabbed(const char *string)
	{
	const char *p = string;
	wchar_t wc;
	while((wc = gu_utf8_sgetwc(&p)))
		{
		if(wc != '\t')
			gu_putwc(wc);
		else
			gu_putwc(' ');
		}
	}

/*======================================================================
** IPC functions
======================================================================*/

static sigset_t oset;						/* old signal set */
static FILE *FIFO = (FILE*)NULL;			/* channel to pprd */
static volatile gu_boolean sigcaught;		/* flag which is set when SIGUSR1 received */
static volatile gu_boolean timeout;			/* set when SIGALRM is caught */
static char temp_file_name[MAX_PPR_PATH];	/* name of temporary file we get answer in */
static FILE *reply_file;					/* streams library thing for open reply file */
static int pprd_retcode;

/*
** Handler for signal from pprd (user signal 1).
** We receive this signal when pprd wants us to know
** that the reply file is ready.
*/
static void user_sighandler(int sig)
	{
	sigcaught = TRUE;
	}

/*
** This SIGALRM handler is used to limit the time we will wait
** for SIGUSR1 from pprd.
*/
static void alarm_sighandler(int sig)
	{
	timeout = TRUE;
	}

/*
** Get ready to communicate with the spooler daemon. This involves getting
** ready for a SIGUSR1 from pprd.  To do this, we clear the signal received
** flag and block the signal.  (We will unblock it later.)
*/
FILE *get_ready(void)
	{
	int fifo;
	sigset_t set;				/* storage for set containing SIGUSR1 */

	if(!FIFO)
		{
		if((fifo = open(FIFO_NAME, (O_WRONLY | O_NONBLOCK))) == -1)
			fatal(EXIT_NOSPOOLER, _("can't open FIFO, pprd is probably not running"));
		FIFO = fdopen(fifo, "w");
		}

	sigcaught = FALSE;

	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	sigaddset(&set, SIGALRM);
	sigprocmask(SIG_BLOCK, &set, &oset);

	/* Before the command we must give the "address"
	   to reply to.  This address is our PID. */
	fprintf(FIFO, "%ld ", (long int)getpid());

	return FIFO;
	} /* end of get_ready() */

/*
** Wait to receive SIGUSR1 from pprd.
** (This function uses oset which is defined above.)
** If pprd reports an error, we return a NULL file
** pointer and the caller must call print_reply();
** otherwise we will return a pointer to the reply file.
*/
FILE *wait_for_pprd(int do_timeout)
	{
	const char *function = "wait_for_pprd";

	/* Start a timeout period */
	timeout = FALSE;
	if(do_timeout)
		alarm(60);

	/* wait for pprd to fill the file */
	while(!sigcaught && !timeout)
		sigsuspend(&oset);

	/* Cancel the timeout */
	if(do_timeout)
		alarm(0);

	/* restore the old signal mask */
	sigprocmask(SIG_SETMASK, &oset, (sigset_t*)NULL);

	if(timeout)
		{
		gu_utf8_fprintf(stderr, _("%s: timeout waiting for response"), myname);
		reply_file = (FILE*)NULL;
		return (FILE*)NULL;
		}

	if((reply_file = fopen(temp_file_name, "r")) == (FILE*)NULL)
		{
		gu_utf8_fprintf(stderr, "%s(): couldn't open reply file \"%s\", errno=%d (%s)\n", function, temp_file_name, errno, gu_strerror(errno) );
		reply_file = (FILE*)NULL;
		return (FILE*)NULL;
		}

	/* now that file is open, we can dispense with the name */
	if(unlink(temp_file_name) < 0)
		gu_utf8_fprintf(stderr, "%s(): unlink(\"%s\") failed, errno=%d (%s)\n", function, temp_file_name, errno, gu_strerror(errno));

	/* Read the code which summarizes the result of the operation. */
	{
	char reply[8];
	if(!fgets(reply, sizeof(reply), reply_file) || gu_sscanf(reply, "%d", &pprd_retcode) != 1)
		{
		gu_utf8_fprintf(stderr, "%s(): return code missing in reply file.\n", function);
		fclose(reply_file);
		reply_file = (FILE*)NULL;
		return (FILE*)NULL;
		}
	}

	/* EXIT_OK_DATA indicates data which the caller should
	   read using the returned file handle.  Otherwise,
	   print_reply() should be used.  */
	return pprd_retcode == EXIT_OK_DATA ? reply_file : (FILE*)NULL ;
	} /* end of wait_for_pprd() */

/*
** Print a plain text reply from pprd.  Returns the value at the start of the 
** plain text reply (which we get from a module-scope variable).
** If an internal error occurs, return -1.
*/
int print_reply(void)
	{
	char *line = NULL;
	int line_len = 80;

	/* reply_file will probably only be NULL if we
	   caught an error in wait_for_pprd(). */
	if(!reply_file)
		return EXIT_INTERNAL;

	/* print the rest of the text */
	while((line = gu_getline(line, &line_len, reply_file)))
		{
		gu_utf8_putline(line);
		}

	fclose(reply_file);

	/* return the code from the top of the reply */
	return pprd_retcode;
	} /* end of print_reply() */

/*====================================================================
** Job and destionation name parsing functions.
**==================================================================*/

/*
** Function for breaking up a queue file name into its constituent parts.
** If it is not a valid job name, this function returns NULL.
** It deliberately leaks memory.  We rely on the pools mechanism to clean 
** it up.
*/
const struct Jobname *parse_jobname(const char *jobname)
	{
	struct Jobname *job;
	size_t len;
	const char *ptr;
	int c;

	job = gu_alloc(1, sizeof(struct Jobname));
	ptr = jobname;

	/*
	** Find the full extent of the destination name.  Keep in mind that
	** embedded hyphens are now allowed.  Note that this is not perfect
	** because things such as "busprn-2" are still ambiguous.  It one 
	** want to say "all jobs on busprn-2", one must write "busprn-2-*".
	*/
	for(len=0; TRUE; len++)
		{
		len += strcspn(&ptr[len], "-");		/* get length until next hyphen */

		if(ptr[len] == '\0')				/* check for end of string */
			break;

		if(ptr[len + 1] == '*')				/* if explicit wildcard queue id as in "busprn-2-*", */
			break;

		if((c = ptr[len+1+strspn(&ptr[len+1], "0123456789")]) == '.' || c=='\0')
			break;
		}

	if(len == 0)
		{
		gu_utf8_fputs(_("Destination (printer or group) name is empty.\n"), stderr);
		return NULL;
		}

	/*
	** Copy the destination (printer or group) name into the structure.
	*/
	job->destname = gu_strndup(ptr, len);
	ptr += len;

	/*
	** If there is a hyphen, read the jobid after the hyphen.
	*/
	job->id = WILDCARD_JOBID;
	job->subid = WILDCARD_SUBID;
	if(*ptr == '-')
		{
		ptr++;

		if(*ptr == '*')
			{
			ptr++;
			}
		else
			{
			job->id = atoi(ptr);
			ptr += strspn(ptr, "0123456789");
			}

		/*
		** If a dot comes next, read the number after it;
		** otherwise, use the subid zero.
		*/
		if(*ptr == '.')
			{
			ptr++;
			job->subid = atoi(ptr);
			ptr += strspn(ptr, "0123456789");
			}
		}

	/*
	** If there is anything left, it is an error.
	*/
	if(*ptr)
		{
		gu_utf8_fprintf(stderr, _("Destination or job name \"%s\" is invalid.\n"), jobname);
		return NULL;
		}

	job->destname = parse_destname(job->destname, TRUE);
	
	return job;
	} /* end of parse_jobname() */

/*
** Validate an destination name, possibly resolving aliases.  If the name is
** valid, it will be returned.  If it is an alias, then the name to which
** it resolves will be returned in allocated memory.
*/
const char *parse_destname(const char *destname, gu_boolean resolve_aliases)
	{
	if(strpbrk(destname, DEST_DISALLOWED))
		{
		gu_utf8_fprintf(stderr, _("Destination name \"%s\" contains a disallowed character.\n"), destname);
		return NULL;
		}

	if(strchr(DEST_DISALLOWED_LEADING, (int)destname[0]))
		{
		gu_utf8_fprintf(stderr, _("Destination name \"%s\" begins with a disallowed character.\n"), destname);
		return NULL;
		}

	if(resolve_aliases && strcmp(destname, "all") != 0)
		{
		char fname[MAX_PPR_PATH];
		FILE *f;
		char *line = NULL;
		int linelen = 128;
	
		ppr_fnamef(fname, "%s/%s", ALIASCONF, destname);
		if((f = fopen(fname, "r")))
			{
			while((line = gu_getline(line, &linelen, f)))
				{
				if(gu_sscanf(line, "ForWhat: %S", &destname) == 1)
					break;
				}
			fclose(f);
			if(line)
				gu_free(line);
			else
				{
				gu_utf8_fprintf(stderr, _("The alias \"%s\" does not have a forwhat value.\n"), destname);
				return NULL;
				}
			}
		}

	return destname;
	} /* end of parse_destname() */

/*====================================================================
** Security functions
**==================================================================*/

/*
** Is the user privileged?  If the user identity has been changed
** (by the --su switch) since last time this function was called,
** the answer is found again, otherwise a cached answer is returned.
*/
static gu_boolean privileged(void)
	{
	static gu_boolean answer = FALSE;
	static char *answer_username = NULL;

	if(!answer_username || strcmp(opt_user, answer_username) != 0)
		{
		if(answer_username)
			gu_free(answer_username);
		answer_username = gu_strdup(opt_user);
		answer = user_acl_allows(opt_user, "ppop");
		}

	return answer;
	} /* end of privileged() */

/*
** Return zero if the user has the operator privledge,
** if not, print an error message and return -1.
**
** This function is used to make sure it is ok to go ahead
** with a command that requires PPR operator privledge.
**
** Note that this function is affected by the --su switch.
*/
gu_boolean assert_am_operator(void)
	{
	if(privileged())
		{
		return TRUE;
		}
	else
		{
		gu_utf8_fputs(_("You are not allowed to perform the requested\n"
			"operation because you are not a PPR operator.\n"), stderr);
		return FALSE;
		}
	} /* end of assert_am_operator() */

/*
** This function returns TRUE if the current user has permission to touch
** the specified job.  If he does not, an error message is printed and
** FALSE is returned.
*/
gu_boolean job_permission_check(const struct Jobname *job)
	{
	char *job_username = NULL;
	char *job_magic_cookie = NULL;
	int retval = FALSE;

	/* start of exception-handling block */
	do {
		/*
		** If the user (possibily the user specified by the -u switch) and
		** is not limiting his authority with the --magic-cookie switch, then
		** we can bail out right now because operators can manipulate any
		** job.  This will save us the time and trouble of opening the
		** queue file to figure out whose job it is.
		*/
		if(privileged() && !opt_magic_cookie)
			{
			retval = TRUE;
			break;
			}
	
		/*
		** In this block, we open the queue file and extract the information
		** from the "User:" line.
		**
		** If we can't open it, just say it is ok to manipulate the job.  This is
		** because, for non-existent jobs we want to user to be told it does not
		** exist, not that he can't touch it!
		**
		** Yes, this is a race condition.  Any practical way to exploit it?
		*/
		{
		char fname[MAX_PPR_PATH];
		FILE *f;
		char *line = NULL;
		int line_space = 80;
	
		ppr_fnamef(fname, "%s/%s-%d.%d",
			QUEUEDIR,
			job->destname,
			job->id,
			job->subid != WILDCARD_SUBID ? job->subid : 0
			);
		if((f = fopen(fname, "r")) == (FILE*)NULL)
			{
			if(errno == ENOENT)		 /* See note above. */
				{
				break;
				}
			gu_utf8_fprintf(stderr, X_("Can't open queue file \"%s\" to verify access rights, errno=%d (%s).\n"), fname, errno, gu_strerror(errno));
			break;
			}
	
		/* Read the queue file and find the "User:" line. */
		while((line = gu_getline(line, &line_space, f)))
			{
			if(gu_sscanf(line, "User: %S", &job_username) == 1)
				continue;
			if(gu_sscanf(line, "MagicCookie: %S", &job_magic_cookie) == 1)
				continue;
			}
	
		/* Close the queue file. */
		if(line)
			gu_free(line);
		fclose(f);
		}
	
		/* Check to see that we got a "User:" line: */
		if(!job_username)
			{
			gu_utf8_fputs("Queue file error, no \"User:\" line.\n", stderr);
			break;
			}
	
		if(!privileged() && !username_match(job_username, opt_user))
			{
			gu_utf8_fprintf(stderr,
				_("You may not manipulate the job \"%s\" because it\n"
				  "does not belong to the user \"%s\".\n"),
					jobid(job->destname, job->id, job->subid),
					opt_user
				);
			break;
			}
		if(opt_magic_cookie && strcmp(opt_magic_cookie, job_magic_cookie) != 0)
			{
			gu_utf8_fputs("Magic cookie doesn't match.\n", stderr);
			break;
			}
		else
			{
			retval = TRUE;
			}

		/* end of exception-handling block */
		} while(FALSE);

	if(job_username)
		gu_free(job_username);
	if(job_magic_cookie)
		gu_free(job_magic_cookie);

	return retval;
	} /* end of job_permission_check() */

/*
** Return TRUE if the job matches the current user id
** Return FALSE if it does not.  Notice that operator
** privilege plays no part in this.  Other than that, the
** criteria should be the same as for job_permission_check().
*/
int is_my_job(const struct QEntry *qentry, const struct QEntryFile *qentryfile)
	{
	if(!username_match(qentryfile->user, opt_user))
		return TRUE;
	return FALSE;
	} /* end of is_my_job() */

/*=========================================================================
** Main function and associated functions, dispatch command
** to proper command handler.
=========================================================================*/
#if 0
static int main_help(FILE *out)
	{
	int i;
	const char *help_lines[] =
		{
		N_("Destination commands:"),
			N_("ppop destination {<destination>, all}"),
			N_("    Abbreviation: ppop dest {<destination>, all}"),
			N_("ppop destination-comment {<destination>, all}"),
			N_("    Abbreviation: ppop ldest {<destination>, all}"),
			N_("ppop destination-comment-address {<destination>, all}"),
			N_("ppop accept <destination>"),
			N_("ppop reject <destination>"),
		N_("Print job commands:"),
			N_("ppop list {<destination>, <job>, all} ..."),
			N_("ppop details {<destination>, <job>, all} ..."),
			N_("ppop lpq {<destination>, <job>, all} [<user>] [<id>] ..."),
			N_("ppop qquery {<destination>, <job>, all} <field name 1> ..."),
			N_("ppop move {<job>, <old_destination>} <new_destination>"),
			N_("ppop hold <job> ..."),
			N_("ppop release <job> ..."),
			N_("ppop [s]cancel {<job>, <destination>, all} ..."),
			N_("ppop [s]purge {<destination>, all} ..."),
			N_("ppop [s]cancel-active {<destination>, all} ..."),
			N_("ppop [s]cancel-my-active {<destination>, all} ..."),
			N_("ppop clean <destination> ..."),
			N_("ppop rush <job> ..."),
			N_("ppop last <job> ..."),
			N_("ppop log <job>"),
			N_("ppop progress <job>"),
			N_("ppop modify <job> <name>=<value> ..."),
		N_("Printer commands:"),
			N_("ppop status {<destination>, all} ..."),
			N_("ppop start <printer> ..."),
			N_("ppop halt <printer> ..."),
			N_("ppop stop <printer> ..."),
			N_("ppop wstop <printer>"),
			N_("ppop message <printer>"),
			N_("ppop alerts <printer>"),
		N_("Media commands:"),
			N_("ppop media {<destination>, all}"),
			N_("ppop mount <printer> <bin> <medium>"),
		NULL
		};
			
	for(i = 0; help_lines[i]; i++)
		{
		const char *p = help_lines[i];
		const char *pxlate = gettext(p);

		if(!lmatch(p, "ppop "))		/* if is a heading, */
			{
			if(i == 0)
				gu_fputwc('\n', out);
			gu_utf8_fprintf(out, "%s\n", pxlate);
			}
		else
			{
			gu_utf8_fprintf(out, "    %s\n", pxlate);
			}
		}
	return EXIT_OK;
	} /* end of main_help() */
#endif

#if 0
/*
** Examine the command and run the correct proceedure.
** Return the value returned by the proceedure function.
** If the command is unknown, return -1.
*/
static int dispatch(char *argv[])
	{
	int retcode = -1;
	void *my_pool = gu_pool_push(gu_pool_new());

	/*
	** Since these are used by other programs we
	** put them first so they will be the fastest.
	*/
	if(strcmp(argv[0],"qquery") == 0)
		return ppop_qquery(&argv[1]);
	else if(strcmp(argv[0],"message") == 0)
		return ppop_message(&argv[1]);
	else if(strcmp(argv[0],"progress") == 0)
		return ppop_progress(&argv[1]);

	else if(strcmp(argv[0],"list") == 0)
		return ppop_list(&argv[1], 0);
	else if(strcmp(argv[0],"nhlist") == 0)		/* For lprsrv, no-header-list */
		return ppop_list(&argv[1], 1);
	else if(gu_strcasecmp(argv[0],"lpq") == 0)
		return ppop_lpq(&argv[1]);
	else if(strcmp(argv[0],"mount") == 0)
		return ppop_mount(&argv[1]);
	else if(strcmp(argv[0],"media") == 0)
		return ppop_media(&argv[1]);
	else if(strcmp(argv[0],"start") == 0)
		return ppop_start_stop_wstop_halt(&argv[1], 0);
	else if(strcmp(argv[0],"stop") == 0)
		return ppop_start_stop_wstop_halt(&argv[1], 1);
	else if(strcmp(argv[0],"wstop") == 0)
		return ppop_start_stop_wstop_halt(&argv[1], 2);
	else if(strcmp(argv[0], "halt") == 0)
		return ppop_start_stop_wstop_halt(&argv[1], 3);
	else if(strcmp(argv[0], "cancel") == 0)
		return ppop_cancel(&argv[1], 1);
	else if(strcmp(argv[0], "scancel") == 0)
		return ppop_cancel(&argv[1], 0);
	else if(strcmp(argv[0], "purge") == 0)
		return ppop_purge(&argv[1], 1);
	else if(strcmp(argv[0], "spurge") == 0)
		return ppop_purge(&argv[1], 0);
	else if(strcmp(argv[0], "clean") == 0)
		return ppop_clean(&argv[1]);
	else if(strcmp(argv[0], "cancel-active") == 0)
		return ppop_cancel_active(&argv[1], FALSE, 1);
	else if(strcmp(argv[0], "scancel-active") == 0)
		return ppop_cancel_active(&argv[1], FALSE, 0);
	else if(strcmp(argv[0], "cancel-my-active") == 0)
		return ppop_cancel_active(&argv[1], TRUE, 1);
	else if(strcmp(argv[0], "scancel-my-active") == 0)
		return ppop_cancel_active(&argv[1], TRUE, 0);
	else if(strcmp(argv[0], "move") == 0)
		return ppop_move(&argv[1]);
	else if(strcmp(argv[0], "rush") == 0)
		return ppop_rush(&argv[1], 0);
	else if(strcmp(argv[0], "last") == 0)
		return ppop_rush(&argv[1], 10000);
	else if(strcmp(argv[0], "hold") == 0)
		return ppop_hold_release(&argv[1], FALSE);
	else if(strcmp(argv[0], "release") == 0)
		return ppop_hold_release(&argv[1], TRUE);
	else if(strcmp(argv[0], "status") == 0)
		return ppop_status(&argv[1]);
	else if(strcmp(argv[0], "accept") == 0)
		return ppop_accept_reject(&argv[1], FALSE);
	else if(strcmp(argv[0], "reject") == 0)
		return ppop_accept_reject(&argv[1], TRUE);
	else if(strcmp(argv[0], "destination") == 0 || strcmp(argv[0],"dest") == 0)
		return ppop_destination(&argv[1], 0);
	else if(strcmp(argv[0], "destination-comment") == 0 || strcmp(argv[0],"ldest") == 0)
		return ppop_destination(&argv[1], 1);
	else if(strcmp(argv[0], "destination-comment-address") == 0)
		return ppop_destination(&argv[1], 2);
	else if(strcmp(argv[0], "details") == 0)
		return ppop_details(&argv[1]);
	else if(strcmp(argv[0], "alerts") == 0)
		return ppop_alerts(&argv[1]);
	else if(strcmp(argv[0], "log") == 0)
		return ppop_log(&argv[1]);
	else if(strcmp(argv[0], "modify") == 0)
		return ppop_modify(&argv[1]);
	else if(strcmp(argv[0], "help") == 0)
		return main_help(stdout);

	gu_pool_free(gu_pool_pop(my_pool));
	return retcode;
	} /* end of dispatch() */
#endif

/*
** Handler for sigpipe.
*/
static void pipe_sighandler(int sig)
	{
	gu_utf8_fputs(_("Spooler has shut down.\n"), stderr);

	if(opt_machine_readable)
		gu_utf8_fprintf(stderr, "*DONE %d\n", EXIT_NOSPOOLER);

	exit(EXIT_NOSPOOLER);
	} /* end of pipe_sighandler() */

/*
** --help
*/
static void help(void)
	{
	int i;
	const char *switch_list[] =
		{
		N_("-M\tselect machine-readable output"),
		N_("--machine-readable\tsame as -M"),
		N_("-A <seconds>\thide arrested jobs older than <seconds>"),
		N_("--arrest-interest-time=<seconds>\tsame as -A"),
		N_("-u <user>\tcheck access as if run by <user>"),
		N_("--user <user>\tsame as -u"),
		N_("--magic-cookie <string>\tpresent job access token"),
		N_("--verbose\tprint more information"),
		N_("--version\tprint the PPR version information"),
		N_("--help\tprint this help message"),
		NULL
		};

	gu_utf8_putline(_("Valid switches:\n"));
	for(i = 0; switch_list[i]; i++)
		{
		const char *p = gettext(switch_list[i]);
		int to_tab = strcspn(p, "\t");
		gu_utf8_printf("    %-35.*s %s\n",
			to_tab, p,
			p[to_tab] == '\t' ? &p[to_tab + 1] : ""
			);
		}

	gu_putwc('\n');

	{
	const char *args[] = {"help", NULL};
	dispatch(myname, args);
	}

	gu_utf8_printf(
		_(	"The %s manpage may be viewed by entering this command at a shell prompt:\n"
			"    ppdoc %s\n"
			),
		"ppop(1)",
		"ppop"
		);
	} /* end of help() */

/*
** Command line options:
*/
static const char *option_chars = "MA:u:";
static const struct gu_getopt_opt option_words[] =
	{
	{"machine-readable", 'M', FALSE},
	{"arrest-interest-interval", 'A', TRUE},
	{"user", 'u', TRUE},
	{"help", 1000, FALSE},
	{"version", 1001, FALSE},
	{"verbose", 1003, FALSE},
	{"magic-cookie", 1004, TRUE},
	{(char*)NULL, 0, FALSE}
	} ;

/*
** main function
** If there is a command on the invokation line, execute it,
** otherwise, go into interactive mode
*/
int main(int argc, char *argv[])
	{
	int result;			/* value to return */
	int optchar;		/* for ppr_getopt() */
	struct gu_getopt_state getopt_state;

	/* Initialize internation messages library. */
	gu_locale_init(argc, argv, PACKAGE, LOCALEDIR);

	/* paranoia */
	umask(PPR_UMASK);

	/* Figure out the user's name and make it the initial value for opt_user. */
	{
	struct passwd *pw;
	uid_t uid = getuid();
	if(!(pw = getpwuid(uid)))
		{
		gu_utf8_fprintf(stderr, "%s: getpwuid(%ld) failed, errno=%d (%s)\n", myname, (long)uid, errno, gu_strerror(errno));
		return EXIT_INTERNAL;
		}
	opt_user = gu_strdup(pw->pw_name);
	dispatch_set_user(NULL, pw->pw_name);
	}

	/* Parse the options. */
	gu_getopt_init(&getopt_state, argc, argv, option_chars, option_words);
	while((optchar = ppr_getopt(&getopt_state)) != -1)
		{
		switch(optchar)
			{
			case 'M':					/* -M or --machine-readable */
				opt_machine_readable = TRUE;
				/* Send error messages to stdout. */
				/*stderr = stdout;*/
				dup2(1,2);
				break;

			case 'A':					/* -A or --arrest-interest-interval */
				opt_arrest_interest_interval = atoi(getopt_state.optarg);
				break;

			case 'u':
				if(dispatch_set_user("ppop", getopt_state.optarg) == -1)
					{
					gu_utf8_fprintf(stderr, _("%s: you are not allowed to use --user\n"), myname);
					exit(EXIT_DENIED);
					}
				opt_user = getopt_state.optarg;
				break;

			case 1000:					/* --help */
				help();
				exit(EXIT_OK);

			case 1001:					/* --version */
				if(opt_machine_readable)
					{
					gu_utf8_putline(SHORT_VERSION);
					}
				else
					{
					gu_utf8_putline(VERSION);
					gu_utf8_putline(COPYRIGHT);
					gu_utf8_putline(AUTHOR);
					}
				exit(EXIT_OK);

			case 1003:					/* --verbose */
				opt_verbose = TRUE;
				break;

			case 1004:					/* --magic-cookie */
				opt_magic_cookie = getopt_state.optarg;
				break;

			default:
				gu_getopt_default(myname, optchar, &getopt_state, stderr);

				if(optchar == '?')
					help();

				exit(EXIT_SYNTAX);
				break;
			}
		}

	/*
	 * Change to the home directory of PPR.
	 * Why do we still do this?
	 */ 
	/*chdir(LIBDIR);*/

	/*
	** Install a SIGPIPE handler so we can produce an intelligible message
	** when we try to send a command over the FIFO when pprd is not running.
	*/
	signal(SIGPIPE, pipe_sighandler);

	/*
	** Install handler for SIGUSR1.  PPRD will send
	** us SIGUSR1 when the reply is ready.
	*/
	signal(SIGUSR1, user_sighandler);

	/*
	** This handles timeouts waiting for SIGUSR1.
	*/
	signal(SIGALRM, alarm_sighandler);

	/*
	** Construct the name of the temporary file.  We will
	** use the same temporary file over and over again.
	*/
	ppr_fnamef(temp_file_name, "%s/ppr-ppop-%ld", TEMPDIR, (long)getpid());

	/*
	** Clear the signal mask.  There are reports of certain
	** Perl ports running ppop with SIGUSR1 blocked.
	*/
	{
	sigset_t nset;
	sigemptyset(&nset);
	sigaddset(&nset, SIGCHLD);
	sigprocmask(SIG_BLOCK, &nset, NULL);
	}

	/*
	** If no subcommand, go interactive, otherwise, execute commmand.
	*/
	if(getopt_state.optind < argc)
		result = dispatch(myname, (const char **)&argv[getopt_state.optind]);
	else
		result = dispatch_interactive(myname, _("PPOP, Page Printer Operator's utility"), "ppop>", opt_machine_readable);

	/* Clean up by closing the FIFOs which may have
	   been used to communicate with pprd or rpprd. */
	if(FIFO)
		fclose(FIFO);

	/* Exit with the result of the last command. */
	return result;
	} /* end of main() */

/* end of file */

