/*
** mouse:~ppr/src/makeprogs/ppr-config.c
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
** Last modified 17 January 2005.
*/

#include "config.h"
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

	printf(format, "SHORT_VERSION", SHORT_VERSION);

	/*
	** These appear in all but paths.mk.  They don't appear in
	** paths.mk because they are already in global.mk.
	*/
	if(strcmp(language, "--mk") != 0)
		{
		printf(format, "USER_PPR", USER_PPR);
		printf(format, "USER_PPRWWW", USER_PPRWWW);
		printf(format, "GROUP_PPR", GROUP_PPR);
		printf(format, "CONFDIR", CONFDIR);
		printf(format, "LIBDIR", LIBDIR);
		printf(format, "SHAREDIR", SHAREDIR);
		printf(format, "VAR_SPOOL_PPR", VAR_SPOOL_PPR);
		printf(format, "TEMPDIR", TEMPDIR);
		printf(format, "SYSBINDIR", SYSBINDIR);

		/* non-path stuff */
		printf(format, "VERSION", VERSION);
		printf(format, "SHORT_VERSION", SHORT_VERSION);
		}

	/* Second Level Directories */
	printf(format, "BINDIR", BINDIR);
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
	printf(format, "SENDMAIL_PATH", SENDMAIL_PATH);

	/* Indexes */
	printf(format, "PPD_INDEX", PPD_INDEX);

	/* Internationalization */
	printf(format, "LOCALEDIR", LOCALEDIR);
	printf(format, "PACKAGE", PACKAGE);
	printf(format, "PACKAGE_INTERFACES", PACKAGE_INTERFACES);
	printf(format, "PACKAGE_PPRD", PACKAGE_PPRD);
	printf(format, "PACKAGE_PPRDRV", PACKAGE_PPRDRV);
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
				"\t--tcl\n"
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
		else if(strcmp(argv[i], "--tcl") == 0)
			do_paths("set %s \"%s\"\n", argv[i]);
		else if(strcmp(argv[i], "--confdir") == 0)
			printf("%s\n", CONFDIR);
		else if(strcmp(argv[i], "--libdir") == 0)
			printf("%s\n", LIBDIR);
		else if(strcmp(argv[i], "--sharedir") == 0)
			printf("%s\n", SHAREDIR);
		else if(strcmp(argv[i], "--var-spool-ppr") == 0)
			printf("%s\n", VAR_SPOOL_PPR);
		else if(strcmp(argv[i], "--tempdir") == 0)
			printf("%s\n", TEMPDIR);
		else
			{
			fprintf(stderr, "%s: unknown option %s\n", myname, argv[i]);
			return 1;
			}
		}

	return 0;
	}

/* end of file */
