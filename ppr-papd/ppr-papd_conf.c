/*
** mouse:~ppr/src/ppr-papd/ppr-papd_conf.c
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
** Last modified 27 December 2002.
*/

#include "before_system.h"
#include <string.h>
#include <errno.h>
#include <dirent.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "ppr-papd.h"

/*
** The code in this module opens each alias, group, and printer config file 
** and gets a list of the AppleTalk names to advertise.
*/

struct DIRS {
    const char *name;
    } ;

static struct DIRS dirs[] = {
    {ALIASCONF},
    {GRCONF},
    {PRCONF}
    };

/*
** This is called for each config file.
*/
static void do_config_file(const char fname[], FILE *f)
    {
    char *line = NULL;
    int line_available = 80;
    char *p;

    while((line = gu_getline(line, &line_available, f)))
	{
	if((p = lmatchp(line, "ppr-papd papname:")))
	    {
	    }
	}

    if(line)
	gu_free(line);
    }

/*
** This function is called from main().
*/
void read_conf(void)
    {
    const char function[] = "read_conf";
    struct DIRS *dir;
    DIR *dirobj;
    struct dirent *direntp;
    char fname[MAX_PPR_PATH];
    int len;
    FILE *f;
    
    for(dir=dirs; dir->name; dir++)
	{
	DODEBUG_STARTUP(("%s", dir->name));
	if(!(dirobj = opendir(dir->name)))
	    fatal(0, _("%s(): opendir(\"%s\") failed, errno=%d (%s)"), function, dir, errno, gu_strerror(errno));
	while((direntp = readdir(dirobj)))
	    {
	    /* Skip . and .. and hidden files. */
	    if(direntp->d_name[0] == '.')
		continue;

	    /* Skip Emacs style backup files. */
	    len = strlen(direntp->d_name);
	    if(len > 0 && direntp->d_name[len-1] == '~')
		continue;

	    DODEBUG_STARTUP(("  %s", direntp->d_name));
	    ppr_fnamef(fname, "%s/%s", dir, direntp->d_name);
	    if(!(f = fopen(fname, "r")))
	    	fatal(0, "%s(): fopen(\"%s\", \"r\") failed, errno=%d (%s)", function, fname, errno, gu_strerror(errno));

	    do_config_file(fname, f);

	    fclose(f);
	    }
	closedir(dirobj);
	}
    }

/* end of file */
