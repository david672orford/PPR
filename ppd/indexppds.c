/*
** mouse:~ppr/src/ppd/indexppds.c
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
** Last modified 31 October 2003.
*/

#include "before_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif

#include "gu.h"
#include "global_defines.h"
#include "util_exits.h"
#include "version.h"

#include "pool.h"
#include "vector.h"
#include "pstring.h"
#include "pre.h"

const char myname[] = "lib/indexppds";
const char ppr_conf[] = PPR_CONF;
const char section_name[] = "ppds";
const char ppdindex_db[] = VAR_SPOOL_PPR"/ppdindex.db";

/*==========================================================================
** Extra routines for PPD parsing 
** These should probably go into libppr eventually.
==========================================================================*/

/*
 * Edit a string in place, decoding hexadecimal substrings.  QuotedValues with hexadecimal
 * substrings are described in the "PostScript Printer File Format Specification"
 * version 4.0 on page 5.
 */
static int ppd_decode_QuotedValue(char *p)
	{
	int len;
	int si, di;

	len = strlen(p); 
	
	for(si=di=0; si < len; si++)
		{
		if(p[si] == '<')
			{
			int count = 0;
			int c, high_nibble = 0;

			si++;
			while((c = p[si]) && c != '>')
				{
				if(c >= '0' && c <= '9')
					c -= '0';
				else if(c >= 'a' && c <= 'f')
					c -= ('a' + 10);
				else if(c >= 'A' && c <= 'F')
					c -= ('A' + 10);
				else
					continue;

				if(count % 2 == 0)
					high_nibble = c;
				else
					p[di++] = (high_nibble << 4) + c;
				
				count++;
				}
			}
		else
			{
			p[di++] = p[si];
			}
		}
	
	return di;
	} /* end of ppd_decode_QuotedValue() */

/*
 * Take p and possibly subsequent lines (until one of them ends with a quote)
 * and assemble them into a PCS.
 */
static void *ppd_finish_quoted_string(char *initial_segment)
	{
	char *p = initial_segment;
	gu_boolean end_quote = FALSE;
	int len;
	void *text = gu_pcs_new();

	do	{
		len = strlen(p);
		if(len > 0 && p[len-1] == '"')
			{
			p[len-1] = '\0';
			end_quote = TRUE;
			}
		if(gu_pcs_length(&text) > 0)
			gu_pcs_append_char(&text, '\n');
		gu_pcs_append_cstr(&text, p);
		} while(!end_quote && (p = ppd_readline()));

	return gu_pcs_free_keep_cstr(&text);
	} /* end of ppd_finish_quoted_string() */

