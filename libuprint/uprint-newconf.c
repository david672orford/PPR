/*
** mouse:~ppr/src/libuprint/uprint-newconf.c
** Copyright 1995--2003, Trinity College Computing Center.
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
** Last modified 18 November 2003.
*/

#include "before_system.h"
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "util_exits.h"
#include "uprint.h"
#include "uprint_conf.h"
#include "version.h"

static const char *const myname = "uprint-newconf";

/*
** These are the locations of the UPRINT substitute commands.
*/
#define BINDIR HOMEDIR"/bin"
const char *path_uprint_lp = BINDIR"/uprint-lp";
const char *path_uprint_cancel = BINDIR"/uprint-cancel";
const char *path_uprint_lpstat = BINDIR"/uprint-lpstat";
const char *path_uprint_lpr = BINDIR"/uprint-lpr";
const char *path_uprint_lprm = BINDIR"/uprint-lprm";
const char *path_uprint_lpq = BINDIR"/uprint-lpq";

void uprint_error_callback(const char *format, ...)
	{
	va_list va;
	fprintf(stderr, "%s: ", myname);
	va_start(va, format);
	vfprintf(stderr, format, va);
	fputc('\n', stderr);
	va_end(va);
	} /* end of uprint_error_callback() */

static void fatal(int exitcode, const char *format, ...)
	{
	va_list va;
	va_start(va, format);
	fprintf(stderr, "%s: ", myname);
	vfprintf(stderr, format, va);
	fputc('\n', stderr);
	va_end(va);
	exit(exitcode);
	}

/*
** This function creates a link in a mainstream directory
** such as "/usr/bin".  The link points to a uprint substitute
** for a standard Unix program.
**
** The standard Unix program is renamed first to the name
** specified by the parameter "sidelined".  If the sidelined
** name already exists, then it is renamed too.
*/
static void install_mainstream_link(const char linkname[], const char linkto[], const char sidelinename[])
	{
	struct stat statbuf, junk;

	#ifdef DEBUG
	printf("install_mainstream_link(linkname=\"%s\", linkto=\"%s\", sidelinename=\"%s\")\n", linkname, linkto, sidelinename);
	#endif

	/* If the well known name (such as "/usr/bin/lp") exists, */
	if(lstat(linkname, &statbuf) >= 0)
		{
		/* If it is a symbolic link, simply delete it. */
		if(S_ISLNK(statbuf.st_mode))
			{
			if(unlink(linkname) < 0)
				fatal(1, "unlink(\"%s\") failed, errno=%d (%s)", linkname, errno, gu_strerror(errno));
			}
		/*
		** If it is not a symbolic link, rename it to the name
		** specified by sidelinename.
		*/
		else
			{
			int x;

			/* Rename any existing sidelined. */
			if(lstat(sidelinename, &junk) >= 0)			/* if exists, */
				{
				for(x=1; TRUE; x++)
					{
					char newname[MAX_PPR_PATH];

					snprintf(newname, sizeof(newname), "%s-old-%d", sidelinename, x);

					if(lstat(newname, &junk) >= 0)
						continue;

					if(errno != ENOENT)
						fatal(1, "Can't stat \"%s\", errno=%d (%s)", newname, errno, gu_strerror(errno));

					printf(_("Renaming \"%s\" -> \"%s\".\n"), sidelinename, newname);
					if(rename(sidelinename, newname) < 0)
						fatal(1, "rename(\"%s\", \"%s\") failed, errno=%d (%s)", linkname, sidelinename, errno, gu_strerror(errno));
					break;
					}
				}
			else										/* if probably doesn't exist, */
				{
				if(errno != ENOENT)
					fatal(1, "Can't stat \"%s\", errno=%d (%s)", sidelinename, errno, gu_strerror(errno));
				}

			/* Do the actually renaming. */
			printf(_("Renaming \"%s\" -> \"%s\".\n"), linkname, sidelinename);
			if(rename(linkname, sidelinename) < 0)
				fatal(1, "rename(\"%s\", \"%s\") failed, errno=%d (%s)", linkname, sidelinename, errno, gu_strerror(errno));
			}
		}

	/* Create the link to the UPRINT version. */
	printf(_("Creating symbolic link \"%s\" -> \"%s\".\n"), linkname, linkto);
	if(symlink(linkto, linkname) < 0)
		fatal(1, "symlink(\"%s\", \"%s\") failed, errno=%d (%s)", linkto, linkname, errno, gu_strerror(errno));

	/* Do a few checks on the sidelined spooler program. */
	if(lstat(sidelinename, &statbuf) < 0)
		{
		if(errno != ENOENT)
			fatal(1, "Can't stat \"%s\", errno=%d (%s)", sidelinename, errno, gu_strerror(errno));
		printf(_("  (Note: \"%s\" does not exist.)\n"), sidelinename);
		}
	else
		{
		if(! S_ISREG(statbuf.st_mode) || ! (S_IXUSR & statbuf.st_mode))
			printf(_("  (Warning: \"%s\" is not executable.)\n"), sidelinename);
		}
	}

