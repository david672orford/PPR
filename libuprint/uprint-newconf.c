/*
** mouse:~ppr/src/uprint/uprint-newconf.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 21 December 2000.
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

#include "uprint.h"
#include "uprint_conf.h"

static const char *const myname = "uprint-newconf";

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

void fatal(int exitcode, const char *format, ...)
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
	    if(lstat(sidelinename, &junk) >= 0)		/* if exists, */
		{
                for(x=1; TRUE; x++)
                    {
                    char newname[MAX_PPR_PATH];

                    ppr_fnamef(newname, "%s-old-%d", sidelinename, x);

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
	    else					/* if probably doesn't exist, */
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
** We are linked into the guts of the uprint_conf.c code.
** We will move files and create symbolic links to make
** the file system conform to uprint.conf.
*/
int main(int argc, char *argv[])
    {
    gu_boolean opt_remove = FALSE;

    /* Initialize international messages library. */
    #ifdef INTERNATIONAL
    setlocale(LC_MESSAGES, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);
    #endif

    if(getuid() != 0)
    	fatal(1, _("must be run by root"));

    /* Temporary hack for command line parsing code. */
    if(argv[1] && strcmp(argv[1], "--remove") == 0)
    	opt_remove = TRUE;

    printf(_("Adjusting file system to conform to %s:\n"), UPRINTCONF);
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



