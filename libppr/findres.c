/*
** mouse:~ppr/src/libppr/findres.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 13 February 2001.
*/

/*
** The purpose of the functions in this module is to search for PostScript
** resources in the resource cache.  This cache has two parts.  The first
** one is the "permanent cache".  This is located in /usr/share/ppr/cache.  Files
** must be deliverately placed in the cache.  The second is the "automatic
** cache".  Files are placed in this cache automatically when they are found
** in incoming print jobs.
*/

#include "before_system.h"
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"

static char resource_fname[MAX_PPR_PATH];

static char *try_cachedir(const char cachedir[],
				const char res_type[], const char res_name[],
				double version, int revision, int *newrev, int *features)
	{
	struct stat statbuf;

	if(strcmp(res_type, "procset") == 0)
		ppr_fnamef(resource_fname, "%s/procset/%s-%s-%d", cachedir, res_name, gu_dtostr(version), revision);
	else
		ppr_fnamef(resource_fname, "%s/%s/%s", cachedir, res_type, res_name);

	/* Try the permanent cache. */
	if(stat(resource_fname, &statbuf) == 0)
		{
		if(features)
			{
			*features = 0;				/* redundant */
			if(strcmp(res_type, "font") == 0)
				{
				if(statbuf.st_mode & FONT_MODE_MACTRUETYPE)
					*features |= FONT_MACTRUETYPE;
				if(statbuf.st_mode & FONT_MODE_TYPE_1)
					*features |= FONT_TYPE_1;
				if(statbuf.st_mode & FONT_MODE_TYPE_42)
					*features |= FONT_TYPE_42;
				}
			}
		return resource_fname;
		}

	/* Looks like it is not in either cache. */
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
		error("%s(): can't open \"%s\", errno=%d (%s)", function, filename, errno, gu_strerror(errno));
		return NULL;
		}

	while((line = gu_getline(line, &line_space, dbf)))
		{
		linenum++;
		if(line[0] == '#' || line[0] == ';' || line[0] == '\0') continue;

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

				ppr_fnamef(resource_fname, "%s", f3);
				answer = resource_fname;

				if(features)
					{
					*features = 0;				/* redundant */
					if(strcmp(f2, "ttf") == 0)
						*features |= FONT_TYPE_TTF;
					}

				gu_free(line);
				break;
				}
			}
		}

	fclose(dbf);

	return answer;
	} /* end of try_fontindex() */

/*
** Return a pointer to the filename of a resource in the cache.
**
** If we substitute a newer revision, we will store the new revision
** number in the integer pointed to by "newrev".  This isn't implemented yet.
*/
const char *noalloc_find_cached_resource(const char res_type[], const char res_name[], double version, int revision,
				const enum RES_SEARCH sequence[], int *newrev, int *features, enum RES_SEARCH *where_found)
	{
	int x;
	char *ptr;

	if(features) *features = 0;

	for(x=0; sequence[x] != RES_SEARCH_END; x++)
		{
		switch(sequence[x])
			{
			case RES_SEARCH_FONTINDEX:
				if(strcmp(res_type, "font") == 0 && (ptr = try_fontindex(res_name, features)))
					{
					if(where_found) *where_found = RES_SEARCH_FONTINDEX;
					return ptr;
					}
				break;

			case RES_SEARCH_CACHE:
				if((ptr = try_cachedir(STATIC_CACHEDIR, res_type, res_name, version, revision, newrev, NULL))
						|| (ptr = try_cachedir(CACHEDIR, res_type, res_name, version, revision, newrev, features)))
					{
					if(where_found) *where_found = RES_SEARCH_CACHE;
					return ptr;
					}
				break;

			#ifdef GNUC_HAPPY
			case RES_SEARCH_END:
				break;
			#endif
			}
		}

	return (char*)NULL;
	} /* end of noalloc_find_cached_resource() */

/*
** In this version, if the pointer is not NULL, it points
** into newly allocated memory.
*/
char *find_cached_resource(const char res_type[], const char res_name[], double version, int revision,
		const enum RES_SEARCH sequence[], int *newrev, int *features, enum RES_SEARCH *where_found)
	{
	const char *ptr;

	if((ptr = noalloc_find_cached_resource(res_type, res_name, version, revision, sequence, newrev, features, where_found)))
		return gu_strdup(ptr);

	return (char*)NULL;
	} /* end of find_cached_resource() */

/* end of file */