/*==========================================================================
** This routine is called for each file found in the PPD
** directories.
==========================================================================*/
static int do_file(FILE *indexfile, const char filename[])
	{
	{
	int ret;
	printf("  %s", filename);

	if((ret = ppd_open(filename, stderr)) != EXIT_OK)
		return ret;
	}
	{
	pool temp_pool = new_subpool(global_pool);
	char *pline, *p;

	/* the information we are gathering */
	char *Manufacturer = NULL;
	char *ModelName = NULL;
	char *NickName = NULL;
	char *ShortNickName = NULL;
	char *Product = NULL;
	vector PSVersion = new_vector(temp_pool, char*);
	gu_boolean DeviceID = FALSE;
	char *DeviceID_MFG = NULL;
	char *DeviceID_MDL = NULL;
	char *pprPJLID = NULL;

	/* we will choose these from those above */
	char *vendor;
	const char *description;
	
	while((pline = ppd_readline()))
		{
		if(!Manufacturer && (p = lmatchp(pline, "*Manufacturer:")) && *p == '"')
			{
			Manufacturer = ppd_finish_quoted_string(p + 1);
			ppd_decode_QuotedValue(Manufacturer);
			continue;
			}
		if(!ModelName && (p = lmatchp(pline, "*ModelName:")) && *p == '"')
			{
			ModelName = ppd_finish_quoted_string(p + 1);
			ppd_decode_QuotedValue(ModelName);
			continue;
			}
		if(!NickName && (p = lmatchp(pline, "*NickName:")) && *p == '"')
			{
			NickName = ppd_finish_quoted_string(p + 1);
			ppd_decode_QuotedValue(NickName);
			continue;
			}
		/* ShortNickName is only valid if it comes before NickName. */
		if(!ShortNickName && !NickName && (p = lmatchp(pline, "*ShortNickName:")) && *p == '"')
			{
			ShortNickName = ppd_finish_quoted_string(p + 1);
			ppd_decode_QuotedValue(ShortNickName);
			continue;
			}
		if(!Product && (p = lmatchp(pline, "*Product:")) && *p == '"') 
			{
			p = ppd_finish_quoted_string(p + 1);
			ppd_decode_QuotedValue(p);
			if(*p == '(' && p[strlen(p) - 1] == ')')
				{
				Product = gu_strndup(p + 1, strlen(p) - 2);
				}
			gu_free(p);
			continue;
			}
		if((p = lmatchp(pline, "*PSVersion:")) && *p == '"')
			{
			/* this is wasteful */
			vector matches = prematch(temp_pool, p,
					precomp(temp_pool, "^\"\\(([0-9\\.]+)\\)\\s+(\\d+)\"$", 1),
					0);
			if(matches && vector_size(matches) == 3)
				{
				vector_pop_front(matches, p);	/* discard */
				vector_push_back_vector(PSVersion, matches);
				}
			continue;
			}
		/* These are hard to parse becuase, at least in Foomatic generated PPD files:
		 * 1) They are aften split over multiple lines (after semicolons). 
		 * 2) They often have leading and trailing spaces.
		 * 3) They have to be split on semicolons (possibly with whitespace)
		 * 4) They have to be split again on colons into name and value
		 * 5) The IEEE 1284 standard allows names to be abbreviated
		 */
		if(!DeviceID && (p = lmatchp(pline, "*1284DeviceID:")) && *p == '"')
			{
			int i;
			pcre *split_pattern;
			vector pairs;

			p = ppd_finish_quoted_string(p + 1);
			ppd_decode_QuotedValue(p);
			ptrim(p);

			/* use regular expression to split on semicolons */
			split_pattern = precomp(temp_pool, ";\\s*", 0);
			pairs = pstrresplit(temp_pool, p, split_pattern);
		
			/* Iterate over the resulting list */
			for(i = 0; i < vector_size(pairs); i++)
				{
				char *p2, *p3;
				vector_get(pairs, i, p2);
				if((p3 = lmatchp(p2, "MANUFACTURER:")) || (p3 = lmatchp(p2, "MFG:")))
					{
					DeviceID_MFG = pstrdup(temp_pool, p3);
					}
				else if((p3 = lmatchp(p2, "MODEL:")) || (p3 = lmatchp(p2, "MDL:")))
					{
					DeviceID_MDL = pstrdup(temp_pool, p3);
					}
				}
			
			gu_free(p);	
			DeviceID = TRUE;			/* We have seen one */
			continue;
			}
		if(pprPJLID && (p = lmatchp(pline, "*pprPJLID:")) && *p == '"')
			{
			pprPJLID = ppd_finish_quoted_string(p + 1);
			ppd_decode_QuotedValue(pprPJLID);
			continue;
			}
		}

	#ifdef DEBUG
	printf("%s: Manufacturer=\"\", Product=\"%s\", ModelName=\"%s\", NickName=\"%s\", ShortNickName=\"%s\"",
		Manufacturer ? Manufacturer : "",
		Product ? Product : "",
		ModelName ? ModelName : "",
		NickName ? NickName : "",
		ShortNickName ? ShortNickName : ""
		);
	#endif

	fputc('\n', stdout);

	/*
	** Find a vendor string.  If there was a *Manufacturer defined in the PPD file, use that.
	** If not, use the part of the Nickname before the first space or hyphen.  If that doesn't
	** work, set vendor to NULL and thus let it be "Other".
	*/
	if(Manufacturer)
		{
		vendor = pstrdup(temp_pool, Manufacturer);
		}
	else if(NickName && strlen(NickName) > 3 && isupper(NickName[0]) && isalpha(NickName[1]) && strchr(NickName, ' '))
		{
		vendor = pstrndup(temp_pool, NickName, strcspn(NickName, " -"));
		}
	else
		{
		vendor = NULL;
		}

	/*
	** If the vendor name is longer than three letters and is all upper 
	** case latin letters, lower case all but the first.  The result is
	** that "IBM" will be left as it is but "EPSON" will become "Epson".
	*/
	if(vendor && strlen(vendor) > 3 && strspn(vendor, "ABCDEFGHIJKLMNOPQRSTUVWXYZ") == strlen(vendor))
		{
		int i;
		for(i = 1; vendor[i]; i++)
			vendor[i] = tolower(vendor[i]);
		}

	/*
	** Decide on a descriptive name to display in UI pick lists.
	*/
	if(NickName)
		description = NickName;
	else if(ModelName)
		description = ModelName;
	else if(Product)
		description = Product;
	else
		description = filename;

	/*
	** OK, here goes.
	*/
	fprintf(indexfile, "%s:%s:%s:%s:%s:%s:%s:%s\n",
		description,
		filename,
		vendor ? vendor : "Other",
		Product ? Product : "",
		pjoin(temp_pool, PSVersion, ","),	
		DeviceID_MFG ? DeviceID_MFG : "",
		DeviceID_MDL ? DeviceID_MDL : "",
		pprPJLID ? pprPJLID : ""
		);

	delete_pool(temp_pool);
		
	return EXIT_OK;
	}
	} /* end of do_file() */

