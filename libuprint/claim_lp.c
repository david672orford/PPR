/*
** mouse:~ppr/src/libuprint/claim_lp.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 29 July 1999.
*/

#include "before_system.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include "gu.h"
#include "global_defines.h"

#include "uprint.h"

/*
** Return TRUE if the destname is the name
** of a valid LP destination.
*/
int printdest_claim_lp(const char *destname)
    {
    #ifdef HAVE_LP
    char fname[MAX_PPR_PATH];
    struct stat statbuf;

    /*
    ** Test for a group of printers which LP calls a "class".
    ** Presumably we are looking for a file which contains
    ** a list of the members.
    */
    ppr_fnamef(fname, "%s/%s", LP_LIST_CLASSES, destname);
    if(stat(fname, &statbuf) == 0)
	{
	return TRUE;
	}

    /*
    ** Try for a printer.
    **
    ** On most systems we are looking for the interface
    ** program, but on Solaris 5.x we are looking for a
    ** directory which contains configuration files.
    **
    ** On SGI systems we have to contend with the program
    ** "glp" and friends which go snooping in the LP
    ** configuration directories to build a list of
    ** printers.  The script sgi_glp_hack for each PPR queue
    ** creates a file in /var/spool/lp/members
    ** which contains something like "/dev/null" and
    ** non-executable file begining with the string "#UPRINT"
    ** in /var/spool/lp/interface.
    */
    ppr_fnamef(fname, "%s/%s", LP_LIST_PRINTERS, destname);
    if(stat(fname, &statbuf) == 0)
	{
	if(S_ISDIR(statbuf.st_mode))	/* Solaris 5.x */
	    return TRUE;
	if(statbuf.st_mode & S_IXUSR)	/* Executable */
	    return TRUE;

	/*
	** Check for the string "#UPRINT" at the start of the
	** first line and return FALSE if we find it since
	** its presence would indicate that it is not a real
	** printer but rather a fake one we made for some reason
	** such as to satisfy SGI printing utilities.
	*/
	{
	int f;

	if((f = open(fname, O_RDONLY)) != -1)
	    {
	    char magic[7];
	    int compare = -1;

	    if(read(f, magic, 7) == 7)
		compare = memcmp(magic, "#UPRINT", 7);

	    close(f);

	    if(compare == 0)
	        return FALSE;
	    }
	}

	return TRUE;
	}

    /*
    ** If the macro specifying its name is defined, try the
    ** Solaris 2.6 printing system configuration file.
    */
    #ifdef LP_PRINTERS_CONF
    {
    FILE *f;
    if((f = fopen(LP_PRINTERS_CONF, "r")))
    	{
	char line[256];
	char *p;

	while(fgets(line, sizeof(line), f))
	    {
	    if(isspace(line[0]))
	    	continue;

	    if((p = strchr(line, ':')) == (char*)NULL)
	    	continue;

	    *p = '\0';

	    if(strcmp(line, destname) == 0)
	    	{
	    	fclose(f);
	    	return TRUE;
	    	}
	    }

	fclose(f);
    	}
    }
    #endif

    #endif	/* HAVE_LP */

    return FALSE;
    } /* end of printdest_claim_lp() */

/* end of file */
