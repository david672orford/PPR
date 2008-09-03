/*
** mouse:~ppr/src/samba/sambaprint.c
** Copyright 1995--2008, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 20 August 2008.
*/

#include "config.h"
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
#include <pwd.h>
#include <signal.h>
#include <tdb.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "util_exits.h"

static const char myname[] = "sambaprint";

static const char tdb_base_path[] = "/var/lib/samba";

static const char tdb_drivers_file[] = "ntdrivers.tdb";

static const char tdb_printers_file[] = "ntprinters.tdb";

/* destination buffer resizer for gu_pack() and gu_unpack() */
static void gu_pack_resize(char **buf, int *bufsize, int desired)
	{
	if(!*buf || *bufsize < desired)
		{
		while(*bufsize < desired)
			*bufsize += 256;
		*buf = gu_realloc(*buf, *bufsize, sizeof(char));
		}
	}

/* Partial reimplementation of the structure packing routine from Samba. */
static size_t gu_pack(char **buf, int *bufsize, const char *fmt, ...)
	{
	va_list args;
	va_start(args, fmt);
	int len, iii;
	for(iii=0 ; *fmt; fmt++)
		{
		switch(*fmt)
			{
			case 'd':		/* 4 byte little-endian signed integer */
				{
				int n = va_arg(args, int);
				len = 4;
				gu_pack_resize(buf, bufsize, iii + len);
				while(len--)
					{
					*buf[iii++] = n & 0xFF;
					n >>= 8;
					}
				}
				break;
			case 's':		/* NULL terminated string */
				{
				char *str = va_arg(args, char*);
				len = strlen(str) + 1;
				gu_pack_resize(buf, bufsize, iii + len);
				memcpy(*buf + iii, str, len);
				iii += len;
				}
				break;
			}
		}
	va_end(args);
	return iii;
	}

/* Partial reimplementation of the structure unpacking routine from Samba. */
static int gu_unpack(char *buf, int bufsize, const char *fmt, ...)
	{
	va_list args;
	int len;
	int count = 0;
	va_start(args, fmt);
	for( ; *fmt; fmt++)
		{
		switch(*fmt)
			{
			case 'd':		/* 4 byte little-endian signed integer */
				{
				int *n = va_arg(args, int*);
				len = 4;
				if(bufsize >= len)
					{
					int iii, place = 0;
					for(iii=0,*n=0; iii<len; iii++,place+=8)
						{
						*n |= (buf[iii] << place);
						}
					buf += len;
					bufsize -= len;
					count++;
					}
				}
				break;
			case 's':		/* null-terminated string */
				{
				char **p = va_arg(args, char**);
				if(bufsize > 0)
					{
					len = strlen(buf) + 1;
					if(bufsize >= len)
						{
						*p = gu_strdup(buf);
						buf += len;
						bufsize -= len;
						count++;
						}
					}
				}
				break;
			}
		}
	va_end(args);
	return count;
	}

static int drivers_import(void)
	{
	char fname[MAX_PPR_PATH];
	TDB_CONTEXT *tdb_drivers;
	char *line = NULL;
	int line_available = 256;
	int linenum = 0;
	char keytext[256];
	TDB_DATA key, data;
	char *lineptr;
	char *field_cversion,				/* OS Print Driver Version */
		*field_drivername,				/* Long Name of Printer Driver */
		*field_os,						/* Long OS Name */
		*field_driverpath,				/* Path the Driver DLL */
		*field_datafile,				/* Path the Configuration DLL */
		*field_configfile,				/* Path the PPD file */
		*field_helpfile,				/* Path to .HLP file */
		*field_monitorname,				/* Port Monitor (NULL for Samba) */
		*field_defaultdatatype,			/* Default Data Type (RAW) */
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
	ppr_fnamef(fname, "%s/%s", tdb_base_path, tdb_drivers_file);
	if(!(tdb_drivers = tdb_open(fname, 0, TDB_DEFAULT, O_RDWR|O_CREAT, 0600)))
		{
		fprintf(stderr, "%s: can't open \"%s\", errno=%d (%s)\n", myname, fname, errno, strerror(errno));
		return EXIT_INTERNAL;
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
			fprintf(stderr, "%s: parse error on line %d.\n", myname, linenum);
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

		key.dptr = (unsigned char *)keytext;
		key.dsize = strlen(keytext)+1;
		data.dptr = (unsigned char *)scratch;
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
		return EXIT_INTERNAL;
		}

	return EXIT_OK;
	} /* drivers_import() */

