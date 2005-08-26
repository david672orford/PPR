/*
** mouse:~ppr/src/ppop/ppop.c
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
** Last modified 25 August 2005.
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
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "pprd.h"
#include "ppop.h"
#include "util_exits.h"
#include "version.h"

const char myname[] = "ppop";

/* Command line options */
gu_boolean			opt_verbose = FALSE;
int 				opt_machine_readable = FALSE;
static char			*opt_user = NULL;	
static const char	*opt_magic_cookie = NULL;
int					opt_arrest_interest_interval = -1;

/* File to which to send errors, generally equal to stderr,
 * but set to stdout when -M is used.
 */ 
FILE *errors;

/*
** Handle fatal errors.
** Print a message and exit.
*/
void fatal(int exitval, const char message[], ... )
	{
	va_list va;

	if(opt_machine_readable)
		fputs("*FATAL\t", errors);
	else
		fputs(_("Fatal: "), errors);

	va_start(va, message);
	vfprintf(errors, message, va);
	va_end(va);

	fputc('\n', errors);

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
	fprintf(errors, _("Error: "));
	vfprintf(errors, message,va);
	fprintf(errors, "\n");
	va_end(va);
	} /* end of error() */

/*
** This is used in machine-readable output (which is often
** tab-delimited).  This function replaces each tab with
** a single space.
*/
void puts_detabbed(const char *string)
	{
	for( ; *string; string++)
		{
		if(*string != '\t')
			fputc(*string, stdout);
		else
			fputc(' ', stdout);
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
** for SIGUSR1 from pprd or rpprd.
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

	/* Are we using a real FIFO or just an append to a file? */
	#ifdef HAVE_MKFIFO
	#define FIFO_OPEN_FLAGS (O_WRONLY | O_NONBLOCK)
	#else
	#define FIFO_OPEN_FLAGS (O_WRONLY | O_APPEND)
	#endif

	if(!FIFO)
		{
		if((fifo = open(FIFO_NAME, FIFO_OPEN_FLAGS)) < 0)
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
		fprintf(errors, _("%s: timeout waiting for response"), myname);
		reply_file = (FILE*)NULL;
		return (FILE*)NULL;
		}

	if((reply_file = fopen(temp_file_name, "r")) == (FILE*)NULL)
		{
		fprintf(errors, "%s(): couldn't open reply file \"%s\", errno=%d (%s)\n", function, temp_file_name, errno, gu_strerror(errno) );
		reply_file = (FILE*)NULL;
		return (FILE*)NULL;
		}

	/* now that file is open, we can dispense with the name */
	if(unlink(temp_file_name) < 0)
		fprintf(errors, "%s(): unlink(\"%s\") failed, errno=%d (%s)\n", function, temp_file_name, errno, gu_strerror(errno));

	if(gu_fscanf(reply_file, "%d\n", &pprd_retcode) != 1)
		{
		fprintf(errors, "%s(): return code missing in reply file.\n", function);
		fclose(reply_file);
		reply_file = (FILE*)NULL;
		return (FILE*)NULL;
		}

	/* Other execute indicates data which the caller should
	   read using the returned file handle.  Otherwise,
	   print_reply() should be used.  */
	return pprd_retcode == EXIT_OK_DATA ? reply_file : (FILE*)NULL ;
	} /* end of wait_for_pprd() */

/*
** Print a plain text reply from pprd.
** Return the value at the start of the plain text reply.
** If an internal error occurs, return -1.
*/
int print_reply(void)
	{
	int c;				/* one character from reply file */

	/* reply_file will probably only be NULL if we
	   caught an error in wait_for_pprd(). */
	if(reply_file == (FILE*)NULL)
		return EXIT_INTERNAL;

	/* print the rest of the text */
	while((c = fgetc(reply_file)) != EOF)
		fputc(c,stdout);

	fclose(reply_file);

	/* return the code from the top of the reply */
	return pprd_retcode;
	} /* end of print_reply() */

/*====================================================================
** Job and destionation name parsing functions.
**==================================================================*/

/*
** This is the routine which makes aliasing work.
*/
static int do_aliasing(char *destname)
	{
	static int depth = 0;
	int retval = 0;

	/* printf("do_aliasing(\"%s\")\n", destname); */

	/*
	** If we are not being called recursively, see if it is a queue 
	** alias and if it is, replace it with the real queue name.
	*/
	if(depth == 0)
		{
		char fname[MAX_PPR_PATH];
		FILE *f;
		char *line = NULL;
		int linelen = 128;

		ppr_fnamef(fname, "%s/%s", ALIASCONF, destname);
		if((f = fopen(fname, "r")))
			{
			char *forwhat;
			while((line = gu_getline(line, &linelen, f)))
				{
				if(gu_sscanf(line, "ForWhat: %S", &forwhat) == 1)
					{
					static int depth = 0;
					struct Destname dest;

					depth++;
					retval = parse_dest_name(&dest, forwhat);
					depth--;

					if(retval == 0)
						{
						strcpy(destname, dest.destname);
						}

					gu_free(forwhat);
					break;
					}
				}
			fclose(f);

			if(line)				/* if stopped before end, */
				{
				gu_free(line);
				}
			else
				{
				fprintf(errors, _("The alias \"%s\" doesn't have a forwhat value.\n"), destname);
				retval = -1;
				}
			}
		}

	/* printf("do_aliasing(): done\n"); */
	return retval;
	}

/*
** Function for breaking up a queue file name into its constituent parts.
** If it is not a valid job name return -1, else return 0.
*/
int parse_job_name(struct Jobname *job, const char *jobname)
	{
	size_t len;
	const char *ptr;
	int c;

	/* Set pointer to start of job name. */
	ptr = jobname;

	/*
	** Find the full extent of the destination name.  Keep in mind that
	** embedded hyphens are now allowed.  Note that this is not perfect
	** because things such as "busprn-2" are still ambiguous.
	*/
	for(len=0; TRUE; len++)
		{
		len += strcspn(&ptr[len], "-");			/* get length until next hyphen */

		if(ptr[len] == '\0')					/* check for end of string */
			break;

		if(ptr[len + 1] == '*')					/* if explicit wildcard queue id as in "busprn-2-*", */
			break;

		if((c = ptr[len+1+strspn(&ptr[len+1], "0123456789")]) == '.' || c == '(' || c=='\0')
			break;
		}

	/*
	** If the job name we found above is too long
	** or it does not have a hyphen after it, this is
	** a syntax error.
	*/
	if(len > MAX_DESTNAME)
		{
		fprintf(errors, _("Destination (printer or group) name \"%*s\" is too long.\n"), (int)len, ptr);
		return -1;
		}
	if(len == 0)
		{
		fprintf(errors, _("Destination (printer or group) name is empty.\n"));
		return -1;
		}

	/*
	** Copy the destination (printer or group) name into the structure.
	*/
	strncpy(job->destname, ptr, len);
	job->destname[len] = '\0';
	ptr += len;

	/*
	** Do the alias replacement if the destination name is an alias.
	*/
	if(do_aliasing(job->destname) == -1)
		return -1;

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
		fprintf(errors, _("Destination or job name \"%s\" is invalid.\n"), jobname);
		return -1;
		}

	return 0;	/* indicate sucess */
	} /* end of parse_job_name() */