/*============================================================================
** This routine is called for each directory containing PPD file which
** are supposed to be included in the index.
============================================================================*/
static int do_dir(FILE *indexfile, const char dirname[])
	{
	DIR *dirobj;
	struct dirent *fileobj;
	char filename[MAX_PPR_PATH];
	struct stat statbuf;
	int retval = EXIT_OK;

	if(!(dirobj = opendir(dirname)))
		{
		if(errno == ENOENT)		/* it is ok if the directory doesn't exist */
			{
			printf("  %s\n", _("Skipped because it doesn't exist."));
			return EXIT_OK;
			}

		fprintf(stderr, "%s: diropen(\"%s\") failed, errno=%d (%s)\n", myname, dirname, errno, gu_strerror(errno));
		return EXIT_INTERNAL;
		}

	while((fileobj = readdir(dirobj)))
		{
		/* Skip hidden files and the "." and ".." directories. */
		if(fileobj->d_name[0] == '.') continue;

		/* Build the full name of the file or directory. */
		ppr_fnamef(filename, "%s/%s", dirname, fileobj->d_name);

		if(stat(filename, &statbuf) == -1)
			{
			fprintf(stderr, "%s: stat() failed on \"%s\", errno=%d (%s)\n", myname, filename, errno, gu_strerror(errno));
			return EXIT_INTERNAL;
			}

		/* Skip editor backups. */
		if(strlen(filename) > 0 && filename[strlen(filename)-1] == '~')
			continue;

		/* Process regular files as PPD files. */
		if(S_ISREG(statbuf.st_mode))
			{
			if((retval = do_file(indexfile, filename)) != EXIT_OK)
				break;
			continue;
			}
		}

	closedir(dirobj);

	return retval;
	} /* end of do_dir() */

/*============================================================================
** Main
============================================================================*/
int main(int argc, char *argv[])
	{
	FILE *indexfile;
	FILE *cf;
	struct GU_INI_ENTRY *section;
	int i;
	const char *dirname;
	int retval = EXIT_OK;

	/* Initialize international messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	/* Parse the options.  This is so simple we don't bother with gu_getopt(). */
	if(argc > 1)
		{
		if(strcmp(argv[1], "--delete") == 0)
			{
			unlink(ppdindex_db);
			return EXIT_OK;
			}
		else
			{
			fprintf(stderr, "%s: unknown option: %s\n", myname, argv[1]);
			return EXIT_SYNTAX;
			}
		}

	/* This is the start of a retry loop in which we attempt to retrive the [PPDs] section
	 * from ppr.conf.  We look for the section and if we don't find it 
	 * we create one and look for it again. 
	 */
	while(TRUE)
		{
		/* open ppr.conf */
		if(!(cf = fopen(ppr_conf, "r")))
			{
			fprintf(stderr, _("%s: can't open \"%s\", errno=%d (%s)\n"), myname, ppr_conf, errno, gu_strerror(errno));
			return EXIT_INTERNAL;
			}

		/* Fetch the [ppds] section */
		section = gu_ini_section_load(cf, section_name);

		fclose(cf);

		/* If there wasn't such a section, append the one from ppr.conf.sample. */
		if(!section)
			{
			fprintf(stderr, _("%s: warning: no [%s] section in \"%s\", copying from \"%s.sample\"\n"), myname, section_name, ppr_conf, ppr_conf);
			if(gu_ini_section_from_sample(ppr_conf, section_name) == -1)
				return EXIT_INTERNAL;
			continue;
			}

		/* If we reach this point, we have suceeded. */
		break;
		}

	/* Create the output file */
	if(!(indexfile = fopen(ppdindex_db, "w")))
		{
		fprintf(stderr, _("%s: can't create \"%s\", errno=%d (%s)\n"), myname, ppdindex_db, errno, gu_strerror(errno));
		return EXIT_INTERNAL;
		}

	/* Print the format version */
	fprintf(indexfile, "# 2\n");
	
	/* Warn */
	fprintf(indexfile, "# This file format is a temporary hack.  Its format will change.\n");

	/* Index the PPD files distributed with PPR. */
	printf("%s\n", PPDDIR);
	do_dir(indexfile, PPDDIR);

	/* Iterate over the nameless values in the [PPDs] section. */
	for(i=0; section[i].name; i++)
		{
		if(section[i].name[0] == '\0')
			{
			dirname = gu_ini_value_index(&section[i], 0, "<MISSING VALUE>");

			printf("%s\n", dirname);
	
			if(strcmp(dirname, PPDDIR) == 0)
				{
				printf("  %s\n", _("Skipped because it has already been indexed."));
				continue;
				}

			if((retval = do_dir(indexfile, dirname)) != EXIT_OK)
				break;
			}
		}

	/* Free the in-memory representation of the [PPDs] section. */
	gu_ini_section_free(section);

	/* Close the completed PPD index file. */
	fclose(indexfile);

	return retval;

	} /* end of main() */

/* end of file */
