/*
** mouse:~ppr/src/samba/ppr2samba.c
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
** Last modified 13 October 2005.
*/

/*
** This program scans PPR's printer and group configuration files
** and generates a file for Samba to include from smb.conf.
*/

#include "config.h"
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "util_exits.h"
#include "queueinfo.h"
#include "version.h"

/* Here we define the names of the files we use. */
const char smb_include_conf[] = CONFDIR"/smb-include.conf";
const char smb_include_x_conf[] = CONFDIR"/smb-include-%d.conf";
const char smb_protos_conf[] = LIBDIR"/lib/smb-protos.conf";

/* And here are the directories we search. */
struct DIRS {
	const char *name;
	enum QUEUEINFO_TYPE type;
	} dirs[] = {
	{ALIASCONF, QUEUEINFO_ALIAS},
	{GRCONF, QUEUEINFO_GROUP},
	{PRCONF, QUEUEINFO_PRINTER},
	{NULL, QUEUEINFO_SEARCH}
	};

/* The files we will be sending our output to: */
#define MAX_VSERVERS 10
FILE *outfiles[MAX_VSERVERS + 1];

/* Should be be verbose? */
int debug = FALSE;

void warning(const char *message, ...)
#ifdef __GNUC__
	__attribute__ (( format (printf, 1, 2) ))
#endif
	;
void warning(const char *message, ... )
	{
	va_list va;
	va_start(va, message);
	fputc('\n', stderr);
	fputs(_("Warning: "), stderr);
	vfprintf(stderr, message, va);
	fputc('\n', stderr);
	fputc('\n', stderr);
	va_end(va);
	} /* end of warning() */

/*
** Create one of the files that will be included from smb.conf.
*/
static FILE *create_output_file(int vserver)
	{
	char fname[MAX_PPR_PATH];
	const char *fnp;
	FILE *f;
    static const char *timestamp_string = NULL;

	if(vserver == 0)
		{
		fnp = smb_include_conf;
		}
	else
		{
		ppr_fnamef(fname, smb_include_x_conf, vserver);
		fnp = fname;
		}

	if(!(f = fopen(fnp, "w")))
		gu_Throw(_("Can't open \"%s\", errno=%d (%s)\n"), fnp, errno, gu_strerror(errno));

	if(!timestamp_string)
		{
		time_t time_now;
		char *temp;
		time(&time_now);
		temp = ctime(&time_now);
		timestamp_string = gu_strndup(temp, strcspn(temp, "\n"));
		}

	/* Put an explanatory header on the file: */
	fprintf(f,
	";\n"
	"; %s\n"
	"; %s\n"
	";\n"
	"; This file was generated by ppr2samba.  Do not edit it.\n"
	"; To update it, rerun ppr2samba.  For furthur information,\n"
	"; including how to exclude printers and groups from this file,\n"
	"; consult the ppr2samba(8) man page.\n"
	";\n\n", fnp, timestamp_string);

	return f;
	}

/*
** This is called from emmit_record(), once for the main file, and once
** for the virtual server file, if a virtual server entry was requested.
*/
static void write_smb_conf_record(const char directory[], const char name[], const char comment[], const char drivername[], const char proto[], int vserver)
	{
	FILE *f;

	if(!outfiles[vserver])
		outfiles[vserver] = create_output_file(vserver);

	f = outfiles[vserver];

	/* Emmit the smb.conf section. */
	fprintf(f, "; %s/%s\n", directory, name);
	fprintf(f, "[%s]\n", name);
	fprintf(f, "  comment = %s\n", comment ? comment : "");
	fprintf(f, "  printer = %s\n", name);
	if(drivername)
		fprintf(f, "  printer driver = %s\n", drivername);
	fprintf(f, "  copy = %s\n", proto ? proto : "pprproto");
	fprintf(f, "  browseable = yes\n\n");
	} /* end of write_smb_conf_record() */

static void emmit_record(const char directory[], const char name[], const char comment[], const char drivername[], const char proto[], int vserver)
	{
	write_smb_conf_record(directory, name, comment, drivername, proto, 0);
	if(vserver != 0)
		write_smb_conf_record(directory, name, comment, drivername, proto, vserver);
	} /* end of emmit_record() */