/*
** Parse a printer or group name.  This involves separating the host
** and printer parts.  This process also includes alias resolution.
*/
int parse_dest_name(struct Destname *dest, const char *destname)
	{
	int len;
	const char *ptr;

	ptr = destname;

	if((len = strlen(ptr)) > MAX_DESTNAME)
		{
		fprintf(errors, _("Destination name \"%*s\" is too long.\n"), len, ptr);
		return -1;
		}

	if(strpbrk(ptr, DEST_DISALLOWED))
		{
		fprintf(errors, _("Destination name \"%*s\" contains a disallowed character.\n"), len, ptr);
		return -1;
		}

	if(strchr(DEST_DISALLOWED_LEADING, (int)ptr[0]))
		{
		fprintf(errors, _("Destination name \"%*s\" begins with a disallowed character.\n"), len, ptr);
		return -1;
		}

	strcpy(dest->destname, ptr);

	/* Do the alias replacement. */
	if(do_aliasing(dest->destname) == -1)
		return -1;

	return 0;
	} /* end of parse_dest_name() */

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
** Set the user who should be considered to be running this program.
** Only privileged users may do this.  Thus, a privileged user may
** become a different privileged user or become an unprivileged user.
** Thus a privileged user can use this feature to drop privledge, but
** not to gain additional access.
**
** Generally, this will be used by servers running under privileged
** user identities.  They will use this so as not to exceed the privledge
** of the user for whom they are acting.
*/
static int do_u_option(const char username[])
	{
	if(privileged())
		{
		gu_free(opt_user);
		opt_user = gu_strdup(username);
		return 0;
		}
	else
		{
		return -1;
		}
	}

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
		fputs(_("You are not allowed to perform the requested\n"
			"operation because you are not a PPR operator.\n"), errors);
		return FALSE;
		}
	} /* end of assert_am_operator() */

