/*
** mouse:~ppr/src/makeprogs/gen_paths.c
** Copyright 1995--2002, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 31 July 2002.
*/

#include "before_system.h"
#include <stdio.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "version.h"

const char myname[] = "ppr-config";

static void do_paths(char format[], char language[])
    {
    printf("# This is NOT a configuration file!  Don't modify it!\n");

    if(strcmp(language, "--pm") == 0)
    	printf("package PPR;\n");

    /*
    ** These appear in all but paths.mk.  They don't appear in
    ** paths.mk because they are already in global.mk.
    */
    if(strcmp(language, "--mk"))
    	{
	printf(format, "USER_PPR", USER_PPR);
	printf(format, "GROUP_PPR", GROUP_PPR);
	printf(format, "CONFDIR", CONFDIR);
	printf(format, "HOMEDIR", HOMEDIR);
	printf(format, "SHAREDIR", SHAREDIR);
	printf(format, "VAR_SPOOL_PPR", VAR_SPOOL_PPR);
	printf(format, "TEMPDIR", TEMPDIR);

	/* non-path stuff */
	printf(format, "VERSION", VERSION);
	printf(format, "SHORT_VERSION", SHORT_VERSION);
    	}

    /* Second Level Directories */
    printf(format, "PRCONF", PRCONF);
    printf(format, "GRCONF", GRCONF);
    printf(format, "ALIASCONF", ALIASCONF);
    printf(format, "ACLDIR", ACLDIR);
    printf(format, "LOGDIR", LOGDIR);
    printf(format, "RUNDIR", RUNDIR);
    printf(format, "INTDIR", INTDIR);
    printf(format, "PPDDIR", PPDDIR);
    printf(format, "RESPONDERDIR", RESPONDERDIR);

    /* Configuration Files */
    printf(format, "PPR_CONF", PPR_CONF);
    printf(format, "UPRINTCONF", UPRINTCONF);
    printf(format, "UPRINTREMOTECONF", UPRINTREMOTECONF);
    printf(format, "MEDIAFILE", MEDIAFILE);

    /* Search Path */
    printf(format, "SAFE_PATH", SAFE_PATH);

    /* Program Paths */
    printf(format, "PPR_PATH", PPR_PATH);
    printf(format, "PPOP_PATH", PPOP_PATH);
    printf(format, "PPAD_PATH", PPAD_PATH);
    printf(format, "PPR2SAMBA_PATH", PPR2SAMBA_PATH);
    printf(format, "TAIL_STATUS_PATH", TAIL_STATUS_PATH);
    /* printf(format, "PPJOB_PATH", PPJOB_PATH); */
    printf(format, "SENDMAIL_PATH", SENDMAIL_PATH);

    /* Internationalization */
    printf(format, "LOCALEDIR", LOCALEDIR);
    printf(format, "PACKAGE", PACKAGE);
    printf(format, "PACKAGE_INTERFACES", PACKAGE_INTERFACES);
    printf(format, "PACKAGE_PPRD", PACKAGE_PPRD);
    printf(format, "PACKAGE_PPRDRV", PACKAGE_PPRDRV);
    printf(format, "PACKAGE_PAPSRV", PACKAGE_PAPSRV);
    printf(format, "PACKAGE_PPRWWW", PACKAGE_PPRWWW);

    /* More stuff for PPR.pm. */
    if(strcmp(language, "--pm") == 0)
	{
	printf("$SIZEOF_struct_Media=%d;\n", (int)sizeof(struct Media));
	}

    /* Perl modules and libraries must end with something that evaluates to true. */
    if(strcmp(language, "--ph") == 0 || strcmp(language, "--pm") == 0)
    	printf("1;\n");
    }

int main(int argc, char *argv[])
    {
    int i;

    if(argc < 2)
    	{
    	fprintf(stderr,
    		"Usage: %s [OPTIONS]\n"
    		"Options:\n"
		"\t--version\n"
    		"\t--mk\n"
    		"\t--sh\n"
    		"\t--ph\n"
    		"\t--pm\n"
    		"\t--confdir\n"
    		"\t--homedir\n"
    		"\t--sharedir\n"
    		"\t--var-spool-ppr\n",
    		myname);
    	return 1;
    	}

    for(i=1; i<argc; i++)
	{
	if(strcmp(argv[i], "--version") == 0)
	    printf("%s\n", SHORT_VERSION);
	else if(strcmp(argv[i], "--mk") == 0)
	    do_paths("%s=%s\n", argv[i]);
	else if(strcmp(argv[i], "--sh") == 0)
	    do_paths("%s=\"%s\"\n", argv[i]);
	else if(strcmp(argv[i], "--ph") == 0 || strcmp(argv[i], "--pm") == 0)
	    do_paths("$%s=\"%s\";\n", argv[i]);
	else if(strcmp(argv[i], "--confdir") == 0)
	    printf("%s\n", CONFDIR);
	else if(strcmp(argv[i], "--homedir") == 0)
	    printf("%s\n", HOMEDIR);
	else if(strcmp(argv[i], "--sharedir") == 0)
	    printf("%s\n", SHAREDIR);
	else if(strcmp(argv[i], "--var-spool-ppr") == 0)
	    printf("%s\n", VAR_SPOOL_PPR);
	else
	    {
	    fprintf(stderr, "%s: unknown option %s\n", myname, argv[i]);
	    return 1;
	    }
	}

    return 0;
    }

/* end of file */
