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
** Last modified 14 March 2003.
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

const char myname[] = "lib/indexppds";
const char ppr_conf[] = PPR_CONF;
const char section_name[] = "ppds";
const char ppdindex_db[] = VAR_SPOOL_PPR"/ppdindex.db";

/*==========================================================================
** This routine is called for each file found in the PPD
** directories.
==========================================================================*/
static int do_file(FILE *indexfile, const char filename[])
    {
    int ret;
    char *pline, *p;
    char *Manufacturer = NULL;
    char *Product = NULL;
    char *ModelName = NULL;
    char *NickName = NULL;
    char *ShortNickName = NULL;

    printf("  %s", filename);

    if((ret = ppd_open(filename, stderr)) != EXIT_OK)
	return ret;

    while((pline = ppd_readline()))
	{
	if(!Manufacturer && (p = lmatchp(pline, "*Manufacturer:")) && *p == '"')
	    {
	    p++;
	    Manufacturer = gu_strndup(p, strcspn(p, "\""));
	    continue;
	    }
	if(!Product && (p = lmatchp(pline, "*Product:")) && *p == '"' && p[1] == '(')
	    {
	    p += 2;
	    Product = gu_strndup(p, strcspn(p, ")"));
	    continue;
	    }
	if(!ModelName && (p = lmatchp(pline, "*ModelName:")) && *p == '"')
	    {
	    p++;
	    ModelName = gu_strndup(p, strcspn(p, "\""));
	    continue;
	    }
	if(!NickName && (p = lmatchp(pline, "*NickName:")))
	    {
	    p++;
	    NickName = gu_strndup(p, strcspn(p, "\""));
	    continue;
	    }
	/* ShortNickName is only valid if it comes before NickName. */
	if(!ShortNickName && !NickName && (p = lmatchp(pline, "*ShortNickName:")))
	    {
	    p++;
	    ShortNickName = gu_strndup(p, strcspn(p, "\""));
	    continue;
	    }
	}

    #ifdef DEBUG
    printf(": Manufacturer=\"%s\", Product=\"%s\", ModelName=\"%s\", NickName=\"%s\", ShortNickName=\"%s\"",
	Manufacturer ? Manufacturer : "",
	Product ? Product : "",
	ModelName ? ModelName : "",
	NickName ? NickName : "",
	ShortNickName ? ShortNickName : ""
	);
    #endif

    fputc('\n', stdout);

    {
    char *vendor;
    const char *description;

    /*
    ** Find a vendor string.  If there was a *Manufacturer defined in the PPD file, use that.
    ** If not, use the part of the Nickname before the first space or hyphen.  If that doesn't
    ** work, set vendor to NULL and thus let it be "Other".
    */
    if(Manufacturer)
	{
	vendor = gu_strdup(Manufacturer);
	}
    else if(NickName && strlen(NickName) > 3 && isupper(NickName[0]) && isalpha(NickName[1]) && strchr(NickName, ' '))
	{
	vendor = gu_strndup(NickName, strcspn(NickName, " -"));
	}
    else
	{
	vendor = NULL;
	}

    /*
    ** If the vendor name is longer than three letters and is all upper 
    ** case latin letters, lower case all but the first.
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
    if(ShortNickName)
	description = ShortNickName;
    else if(NickName)
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
    fprintf(indexfile, "%s:%s:%s\n",
	filename,
	vendor ? vendor : "Other",
	description
	);

    if(vendor)
    	gu_free(vendor);
    }

    if(Manufacturer)
    	gu_free(Manufacturer);
    if(Product)
    	gu_free(Product);
    if(ModelName)
    	gu_free(ModelName);
    if(NickName)
    	gu_free(NickName);
    if(ShortNickName)
    	gu_free(ShortNickName);

    return EXIT_OK;
    } /* end of do_file() */

/*============================================================================
** This routine is called for each PPD directory.
============================================================================*/
static int do_dir(FILE *indexfile, const char dirname[])
    {
    DIR *dirobj;
    struct dirent *fileobj;
    char filename[MAX_PPR_PATH];
    struct stat statbuf;
    int retval = EXIT_OK;

    printf("%s\n", dirname);

    if(!(dirobj = opendir(dirname)))
    	{
	if(errno == ENOENT)	/* it is ok if the directory doesn't exist */
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
    setlocale(LC_MESSAGES, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);
    #endif

    /* Parse the options. */
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

    /* Start a retry loop. */
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

	if(!section)
	    {
	    fprintf(stderr, _("%s: warning: no [%s] section in \"%s\", copying from \"%s.sample\"\n"), myname, section_name, ppr_conf, ppr_conf);
	    if(gu_ini_section_from_sample(ppr_conf, section_name) == -1)
	    	return EXIT_INTERNAL;
	    continue;
	    }

	break;
	}

    /* Create the output file */
    if(!(indexfile = fopen(ppdindex_db, "w")))
    	{
	fprintf(stderr, _("%s: can't create \"%s\", errno=%d (%s)\n"), myname, ppdindex_db, errno, gu_strerror(errno));
	return EXIT_INTERNAL;
    	}

    /* Warn */
    fprintf(indexfile, "# This file format is a temporary hack.  Its format will change.\n");

    /* iterate over the nameless values */
    for(i=0; section[i].name; i++)
	{
	if(section[i].name[0] == '\0')
	    {
	    dirname = gu_ini_value_index(&section[i], 0, "<MISSING VALUE>");

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