/*
** This function compares a job's username to a username pattern
** which can include basic wildcards.  If they match, it returns TRUE.
*/
static gu_boolean username_match(const char username[], const char pattern[])
	{
	char *username_at_host, *pattern_at_host;
	size_t len;

	/* If they match exactly, */
	if(strcmp(username, pattern) == 0)
		return TRUE;

	/* If the username is in username@hostname format 
	 * and pattern is in the same format,
	 */
	if((username_at_host = strchr(username, '@')) && (pattern_at_host = strchr(pattern, '@')))
		{
		/* If both username and hostname are wildcards, */
		if(strcmp(pattern, "*@*") == 0)
			return TRUE;

		/* If username is a wildcard and the host names match, */
		if(lmatch(pattern, "*@")
				&& strcmp(username_at_host, pattern_at_host) == 0)
			return TRUE;

		/* If hostname is a wildcard and the usernames match, */
		if(strcmp(pattern_at_host, "@*") == 0
				&& (len = (pattern_at_host - pattern)) == (username_at_host - username)
				&& strncmp(username, pattern, len) == 0)
			return TRUE;
		}

	return FALSE;
	} /* username_match() */

/*
** This function returns TRUE if the current user has permission to touch
** the specified job.  If he does not, an error message is printed and
** FALSE is returned.
*/
gu_boolean job_permission_check(struct Jobname *job)
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
			fprintf(errors, X_("Can't open queue file \"%s\" to verify access rights, errno=%d (%s).\n"), fname, errno, gu_strerror(errno));
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
			fprintf(errors, "Queue file error, no \"User:\" line.\n");
			break;
			}
	
		if(!privileged() && !username_match(job_username, opt_user))
			{
			fprintf(errors,
				_("You may not manipulate the job \"%s\" because it\n"
				"does not belong to the user \"%s\".\n"),
					jobid(job->destname, job->id, job->subid),
					opt_user
				);
			break;
			}
		if(opt_magic_cookie && strcmp(opt_magic_cookie, job_magic_cookie) != 0)
			{
			fprintf(errors, "Magic cookie doesn't match.\n");
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
			N_("ppop short {<destination>, <job>, all} ..."),
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
				fputc('\n', out);
			fprintf(out, "%s\n", pxlate);
			}
		else
			{
			fprintf(out, "    %s\n", pxlate);
			}
		}
	return EXIT_OK;
	} /* end of main_help() */

/*
** Examine the command and run the correct proceedure.
** Return the value returned by the proceedure function.
** If the command is unknown, return -1.
*/
static int dispatch(char *argv[])
	{
	/*
	** Since these are used by other programs we
	** put them first so they will be the fastest.
	*/
	if(strcmp(argv[0],"qquery") == 0)
		return ppop_qquery(&argv[1]);
	if(strcmp(argv[0],"message") == 0)
		return ppop_message(&argv[1]);
	if(strcmp(argv[0],"progress") == 0)
		return ppop_progress(&argv[1]);

	if(strcmp(argv[0],"short") == 0)
		return ppop_short(&argv[1]);
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
		return main_help(stderr);
	else
		return -1;						/* return `dispatcher failed' code */
	} /* end of dispatch() */