/*
** Remove a symbolic link from the mainstream directory and move the
** sidelined program back into place.
*/
static void uninstall_mainstream_link(const char linkname[], const char linkto[], const char sidelinename[])
	{
	struct stat statbuf, junk;

	#ifdef DEBUG
	printf("uninstall_mainstream_link(linkname=\"%s\", linkto=\"%s\", sidelinename=\"%s\")\n", linkname, linkto, sidelinename);
	#endif

	if(lstat(linkname, &statbuf) >= 0)
		{
		/* If it is a symbolic link, simply delete it. */
		if(S_ISLNK(statbuf.st_mode))
			{
			printf(_("Removing symbolic link \"%s\".\n"), linkname);
			if(unlink(linkname) < 0)
				fatal(1, "unlink(\"%s\") failed, errno=%d (%s)", linkname, errno, gu_strerror(errno));
			}
		else if(lstat(sidelinename, &junk) >= 0)
			{
			fprintf(stderr, _("Will not move \"%s\" back to \"%s\" because target isn't a symlink.\n"), linkname, sidelinename);
			return;
			}
		else
			{
			printf(_("Appearently, \"%s\" never was sidelined.\n"), linkname);
			return;
			}
		}

	if(lstat(sidelinename, &junk) < 0)
		{
		printf(_("No \"%s\" to move to \"%s\".\n"), sidelinename, linkname);
		}
	else
		{
		printf(_("Moving \"%s\" back to \"%s\".\n"), sidelinename, linkname);
		if(rename(sidelinename, linkname) < 0)
			fatal(1, "rename(\"%s\", \"%s\") failed, errno=%d (%s)", sidelinename, linkname, errno, gu_strerror(errno));
		}
	}

/*
** Command line options:
*/
static const char *option_chars = "";
static const struct gu_getopt_opt option_words[] =
		{
		{"remove", 1000, FALSE},
		{"force", 1001, FALSE},
		{"help", 9000, FALSE},
		{"version", 9001, FALSE},
		{(char*)NULL, 0, FALSE}
		} ;

/*
** Print help.
*/
static void help_switches(FILE *outfile)
	{
	fputs(_("Valid switches:\n"), outfile);

	fputs(_("\t--remove     back out changes\n"
			"\t--force      change system against advice\n"), outfile);

	fputs(_("\t--version\n"
			"\t--help\n"), outfile);
	}

/*
** We are linked into the guts of the uprint_conf.c code.
** We will move files and create symbolic links to make
** the file system conform to uprint.conf.
*/
int main(int argc, char *argv[])
	{
	gu_boolean opt_remove = FALSE;
	gu_boolean opt_force = FALSE;

	/* Initialize international messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	/* Parse the options. */
	{
	struct gu_getopt_state getopt_state;
	int optchar;
	gu_getopt_init(&getopt_state, argc, argv, option_chars, option_words);
	while((optchar = ppr_getopt(&getopt_state)) != -1)
		{
		switch(optchar)
			{
			case 1000:					/* --remove */
				opt_remove = TRUE;
				break;
			
			case 1001:					/* --force */
				opt_force = TRUE;
				break;

			case 9000:					/* --help */
				help_switches(stdout);
				exit(EXIT_OK);

			case 9001:					/* --version */
				puts(VERSION);
				puts(COPYRIGHT);
				puts(AUTHOR);
				exit(EXIT_OK);

			default:					/* other getopt errors or missing case */
				gu_getopt_default(myname, optchar, &getopt_state, stderr);
				exit(EXIT_SYNTAX);
				break;
			}
		}
	}

	if(getuid() != 0)
		fatal(1, _("must be run by root"));

	#if 0
	if(!opt_remove && !opt_force && access("/etc/alternatives", X_OK) != -1)
		{
		fprintf(stderr, _("This system uses /etc/alternatives.  Using uprint-newconf\n"
						"is not advised and it will not run without --force or --remove.\n"));
		return 1;
		}
	#endif

	if(opt_remove)
		printf(_("Restoring original spooler program names:\n"));
	else		
		printf(_("Adjusting spooler program names to conform to %s:\n"), UPRINTCONF);

	printf("\n");

	uprint_read_conf();

	if(conf.lp.sidelined && !opt_remove)
		{
		printf(_("Files for lp should be in their sidelined locations.\n"));
		install_mainstream_link(conf.well_known.lp, path_uprint_lp, conf.sidelined.lp);
		install_mainstream_link(conf.well_known.cancel, path_uprint_cancel, conf.sidelined.cancel);
		install_mainstream_link(conf.well_known.lpstat, path_uprint_lpstat, conf.sidelined.lpstat);
		}
	else
		{
		printf(_("Files for lp should be in their normal locations.\n"));
		uninstall_mainstream_link(conf.well_known.lp, path_uprint_lp, conf.sidelined.lp);
		uninstall_mainstream_link(conf.well_known.cancel, path_uprint_cancel, conf.sidelined.cancel);
		uninstall_mainstream_link(conf.well_known.lpstat, path_uprint_lpstat, conf.sidelined.lpstat);
		}
	printf("\n");

	if(conf.lpr.sidelined && !opt_remove)
		{
		printf(_("Files for lpr should be in their sidelined locations.\n"));
		install_mainstream_link(conf.well_known.lpr, path_uprint_lpr, conf.sidelined.lpr);
		install_mainstream_link(conf.well_known.lprm, path_uprint_lprm, conf.sidelined.lprm);
		install_mainstream_link(conf.well_known.lpq, path_uprint_lpq, conf.sidelined.lpq);
		}
	else
		{
		printf(_("Files for lpr should be in their normal locations.\n"));
		uninstall_mainstream_link(conf.well_known.lpr, path_uprint_lpr, conf.sidelined.lpr);
		uninstall_mainstream_link(conf.well_known.lprm, path_uprint_lprm, conf.sidelined.lprm);
		uninstall_mainstream_link(conf.well_known.lpq, path_uprint_lpq, conf.sidelined.lpq);
		}
	printf("\n");

	printf("Done.\n");

	return 0;
	} /* end of main() */

/* end of file */