/*
** Search the printer configuration file directory and make an
** entry in the printcap and smb.conf file for each printer found there.
**
** Rememer the drivername of each printer because we will need
** it to determine the drivername of any group which has it as
** a member.
*/
static void do_config_file(struct DIRS *dir, const char qname[], int *total_exported, int *total_error)
	{
	char fname[MAX_PPR_PATH];
	FILE *f = NULL;
	char *line = NULL;
	int line_len = 128;
	int exported = 1;					/* boolean */
	char *p;
	char *drivername = NULL;
	char *proto = NULL;
	int vserver = 0;
	void *qobj = NULL;

	if(debug)
		printf("    %s", qname);
	
	gu_Try {
		ppr_fnamef(fname, "%s/%s", dir->name, qname);
		if(!(f = fopen(fname, "r")))
			gu_Throw(_("can't open \"%s\", errno=%d (%s)"), fname, errno, gu_strerror(errno));
	
		while((line = gu_getline(line, &line_len, f)))
			{
			if((p = lmatchp(line, "ppr2samba:")))
				{
				char *temp = NULL;
				if(gu_sscanf(p, "%d %S", &exported, &temp) < 1)
					warning(_("queue has an invalid \"%s\" line"), "ppr2samba");
				else
					{
					if(proto)
						gu_free(proto);
					proto = temp;
					}
				}
			else if((p = lmatchp(line, "ppr2samba-prototype:")))
				{
				char *temp = NULL;
				if(gu_sscanf(p, "%S", &temp) < 1)
					warning(_("queue has an invalid \"%s\" line"), "ppr2samba-prototype");
				else
					{
					if(proto)
						gu_free(proto);
					proto = temp;
					}
				}
			else if((p = lmatchp(line, "ppr2samba-drivername:")))
				{
				char *temp;
				if(gu_sscanf(p, "%A", &temp) != 1)
					warning(_("queue has an invalid \"%s\" line"), "ppr2samba-drivername");
	
				else
					{
					if(drivername)
						gu_free(drivername);
					drivername = p;
					if(debug)
						printf("  drivername = %s\n", drivername);
					}
				}
			else if((p = lmatchp(line, "ppr2samba-vserver:")))
				{
				warning("the option \"ppr2samba-vserver\" is obsolete and will be removed");
				vserver = atoi(p);
				if(vserver < 1 || vserver > MAX_VSERVERS)
					{
					warning(_("queue has an invalid \"%s\" line"), "ppr2samba-vserver");
					vserver = 0;
					}
				}
			}
	
		if(exported)
			{
			qobj = queueinfo_new_load_config(dir->type, qname);

			emmit_record(dir->name, qname,
				queueinfo_comment(qobj),
				drivername ? drivername : queueinfo_shortNickName(qobj),
				proto,
				vserver
				);

			if(debug)
				printf(" exported");
			(*total_exported)++;
			}
		else if(debug)
			{
			printf(" not exported");
			}
		}
	gu_Final {
		if(debug)
			printf("\n");
		if(line)
			gu_free(line);
		if(f)
			fclose(f);
		if(proto)
			gu_free(proto);
		if(drivername)
			gu_free(drivername);
		if(qobj)
			queueinfo_free(qobj);
		}
	gu_Catch {
		switch(dir->type)
			{
			case QUEUEINFO_ALIAS:
				gu_wrap_eprintf(_("Skipping alias \"%s\": %s\n"), qname, gu_exception);
				break;
			case QUEUEINFO_GROUP:
				gu_wrap_eprintf(_("Skipping group \"%s\": %s\n"), qname, gu_exception);
				break;
			case QUEUEINFO_PRINTER:
				gu_wrap_eprintf(_("Skipping printer \"%s\": %s\n"), qname, gu_exception);
				break;
			case QUEUEINFO_SEARCH:
				gu_Throw("assertion failed");
			}
		(*total_error)++;
		}
	} /* end of do_printers() */

/*
** Look over the Samba configuration and point out problems.
*/
static void check_for_problems(void)
	{
	FILE *f;
	char *line = NULL;
	int line_space = 80;
	gu_boolean saw_pprproto = FALSE;
	gu_boolean saw_pprprnt = FALSE;
	char *si, *di;

	if(!(f = fopen(smb_protos_conf, "r")))
		{
		fprintf(stderr, _("Can't open \"%s\", errno=%d (%s)\n"), smb_protos_conf, errno, gu_strerror(errno));
		return;
		}

	while((line = gu_getline(line, &line_space, f)))
		{
		for(si=di=line; *si; si++)
			{
			if(!isspace(*si))
				*(di++) = *si;
			}
		*di = '\0';

		if(strncmp(line, "[pprproto]", 10) == 0)
			saw_pprproto = TRUE;
		else if(strncasecmp(line, "[pprprnt$]", 10) == 0)
			saw_pprprnt = TRUE;
		}

	fclose(f);

	if(!saw_pprproto)
		{
		warning(_("The \"%s\" file doesn't contain a [pprproto] section."), smb_protos_conf);
		}

	if(!saw_pprprnt)
		{
		warning(_("The \"%s\" file doesn't contain a [pprprnt$] section."), smb_protos_conf);
		}

	} /* end of check_for_problems() */