/*
** Interactive Mode Function
** Return the result code of the last command executed.
**
** In interactive mode, we present a prompt, read command
** lines, and execute them.
*/
static int interactive_mode(void)
	{
	#define MAX_CMD_WORDS 64
	char *ar[MAX_CMD_WORDS+1];	/* argument vector constructed from line[] */
	char *ptr;					/* used to parse arguments */
	unsigned int x;				/* used to parse arguments */
	int errorlevel=0;			/* return value from last command */

	if( ! opt_machine_readable )	/* If a human will be reading our output, */
		{
		puts(_("PPOP, Page Printer Operator's utility"));
		puts(VERSION);
		puts(COPYRIGHT);
		puts(AUTHOR);
		puts("");
		puts(_("Type \"help\" for command list, \"exit\" to quit."));
		puts("");
		}
	else						/* If a machine will be reading our output, */
		{
		puts("*READY\t"SHORT_VERSION);
		fflush(stdout);
		}

	/*
	** Read input lines until end of file.
	**
	** Notice that the prompt printed when in machine readable
	** mode is blank.  Also notice that we do not explicitly
	** flush stdout and yet the prompt is printed even though
	** it does not end with a line feed.  This is mysterious.
	*/
	while((ptr = ppr_get_command("ppop>", opt_machine_readable)))
		{
		/*
		** Break the string into white-space separated "words".  A quoted string
		** will be treated as one word.
		*/
		for(x=0; (ar[x] = gu_strsep_quoted(&ptr, " \t", NULL)); x++)
			{
			if(x == MAX_CMD_WORDS)
				{
				puts("Warning: command buffer overflow!");		/* temporary code, don't internationalize */
				ar[x] = NULL;
				break;
				}
			}

		/*
		** The variable x will be an index into ar[] which will
		** indicate the first element that has any significance.
		** If the line begins with the word "ppop" we will
		** increment x.
		*/
		x = 0;
		if(ar[0] && strcmp(ar[0], "ppop") == 0)
			x++;

		/*
		** If no tokens remain in this command line,
		** go on to the next command line.
		*/
		if(ar[x] == (char*)NULL)
			continue;

		/*
		** If the command is "exit", break out of
		** the line reading loop.
		*/
		if(strcmp(ar[x], "exit") == 0 || strcmp(ar[x], "quit") == 0)
			break;

		/*
		** Call the dispatch() function to execute the command.  If the
		** command is not recognized, dispatch() will return -1.  In that
		** case we print a helpful message and change the errorlevel to
		** zero since -1 is not a valid exit code for a program.
		*/
		if((errorlevel = dispatch(&ar[x])) == -1)
			{
			if( ! opt_machine_readable )					/* A human gets english */
				puts(_("Try \"help\" or \"exit\"."));
			else										/* A program gets a code */
				puts("*UNKNOWN");

			errorlevel = EXIT_SYNTAX;
			}
		else if(opt_machine_readable)				/* If a program is reading our output, */
			{									/* say the command is done */
			printf("*DONE\t%d\n", errorlevel);	/* and tell the exit code. */
			}

		if(opt_machine_readable)					/* If stdout is a pipe as seems likely */
			fflush(stdout);						/* when -M is used, we must flush it. */
		} /* While not end of file */

	return errorlevel;					/* return result of last command (not counting exit) */
	} /* end of interactive_mode() */

/*
** Handler for sigpipe.
*/
static void pipe_sighandler(int sig)
	{
	fputs(_("Spooler has shut down.\n"), errors);

	if(opt_machine_readable)
		fprintf(errors, "*DONE %d\n", EXIT_NOSPOOLER);

	exit(EXIT_NOSPOOLER);
	} /* end of pipe_sighandler() */

