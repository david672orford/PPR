/*
** mouse:~ppr/src/samba/sambaprint.c
** Copyright 1995--2002, Trinity College Computing Center.
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
** Last modified 1 May 2002.
*/

#include "before_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <stdarg.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "tdb.h"

static const char tdb_drivers_file[] = "/usr/local/samba/var/locks/ntdrivers.tdb";

static int drivers_import(void)
    {
    TDB_CONTEXT *tdb_drivers;
    char *line = NULL;
    int line_available = 256;
    int linenum = 0;
    char keytext[256];
    TDB_DATA key, data;
    char *lineptr;
    char *field_cversion,		/* OS Print Driver Version */
	*field_drivername,		/* Long Name of Printer Driver */
	*field_os,			/* Long OS Name */
	*field_driverpath,		/* Path the Driver DLL */
	*field_datafile,		/* Path the Configuration DLL */
	*field_configfile,		/* Path the PPD file */
	*field_helpfile,		/* Path to .HLP file */
	*field_monitorname,		/* Port Monitor (NULL for Samba) */
	*field_defaultdatatype,		/* Default Data Type (RAW) */
	*field_dependentfile;
    char **packlist[] = {
	&field_drivername,
	&field_os,
	&field_driverpath,
	&field_datafile,
	&field_configfile,
	&field_helpfile,
	&field_monitorname,
	&field_defaultdatatype,
	NULL};
    const char *architecture;
    char *scratch = NULL;
    int scratch_available = 0;
    int scratch_len;

    /* Open the TDB database that contains NT driver information. */
    if(!(tdb_drivers = tdb_open((char*)tdb_drivers_file, 0, TDB_DEFAULT, O_RDWR|O_CREAT, 0600)))
	{
	fprintf(stderr, "Can't open \"%s\", errno=%d (%s)\n", tdb_drivers_file, errno, strerror(errno));
	return 1;
	}

    while((line = gu_getline(line, &line_available, stdin)))
	{
	linenum++;
	lineptr = line;

	if(!(field_os = gu_strsep(&lineptr, ":"))
		|| !(field_cversion = gu_strsep(&lineptr, ":"))
		|| !(field_drivername = gu_strsep(&lineptr, ":"))
		|| !(field_driverpath = gu_strsep(&lineptr, ":"))
		|| !(field_datafile = gu_strsep(&lineptr, ":"))
		|| !(field_configfile = gu_strsep(&lineptr, ":"))
		|| !(field_helpfile = gu_strsep(&lineptr, ":"))
		|| !(field_monitorname = gu_strsep(&lineptr, ":"))
		|| !(field_defaultdatatype = gu_strsep(&lineptr, ":"))
	    )
	    {
	    fprintf(stderr, "Parse error on line %d.\n", linenum);
	    break;
	    }
	    	
	if(strcmp(field_os, "Windows NT x86") == 0)
	    architecture = "W32X86";
	else if(strcmp(field_os, "Windows 4.0") == 0)
	    architecture = "WIN40";
	else
	    {
	    fprintf(stderr, "Unrecognized OS: %s\n", field_os);
	    break;
	    }

	/* Create the TDB key.  Note that the Samba code converts this to the Unix
	   codepage but we don't. */
	snprintf(keytext, sizeof(keytext), "DRIVERS/%s/%d/%s", architecture, atoi(field_cversion), field_drivername);

	/* Make sure scratch buffer is big enough. */
	if((line_available + 10) > scratch_available)
	    {
	    scratch_available = line_available + 10;
	    scratch = gu_realloc(scratch, scratch_available, sizeof(char));
	    }

	scratch_len = 0;

	/* Copy the little-endian 32 bit integer field. */
	{
	unsigned int val = atoi(field_cversion);
	int x;
	for(x=0; x<4; x++)
	    {
	    scratch[scratch_len++] = val & 0xFF;
	    val >>= 8;
	    }
	}

	/* Copy the text fields. */
	{
	int x;
	char **p;
	for(x = 0; (p = packlist[x]); x++)
	    {
	    int len = strlen(*p) + 1;
	    memcpy(&scratch[scratch_len], *p, len);
	    scratch_len += len;
	    }
	}

	/* Now add the list of other files that must be downloaded. */
	for( ; (field_dependentfile = gu_strsep(&lineptr, ":")); )
	    {
	    int len = strlen(field_dependentfile) + 1;
	    memcpy(&scratch[scratch_len], field_dependentfile, len);
	    scratch_len += len;
	    }

	key.dptr = keytext;
	key.dsize = strlen(keytext)+1;
	data.dptr = scratch;
	data.dsize = scratch_len;
	
	if(tdb_store(tdb_drivers, key, data, TDB_REPLACE))
	    {
	    fprintf(stderr, "Failed to insert key \"%s\", %s.\n", keytext, tdb_errorstr(tdb_drivers));
	    break;
	    }
	}

    tdb_close(tdb_drivers);

    /* If the line buffer hasn't been freed, it means there was an error. */
    if(line)
	{
	gu_free(line);
	return 1;
	}

    return 0;
    }

int main(int argc, char *argv[])
    {
    /* Initialize international messages library. */
    #ifdef INTERNATIONAL
    setlocale(LC_MESSAGES, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);
    #endif

    /* We don't have real command line parsing yet, so just make sure the 
       user has asked for the one feature we already support.
       */
    if(argc == 3 && strcmp(argv[1], "drivers") == 0 && strcmp(argv[2], "import") == 0)
	{
	return drivers_import();
	}
    else
    	{
	fprintf(stderr, "Bad usage.\n");
	return 1;
    	}
    }

/* end of file */
