/*
** mouse:~ppr/src/ppr/ppr_rcache.c
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
** Last modified 9 August 1999.
*/

/*
** Resource Cache routines.
**
** These routines are called when "%%BeginResource:" and "%%EndResource"
** comments are seen.  The routines may decide to put the resource
** in the cache.  If the strip resources switch (-S) was used, the
** resource will be removed from the incoming file and will later be
** re-inserted from the cache.
*/

#include "config.h"
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "ppr.h"
#include "ppr_infile.h"
#include "ppr_exits.h"

/* Options set from command line: */
extern gu_boolean option_strip_fontindex;

/*
** This routine is called by getline_simplify() in ppr_simplify.c in order
** to allow the resource cache to examine the start of a new resource.
** It is very important that nest_level()==1 when this routine is called;
** it must not be called at the start of a nested resource.
*/
void begin_resource(void)
	{
	const char function[] = "begin_resource";
	char *type = tokens[1];		/* DSC comment type of resource */
	char *name = tokens[2];		/* DSC comment name of resource */
	double version;				/* DSC comment version number of resource */
	int revision;				/* DSC comment revision number of resource */
	gu_boolean is_procset;
	gu_boolean is_font;
	char *found;
	int font_features = 0;

	if(nest_level() != 1)
		fatal(PPREXIT_OTHERERR, "%s(): nest_level() != 1\n", function);

	/*
	** If the type or resource or the resource name is missing,
	** Issue a warning message and don't cache it.
	*/
	if(!type || !name)
		{
		warning(WARNING_SEVERE, _("Invalid \"%%%%BeginResource:\" line:\n%s"), line);
		return;
		}

	/* Set flags indicating which type of resource it is for later use. */
	is_procset = FALSE;
	is_font = FALSE;
	if(strcmp(type, "procset") == 0)
		is_procset = TRUE;
	else if(strcmp(type, "font") == 0)
		is_font = TRUE;

	/*
	** If this is a procedure set, read the
	** version and revision numbers.
	*/
	version = 0.0;
	revision = 0;
	if(is_procset && tokens[3] && tokens[4])
		{
		version = gu_getdouble(tokens[3]);
		revision = atoi(tokens[4]);
		}

	#ifdef DEBUG_RESOURCES
	printf("%s(): %s %s %s %d\n", function, type, quote(name), gu_dtostr(version), revision);
	#endif

	/*
	** Search the cache and the font index for this resource.  Notice that if we have
	** a partial copy of a Macintosh style TrueType font in the cache it takes precedence
	** over a font in the font index.
	*/
	found = find_resource(type, name, version, revision, &font_features);

	/*
	*/
	if(found && is_font && option_strip_fontindex)
		{
		#ifdef DEBUG_RESOURCES
		printf("%s(): striping this font out\n", function);
		#endif

		resource(REREF_REMOVED, tokens[1], 2);	/* Mark as removed. */

		fprintf(text, "%%%%IncludeResource: %s %s\n", type, quote(name));

		/*
		** Recursively call getline_simplify_cache() until
		** the end of the resource or the end of the file.
		*/
		while(nest_level() && !in_eof())
			getline_simplify();

		getline_simplify();		 /* and leave a fresh line in line[] */
		}

	if(found)
		gu_free(found);
	
	#ifdef DEBUG_RESOURCES
	printf("%s() done, rgrab = %d\n", function, rgrab);
	#endif
	} /* end of begin_resource() */

/* end of file */

