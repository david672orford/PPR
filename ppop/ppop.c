/*
** mouse:~ppr/src/ppop/ppop.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 14 December 2001.
*/

/*
** Operators utility for PostScript page printers.  It allows the queue
** to be viewed, jobs to be canceled, jobs to be held, printers to
** be started and stopped, and much more.
*/

#include "before_system.h"
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

/* Misc globals */
const char myname[] = "ppop";
pid_t pid;						/* this process id */
static char *su_user = (char*)NULL;			/* --su switch value */
static const char *proxy_for = (const char*)NULL;	/* -X switch value */
static const char *magic_cookie = (const char*)NULL;	/* --magic-cookie value */
int arrest_interest_interval = -1;
int machine_readable = FALSE;				/* machine readable mode */
FILE *errors;						/* file we should send stderr type messages to */
gu_boolean verbose = FALSE;				/* --verbose switch */

/*
** Handle fatal errors.
** Print a message and exit.
*/
void fatal(int exitval, const char message[], ... )
    {
    va_list va;

    if(machine_readable)
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

static sigset_t oset;			/* old signal set */
static FILE *FIFO = (FILE*)NULL;	/* channel to pprd */
static volatile gu_boolean sigcaught;		/* flag which is set when SIGUSR1 received */
static volatile gu_boolean timeout;		/* set when SIGALRM is caught */
static char temp_file_name[MAX_PPR_PATH];	/* name of temporary file we get answer in */
static FILE *reply_file;		/* streams library thing for open reply file */
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
** Get ready to communicate with a spooler daemon.
** If the nodename is our nodename, we will communicate
** with pprd, otherwise we will be communicating with
** rpprd.
**
** Get ready for a SIGUSR1 from pprd or rpprd.
**
** To do this, we clear the signal received flag
** and block the signal.  (We will unblock it with
** sigsuspend().
*/
FILE *get_ready(const char nodename[])
    {
    int fifo;
    sigset_t set;		/* storage for set containing SIGUSR1 */

    /* Are we using a real FIFO or just an append to a file? */
    #ifdef HAVE_MKFIFO
    #define FIFO_OPEN_FLAGS (O_WRONLY | O_NONBLOCK)
    #else
    #define FIFO_OPEN_FLAGS (O_WRONLY | O_APPEND)
    #endif

    if(FIFO == (FILE*)NULL)
        {
        if((fifo = open(FIFO_NAME, FIFO_OPEN_FLAGS)) < 0)
            fatal(EXIT_NOSPOOLER, _("can't open FIFO, pprd is probably not running."));
        FIFO = fdopen(fifo, "w");
        }

    sigcaught = FALSE;

    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGALRM);
    sigprocmask(SIG_BLOCK, &set, &oset);

    /* Before the command we must give the "address"
       to reply to.  This address is our PID. */
    fprintf(FIFO, "%ld ", (long int)pid);

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
	fprintf(errors, _("Timeout waiting for response"));
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
    int c;		/* one character from reply file */

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
static int do_aliasing(char *destnode, char *destname)
    {
    static int depth = 0;
    int retval = 0;

    /* printf("do_aliasing(\"%s\", \"%s\")\n", destnode, destname); */

    /*
    ** If we are not being called recursively and the queue is on this
    ** local node, see if it is a queue alias and if it is, replace it
    ** with the real queue name.
    */
    if(depth == 0 && strcmp(destnode, ppr_get_nodename()) == 0)
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
			strcpy(destnode, dest.destnode);
			strcpy(destname, dest.destname);
		    	}

		    gu_free(forwhat);
		    break;
		    }
                }
            fclose(f);

            if(line)                /* if stopped before end, */
                {
                gu_free(line);
                }
            else
                {
                fprintf(errors, _("The alias \"%s:%s\" doesn't have a forwhat value.\n"), destnode, destname);
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

    /* If a destination node name is specified, then claim it. */
    if((len = strcspn(ptr, ":")) != strlen(ptr))
	{
	if(len > MAX_NODENAME)
	    {
	    fprintf(errors, _("Destination node name \"%*s\" is too long.\n"), (int)len, ptr);
	    return -1;
	    }
	if(len == 0)
	    {
	    fprintf(errors, _("Destination node name (the part before the colon) is missing.\n"));
	    return -1;
	    }

	strncpy(job->destnode, jobname, len);
	job->destnode[len] = '\0';
	ptr += (len+1);
	}
    /* otherwise, assume the destination node is this node */
    else
    	{
    	strcpy(job->destnode, ppr_get_nodename());
    	}

    /*
    ** Find the full extent of the destination name.  Keep in mind that
    ** embedded hyphens are now allowed.  Note that this is not perfect
    ** because things such as "busprn-2" are still ambiguous.
    */
    for(len=0; TRUE; len++)
	{
	len += strcspn(&ptr[len], "-");		/* get length until next hyphen */

	if(ptr[len] == '\0')			/* check for end of string */
	    break;

	if(ptr[len + 1] == '*')			/* if explicit wildcard queue id as in "busprn-2-*", */
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
	fprintf(errors, _("Destination name \"%*s\" is too long.\n"), (int)len, ptr);
	return -1;
	}
    if(len == 0)
    	{
    	fprintf(errors, _("Destination name is empty.\n"));
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
    if(do_aliasing(job->destnode, job->destname) == -1)
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
    ** If a left parenthesis comes next, read the
    ** sending node name.  If not, use a wildcard.
    */
    if(*ptr == '(')
    	{
    	ptr++;
    	if((len = strcspn(ptr, ")")) > MAX_NODENAME || ptr[len] != ')')
    	    return -1;
	strncpy(job->homenode, ptr, len);
	job->homenode[len] = '\0';
	ptr += len;
	ptr++;
	}
    else
	{
	strcpy(job->homenode, "*");
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

    /* If a colon can be found, their is a destination node specified. */
    if((size_t)(len = strcspn(ptr, ":")) < strlen(ptr))
	{
	if(len > MAX_NODENAME)
	    {
	    fprintf(errors, _("Node name \"%*s\" is too long.\n"), len, ptr);
	    return -1;
	    }
	strncpy(dest->destnode, ptr, len);
	dest->destnode[len] = '\0';
	ptr += len;
	ptr++;
	}
    /* Otherwise, assume local node. */
    else
    	{
    	strcpy(dest->destnode, ppr_get_nodename());
    	}

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
    if(do_aliasing(dest->destnode, dest->destname) == -1)
    	return -1;

    return 0;
    } /* end of parse_dest_name() */

/*====================================================================
** Security functions
**==================================================================*/

/*
** Is the user privledged?  If the user identity has been changed
** (by the --su switch) since last time this function was called,
** the answer is found again, otherwise a cached answer is returned.
*/
static gu_boolean privledged(void)
    {
    static gu_boolean answer = FALSE;
    static char *answer_username = NULL;

    if(!answer_username || strcmp(su_user, answer_username))
	{
	if(answer_username)
	    gu_free(answer_username);
	answer_username = gu_strdup(su_user);
	answer = user_acl_allows(su_user, "ppop");
	}

    return answer;
    } /* end of privledged() */

/*
** Set the user who should be considered to be running this program.
** Only privledged users may do this.  Thus, a privledged user may
** become a different privledged user or become an unprivledged user.
** Thus a privledged user can use this feature to drop privledge, but
** not to gain additional access.
**
** Generally, this will be used by servers running under privledged
** user identities.  They will use this so as not to exceed the privledge
** of the user for whom they are acting.
*/
static int su(const char username[])
    {
    if(privledged())
    	{
	gu_free(su_user);
	su_user = gu_strdup(username);
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
int assert_am_operator(void)
    {
    if(privledged())
	{
	return 0;
	}
    else
	{
	fputs(_("You are not allowed to perform the requested\n"
	    "operation because you are not a PPR operator.\n"), errors);
	return -1;
	}
    } /* end of assert_am_operator() */

/*
** This function compares a job's proxy for identification string to a pattern
** which can include basic wildcards.  If they match, it returns TRUE.
*/
static gu_boolean proxy_for_match(const char job_proxy_for[], const char proxy_for_pattern[])
    {
    char *job_at_host, *pattern_at_host;
    size_t len;

    /* If they match exactly, */
    if(strcmp(job_proxy_for, proxy_for_pattern) == 0)
        return TRUE;

    /* If the job was submitted with a --proxy-for in username@hostname
       format and the proxy pattern is in the same format, */
    if((job_at_host = strchr(job_proxy_for, '@')) && (pattern_at_host = strchr(proxy_for_pattern, '@')))
	{
	/* If both username and hostname are wildcards, */
	if(strcmp(proxy_for_pattern, "*@*") == 0)
	    return TRUE;

	/* If username is a wildcard and the host names match, */
	if(strncmp(proxy_for_pattern, "*@", 2) == 0
		&& strcmp(job_at_host, pattern_at_host) == 0)
	    return TRUE;

	/* If hostname is a wildcard and the usernames match, */
	if(strcmp(pattern_at_host, "@*") == 0
		&& (len = (pattern_at_host - proxy_for_pattern)) == (job_at_host - job_proxy_for)
		&& strncmp(job_proxy_for, proxy_for_pattern, len) == 0)
	    return TRUE;
	}

    return FALSE;
    }

/*
** This function return -1 (to indicate failure) if the indicated job
** does not belong to the user and the user is not an operator.  If -1
** is returned, also prints an error message.
**
** If the user is a proxy (the -X switch has been used) then the proxy
** id's must also match or the user part of the proxy id must be "*" on
** the same machine
** as the job's proxy id.  If the -X switch is used then even operators
** cannot delete jobs that were not submitted under their Unix uid.
*/
int job_permission_check(struct Jobname *job)
    {
    char job_username[17];
    char job_proxy_for[127];
    char job_magic_cookie[33];

    /*
    ** If the user is an operator (as modified by the --su switch) and
    ** is not limiting his authority with the --proxy-for switch, then
    ** we can bail out right now because operators can manipulate any
    ** job.  This will save us the time and trouble of opening the
    ** queue file to figure out whose job it is.
    */
    if(privledged() && !proxy_for && !magic_cookie)
	return 0;

    /*
    ** In this block, we open the queue file and extract the information
    ** from the "User:" line.
    **
    ** If we can't open it, just say it is ok to manipulate the job.  This is
    ** because, for non-existent jobs we want to user to be told it does not
    ** exist, not that he can't touch it!
    **
    ** !!! This code must be fixed for distributed printing !!!
    */
    {
    char fname[MAX_PPR_PATH];
    FILE *f;
    char *line = NULL;
    int line_space = 80;
    long int job_uid;		/* no longer used */

    /*
    ** Open queue file.
    */
    ppr_fnamef(fname, "%s/%s:%s-%d.%d(%s)",
	QUEUEDIR,
    	job->destnode,
    	job->destname,
    	job->id,
    	job->subid != WILDCARD_SUBID ? job->subid : 0,
    	strcmp(job->homenode, "*") ? job->homenode : ppr_get_nodename());
    if((f = fopen(fname, "r")) == (FILE*)NULL)
	{
	/* This is where we need code for remote printing!!! */

	/* See note above. */
	if(errno == ENOENT)
	    return 0;

	fprintf(errors, X_("Can't open queue file \"%s\" to verify access rights, errno=%d (%s).\n"), fname, errno, gu_strerror(errno));
	return -1;
	}

    /* Read the queue file and find the "User:" line. */
    job_username[0] = '\0';
    job_proxy_for[0] = '\0';
    job_magic_cookie[0] = '\0';
    while((line = gu_getline(line, &line_space, f)))
	{
	if(gu_sscanf(line, "User: %ld %#s %#s",
		&job_uid,
		sizeof(job_username), job_username,
		sizeof(job_proxy_for), job_proxy_for) >= 2)
	    continue;
	if(gu_sscanf(line, "MagicCookie: %#s", sizeof(job_magic_cookie), &job_magic_cookie) == 1)
	    continue;
	}

    /* Close the queue file. */
    if(line) gu_free(line);
    fclose(f);

    /* Check to see that we got a "User:" line: */
    if(!job_username[0])
    	{
    	fprintf(errors, "Queue file error, no \"User:\" line.\n");
	return -1;
	}
    }

    /*
    ** In order to delete a job, you must either have operator privledge or
    ** you must be the user who submitted it.
    **
    ** (In all this we obey the --su switch.)
    */
    if(!privledged() && strcmp(job_username, su_user))
	{
	fprintf(errors,
		_("You may not manipulate the job \"%s\" because it\n"
		"does not belong to the user \"%s\".\n"),
			remote_jobid(job->destnode, job->destname, job->id, job->subid, job->homenode),
			su_user);

	/* If the command is a request by proxy, remote LPR users might be
	   confused by the message above because they probably don't know
	   anything about the proxy system.  We will try to make things a
	   little clearer with this message.
	   */
	if(proxy_for)
	    {
	    PUTC('\n');
	    fprintf(errors, _("(The user \"%s\" is acting as your proxy.)\n"), su_user);
	    }

	return -1;
	}

    /*
    ** If the user who invoked ppop provided an -X switch
    ** and it does not match the one which was provided when
    ** submitting the job, then refuse to do it.
    */
    if(proxy_for)				/* if -X (--proxy-for) switch used, */
	{
	/* If --proxy-for wasn't used when the job was submitted or it doesn't match, */
	if(job_proxy_for[0] == '\0' || !proxy_for_match(job_proxy_for, proxy_for))
	    {
	    char *job_at_host, *pattern_at_host;
	    gu_boolean show_hostnames;

	    /* If both proxy-for strings contain a hostname and they are the same or the
	       hostname from our command line is a *, then suppress both of them
	       since they aren't what is at issue and will only serve to confuse 
	       the user. */
    	    if((job_at_host = strchr(job_proxy_for, '@')) && (pattern_at_host = strchr(proxy_for, '@'))
		&& (strcmp(pattern_at_host, "@*") == 0 || strcmp(pattern_at_host, job_at_host) == 0)
    	    	)
	    	{
		show_hostnames = FALSE;
	    	}
	    else
		{	    
		show_hostnames = TRUE;
		}

	    /* Print the error message with or without hostnames. */
	    fprintf(errors,
	    	_("You may not manipulate the job \"%s\" because it belongs to\n"
	    	  "\"%.*s\", while you are \"%.*s\".\n"),
		remote_jobid(job->destnode, job->destname, job->id, job->subid, job->homenode),
		(int)(show_hostnames ? strlen(job_proxy_for) : strcspn(job_proxy_for, "@")), job_proxy_for,
		(int)(show_hostnames ? strlen(proxy_for) : strcspn(proxy_for, "@")), proxy_for);

	    return -1;
	    }
    	}

    /*
    ** If there was an --magic-cookie option,
    */
    if(magic_cookie)
    	{
	if(strcmp(magic_cookie, job_magic_cookie) != 0)
	    {
	    fprintf(errors, "Magic cookie doesn't match.\n");
	    return -1;
	    }
    	}

    /*
    ** All tests passed.  You may do what you want with this job.
    */
    return 0;
    } /* end of job_permission_check() */

/*
** Return 0 if the job matches the current user id and proxy_for.
** Return non-zero if it does not.  Notice that operator
** privledge plays no part in this.  Other than that, the
** criteria should be the same as for job_permission_check().
**
** The global variables uid and proxy_for are used by
** this function.
*/
int is_my_job(const struct QEntry *qentry, const struct QFileEntry *qfileentry)
    {
    /* If the user names don't match, it isn't. */
    if(strcmp(su_user, qfileentry->username))
	return FALSE;

    /* If the user says he is acting as a proxy, the party he is
       acting as proxy for must be the same as the party he acted for
       when he submitted the job. */
    if(proxy_for)
	{
	if(!qfileentry->proxy_for)			/* if not a proxy job, */
	    return FALSE;

	if(!proxy_for_match(qfileentry->proxy_for, proxy_for))
	    return FALSE;
    	}

    return TRUE;
    } /* end of is_my_job() */

/*=========================================================================
** Main function and associated functions, dispatch command
** to proper command handler.
=========================================================================*/
static int main_help(void)
	{
	fputs(_("\nDestination commands:\n"
		"\tppop destination {<destination>, all}\n"
		"\t\tAbbreviation:  ppop dest {<destination>, all}\n"
		"\tppop destination-comment {<destination>, all}\n"
		"\t\tAbbreviation:  ppop ldest {<destination>, all}\n"
		"\tppop destination-comment-address {<destination>, all}\n"
		"\tppop accept <destination>\n"
		"\tppop reject <destination>\n"), stdout);

	fputs(_("\nPrint job commands:\n"
		"\tppop list {<destination>, <job>, all} ...\n"
		"\tppop short {<destination>, <job>, all} ...\n"
		"\tppop details {<destination>, <job>, all} ...\n"
		"\tppop lpq {<destination>, <job>, all} [<user>] [<id>] ...\n"
		"\tppop qquery {<destination>, <job>, all} <field name 1> ...\n"
		"\tppop move {<job>, <old_destination>} <new_destination>\n"
		"\tppop hold <job> ...\n"
		"\tppop release <job> ...\n"
		"\tppop [s]cancel {<job>, <destination>, all} ...\n"
		"\tppop [s]purge {<destination>, all} ...\n"
		"\tppop [s]cancel-active {<destination>, all} ...\n"
		"\tppop [s]cancel-my-active {<destination>, all} ...\n"
		"\tppop clean <destination> ...\n"
		"\tppop rush <job> ...\n"
		"\tppop last <job> ...\n"
		"\tppop log <job>\n"
		"\tppop progress <job>\n"
		"\tppop modify <job> <name>=<value> ...\n"), stdout);

	fputs(_("\nPrinter commands:\n"
		"\tppop status {<destination>, all} ...\n"
		"\tppop start <printer> ...\n"
		"\tppop halt <printer> ...\n"
		"\tppop stop <printer> ...\n"
		"\tppop wstop <printer>\n"
		"\tppop message <printer>\n"
		"\tppop alerts <printer>\n"), stdout);

	fputs(_("\nMedia commands:\n"
		"\tppop media {<destination>, all}\n"
		"\tppop mount <printer> <bin> <medium>\n"), stdout);

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
    else if(strcmp(argv[0],"nhlist") == 0)	/* For lprsrv, no-header-list */
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
	return main_help();
    else
	return -1;			/* return `dispatcher failed' code */
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
    char *ptr;			/* used to parse arguments */
    unsigned int x;		/* used to parse arguments */
    int errorlevel=0;		/* return value from last command */

    if( ! machine_readable )	/* If a human will be reading our output, */
	{
	puts(_("PPOP, Page Printer Operator's utility"));
	puts(VERSION);
	puts(COPYRIGHT);
	puts(AUTHOR);
	puts("");
	puts(_("Type \"help\" for command list, \"exit\" to quit."));
	puts("");
	}
    else			/* If a machine will be reading our output, */
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
    while((ptr = ppr_get_command("ppop>", machine_readable)))
	{
	/*
	** Break the string into white-space separated "words".  A quoted string
	** will be treated as one word.
	*/
	for(x=0; (ar[x] = gu_strsep_quoted(&ptr, " \t\n", NULL)); x++)
	    {
            if(x == MAX_CMD_WORDS)
            	{
		puts("Warning: command buffer overflow!");	/* temporary code, don't internationalize */
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
	    if( ! machine_readable )			/* A human gets english */
		puts(_("Try \"help\" or \"exit\"."));
	    else					/* A program gets a code */
	    	puts("*UNKNOWN");

	    errorlevel = EXIT_SYNTAX;
	    }
	else if(machine_readable)		/* If a program is reading our output, */
	    {					/* say the command is done */
	    printf("*DONE\t%d\n", errorlevel);	/* and tell the exit code. */
	    }

	if(machine_readable)			/* If stdout is a pipe as seems likely */
	    fflush(stdout);			/* when -M is used, we must flush it. */
    	} /* While not end of file */

    return errorlevel;			/* return result of last command (not counting exit) */
    } /* end of interactive_mode() */

/*
** Handler for sigpipe.
*/
static void pipe_sighandler(int sig)
    {
    fputs(_("Spooler has shut down.\n"), errors);

    if(machine_readable)
    	fprintf(errors, "*DONE %d\n", EXIT_NOSPOOLER);

    exit(EXIT_NOSPOOLER);
    } /* end of pipe_sighandler() */

/*
** Print help.
*/
static void help_switches(FILE *outfile)
    {
    fputs(_("Valid switches:\n"), outfile);

    fputs(_(	"\t-X <principal string>\n"
		"\t--proxy-for=<principal string>\n"
		"\t-M\n"
		"\t--machine-readable\n"
		"\t--su <user>\n"
		"\t-A <seconds>\n"
		"\t--arrest-interest-time=<seconds>\n"
		"\t--verbose\n"
		), outfile);

    fputs(_(	"\t--version\n"
		"\t--help\n"
		), outfile);

    fputs("\n", outfile);

    fputs(_("Try \"ppop help\" for help with subcommands.\n"), outfile);
    } /* end of help() */

/*
** Command line options:
*/
static const char *option_chars = "X:MA:";
static const struct gu_getopt_opt option_words[] =
	{
	{"proxy-for", 'X', TRUE},
	{"machine-readable", 'M', FALSE},
	{"arrest-interest-interval", 'A', TRUE},
	{"help", 1000, FALSE},
	{"version", 1001, FALSE},
	{"su", 1002, TRUE},
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
    int result;         /* value to return */
    int optchar;	/* for ppr_getopt() */
    struct gu_getopt_state getopt_state;

    /* Initialize internation messages library. */
    #ifdef INTERNATIONAL
    setlocale(LC_MESSAGES, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);
    #endif

    /* We set this here because Cygnus Win32 doesn't think
       that stderr is a constant!  (It turns out that its
       behavior conforms to ANSI C and POSIX.) */
    errors = stderr;

    /* paranoia */
    umask(PPR_UMASK);

    /* Figure out the user's name and make it the initial value for su_user. */
    {
    struct passwd *pw;
    uid_t uid = getuid();
    if((pw = getpwuid(uid)) == (struct passwd *)NULL)
        {
        fprintf(errors, "%s: getpwuid(%ld) failed, errno=%d (%s)\n", myname, (long)uid, errno, gu_strerror(errno));
        exit(EXIT_INTERNAL);
        }
    su_user = gu_strdup(pw->pw_name);
    }

    /* Parse the options. */
    gu_getopt_init(&getopt_state, argc, argv, option_chars, option_words);
    while((optchar = ppr_getopt(&getopt_state)) != -1)
    	{
    	switch(optchar)
    	    {
    	    case 'X':			/* -X or --proxy-for */
		proxy_for = getopt_state.optarg;
    	    	break;

	    case 'M':			/* -M or --machine-readable */
	    	machine_readable = TRUE;
		errors = stdout;
		break;

	    case 'A':			/* -A or --arrest-interest-interval */
		arrest_interest_interval = atoi(getopt_state.optarg);
		break;

	    case 1000:			/* --help */
	    	help_switches(stdout);
	    	exit(EXIT_OK);

	    case 1001:			/* --version */
		if(machine_readable)
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

	    case 1002:			/* --su */
		if(su(getopt_state.optarg) == -1)
		    {
		    fprintf(errors, _("You aren't allowed to use the --su option.\n"));
		    exit(EXIT_DENIED);
		    }
		break;

	    case 1003:			/* --verbose */
		verbose = TRUE;
		break;

	    case 1004:			/* --magic-cookie */
	    	magic_cookie = getopt_state.optarg;
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
    chdir(HOMEDIR);

    /*
    ** Determine and store our process id.
    ** It is part of the temporary file name.
    */
    pid = getpid();

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
    ppr_fnamef(temp_file_name, "%s/ppr-ppop-%ld", TEMPDIR, (long)pid);

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
	    fprintf(errors, _("Unknown command, try \"ppop help\".\n"));
	    result = EXIT_SYNTAX;
	    }
	}

    /* Clean up by closing the FIFOs which may have
       been used to communicate with pprd or rpprd. */
    if(FIFO) fclose(FIFO);

    /* Exit with the result of the last command. */
    return result;
    } /* end of main() */

/* end of file */

