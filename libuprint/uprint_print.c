/*
** mouse:~ppr/src/libuprint/uprint_print.c
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
** Last modified 18 September 2001.
*/

#include "before_system.h"
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "uprint.h"
#include "uprint_private.h"

#define MAX_ARGV 100

/*
** Function to make a log entry for debugging purposes:
*/
static void logit(const char *command_path, const char **argv)
    {
    #ifdef UPRINT_LOGFILE
    FILE *f;

    if((f = fopen(UPRINT_LOGFILE, "a")) != (FILE*)NULL)
	{
	int x;
	fprintf(f, "--> %s", command_path);
	for(x=1; argv[x]; x++)
	    {
	    if(strlen(argv[x]) == 0 || strchr(argv[x], ' '))
		fprintf(f, " \"%s\"", argv[x]);
	    else
		fprintf(f, " %s", argv[x]);
	    }
	fputc('\n', f);
	fclose(f);
	}
    #endif
    }

/*
** Send the job to the correct spooling system.
*/
int uprint_print(void *p, gu_boolean remote_too)
    {
    const char function[] = "uprint_print";
    struct UPRINT *upr = (struct UPRINT *)p;
    const char *argv[MAX_ARGV];
    int i = 0;
    const char *command_path;
    gu_boolean one_at_a_time = FALSE;		/* general case is FALSE */
    int ret;
    struct REMOTEDEST info;

    DODEBUG(("%s(p=%p)", function, p));

    if(upr->dest == (const char *)NULL)
    	{
	uprint_error_callback("%s(): dest not set", function);
	uprint_errno = UPE_NODEST;
    	return -1;
    	}

    if(upr->user == (const char *)NULL)
    	{
	uprint_error_callback("%s(): user not set", function);
	uprint_errno = UPE_BADORDER;
    	return -1;
    	}

    /* PPR spooling system: */
    if(printdest_claim_ppr(upr->dest))
	{
	one_at_a_time = TRUE;
	command_path = PPR_PATH;
	if((i = uprint_print_argv_ppr(p, argv, MAX_ARGV)) == -1)
	    return -1;
	}

    /* System V spooler: */
    #ifdef HAVE_LP
    else if(printdest_claim_lp(upr->dest))
	{
	command_path = uprint_path_lp();
	if(strcmp(upr->fakername, "lp") == 0)
	    {
	    upr->argv[0] = "lp";
	    }
	else
	    {
	    if((i = uprint_print_argv_lp(p, argv, MAX_ARGV)) == -1)
		return -1;
	    }
	}
    #endif

    /* BSD lpr spooler: */
    #ifdef HAVE_LPR
    else if(printdest_claim_lpr(upr->dest))
	{
	command_path = uprint_path_lpr();
	if(strcmp(upr->fakername, "lpr") == 0)
	    {
	    upr->argv[0] = "lpr";
	    }
	else
	    {
	    if((i = uprint_print_argv_lpr(p, argv, MAX_ARGV)) == -1)
		return -1;
	    }
	}
    #endif

    /* Remote network spooler: */
    else if(remote_too && printdest_claim_remote(uprint_get_dest(upr), &info))
	{
	command_path = (char*)NULL;
	}

    /* Destination unknown. */
    else
	{
	uprint_errno = UPE_UNDEST;
	return -1;
	}

    /* If to remote queue, */
    if(command_path == (char*)NULL)
    	{
	FILE *f;
	if((f = fopen(UPRINT_LOGFILE, "a")) != (FILE*)NULL)
	    {
	    fprintf(f, "to remote %s@%s\n", info.printer, info.node);
	    fclose(f);
	    }
	ret = uprint_print_rfc1179(upr, &info);
	gu_free(info.node);
    	}

    /*
    ** If i is still 0 then we run the command for which
    ** we are the fake and pass it the arguments we got.
    */
    else if(i == 0)
    	{
	if(upr->argc > 0)
	    logit(command_path, upr->argv);
	ret = uprint_run(upr->uid, command_path, upr->argv);
	}

    /*
    ** Append the file names and then run the
    ** command.
    */

    /* If no files specified, just run the command
       and let it read stdin: */
    else if(upr->files == (const char **)NULL)
    	{
	if(command_path == PPR_PATH)
	    {
	    argv[i++] = "--lpq-filename";
	    argv[i++] = "stdin";
	    }
	argv[i] = (const char *)NULL;
	if(upr->argc > 0)
	    logit(command_path, argv);
	ret = uprint_run(upr->uid, command_path, argv);
    	}

    /* If files specified and the command must
       be run once for each one: */
    else if(one_at_a_time)
	{
	int x;
	ret = 0;
	for(x=0; upr->files[x]; x++)
	    {
	    if(command_path == PPR_PATH)
	    	{
		argv[i++] = "--lpq-filename";
		argv[i++] = strcmp(upr->files[x], "-") ? upr->files[x] : "stdin";
	    	}

	    argv[i] = upr->files[x];
	    argv[i+1] = (const char *)NULL;

	    if(upr->argc > 0)
	        logit(command_path, argv);

	    if((ret = uprint_run(upr->uid, command_path, argv)) != 0)
	    	break;
	    }
	}

    /* If they can all go on the command line: */
    else
	{
	int x;
	for(x=0; upr->files[x] && i < (MAX_ARGV - 1); x++, i++)
	    argv[i] = upr->files[x];
	argv[i] = (const char*)NULL;

	if(upr->files[x] != (const char*)NULL)
	    {
	    uprint_errno = UPE_TOOMANY;
	    uprint_error_callback("%s(): argv[] overflow", function);
	    ret = -1;
	    }

	if(upr->argc > 0)
	    logit(command_path, argv);

	ret = uprint_run(upr->uid, command_path, argv);
	}

    #ifdef UPRINT_LOGFILE
    if(upr->argc > 0)
	{
	FILE *f;
	if((f = fopen(UPRINT_LOGFILE, "a")) != (FILE*)NULL)
	    {
	    if(ret == -1)
		fprintf(f, "result=%d, uprint_errno=%d (%s)\n\n", ret, uprint_errno, uprint_strerror(uprint_errno));
	    else
		fprintf(f, "result=%d\n\n", ret);
	    fclose(f);
	    }
	}
    #endif

    return ret;
    } /* end of uprint_print() */

/* end of file */