static void help(FILE *outfile)
	{
	fprintf(outfile, _("Usage: %s\n"), "ppr2samba [--debug] [--nocreate] [--version] [--help]");
	fprintf(outfile, _("\t--debug      explain what is being done\n"));
	fprintf(outfile, _("\t--nocreate   don't create %s if it doesn't exist\n"), smb_include_conf);
	fprintf(outfile, _("\t--version    print the PPR version number\n"));
	fprintf(outfile, _("\t--help       print this message\n"));
	}

int main(int argc, char *argv[])
	{
	int x;
	gu_boolean opt_create = TRUE;
	int total = 0, total_exported = 0, total_errors = 0;

	/* Initialize internation messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	umask(PPR_UMASK);

	for(x=1; x < argc; x++)
		{
		if(strcmp(argv[x], "-d") == 0 || strcmp(argv[x], "--debug") == 0)
			{
			debug = TRUE;
			continue;
			}
		if(strcmp(argv[x], "--version") == 0)
			{
			puts(COPYRIGHT);
			puts(VERSION);
			return EXIT_OK;
			}
		if(strcmp(argv[x], "--nocreate") == 0)
			{
			opt_create = FALSE;
			continue;
			}
		if(strcmp(argv[x], "--help") == 0)
			{
			help(stdout);
			return EXIT_OK;
			}

		help(stderr);
		return EXIT_SYNTAX;
		}

	if(debug)
		printf("ppr2samba\n" VERSION "\n\n");

	gu_Try {
		struct DIRS *dir;
		DIR *dirobj;
		struct dirent *direntp;
		int len;
	
		/* If we shouldn't create smb-include.conf, make sure it exists. */
		if(! opt_create)
			{
			struct stat statbuf;
			if(stat(smb_include_conf, &statbuf) != 0)
				{
				/* If the error is something other than file not found,
				   then say we couldn't open it since that is what would
				   have been true if we had tried. */
				if(errno != ENOENT)
					gu_Throw(_("Can't open \"%s\", errno=%d (%s)"), smb_include_conf, errno, gu_strerror(errno));
	
				/* File not found, say so and get out since we
				   have been asked not to create it. */
				gu_wrap_printf(_("Absence of \"%s\" indicates that export to Samba is not desired.\n"), smb_include_conf);
				return EXIT_OK;
				}
			}
	
		/* Clear the file array. */
		for(x=0; x <= MAX_VSERVERS; x++)
			outfiles[x] = NULL;

		/* Do aliases, groups, and printers. */
		for(dir=dirs; dir->name; dir++)
			{
			if(debug)
				gu_wrap_printf("Searching directory \"%s\"...\n", dir->name);
	
			if(!(dirobj = opendir(dir->name)))
				gu_Throw(_("Can't open directory \"%s\", errno=%d (%s)\n"), dir->name, errno, gu_strerror(errno));
	
			while((direntp = readdir(dirobj)))
				{
				/* Skip . and .. and hidden files. */
				if(direntp->d_name[0] == '.')
					continue;
	
				/* Skip Emacs style backup files. */
				len = strlen(direntp->d_name);
				if(len > 0 && direntp->d_name[len-1] == '~')
					continue;

				total++;
				do_config_file(dir, direntp->d_name, &total_exported, &total_errors);
				}
	
			closedir(dirobj);
			}
	
		/* Close the smb.conf fragment files and delete those
		   we didn't use. */
		for(x=0; x < MAX_VSERVERS; x++)
			{
			if(outfiles[x])
				{
				fclose(outfiles[x]);
				}
			else
				{
				char fname[MAX_PPR_PATH];
				ppr_fnamef(fname, smb_include_x_conf, x);
				unlink(fname);
				}
			}
	
		/* Report on what we did. */
		{
		char *part2;
	   	gu_asprintf(&part2, ngettext("of %d queue", "of %d queues", total), total);
		/* Translators: first %s is "of %d queues". */
		gu_wrap_printf(ngettext(
						"%d %d exported to \"%s\".\n",
						"%d %d exported to \"%s\".\n",
						total_exported
						),
			total_exported, total, smb_include_conf);
		}

		if(total_errors > 0)
			gu_wrap_printf(_("%d of %d queue(s) skipt due to errors.\n"), total_errors, total);
	
		/* Produce warning messages. */
		check_for_problems();
	
		if(debug)
			printf(_("Done.\n"));
		}
	gu_Catch {
		fprintf(stderr, "%s: %s\n", argc > 0 ? argv[0] : "?", gu_exception);
		return EXIT_INTERNAL;
		}
	
	return EXIT_OK;
	} /* end of main() */

/* end of file */

