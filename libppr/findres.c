/*
** mouse:~ppr/src/libppr/findres.c
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
** Last modified 16 March 2005.
*/

/*
** The purpose of the functions in this module is to search for PostScript
** resources in the resource cache.  This cache has two parts.  The first
** one is the "permanent cache".  This is located in /usr/share/ppr/cache.  Files
** must be deliverately placed in the cache.  The second is the "automatic
** cache".  Files are placed in this cache automatically when they are found
** in incoming print jobs.
*/

#include "config.h"
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

static char *try_resource_dir(const char cachedir[],
				const char res_type[], const char res_name[],
				double version, int revision)
	{
	struct stat statbuf;
	char resource_fname[MAX_PPR_PATH];

	if(strcmp(res_type, "procset") == 0)
		ppr_fnamef(resource_fname, "%s/procset/%s-%s-%d", cachedir, res_name, gu_dtostr(version), revision);
	else
		ppr_fnamef(resource_fname, "%s/%s/%s", cachedir, res_type, res_name);

	if(stat(resource_fname, &statbuf) == 0)
		{
		return gu_strdup(resource_fname);
		}

	return (char*)NULL;
	}

static char *try_fontindex(const char res_name[], int *features)
	{
	const char function[] = "try_fontindex";
	const char filename[] = FONT_INDEX;
	FILE *dbf;
	char *line = NULL;
	int line_space = 256;
	int linenum = 0;
	char *ptr;
	char *f1, *f2, *f3;
	char *answer = NULL;

	if(!(dbf = fopen(filename, "r")))
		{
		error(_("%s(): can't open \"%s\", errno=%d (%s)"), function, filename, errno, gu_strerror(errno));
		return NULL;
		}

	while((line = gu_getline(line, &line_space, dbf)))
		{
		linenum++;
		if(line[0] == '#')
			continue;

		ptr = line;
		if((f1 = gu_strsep(&ptr, ":")))
			{
			if(strcmp(f1, res_name) == 0)
				{
				if(!(f2 = gu_strsep(&ptr, ":")) || !f2[0] || !(f3 = gu_strsep(&ptr, ":")) || !f3[0])
					{
					error("%s(): \"%s\" line %d is invalid", function, filename, linenum);
					continue;
					}
				if(features)
					*features = atoi(f2);
				answer = gu_strdup(f3);
				gu_free(line);
				break;
				}
			}
		}

	fclose(dbf);

	return answer;
	} /* end of try_fontindex() */

/*
** Return a pointer to the filename of a resource which we can provide.
** The caller should pass the pointer to gu_free() when it is no longer
** needed.
*/
char *find_resource(
	const char res_type[], const char res_name[], double version, int revision,
	int *features
	)
	{
	char *ptr;

	if(features)
		*features = 0;

	if(strcmp(res_type, "font") == 0)
		{
		if((ptr = try_fontindex(res_name, features)))
			{
			return ptr;
			}
		}

	else
		{
		if((ptr = try_resource_dir(RESOURCEDIR, res_type, res_name, version, revision)))
			{
			return ptr;
			}
		}

	return (char*)NULL;
	} /* end of find_resource() */

/* end of file */