static int drivers_export(void)
	{
	char fname[MAX_PPR_PATH];
	TDB_CONTEXT *tdb_drivers;
	TDB_DATA iii, value;
	int field_cversion;
	char *field_drivername;

	ppr_fnamef(fname, "%s/%s", tdb_base_path, tdb_drivers_file);
	if(!(tdb_drivers = tdb_open(fname, 0, TDB_DEFAULT, O_RDWR|O_CREAT, 0600)))
		{
		fprintf(stderr, "%s: can't open \"%s\", errno=%d (%s)\n", myname, fname, errno, strerror(errno));
		return EXIT_INTERNAL;
		}

	for(iii = tdb_firstkey(tdb_drivers); iii.dptr; iii = tdb_nextkey(tdb_drivers, iii))
		{
		value = tdb_fetch(tdb_drivers, iii);
		printf("%s\n", iii.dptr);
		gu_unpack((char*)value.dptr, value.dsize, "ds", &field_cversion, &field_drivername);
		printf("cversion: %d\n", field_cversion);
		printf("drivername: %s\n", field_drivername);
		}

	tdb_close(tdb_drivers);

	return EXIT_OK;
	}

static int printers_import()
	{
	char fname[MAX_PPR_PATH];
	TDB_CONTEXT *tdb_drivers;
	char *line = NULL;
	int line_available = 256;
	int linenum = 0;
	char keytext[256];
	TDB_DATA key, data;
	char *lineptr;
	char *field_cversion,				/* OS Print Driver Version */
		*field_drivername,				/* Long Name of Printer Driver */
		*field_os,						/* Long OS Name */
		*field_driverpath,				/* Path the Driver DLL */
		*field_datafile,				/* Path the Configuration DLL */
		*field_configfile,				/* Path the PPD file */
		*field_helpfile,				/* Path to .HLP file */
		*field_monitorname,				/* Port Monitor (NULL for Samba) */
		*field_defaultdatatype,			/* Default Data Type (RAW) */
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
	ppr_fnamef(fname, "%s/%s", tdb_base_path, tdb_printers_file);
	if(!(tdb_drivers = tdb_open(fname, 0, TDB_DEFAULT, O_RDWR|O_CREAT, 0600)))
		{
		fprintf(stderr, "%s: can't open \"%s\", errno=%d (%s)\n", myname, fname, errno, strerror(errno));
		return EXIT_INTERNAL;
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
			fprintf(stderr, "%s: parse error on line %d.\n", myname, linenum);
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

		key.dptr = (unsigned char *)keytext;
		key.dsize = strlen(keytext)+1;
		data.dptr = (unsigned char *)scratch;
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
		return EXIT_INTERNAL;
		}

	return EXIT_OK;
	} /* printers_import() */

static int printers_export(void)
	{
	/* not yet implemented */
	return EXIT_INTERNAL;
	}

int main(int argc, char *argv[])
	{
	{
	struct passwd *pw;
	if(!(pw = getpwuid(getuid())))
		{
		fprintf(stderr, "%s: getpwuid(%ld) failed, errno=%d (%s)\n", myname, (long)getuid(), errno, gu_strerror(errno));
		return EXIT_INTERNAL;
		}
	if(strcmp(pw->pw_name, USER_PPR))
		{
		fprintf(stderr, "%s: you are not %s\n", myname, USER_PPR);
		return EXIT_DENIED;
		}
	}

	/* Initialize international messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	/* We don't have real command line parsing yet, so just make sure the 
	   user has asked for the one feature we already support.
	   */
	if(argc == 3 && strcmp(argv[1], "drivers") == 0)
	   	{
		if(strcmp(argv[2], "import") == 0)
			{
			return drivers_import();
			}
		else if(strcmp(argv[2], "export") == 0)
			{
			return drivers_export();
			}
		}
	else if(argc == 3 && strcmp(argv[1], "printers") == 0)
	   	{
		if(strcmp(argv[2], "import") == 0)
			{
			return printers_import();
			}
		else if(strcmp(argv[2], "export") == 0)
			{
			return printers_export();
			}
		}
	else
		{
		fprintf(stderr, "Bad usage.\n");
		return EXIT_SYNTAX;
		}

	return EXIT_OK;
	}

/* end of file */