/*
** Print help.
*/
static void help_switches(FILE *out)
	{
	int i;
	const char *switch_list[] =
		{
		N_("-M\tselect machine-readable output"),
		N_("--machine-readable\tsame as -M"),
		N_("-A <seconds>\tdon't show jobs arrested more than <seconds> ago"),
		N_("--arrest-interest-time=<seconds>\tsame as -A"),
		N_("-u <user>\tcheck access as if run as indicated user"),
		N_("--user <user>\tsame as -u"),
		N_("--magic-cookie <string>\tpresent job access token"),
		N_("--verbose\tprint more information"),
		N_("--version\tprint the PPR version information"),
		N_("--help\tprint this help message"),
		NULL
		};

	fputs(_("Valid switches:\n"), out);
	for(i = 0; switch_list[i]; i++)
		{
		const char *p = gettext(switch_list[i]);
		int to_tab = strcspn(p, "\t");
		fprintf(out, "    %-35.*s %s\n", to_tab, p, p[to_tab] == '\t' ? &p[to_tab + 1] : "");
		}

	fputc('\n', out);

	fputs(_("Try \"ppop help\" for help with subcommands.\n"), out);
	fputs("\n", out);
	fprintf(out, _("The %s manpage may be viewed by entering this command at a shell prompt:\n"
		"    ppdoc %s\n"), "ppop(1)", "ppop");
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
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	/* We set this here because Cygnus Win32 doesn't think
	   that stderr is a constant!  (It turns out that its
	   behavior conforms to ANSI C and POSIX.) */
	errors = stderr;

	/* paranoia */
	umask(PPR_UMASK);

	/* Figure out the user's name and make it the initial value for opt_user. */
	{
	struct passwd *pw;
	uid_t uid = getuid();
	if((pw = getpwuid(uid)) == (struct passwd *)NULL)
		{
		fprintf(errors, "%s: getpwuid(%ld) failed, errno=%d (%s)\n", myname, (long)uid, errno, gu_strerror(errno));
		exit(EXIT_INTERNAL);
		}
	opt_user = gu_strdup(pw->pw_name);
	}

	/* Parse the options. */
	gu_getopt_init(&getopt_state, argc, argv, option_chars, option_words);
	while((optchar = ppr_getopt(&getopt_state)) != -1)
		{
		switch(optchar)
			{
			case 'M':					/* -M or --machine-readable */
				opt_machine_readable = TRUE;
				errors = stdout;
				break;

			case 'A':					/* -A or --arrest-interest-interval */
				opt_arrest_interest_interval = atoi(getopt_state.optarg);
				break;

			case 'u':
				if(do_u_option(getopt_state.optarg) == -1)
					{
					fprintf(errors, _("You aren't allowed to use the %s option.\n"), "-u");
					exit(EXIT_DENIED);
					}
				break;

			case 1000:					/* --help */
				help_switches(stdout);
				exit(EXIT_OK);

			case 1001:					/* --version */
				if(opt_machine_readable)
					{
					puts(SHORT_VERSION);
					}
				else
					{
					puts(VERSION);
					puts(COPYRIGHT);
					puts(AUTHOR);
					}
				exit(EXIT_OK);

			case 1003:					/* --verbose */
				opt_verbose = TRUE;
				break;

			case 1004:					/* --magic-cookie */
				opt_magic_cookie = getopt_state.optarg;
				break;

			default:
				gu_getopt_default(myname, optchar, &getopt_state, errors);

				if(optchar == '?')
					help_switches(errors);

				exit(EXIT_SYNTAX);
				break;
			}
		}

	/* Change to the home directory of PPR. */
	chdir(LIBDIR);

	/*
	** Install a SIGPIPE handler so we can produce an
	** intelligible message when we try to run a command
	** on a spooler which has shut down.
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
	if(argc == getopt_state.optind)
		{
		result = interactive_mode();
		}
	else
		{
		if((result = dispatch(&argv[getopt_state.optind])) == -1)
			{
			fprintf(errors, _("%s: unknown sub-command \"%s\", try \"ppop help\"\n"), myname, argv[getopt_state.optind]);
			result = EXIT_SYNTAX;
			}
		}

	/* Clean up by closing the FIFOs which may have
	   been used to communicate with pprd or rpprd. */
	if(FIFO)
		fclose(FIFO);

	/* Exit with the result of the last command. */
	return result;
	} /* end of main() */

/* end of file */

